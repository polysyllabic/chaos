#include "raw-gadget.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
#include <cstring>
#include <errno.h>

// The logger will be initialized in the main app. It's safe not to use plog at all. The library
// will still link correctly and there will simply be no logging.
#include <plog/Log.h>
#include <plog/Helpers/HexDump.h>

// The initialization below has been hard-coded for a raspberry pi 4. Run the following to discover
// what the settings are on other systems:
//     ls /sys/class/udc/

int RawGadgetPassthrough::initialize() {
  haveProductVendor = false;
  keepRunning = false;
  sessionRunning = false;
  libusbEventThreadStarted = false;

  vendor = 0;
  product = 0;
  fd = -1;
  devices = nullptr;
  deviceHandle = nullptr;

  mEndpointZeroInfo.bNumConfigurations = 0;
  mEndpointZeroInfo.activeConfiguration = -1;
  mEndpointZeroInfo.mConfigurationInfos = nullptr;
  mEndpointZeroInfo.fd = -1;
  mEndpointZeroInfo.dev_handle = nullptr;
  mEndpointZeroInfo.parent = this;

  if (context != nullptr) {
    libusb_exit(context);
    context = nullptr;
  }

  int r = libusb_init(&context);
  if (r < 0) {
    PLOG_ERROR << "libusb_init() Error " << r;
    return 1;
  }

  libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
  return 0;
}

