#include "raw-gadget.hpp"

#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <algorithm>

#include <plog/Log.h>

namespace {
RawGadgetPassthrough* parentFromEndpoint(EndpointInfo* epInfo) {
  return epInfo->parent->parent->parent->parent->parent;
}

constexpr int kEndpointWakeSignal = SIGUSR1;
pthread_once_t gEndpointWakeSignalOnce = PTHREAD_ONCE_INIT;

void endpointWakeSignalHandler(int) {}

void installEndpointWakeSignalHandler() {
  struct sigaction action;
  std::memset(&action, 0, sizeof(action));
  action.sa_handler = endpointWakeSignalHandler;
  sigemptyset(&action.sa_mask);
  if (sigaction(kEndpointWakeSignal, &action, nullptr) != 0) {
    PLOG_ERROR << "Failed to install Raw Gadget wake signal handler";
  }
}

void trackTransfer(EndpointInfo* endpointInfo, libusb_transfer* transfer) {
  if (!endpointInfo->transferMutexInitialized || transfer == nullptr) {
    return;
  }
  pthread_mutex_lock(&endpointInfo->transferMutex);
  endpointInfo->activeTransfers.push_back(transfer);
  pthread_mutex_unlock(&endpointInfo->transferMutex);
}

void untrackTransfer(EndpointInfo* endpointInfo, libusb_transfer* transfer) {
  if (!endpointInfo->transferMutexInitialized || transfer == nullptr) {
    return;
  }
  pthread_mutex_lock(&endpointInfo->transferMutex);
  auto it = std::find(endpointInfo->activeTransfers.begin(), endpointInfo->activeTransfers.end(), transfer);
  if (it != endpointInfo->activeTransfers.end()) {
    endpointInfo->activeTransfers.erase(it);
  }
  pthread_mutex_unlock(&endpointInfo->transferMutex);
}

void requeueInterruptedWrite(EndpointInfo* endpointInfo, std::vector<unsigned char>&& packet) {
  if (!endpointInfo->writeQueueInitialized) {
    return;
  }

  pthread_mutex_lock(&endpointInfo->writeQueueMutex);
  endpointInfo->pendingWrites.emplace_front(std::move(packet));
  pthread_mutex_unlock(&endpointInfo->writeQueueMutex);
}
}

void initializeRawGadgetThreadSignals() {
  pthread_once(&gEndpointWakeSignalOnce, installEndpointWakeSignalHandler);
}

bool rawGadgetIoInterrupted(int error) {
  return error == EINTR || error == ETIMEDOUT;
}

void interruptEndpointThread(EndpointInfo* epInfo) {
  if (epInfo == nullptr || !epInfo->threadStarted) {
    return;
  }

  int result = pthread_kill(epInfo->thread, kEndpointWakeSignal);
  if (result != 0 && result != ESRCH) {
    PLOG_WARNING << "Failed to interrupt endpoint thread. errno=" << result;
  }
}

void clearPendingWrites(EndpointInfo* epInfo) {
  if (epInfo == nullptr || !epInfo->writeQueueInitialized) {
    return;
  }

  pthread_mutex_lock(&epInfo->writeQueueMutex);
  epInfo->pendingWrites.clear();
  pthread_mutex_unlock(&epInfo->writeQueueMutex);
}

bool queuePendingWrite(EndpointInfo* epInfo, const unsigned char* data, int length) {
  if (epInfo == nullptr || !epInfo->writeQueueInitialized || length < 0 || length > EP_MAX_PACKET_INT ||
      (length > 0 && data == nullptr)) {
    return false;
  }

  std::vector<unsigned char> packet(static_cast<size_t>(length));
  if (length > 0) {
    std::memcpy(packet.data(), data, static_cast<size_t>(length));
  }

  pthread_mutex_lock(&epInfo->writeQueueMutex);
  epInfo->pendingWrites.emplace_back(std::move(packet));
  pthread_mutex_unlock(&epInfo->writeQueueMutex);
  return true;
}

bool flushPendingWrite(EndpointInfo* epInfo) {
  if (epInfo == nullptr || !epInfo->writeQueueInitialized) {
    return false;
  }

  std::vector<unsigned char> packet;
  bool hadPacket = false;
  pthread_mutex_lock(&epInfo->writeQueueMutex);
  if (!epInfo->pendingWrites.empty()) {
    packet = std::move(epInfo->pendingWrites.front());
    epInfo->pendingWrites.pop_front();
    hadPacket = true;
  }
  pthread_mutex_unlock(&epInfo->writeQueueMutex);

  if (!hadPacket) {
    return false;
  }

  struct usb_raw_int_io io{};
  io.inner.ep = epInfo->ep_int;
  io.inner.flags = 0;
  io.inner.length = static_cast<__u32>(packet.size());
  if (!packet.empty()) {
    std::memcpy(&io.inner.data[0], packet.data(), packet.size());
  }

  int rv = usb_raw_ep_write(epInfo->fd, (struct usb_raw_ep_io*)&io);
  if (rv < 0) {
    if (rawGadgetIoInterrupted(errno)) {
      if (epInfo->keepRunning && !epInfo->stop) {
        requeueInterruptedWrite(epInfo, std::move(packet));
      }
      return true;
    }
    PLOG_ERROR << "write to host usb_raw_ep_write() returned " << rv;
    parentFromEndpoint(epInfo)->requestReconnect();
  } else if (rv != static_cast<int>(io.inner.length)) {
    PLOG_WARNING << "Only sent " << rv << " bytes instead of " << io.inner.length;
  }
  return true;
}

