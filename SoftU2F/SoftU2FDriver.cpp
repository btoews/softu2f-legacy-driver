//
//  SoftU2FDriver.cpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.

#include <IOKit/IOLib.h>

#include "SoftU2FDevice.hpp"
#include "SoftU2FDriver.hpp"
#include "SoftU2FUserClient.hpp"

#define super IOService
OSDefineMetaClassAndStructors(SoftU2FDriver, IOService);

bool SoftU2FDriver::start(IOService *provider) {
  bool success;

  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  success = super::start(provider);

  if (success) {
    // Publish ourselves so clients can find us
    registerService();
  }

  return success;
}

void SoftU2FDriver::stop(IOService *provider) {
    IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  // Terminate and release every managed HID device.
  OSCollectionIterator *iter = OSCollectionIterator::withCollection(m_hid_devices);
  if (iter) {
    const char *key = nullptr;

    while ((key = (char *)iter->getNextObject())) {
      SoftU2FDevice *device = OSDynamicCast(SoftU2FDevice, m_hid_devices->getObject(key));

      if (device) {
        IOLog("Terminating device.");

        if (!device->isInactive() && !device->terminate())
          IOLog("Error terminating device.");

        device->release();
      }
    }

    iter->release();
  }

  super::stop(provider);
}

bool SoftU2FDriver::init(OSDictionary *dictionary) {
  if (!super::init(dictionary)) {
    return false;
  }

  // This IOLog must follow super::init because getName relies on the superclass
  // initialization.
  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, dictionary);

  // Setup HID devices dictionary.
  m_hid_devices = OSDictionary::withCapacity(1);
  if (!m_hid_devices) {
    IOLog("Unable to inizialize HID devices dictionary.");
    return false;
  }

  return true;
}

// We override free only to log that it has been called to make it easier to
// follow the driver's lifecycle.
void SoftU2FDriver::free(void) {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  // Clear the HID devices dictionary.
  if (m_hid_devices) {
    m_hid_devices->release();
  }

  super::free();
}

OSString *SoftU2FDriver::userClientKey(IOService *userClient) {
  char cKey[64]; // extra space, since %p format isn't guaranteed
  snprintf(cKey, 64, "%p", userClient);

  return OSString::withCString(cKey);
}

IOService *SoftU2FDriver::userClientDevice(IOService *userClient) {
  SoftU2FDevice *device = nullptr;

  OSString *key = userClientKey(userClient);
  if (!key)
    goto fail_key;

  device = OSDynamicCast(SoftU2FDevice, m_hid_devices->getObject(key));

  if (!device) {
    device = OSTypeAlloc(SoftU2FDevice);
    if (!device)
      goto fail_device;

    if (!device->init(nullptr))
      goto fail_device;

    if (!m_hid_devices->setObject(key, device))
      goto fail_device;

    if (!device->attach(this))
      goto fail_device;

    if (!device->start(this))
      goto fail_device;

    device->setUserClient(userClient);
  }

  return device;

fail_device:
  if (device)
    device->release();

fail_key:
  if (key)
    key->release();

  return nullptr;
}

bool SoftU2FDriver::destroyUserClientDevice(IOService *userClient) {
  SoftU2FDevice *device = nullptr;

  OSString *key = userClientKey(userClient);
  if (!key)
    goto fail_key;

  device = OSDynamicCast(SoftU2FDevice, m_hid_devices->getObject(key));
  if (!device)
    goto fail_device;

  if (!device->isInactive())
    device->terminate();

  m_hid_devices->removeObject(key);
  key->release();
  device->release();

  return true;

fail_device:
  if (device)
    device->release();

fail_key:
  if (key)
    key->release();

  return false;
}

bool SoftU2FDriver::userClientDeviceSend(IOService *userClient, U2FHID_FRAME *frame) {
  IOMemoryDescriptor *report = nullptr;
  SoftU2FDevice *device;
  bool ret = false;

  device = OSDynamicCast(SoftU2FDevice, userClientDevice(userClient));
  if (!device)
    return false;

  report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, HID_RPT_SIZE);
  if (!report)
    return false;

  report->writeBytes(0, frame, HID_RPT_SIZE);

  if (device->handleReport(report, kIOHIDReportTypeInput) == kIOReturnSuccess)
    ret = true;

  report->release();

  return ret;
}