int RawGadgetPassthrough::connectDevice() {
  if (context == nullptr) {
    PLOG_ERROR << "Cannot connect device: libusb context is null.";
    return 1;
  }
  if (fd < 0) {
    PLOG_ERROR << "Cannot connect device: raw gadget fd is invalid.";
    return 1;
  }

  ssize_t deviceCount = libusb_get_device_list(context, &devices);
  if (deviceCount < 0) {
    PLOG_ERROR << "Could not get libusb device list";
    return 1;
  }
  PLOG_VERBOSE << "Found " << deviceCount << " USB devices.";

  int devIndex = -1;
  for (int i = 0; i < deviceCount; i++) {
    PLOG_VERBOSE << "Device: " << i << " | Bus " << (int)libusb_get_bus_number(devices[i])
                 << " Port " << (int)libusb_get_port_number(devices[i]);
    // Specific for lower USB2 port on rPi 4 with raspbian
    if (libusb_get_bus_number(devices[i]) == 1 &&
        (int)libusb_get_port_number(devices[i]) == 4) {
      devIndex = i;
      PLOG_VERBOSE << " |-- This is the device of interest!";
    }
  }
  if (devIndex < 0) {
    libusb_free_device_list(devices, 1);
    devices = nullptr;
    PLOG_VERBOSE << "No controller currently attached to intercepted USB port.";
    return 1;
  }

  int openResult = libusb_open(devices[devIndex], &deviceHandle);
  libusb_free_device_list(devices, 1);
  devices = nullptr;
  if (openResult != LIBUSB_SUCCESS || deviceHandle == nullptr) {
    PLOG_ERROR << "Failed to open libusb device";
    return 1;
  }
  PLOG_VERBOSE << "USB device has been opened.";

  if (libusb_kernel_driver_active(deviceHandle, 0) == 1) {
    PLOG_VERBOSE << "Kernel driver active on interface 0";
    if (libusb_detach_kernel_driver(deviceHandle, 0) == 0) {
      PLOG_VERBOSE << "Kernel driver detached from interface 0";
    }
  }
  if (libusb_set_auto_detach_kernel_driver(deviceHandle, true) != LIBUSB_SUCCESS) {
    PLOG_ERROR << "Failed to perform libusb_set_auto_detach_kernel_driver().";
    cleanupDevice();
    return 1;
  }

  struct libusb_device_descriptor deviceDescriptor;
  if (libusb_get_device_descriptor(libusb_get_device(deviceHandle), &deviceDescriptor) != LIBUSB_SUCCESS) {
    PLOG_ERROR << "Failed to call libusb_get_device_descriptor().";
    cleanupDevice();
    return 1;
  }

  PLOG_VERBOSE << "Have Device Descriptor!";
  PLOG_VERBOSE << " - idVendor           : 0x" << std::hex << (int)deviceDescriptor.idVendor << std::dec;
  PLOG_VERBOSE << " - idProduct          : 0x" << std::hex << (int)deviceDescriptor.idProduct << std::dec;
  PLOG_VERBOSE << " - bNumConfigurations : " << (int)deviceDescriptor.bNumConfigurations;

  vendor = deviceDescriptor.idVendor;
  product = deviceDescriptor.idProduct;
  haveProductVendor = true;

  mEndpointZeroInfo.bNumConfigurations = deviceDescriptor.bNumConfigurations;
  mEndpointZeroInfo.activeConfiguration = -1;
  mEndpointZeroInfo.mConfigurationInfos = (ConfigurationInfo*)calloc(
      deviceDescriptor.bNumConfigurations, sizeof(ConfigurationInfo));
  mEndpointZeroInfo.fd = fd;
  mEndpointZeroInfo.dev_handle = deviceHandle;
  mEndpointZeroInfo.parent = this;

  if (mEndpointZeroInfo.mConfigurationInfos == nullptr && deviceDescriptor.bNumConfigurations > 0) {
    PLOG_ERROR << "Failed to allocate configuration info table.";
    cleanupDevice();
    return 1;
  }

  for (int configIndex = 0; configIndex < deviceDescriptor.bNumConfigurations; configIndex++) {
    struct libusb_config_descriptor* configDescriptor = nullptr;
    if (libusb_get_config_descriptor(libusb_get_device(deviceHandle), configIndex, &configDescriptor) !=
        LIBUSB_SUCCESS) {
      PLOG_ERROR << "Failed to get USB config descriptor";
      cleanupDevice();
      return 1;
    }

    ConfigurationInfo* configInfo = &mEndpointZeroInfo.mConfigurationInfos[configIndex];
    configInfo->parent = &mEndpointZeroInfo;
    configInfo->activeInterface = -1;
    configInfo->bNumInterfaces = configDescriptor->bNumInterfaces;
    configInfo->mInterfaceInfos = (InterfaceInfo*)calloc(configDescriptor->bNumInterfaces, sizeof(InterfaceInfo));
    if (configInfo->mInterfaceInfos == nullptr && configDescriptor->bNumInterfaces > 0) {
      libusb_free_config_descriptor(configDescriptor);
      PLOG_ERROR << "Failed to allocate interface info table.";
      cleanupDevice();
      return 1;
    }

    for (int i = 0; i < configDescriptor->bNumInterfaces; i++) {
      int numAlternates = configDescriptor->interface[i].num_altsetting;
      InterfaceInfo* interfaceInfo = &configInfo->mInterfaceInfos[i];
      interfaceInfo->activeAlternate = -1;
      interfaceInfo->bNumAlternates = numAlternates;
      interfaceInfo->mAlternateInfos = (AlternateInfo*)calloc(numAlternates, sizeof(AlternateInfo));
      interfaceInfo->parent = configInfo;
      if (interfaceInfo->mAlternateInfos == nullptr && numAlternates > 0) {
        libusb_free_config_descriptor(configDescriptor);
        PLOG_ERROR << "Failed to allocate alternate info table.";
        cleanupDevice();
        return 1;
      }

      if (numAlternates > 0) {
        int ifaceNumber = configDescriptor->interface[i].altsetting[0].bInterfaceNumber;
        int claimResult = libusb_claim_interface(deviceHandle, ifaceNumber);
        if (claimResult < 0) {
          libusb_free_config_descriptor(configDescriptor);
          PLOG_ERROR << "Cannot claim interface " << ifaceNumber << ": "
                     << libusb_error_name(claimResult);
          cleanupDevice();
          return 1;
        }
      }

      for (int a = 0; a < numAlternates; a++) {
        const struct libusb_interface_descriptor* interfaceDescriptor =
            &configDescriptor->interface[i].altsetting[a];
        AlternateInfo* alternateInfo = &interfaceInfo->mAlternateInfos[a];
        alternateInfo->bInterfaceNumber = interfaceDescriptor->bInterfaceNumber;
        alternateInfo->bNumEndpoints = interfaceDescriptor->bNumEndpoints;
        alternateInfo->mEndpointInfos =
            (EndpointInfo*)calloc(interfaceDescriptor->bNumEndpoints, sizeof(EndpointInfo));
        alternateInfo->parent = interfaceInfo;
        if (alternateInfo->mEndpointInfos == nullptr && interfaceDescriptor->bNumEndpoints > 0) {
          libusb_free_config_descriptor(configDescriptor);
          PLOG_ERROR << "Failed to allocate endpoint info table.";
          cleanupDevice();
          return 1;
        }

        PLOG_VERBOSE << " | - Interface " << (int)interfaceDescriptor->bInterfaceNumber
                     << " Alternate " << a;
        for (int e = 0; e < interfaceDescriptor->bNumEndpoints; e++) {
          const struct libusb_endpoint_descriptor* endpointDescriptor = &interfaceDescriptor->endpoint[e];
          EndpointInfo* endpointInfo = &alternateInfo->mEndpointInfos[e];
          endpointInfo->fd = fd;
          endpointInfo->ep_int = -1;
          endpointInfo->deviceHandle = deviceHandle;
          endpointInfo->keepRunning = false;
          endpointInfo->stop = true;
          endpointInfo->threadStarted = false;
          endpointInfo->busyPackets = 0;
          endpointInfo->usb_endpoint.bLength = endpointDescriptor->bLength;
          endpointInfo->usb_endpoint.bDescriptorType = endpointDescriptor->bDescriptorType;
          endpointInfo->usb_endpoint.bEndpointAddress = endpointDescriptor->bEndpointAddress;
          endpointInfo->usb_endpoint.bmAttributes = endpointDescriptor->bmAttributes;
          endpointInfo->usb_endpoint.wMaxPacketSize = endpointDescriptor->wMaxPacketSize;
          endpointInfo->usb_endpoint.bInterval = endpointDescriptor->bInterval;
          endpointInfo->bIntervalInMicroseconds = pow(2, endpointDescriptor->bInterval) * 125;
          size_t maxPacket = endpointDescriptor->wMaxPacketSize > 0 ? endpointDescriptor->wMaxPacketSize : 1;
          endpointInfo->data = (unsigned char*)malloc(maxPacket * sizeof(unsigned char));
          endpointInfo->parent = alternateInfo;
          if (endpointInfo->data == nullptr) {
            libusb_free_config_descriptor(configDescriptor);
            PLOG_ERROR << "Failed to allocate endpoint transfer buffer.";
            cleanupDevice();
            return 1;
          }
        }
      }
    }
    libusb_free_config_descriptor(configDescriptor);
  }

  return 0;
}