static void cb_transfer_out(struct libusb_transfer* xfr) {
  EndpointInfo* epInfo = (EndpointInfo*)xfr->user_data;
  untrackTransfer(epInfo, xfr);
  RawGadgetPassthrough* passthrough = parentFromEndpoint(epInfo);
  epInfo->busyPackets.fetch_sub(1);

  if (xfr->status != LIBUSB_TRANSFER_COMPLETED) {
    PLOG_ERROR << "transfer status " << xfr->status;
    if (xfr->status == LIBUSB_TRANSFER_NO_DEVICE || xfr->status == LIBUSB_TRANSFER_ERROR) {
      passthrough->requestReconnect();
    }
  }
  libusb_free_transfer(xfr);
}

static void cb_transfer_in(struct libusb_transfer* xfr) {
  EndpointInfo* epInfo = (EndpointInfo*)xfr->user_data;
  untrackTransfer(epInfo, xfr);
  RawGadgetPassthrough* passthrough = parentFromEndpoint(epInfo);
  if (xfr->status != LIBUSB_TRANSFER_COMPLETED) {
    PLOG_INFO << "transfer status " << xfr->status;
    epInfo->busyPackets.fetch_sub(1);
    if (xfr->status == LIBUSB_TRANSFER_NO_DEVICE || xfr->status == LIBUSB_TRANSFER_ERROR) {
      passthrough->requestReconnect();
    }
    libusb_free_transfer(xfr);
    return;
  }

  if (!epInfo->keepRunning || epInfo->stop) {
    epInfo->busyPackets.fetch_sub(1);
    libusb_free_transfer(xfr);
    return;
  }

  struct usb_raw_int_io io;
  io.inner.ep = epInfo->ep_int;
  io.inner.flags = 0;

  if (xfr->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
    for (int i = 0; i < xfr->num_iso_packets; i++) {
      struct libusb_iso_packet_descriptor* pack = &xfr->iso_packet_desc[i];

      if (pack->status != LIBUSB_TRANSFER_COMPLETED) {
        PLOG_ERROR << "pack " << i << " status " << pack->status;
        continue;
      }

      io.inner.length = pack->actual_length > EP_MAX_PACKET_INT ? EP_MAX_PACKET_INT : pack->actual_length;
      unsigned char* packetBuffer = libusb_get_iso_packet_buffer_simple(xfr, i);
      memcpy(&io.inner.data[0], packetBuffer, io.inner.length);

      if (!queuePendingWrite(epInfo, reinterpret_cast<unsigned char*>(&io.inner.data[0]),
                             static_cast<int>(io.inner.length))) {
        PLOG_ERROR << "Failed to queue isochronous packet for Raw Gadget write";
        passthrough->requestReconnect();
        break;
      }
    }
  } else {
    io.inner.length = xfr->actual_length > EP_MAX_PACKET_INT ? EP_MAX_PACKET_INT : xfr->actual_length;
    memcpy(&io.inner.data[0], xfr->buffer, io.inner.length);

    if (!queuePendingWrite(epInfo, reinterpret_cast<unsigned char*>(&io.inner.data[0]),
                           static_cast<int>(io.inner.length))) {
      PLOG_ERROR << "Failed to queue bulk/interrupt packet for Raw Gadget write";
      passthrough->requestReconnect();
    }
  }

  epInfo->busyPackets.fetch_sub(1);
  libusb_free_transfer(xfr);
}

