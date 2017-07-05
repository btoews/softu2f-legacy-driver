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
OSDefineMetaClassAndStructors(SoftU2FDevice, IOHIDDevice)

void SoftU2FDevice::free() {
  if (dUserClient) {
    dUserClient->release();
  }

  super::free();
}

IOReturn SoftU2FDevice::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
  IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withBytes(u2fhid_report_descriptor, sizeof(u2fhid_report_descriptor), kIODirectionNone);
  if (!buffer)
    return kIOReturnNoResources;

  *descriptor = buffer;

  return kIOReturnSuccess;
}

IOReturn SoftU2FDevice::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
  if (dUserClient)
    dUserClient->frameReceived(report);

  // Sleep for a bit to make the HID conformance tests happy.
  IOSleep(1); // 1ms

  return kIOReturnSuccess;
}

OSString *SoftU2FDevice::newProductString() const {
  return OSString::withCString("SoftU2F");
}

OSString *SoftU2FDevice::newSerialNumberString() const {
  return OSString::withCString("123");
}

OSNumber *SoftU2FDevice::newVendorIDNumber() const {
  return OSNumber::withNumber(123, 32);
}

OSNumber *SoftU2FDevice::newProductIDNumber() const {
  return OSNumber::withNumber(123, 32);
}

OSNumber* SoftU2FDevice::newPrimaryUsageNumber() const {
  return OSNumber::withNumber(kHIDUsage_PID_TriggerButton, 32);
}

bool SoftU2FDevice::setUserClient(IOService *userClient) {
  dUserClient = OSDynamicCast(SoftU2FUserClient, userClient);

  if (!dUserClient)
    return false;

  dUserClient->retain();
  return true;
}