void RawGadgetPassthrough::teardownActiveEndpoints() {
  if (mEndpointZeroInfo.mConfigurationInfos == nullptr) {
    return;
  }

  for (int c = 0; c < mEndpointZeroInfo.bNumConfigurations; c++) {
    ConfigurationInfo* configInfo = &mEndpointZeroInfo.mConfigurationInfos[c];
    for (int i = 0; i < configInfo->bNumInterfaces; i++) {
      InterfaceInfo* interfaceInfo = &configInfo->mInterfaceInfos[i];
      for (int a = 0; a < interfaceInfo->bNumAlternates; a++) {
        AlternateInfo* alternateInfo = &interfaceInfo->mAlternateInfos[a];
        for (int e = 0; e < alternateInfo->bNumEndpoints; e++) {
          EndpointInfo* endpointInfo = &alternateInfo->mEndpointInfos[e];
          endpointInfo->stop = true;
          endpointInfo->keepRunning = false;
          if (endpointInfo->threadStarted) {
            pthread_join(endpointInfo->thread, NULL);
            endpointInfo->threadStarted = false;
          }
          if (endpointInfo->ep_int >= 0 && fd >= 0) {
            int endpointHandle = endpointInfo->ep_int;
            endpointInfo->ep_int = -1;
            int ret = usb_raw_ep_disable(fd, endpointHandle);
            if (ret < 0) {
              PLOG_WARNING << "Failed to disable endpoint handle " << endpointHandle;
            }
          }
          endpointInfo->fd = -1;
          endpointInfo->busyPackets = 0;
        }
      }
    }
  }
}

void RawGadgetPassthrough::cleanupDevice() {
  teardownActiveEndpoints();

  if (deviceHandle != nullptr && mEndpointZeroInfo.mConfigurationInfos != nullptr) {
    bool released[256] = {false};
    for (int c = 0; c < mEndpointZeroInfo.bNumConfigurations; c++) {
      ConfigurationInfo* configInfo = &mEndpointZeroInfo.mConfigurationInfos[c];
      for (int i = 0; i < configInfo->bNumInterfaces; i++) {
        InterfaceInfo* interfaceInfo = &configInfo->mInterfaceInfos[i];
        if (interfaceInfo->bNumAlternates <= 0 || interfaceInfo->mAlternateInfos == nullptr) {
          continue;
        }
        int ifaceNum = interfaceInfo->mAlternateInfos[0].bInterfaceNumber;
        if (ifaceNum < 0 || ifaceNum > 255 || released[ifaceNum]) {
          continue;
        }
        int releaseResult = libusb_release_interface(deviceHandle, ifaceNum);
        if (releaseResult < 0) {
          PLOG_VERBOSE << "Failed to release interface " << ifaceNum
                       << ": " << libusb_error_name(releaseResult);
        }
        released[ifaceNum] = true;
      }
    }
  }

  if (mEndpointZeroInfo.mConfigurationInfos != nullptr) {
    for (int c = 0; c < mEndpointZeroInfo.bNumConfigurations; c++) {
      ConfigurationInfo* configInfo = &mEndpointZeroInfo.mConfigurationInfos[c];
      if (configInfo->mInterfaceInfos == nullptr) {
        continue;
      }
      for (int i = 0; i < configInfo->bNumInterfaces; i++) {
        InterfaceInfo* interfaceInfo = &configInfo->mInterfaceInfos[i];
        if (interfaceInfo->mAlternateInfos == nullptr) {
          continue;
        }
        for (int a = 0; a < interfaceInfo->bNumAlternates; a++) {
          AlternateInfo* alternateInfo = &interfaceInfo->mAlternateInfos[a];
          if (alternateInfo->mEndpointInfos == nullptr) {
            continue;
          }
          for (int e = 0; e < alternateInfo->bNumEndpoints; e++) {
            EndpointInfo* endpointInfo = &alternateInfo->mEndpointInfos[e];
            free(endpointInfo->data);
            endpointInfo->data = nullptr;
          }
          free(alternateInfo->mEndpointInfos);
          alternateInfo->mEndpointInfos = nullptr;
        }
        free(interfaceInfo->mAlternateInfos);
        interfaceInfo->mAlternateInfos = nullptr;
      }
      free(configInfo->mInterfaceInfos);
      configInfo->mInterfaceInfos = nullptr;
    }
    free(mEndpointZeroInfo.mConfigurationInfos);
    mEndpointZeroInfo.mConfigurationInfos = nullptr;
  }

  mEndpointZeroInfo.bNumConfigurations = 0;
  mEndpointZeroInfo.activeConfiguration = -1;

  if (devices != nullptr) {
    libusb_free_device_list(devices, 1);
    devices = nullptr;
  }
  if (deviceHandle != nullptr) {
    libusb_close(deviceHandle);
    deviceHandle = nullptr;
  }
  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
  mEndpointZeroInfo.fd = -1;
  mEndpointZeroInfo.dev_handle = nullptr;
  sessionRunning = false;
}