void ep_out_work_interrupt(EndpointInfo* epInfo) {
  if (epInfo->busyPackets.load() >= 1) {
    usleep(epInfo->bIntervalInMicroseconds);
    return;
  }

  struct usb_raw_int_io io;
  io.inner.ep = epInfo->ep_int;
  io.inner.flags = 0;
  io.inner.length = epInfo->usb_endpoint.wMaxPacketSize;
  if (io.inner.length > EP_MAX_PACKET_INT) {
    io.inner.length = EP_MAX_PACKET_INT;
  }

  int transferred = usb_raw_ep_read(epInfo->fd, (struct usb_raw_ep_io*)&io);
  if (transferred <= 0) {
    if (transferred < 0 && !rawGadgetIoInterrupted(errno)) {
      parentFromEndpoint(epInfo)->requestReconnect();
    }
    usleep(epInfo->bIntervalInMicroseconds);
    return;
  }

  struct libusb_transfer* transfer = libusb_alloc_transfer(0);
  if (transfer == NULL) {
    PLOG_ERROR << "libusb_alloc_transfer(0) no memory";
    return;
  }

  unsigned char* transferBuffer = (unsigned char*)malloc((size_t)transferred);
  if (transferBuffer == nullptr) {
    PLOG_ERROR << "Failed to allocate transfer buffer";
    libusb_free_transfer(transfer);
    return;
  }
  memcpy(transferBuffer, &io.inner.data[0], (size_t)transferred);

  switch (epInfo->usb_endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
      libusb_fill_interrupt_transfer(
          transfer, epInfo->deviceHandle, epInfo->usb_endpoint.bEndpointAddress, transferBuffer,
          transferred, cb_transfer_out, epInfo, 0);
      break;
    case LIBUSB_TRANSFER_TYPE_BULK:
      libusb_fill_bulk_transfer(
          transfer, epInfo->deviceHandle, epInfo->usb_endpoint.bEndpointAddress, transferBuffer,
          transferred, cb_transfer_out, epInfo, 0);
      break;
    default:
      PLOG_ERROR << "Unknown transfer type";
      free(transferBuffer);
      libusb_free_transfer(transfer);
      return;
  }
  transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

  epInfo->busyPackets.fetch_add(1);
  trackTransfer(epInfo, transfer);
  if (libusb_submit_transfer(transfer) != LIBUSB_SUCCESS) {
    epInfo->busyPackets.fetch_sub(1);
    untrackTransfer(epInfo, transfer);
    libusb_free_transfer(transfer);
    parentFromEndpoint(epInfo)->requestReconnect();
    return;
  }
}

void ep_in_work_isochronous(EndpointInfo* epInfo) {
  if (epInfo->busyPackets.load() >= 1) {
    usleep(epInfo->bIntervalInMicroseconds);
    return;
  }

  int num_iso_packets = 1;
  struct libusb_transfer* transfer = libusb_alloc_transfer(num_iso_packets);
  if (transfer == nullptr) {
    PLOG_ERROR << "libusb_alloc_transfer() failed for isochronous IN";
    return;
  }

  libusb_fill_iso_transfer(transfer, epInfo->deviceHandle, epInfo->usb_endpoint.bEndpointAddress,
                           epInfo->data, epInfo->usb_endpoint.wMaxPacketSize, num_iso_packets,
                           cb_transfer_in, epInfo, 0);
  libusb_set_iso_packet_lengths(transfer, epInfo->usb_endpoint.wMaxPacketSize / num_iso_packets);

  epInfo->busyPackets.fetch_add(1);
  trackTransfer(epInfo, transfer);
  if (libusb_submit_transfer(transfer) != LIBUSB_SUCCESS) {
    epInfo->busyPackets.fetch_sub(1);
    untrackTransfer(epInfo, transfer);
    libusb_free_transfer(transfer);
    parentFromEndpoint(epInfo)->requestReconnect();
  }
}

void ep_out_work_isochronous(EndpointInfo* epInfo) {
  if (epInfo->busyPackets.load() >= 128) {
    usleep(epInfo->bIntervalInMicroseconds);
    return;
  }

  struct usb_raw_int_io io;
  io.inner.ep = epInfo->ep_int;
  io.inner.flags = 0;
  io.inner.length = epInfo->usb_endpoint.wMaxPacketSize;
  if (io.inner.length > EP_MAX_PACKET_INT) {
    io.inner.length = EP_MAX_PACKET_INT;
  }

  int transferred = usb_raw_ep_read(epInfo->fd, (struct usb_raw_ep_io*)&io);
  if (transferred <= 0) {
    if (transferred < 0 && !rawGadgetIoInterrupted(errno)) {
      parentFromEndpoint(epInfo)->requestReconnect();
    }
    usleep(epInfo->bIntervalInMicroseconds);
    return;
  }

  int num_iso_packets = 1;
  struct libusb_transfer* transfer = libusb_alloc_transfer(num_iso_packets);
  if (transfer == nullptr) {
    PLOG_ERROR << "libusb_alloc_transfer() failed for isochronous OUT";
    return;
  }

  unsigned char* transferBuffer = (unsigned char*)malloc((size_t)transferred);
  if (transferBuffer == nullptr) {
    PLOG_ERROR << "Failed to allocate transfer buffer";
    libusb_free_transfer(transfer);
    return;
  }
  memcpy(transferBuffer, &io.inner.data[0], (size_t)transferred);

  libusb_fill_iso_transfer(transfer, epInfo->deviceHandle, epInfo->usb_endpoint.bEndpointAddress,
                           transferBuffer, transferred, num_iso_packets, cb_transfer_out, epInfo, 0);
  libusb_set_iso_packet_lengths(transfer, transferred / num_iso_packets);
  transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

  epInfo->busyPackets.fetch_add(1);
  trackTransfer(epInfo, transfer);
  if (libusb_submit_transfer(transfer) != LIBUSB_SUCCESS) {
    epInfo->busyPackets.fetch_sub(1);
    untrackTransfer(epInfo, transfer);
    libusb_free_transfer(transfer);
    parentFromEndpoint(epInfo)->requestReconnect();
  }
}
