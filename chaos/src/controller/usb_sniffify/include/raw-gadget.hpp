#pragma once
#include <libusb.h>
#include <atomic>
#include <vector>

#include "raw-helper.h"

// Functions to help maintain the configuration state:
//void setEndpoint(AlternateInfo* info, int endpoint, bool enable);
//void setAlternate(InterfaceInfo* info, int alternate);
//void setInterface(libusb_device_handle *deviceHandle, ConfigurationInfo* info, int interface, int alternate);
//void setConfiguration(EndpointZeroInfo* info, int configuration);

//char dummyBuffer[4096];
//bool ep0_request(EndpointZeroInfo* info, struct usb_raw_control_event *event, struct usb_raw_control_io *io, bool *done);

//bool ep0_loop(EndpointZeroInfo* info);
//void* ep0_loop_thread( void* data );

//static void cb_transfer_out(struct libusb_transfer *xfr);
void ep_out_work_interrupt( EndpointInfo* epInfo );

//static void cb_transfer_in(struct libusb_transfer *xfr);
void ep_in_work_interrupt( EndpointInfo* epInfo );

void ep_in_work_isochronous( EndpointInfo* epInfo );
void ep_out_work_isochronous( EndpointInfo* epInfo );

void* ep_loop_thread( void* data );

class EndpointObserver {
  friend RawGadgetPassthrough;
public:
  void setEndpoint(unsigned char endpoint) {
    this->endpoint = endpoint;
  };
  char getEndpoint() {
    return endpoint;
  }
private:
  unsigned char endpoint;
protected:
  /*
   the buffer is specifically non-const so that is can be modified
   */
  virtual void notification(unsigned char* buffer, int length) = 0;

};

class RawGadgetPassthrough {

public:
  int initialize();
  
  /*
   Begins the passthrough structure
   */
  void start();
  
  /*
   Stops the passthrough structure
   */
  void stop();
  
  void addObserver(EndpointObserver* observer);
  
  
  bool readyProductVendor();
  int getVendor();
  int getProduct();
  void requestReconnect();
  
private:
  int product = 0;
  int vendor = 0;
  std::atomic<bool> haveProductVendor{false};
  std::vector<EndpointObserver*> observers;
  
  std::atomic<bool> keepRunning{false};
  std::atomic<bool> sessionRunning{false};
  bool libusbEventThreadStarted = false;
  pthread_t libusbEventThread;
  
  pthread_t threadEp0;
  
  int fd = -1;  // for ioctl raw_gadget
  EndpointZeroInfo mEndpointZeroInfo;
  pthread_t endpointThreads[60]; // crawl before I walk
  
  
  libusb_device **devices = nullptr;
  libusb_device_handle *deviceHandle = nullptr;
  libusb_context *context = nullptr;

  int connectDevice();
  void teardownActiveEndpoints();
  void cleanupDevice();
  
  void setEndpoint(AlternateInfo* info, int endpoint, bool enable);
  void setAlternate(InterfaceInfo* info, int alternate);
  void setInterface(ConfigurationInfo* info, int interface, int alternate);
  void setConfiguration( int configuration);
  
  static bool ep0Request(RawGadgetPassthrough* mRawGadgetPassthrough, struct usb_raw_control_event *event,
               struct usb_raw_control_io *io, bool *done);
  static bool ep0Loop(void* rawgadgetobject );
  static void* ep0LoopThread( void* rawgadgetobject );
  static void* libusbEventHandler( void* rawgadgetobject );
  static void* epLoopThread( void* rawgadgetobject );
  
  void epDeviceToHostWorkInterrupt( EndpointInfo* epInfo );
  static void cbTransferIn(struct libusb_transfer *xfr);
};