void RawGadgetPassthrough::setEndpoint(AlternateInfo* info, int endpoint, bool enable) {
  EndpointInfo* endpointInfo = &info->mEndpointInfos[endpoint];
  
  if (enable) {
    PLOG_VERBOSE << "Attempting to enable EP 0x" << std::hex << (int) endpointInfo->usb_endpoint.bEndpointAddress;
    endpointInfo->fd = fd;
    endpointInfo->deviceHandle = deviceHandle;
    endpointInfo->ep_int = usb_raw_ep_enable(endpointInfo->fd, &endpointInfo->usb_endpoint);
    if (endpointInfo->ep_int < 0) {
      PLOG_ERROR << "Failed to enable endpoint for passthrough.";
      requestReconnect();
      return;
    }
    endpointInfo->stop = false;
    endpointInfo->keepRunning = true;
      
    if (pthread_create(&endpointInfo->thread, NULL, epLoopThread, endpointInfo) == 0) {
      endpointInfo->threadStarted = true;
    } else {
      endpointInfo->keepRunning = false;
      endpointInfo->stop = true;
      endpointInfo->ep_int = -1;
      PLOG_ERROR << "Failed to start endpoint thread.";
      requestReconnect();
      return;
    }
      
    } else {  // may need mutex here
      endpointInfo->stop = true;
      endpointInfo->keepRunning = false;
      if (endpointInfo->threadStarted) {
        pthread_join(endpointInfo->thread, NULL);
        endpointInfo->threadStarted = false;
      }
      
      if (endpointInfo->ep_int >= 0) {
        int temp = endpointInfo->ep_int;
        endpointInfo->ep_int = -1;
        PLOG_VERBOSE << "Attempting to disable EP with: 0x" << std::hex << temp << std::dec;
        int ret = usb_raw_ep_disable(endpointInfo->fd, temp);
        PLOG_VERBOSE << "usb_raw_ep_disable returned " << ret;
      }
    }
  PLOG_VERBOSE << " ---- 0x" << std::hex << (int) endpointInfo->usb_endpoint.bEndpointAddress << std::dec
    << " ep_int = " << endpointInfo->ep_int;
}

void RawGadgetPassthrough::setAlternate(InterfaceInfo* info, int alternate) {
  AlternateInfo* alternateInfo = &info->mAlternateInfos[alternate];
  if (alternate >= 0) {
    alternateInfo = &info->mAlternateInfos[alternate];
  } else {
    alternateInfo = &info->mAlternateInfos[info->activeAlternate];
  }
  
  if (info->activeAlternate != alternate &&
    info->activeAlternate >= 0 &&
    alternate >= 0) {
    PLOG_VERBOSE << "Need to disable current Alternate " << info->activeAlternate;  // TODO;
    for (int i = 0; i < info->mAlternateInfos[info->activeAlternate].bNumEndpoints; i++) {
      PLOG_VERBOSE << " - - | setEndpoint(?, " << i << ", false)";
      this->setEndpoint(&info->mAlternateInfos[info->activeAlternate], i, false);
    }
  }
  for (int i = 0; i < alternateInfo->bNumEndpoints; i++) {
    PLOG_VERBOSE << " - - | setEndpoint(?, " << i << ", " << (alternate >= 0 ? "true" : "false") << ")";
    this->setEndpoint(alternateInfo, i, alternate >= 0 ? true : false);
  }
  info->activeAlternate = alternate;
}

void RawGadgetPassthrough::setInterface( ConfigurationInfo* info, int interface, int alternate) {
  InterfaceInfo* interfaceInfo = &info->mInterfaceInfos[interface];
  
  if (info->activeInterface != interface &&  info->activeInterface >= 0 &&  alternate > 0) {
    //PLOG_VERBOSE << "Need to disable current Interface of " << info->activeInterface << "," << info->mInterfaceInfos[info->activeInterface].activeAlternate;
    //setAlternate(&info->mInterfaceInfos[info->activeInterface], -1);
  }
  
  PLOG_VERBOSE << "setAlternate(?, " << alternate << ")";
  this->setAlternate(interfaceInfo, alternate);
  info->activeInterface = interface;
  if (alternate >= 0) {
    if(libusb_set_interface_alt_setting(mEndpointZeroInfo.dev_handle, interface, alternate ) != LIBUSB_SUCCESS)  {
      PLOG_ERROR << "Could not set libusb interface alt setting";
    }
  }
}

void RawGadgetPassthrough::setConfiguration( int configuration) {
  ConfigurationInfo* configInfo = &mEndpointZeroInfo.mConfigurationInfos[configuration];
  
  if (mEndpointZeroInfo.activeConfiguration != configuration &&
    mEndpointZeroInfo.activeConfiguration >= 0 &&
    configuration >= 0) {
    PLOG_VERBOSE << "Need to disable current configuration!";
    for (int i = 0; i < mEndpointZeroInfo.mConfigurationInfos[mEndpointZeroInfo.activeConfiguration].bNumInterfaces; i++) {
      this->setInterface( &mEndpointZeroInfo.mConfigurationInfos[mEndpointZeroInfo.activeConfiguration], i, -1);  // unsure if this is needed in set config
    }
  }
  
  for (int i = 0; i < configInfo->bNumInterfaces; i++) {
    PLOG_VERBOSE << "setInterface(?, " << i << ", 0)";
    this->setInterface(configInfo, i, 0);  // unsure if this is needed in set config
  }
  mEndpointZeroInfo.activeConfiguration = configuration;
}

