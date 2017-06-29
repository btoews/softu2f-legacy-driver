//
//  SoftU2FDevice.hpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#ifndef SoftU2FDevice_hpp
#define SoftU2FDevice_hpp

#include "SoftU2FUserClient.hpp"
#include <IOKit/hid/IOHIDDevice.h>

unsigned char const u2fhid_report_descriptor[] = {
    0x06, 0xD0, 0xF1, // Usage Page (Reserved 0xF1D0)
    0x09, 0x01,       // Usage (0x01)
    0xA1, 0x01,       // Collection (Application)
    0x09, 0x20,       //   Usage (0x20)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x40,       //   Report Count (64)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                      //   Position)
    0x09, 0x21,       //   Usage (0x21)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x40,       //   Report Count (64)
    0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                      //   Position,Non-volatile)
    0xC0,             // End Collection
};

#define SoftU2FDeviceClassName com_github_SoftU2FDevice

class SoftU2FDeviceClassName : public IOHIDDevice {
  OSDeclareDefaultStructors(com_github_SoftU2FDevice)

protected:
  SoftU2FUserClientClassName *dUserClient;

public:
  virtual bool init(OSDictionary *dictionary = 0) override;
  virtual bool start(IOService *provider) override;
  virtual void stop(IOService *provider) override;
  virtual void free(void) override;

  virtual OSString *newProductString() const override;
  virtual OSString *newSerialNumberString() const override;
  virtual OSNumber *newVendorIDNumber() const override;
  virtual OSNumber *newProductIDNumber() const override;
  virtual OSNumber *newPrimaryUsageNumber() const override;
  virtual IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;

  virtual IOReturn setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options = 0) override;

  virtual bool setUserClient(IOService *userClient);
};

#endif /* SoftU2FDevice_hpp */
