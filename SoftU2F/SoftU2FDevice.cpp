//
//  SoftU2FDevice.cpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "SoftU2FDevice.hpp"
#include "SoftU2FUserClient.hpp"
#include <IOKit/IOLib.h>

#define super IOHIDDevice
OSDefineMetaClassAndStructors(com_github_SoftU2FDevice, IOHIDDevice)

void SoftU2FDeviceClassName::free() {
  if (dUserClient) {
    dUserClient->release();
    dUserClient = nullptr;
  }

  super::free();
}

IOReturn SoftU2FDeviceClassName::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
  IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withBytes(u2fhid_report_descriptor, sizeof(u2fhid_report_descriptor), kIODirectionNone);
  if (!buffer)
    return kIOReturnNoResources;

  *descriptor = buffer;

  return kIOReturnSuccess;
}

IOReturn SoftU2FDeviceClassName::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
  if (dUserClient)
    dUserClient->frameReceived(report);

  // Sleep for a bit to make the HID conformance tests happy.
  IOSleep(1); // 1ms

  return kIOReturnSuccess;
}

OSString *SoftU2FDeviceClassName::newProductString() const {
  return OSString::withCString("SoftU2F");
}

OSString *SoftU2FDeviceClassName::newSerialNumberString() const {
  return OSString::withCString("123");
}

OSNumber *SoftU2FDeviceClassName::newVendorIDNumber() const {
  return OSNumber::withNumber(123, 32);
}

OSNumber *SoftU2FDeviceClassName::newProductIDNumber() const {
  return OSNumber::withNumber(123, 32);
}

OSNumber *SoftU2FDeviceClassName::newVersionNumber() const {
  return OSNumber::withNumber(1, 32);
}

OSNumber* SoftU2FDeviceClassName::newPrimaryUsagePageNumber() const {
  return OSNumber::withNumber(0x0F1D0, 32);
}

OSNumber* SoftU2FDeviceClassName::newPrimaryUsageNumber() const {
  return OSNumber::withNumber(0x01, 32);
}

bool SoftU2FDeviceClassName::setUserClient(IOService *userClient) {
  dUserClient = OSDynamicCast(SoftU2FUserClientClassName, userClient);
  
  if (!dUserClient)
    return false;

  dUserClient->retain();
  return true;
}