bool RawGadgetPassthrough::ep0Request(RawGadgetPassthrough* mRawGadgetPassthrough, struct usb_raw_control_event *event,
         struct usb_raw_control_io *io, bool *done) {
  
  EndpointZeroInfo* info = &mRawGadgetPassthrough->mEndpointZeroInfo;
  
  io->inner.length = event->ctrl.wLength;
  
  if ((event->ctrl.bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
    switch(event->ctrl.bRequest) {
      case USB_REQ_SET_CONFIGURATION:
        // The lower byte of the wValue field specifies the desired configuration
        PLOG_VERBOSE << "Setting configuration to: " << (int) (event->ctrl.wValue & 0xff);
        
        // From https://usb.ktemkin.com usb_device_framework_chapter.pdf
        // 8. Based on the configuration information and how the USB device will be used, the host
        // assigns a configuration value to the device. The device is now in the Configured state
        // and all of the endpoints in this configuration have taken on their described
        // characteristics. The USB device may now draw the amount of VBUS power described in its
        // descriptor for the selected configuration. From the device’s point of view, it is now
        // ready for use.
        mRawGadgetPassthrough->setConfiguration( (event->ctrl.wValue & 0xff) -1);
        
        if (usb_raw_vbus_draw(info->fd, 0x32 * 5) < 0) { // TODO: grab from descriptor for passthrough
          mRawGadgetPassthrough->requestReconnect();
          return false;
        }
        if (usb_raw_configure(info->fd) < 0) {
          mRawGadgetPassthrough->requestReconnect();
          return false;
        }
        break;
      case USB_REQ_SET_INTERFACE:
        PLOG_VERBOSE << "Setting Interface to: " << event->ctrl.wIndex << " Alternate: " <<  event->ctrl.wValue;
        mRawGadgetPassthrough->setInterface(&info->mConfigurationInfos[info->activeConfiguration], event->ctrl.wIndex,  event->ctrl.wValue);
        break;
      default:
        break;
    }
  }
  return true;
}

#ifdef FAKE_DATA
unsigned char nerfedDualshock[] = {
  0x09,        // bLength
  0x02,        // bDescriptorType (Configuration)
  0x29, 0x00,  // wTotalLength 41
  0x01,        // bNumInterfaces 1
  0x01,        // bConfigurationValue
  0x00,        // iConfiguration (String Index)
  0xC0,        // bmAttributes Self Powered
  0xFA,        // bMaxPower 500mA

  0x09,        // bLength
  0x04,        // bDescriptorType (Interface)
  0x00,        // bInterfaceNumber 0
  0x00,        // bAlternateSetting
  0x02,        // bNumEndpoints 2
  0x03,        // bInterfaceClass
  0x00,        // bInterfaceSubClass
  0x00,        // bInterfaceProtocol
  0x00,        // iInterface (String Index)

  0x09,        // bLength
  0x21,        // bDescriptorType (HID)
  0x11, 0x01,  // bcdHID 1.11
  0x00,        // bCountryCode
  0x01,        // bNumDescriptors
  0x22,        // bDescriptorType[0] (HID)
  0xFB, 0x01,  // wDescriptorLength[0] 507

  0x07,        // bLength
  0x05,        // bDescriptorType (Endpoint)
  0x84,        // bEndpointAddress (IN/D2H)
  0x03,        // bmAttributes (Interrupt)
  0x40, 0x00,  // wMaxPacketSize 64
  0x05,        // bInterval 5 (unit depends on device speed)

  0x07,        // bLength
  0x05,        // bDescriptorType (Endpoint)
  0x03,        // bEndpointAddress (OUT/H2D)
  0x03,        // bmAttributes (Interrupt)
  0x40, 0x00,  // wMaxPacketSize 64
  0x05,        // bInterval 5 (unit depends on device speed)

  // 41 bytes
};
#endif

bool RawGadgetPassthrough::ep0Loop( void* rawgadgetobject) {
  RawGadgetPassthrough* mRawGadgetPassthrough = (RawGadgetPassthrough*) rawgadgetobject;
  EndpointZeroInfo* info = &mRawGadgetPassthrough->mEndpointZeroInfo;
  bool done = false;
  struct usb_raw_control_event event;
  event.inner.type = 0;
  event.inner.length = sizeof(event.ctrl);
  
  if (usb_raw_event_fetch(info->fd, (struct usb_raw_event *)&event) < 0) {
    mRawGadgetPassthrough->requestReconnect();
    return false;
  }
  log_event((struct usb_raw_event *)&event);
  
  switch (event.inner.type) {
    case USB_RAW_EVENT_CONNECT:
      PLOG_VERBOSE << "ep0Loop(): Recieved a USB_RAW_EVENT_CONNECT";
      if (!process_eps_info(info)) {
        mRawGadgetPassthrough->requestReconnect();
      }
      return false;
      break;
      
    case USB_RAW_EVENT_CONTROL:
      break;  // continue for processing
      
    default:
      PLOG_ERROR << "event.inner.type != USB_RAW_EVENT_CONTROL, event.inner.type = " << event.inner.type;
      return false;
      break;
  }
  
  struct usb_raw_control_io io;
  io.inner.ep = 0;
  io.inner.flags = 0;
  io.inner.length = 0;
  
  bool reply = ep0Request( mRawGadgetPassthrough, &event, &io, &done);
  if (!reply) {
    PLOG_ERROR << "ep0: stalling";
    if (usb_raw_ep0_stall(info->fd) < 0) {
      mRawGadgetPassthrough->requestReconnect();
    }
    return false;
  }
  
  if (event.ctrl.wLength < io.inner.length)
    io.inner.length = event.ctrl.wLength;
  int rv = -1;
  if (event.ctrl.bRequestType & USB_DIR_IN) {
    PLOG_VERBOSE << "copying " << event.ctrl.wLength << " bytes";
#ifndef FAKE_DATA    
    rv = libusb_control_transfer(info->dev_handle,
              event.ctrl.bRequestType,
              event.ctrl.bRequest,
              event.ctrl.wValue,
              event.ctrl.wIndex,
              (unsigned char*)&io.data[0],
              event.ctrl.wLength,
              0);
    if (rv < 0) {
      PLOG_ERROR << "libusb_control_transfer error: " << libusb_error_name(rv);
      PLOG_ERROR << "ep0: stalling";
      usb_raw_ep0_stall(info->fd);
      mRawGadgetPassthrough->requestReconnect();
      return false;
    }
#else
    event.ctrl.bRequest == 0x6 &&
    event.ctrl.bRequestType == 0x80 &&
    event.ctrl.wValue == 0x200 &&
    event.ctrl.wIndex == 0x0 ) {
    PLOG_VERBOSE <<  "FAKING THE DATA!";
    memcpy(&io.data[0], nerfedDualshock, event.ctrl.wLength);
    rv = event.ctrl.wLength;
#endif
    io.inner.length = rv;
    rv = usb_raw_ep0_write(info->fd, (struct usb_raw_ep_io *)&io);
    if (rv < 0) {
      mRawGadgetPassthrough->requestReconnect();
      return false;
    }
    PLOG_VERBOSE << "ep0: transferred " << rv << " bytes (in: DEVICE -> HOST)";
  } else {
    rv = usb_raw_ep0_read(info->fd, (struct usb_raw_ep_io *)&io);
    if (rv < 0) {
      mRawGadgetPassthrough->requestReconnect();
      return false;
    }
    PLOG_VERBOSE << "ep0: transferred " << rv << " bytes (out: HOST -> DEVICE)";
    
    int r = libusb_control_transfer(info->dev_handle,
                    event.ctrl.bRequestType,
                    event.ctrl.bRequest,
                    event.ctrl.wValue,
                    event.ctrl.wIndex,
                    (unsigned char*)&io.data[0],
                    io.inner.length,
                    0);
    
    if (r < 0) {
      PLOG_ERROR << "libusb_control_transfer() returned < 0 in ep0Loop(). r = " << r;
      mRawGadgetPassthrough->requestReconnect();
      return false;
    }
  }
  PLOG_VERBOSE << "data: " << plog::hexdump(&io.inner.data[0], io.inner.length);
  return done;
}

void* RawGadgetPassthrough::ep0LoopThread( void* rawgadgetobject ) {
  RawGadgetPassthrough* mRawGadgetPassthrough = (RawGadgetPassthrough*) rawgadgetobject;
  
  while (mRawGadgetPassthrough->keepRunning && mRawGadgetPassthrough->sessionRunning) {
    ep0Loop(mRawGadgetPassthrough);
  }
  return NULL;
}

void* RawGadgetPassthrough::libusbEventHandler( void* rawgadgetobject ) {
  RawGadgetPassthrough* mRawGadgetPassthrough = (RawGadgetPassthrough*) rawgadgetobject;
  
  // The below has been hard-coded for a raspberry pi 4, run the following to find out on other systems:
  // ls /sys/class/udc/
  //
  const char *device = "fe980000.usb";//dummy_udc.0";
  const char *driver = "fe980000.usb";//dummy_udc";
  
  bool ep0ThreadStarted = false;
  while (mRawGadgetPassthrough->keepRunning) {
    if (!mRawGadgetPassthrough->sessionRunning) {
      mRawGadgetPassthrough->cleanupDevice();
      mRawGadgetPassthrough->fd = usb_raw_open();
      if (mRawGadgetPassthrough->fd < 0) {
        usleep(250000);
        continue;
      }
      mRawGadgetPassthrough->mEndpointZeroInfo.fd = mRawGadgetPassthrough->fd;

      if (mRawGadgetPassthrough->connectDevice() != 0) {
        mRawGadgetPassthrough->cleanupDevice();
        usleep(250000);
        continue;
      }

      // raw-gadget fun
      PLOG_VERBOSE << "Starting raw-gadget";
      if (usb_raw_init(mRawGadgetPassthrough->fd, USB_SPEED_HIGH, driver, device) < 0 ||
          usb_raw_run(mRawGadgetPassthrough->fd) < 0) {
        mRawGadgetPassthrough->cleanupDevice();
        usleep(250000);
        continue;
      }

      // Start ep0 thread after raw-gadget setup.
      PLOG_VERBOSE << "Starting ep0 thread";
      mRawGadgetPassthrough->sessionRunning = true;
      if (pthread_create(&mRawGadgetPassthrough->threadEp0, NULL, ep0LoopThread, mRawGadgetPassthrough) != 0) {
        PLOG_ERROR << "Failed to create ep0 thread";
        mRawGadgetPassthrough->sessionRunning = false;
        mRawGadgetPassthrough->cleanupDevice();
        usleep(250000);
        continue;
      }
      ep0ThreadStarted = true;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    int eventResult = libusb_handle_events_timeout(mRawGadgetPassthrough->context, &timeout);
    if (eventResult != LIBUSB_SUCCESS && eventResult != LIBUSB_ERROR_TIMEOUT) {
      PLOG_ERROR << "libusb_handle_events_timeout() failed: " << libusb_error_name(eventResult);
      mRawGadgetPassthrough->requestReconnect();
    }

    if (!mRawGadgetPassthrough->sessionRunning) {
      if (ep0ThreadStarted) {
        pthread_join(mRawGadgetPassthrough->threadEp0, NULL);
        ep0ThreadStarted = false;
      }
      mRawGadgetPassthrough->cleanupDevice();
      usleep(100000);
    }
  }

  mRawGadgetPassthrough->requestReconnect();
  if (ep0ThreadStarted) {
    pthread_join(mRawGadgetPassthrough->threadEp0, NULL);
  }
  mRawGadgetPassthrough->cleanupDevice();
  return NULL;
}

void RawGadgetPassthrough::start() {
  if (libusbEventThreadStarted) {
    return;
  }
  keepRunning = true;
  
  PLOG_VERBOSE << "Starting libusb Event Thread";
  if (pthread_create(&libusbEventThread, NULL, libusbEventHandler, this) == 0) {
    libusbEventThreadStarted = true;
  } else {
    keepRunning = false;
    PLOG_ERROR << "Failed to start libusb Event Thread";
  }
}

void RawGadgetPassthrough::stop() {
  keepRunning = false;
  requestReconnect();
  if (libusbEventThreadStarted) {
    pthread_join(libusbEventThread, NULL);
    libusbEventThreadStarted = false;
  }
  cleanupDevice();
  if (context != nullptr) {
    libusb_exit(context);
    context = nullptr;
  }
}

void RawGadgetPassthrough::addObserver(EndpointObserver* observer) {
  this->observers.push_back( observer );
}

void* RawGadgetPassthrough::epLoopThread( void* data ) {
  EndpointInfo *ep = (EndpointInfo*)data;
  
  RawGadgetPassthrough* mRawGadgetPassthrough = ep->parent->parent->parent->parent->parent;

  PLOG_VERBOSE << "Starting thread for endpoint 0x" << std::hex << (int) ep->usb_endpoint.bEndpointAddress;
  int idleDelay = 1000000;
  int idleCount = 0;
  bool priorenable = false;
  while ((ep->keepRunning && mRawGadgetPassthrough->sessionRunning) || (ep->busyPackets > 0)) {
    if (ep->ep_int >= 0 && !ep->stop) {
      if (priorenable == false &&
        (ep->usb_endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
        priorenable = true;
        usleep(1000000);
      }
      
      if (ep->usb_endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) {  // data in
        switch (ep->usb_endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
          case LIBUSB_TRANSFER_TYPE_INTERRUPT:
          case LIBUSB_TRANSFER_TYPE_BULK:
            mRawGadgetPassthrough->epDeviceToHostWorkInterrupt( ep );
            break;
          case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
            ep_in_work_isochronous( ep );
            break;
          case LIBUSB_TRANSFER_TYPE_CONTROL:
          default:
            PLOG_ERROR << "Unsupported ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK";
            usleep(1000);
            break;
        }
      } else { // data out
        switch (ep->usb_endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
          case LIBUSB_TRANSFER_TYPE_INTERRUPT:
          case LIBUSB_TRANSFER_TYPE_BULK:
            ep_out_work_interrupt( ep );
            break;
          case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
            ep_out_work_isochronous( ep );
            break;
          case LIBUSB_TRANSFER_TYPE_CONTROL:
          default:
            PLOG_ERROR << "Unsupported ep->bEndpointAddress";
            usleep(1000);
            break;
        }
      }
    } else {  // reaching here means we are simply cleaning things up
      idleCount++;
      if (idleCount > 1000000/idleDelay) {
        idleCount = 0;
        PLOG_VERBOSE << "Idle: Endpoint 0x" << std::hex << (int) ep->usb_endpoint.bEndpointAddress
          << " - ep->busyPackets=" << ep->busyPackets;
      }
      usleep(idleDelay);
    }
  }
  
  PLOG_VERBOSE << "Terminating thread for endpoint 0x" << std::hex << (int) ep->usb_endpoint.bEndpointAddress;
  return NULL;
}

void RawGadgetPassthrough::epDeviceToHostWorkInterrupt( EndpointInfo* epInfo ) {

  if (epInfo->busyPackets >= 1) {
    usleep(epInfo->bIntervalInMicroseconds);
    return;
  }
  epInfo->busyPackets++;
  struct libusb_transfer *transfer;
  transfer = libusb_alloc_transfer(0);
  if (transfer == NULL) {
    PLOG_ERROR << "libusb_alloc_transfer(0) no memory";
  }
  switch(epInfo->usb_endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
    case LIBUSB_TRANSFER_TYPE_INTERRUPT:
      libusb_fill_interrupt_transfer(  transfer,
                       epInfo->deviceHandle,
                       epInfo->usb_endpoint.bEndpointAddress,
                       epInfo->data,
                       epInfo->usb_endpoint.wMaxPacketSize,
                       cbTransferIn,
                       epInfo,
                       0 );
      break;
    case LIBUSB_TRANSFER_TYPE_BULK:  // TODO: need to accounf fo bulk streams maybe
      libusb_fill_bulk_transfer(  transfer,
                       epInfo->deviceHandle,
                       epInfo->usb_endpoint.bEndpointAddress,
                       epInfo->data,
                       epInfo->usb_endpoint.wMaxPacketSize,
                       cbTransferIn,
                       epInfo,
                       0 );
      
      break;
    default:
      PLOG_ERROR << "Unknopwn transfer type";
      return;
  }

  if(libusb_submit_transfer(transfer) != LIBUSB_SUCCESS) {
    PLOG_ERROR << "libusb_submit_transfer(transfer) failed";
    epInfo->busyPackets--;
    libusb_free_transfer(transfer);
    requestReconnect();
    return;
  }
}

void RawGadgetPassthrough::cbTransferIn(struct libusb_transfer *xfr) {
  EndpointInfo* epInfo = (EndpointInfo*)xfr->user_data;
  RawGadgetPassthrough* mRawGadgetPassthrough = epInfo->parent->parent->parent->parent->parent;
  if (xfr->status != LIBUSB_TRANSFER_COMPLETED) {
    PLOG_ERROR << "Transfer status " << xfr->status;
    epInfo->busyPackets--;
    libusb_free_transfer(xfr);
    if (xfr->status == LIBUSB_TRANSFER_NO_DEVICE || xfr->status == LIBUSB_TRANSFER_ERROR) {
      mRawGadgetPassthrough->requestReconnect();
    }
    return;
  }
  
  struct usb_raw_int_io io;
  io.inner.ep = epInfo->ep_int;
  io.inner.flags = 0;
  
  if (xfr->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
    for (int i = 0; i < xfr->num_iso_packets; i++) {
      struct libusb_iso_packet_descriptor *pack = &xfr->iso_packet_desc[i];
      
      if (pack->status != LIBUSB_TRANSFER_COMPLETED) {
        PLOG_ERROR << "pack " << i << " status " << pack->status;
        continue;
      }
      
      io.inner.length = pack->actual_length;//0;//epInfo->wMaxPacketSize;
      
      memcpy(&io.inner.data[0], xfr->buffer, pack->actual_length);
      
      int rv = pack->actual_length;
      if (rv < 0) {
        PLOG_ERROR << "iso write to host  usb_raw_ep_write() returned " << rv;
      } else if (rv != pack->actual_length) {
        PLOG_WARNING << "Only sent " << rv << " bytes instead of " << pack->actual_length;
      }
    }
  } else {
    io.inner.length = xfr->actual_length;//0;//epInfo->wMaxPacketSize;
    
    memcpy(&io.inner.data[0], xfr->buffer, xfr->actual_length);
    
    for (std::vector<EndpointObserver*>::iterator it = mRawGadgetPassthrough->observers.begin();
       it != mRawGadgetPassthrough->observers.end();
       it++) {
      EndpointObserver* observer = *it;
      
      if (observer->getEndpoint() == epInfo->usb_endpoint.bEndpointAddress) {
        observer->notification(&io.inner.data[0], xfr->actual_length);
      }
    }
    
    int rv = usb_raw_ep_write(epInfo->fd, (struct usb_raw_ep_io *)&io);
    if (rv < 0) {
      if (errno != ETIMEDOUT) {
        PLOG_ERROR << "bulk/interrupt write to host  usb_raw_ep_write() returned " << rv;
        mRawGadgetPassthrough->requestReconnect();
      }
      
    } else if (rv != xfr->actual_length) {
      PLOG_WARNING << "Only sent " << rv << " bytes instead of " << xfr->actual_length;
    }
  }
  
  epInfo->busyPackets--;
  libusb_free_transfer(xfr);
}

void RawGadgetPassthrough::requestReconnect() {
  bool wasRunning = sessionRunning.exchange(false);
  if (fd >= 0) {
    close(fd);
    fd = -1;
    mEndpointZeroInfo.fd = -1;
  }
  if (wasRunning) {
    PLOG_WARNING << "Controller transport interrupted. Waiting for reconnect.";
  }
}

bool RawGadgetPassthrough::readyProductVendor() {
  return haveProductVendor;
}

int RawGadgetPassthrough::getVendor() {
  return vendor;
}

int RawGadgetPassthrough::getProduct() {
  return product;
}
