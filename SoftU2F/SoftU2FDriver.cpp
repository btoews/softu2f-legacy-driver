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
OSDefineMetaClassAndStructors(com_github_SoftU2FDriver, IOService);

bool SoftU2FDriverClassName::start(IOService *provider) {
  bool success;

  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  success = super::start(provider);

  if (success) {
    // Publish ourselves so clients can find us
    registerService();
  }

  return success;
}

void SoftU2FDriverClassName::stop(IOService *provider) {
    IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  // Terminate and release every managed HID device.
  OSCollectionIterator *iter =
      OSCollectionIterator::withCollection(m_hid_devices);
  if (iter) {
    const char *key = nullptr;

    while ((key = (char *)iter->getNextObject())) {
      SoftU2FDeviceClassName *device =
          (SoftU2FDeviceClassName *)m_hid_devices->getObject(key);

      if (device) {
        IOLog("Terminating device.");
        device->terminate();
        device->release();
      }
    }

    iter->release();
  }

  super::stop(provider);
}

bool SoftU2FDriverClassName::init(OSDictionary *dictionary) {
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
void SoftU2FDriverClassName::free(void) {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  // Clear the HID devices dictionary.
  if (m_hid_devices) {
    m_hid_devices->release();
  }

  super::free();
}

OSString *SoftU2FDriverClassName::userClientKey(IOService *userClient) {
  char cKey[64]; // extra space, since %p format isn't guaranteed
  snprintf(cKey, 64, "%p", userClient);

  return OSString::withCString(cKey);
}

IOService *SoftU2FDriverClassName::userClientDevice(IOService *userClient) {
  SoftU2FDeviceClassName *device = nullptr;

  OSString *key = userClientKey(userClient);
  if (!key)
    goto fail;

  device = (SoftU2FDeviceClassName *)m_hid_devices->getObject(key);

  if (!device) {
    device = OSTypeAlloc(SoftU2FDeviceClassName);
    if (!device)
      goto fail;
    if (!device->init(nullptr))
      goto fail;
    if (!m_hid_devices->setObject(key, device))
      goto fail;

    device->attach(this);
    device->start(this);
    device->setUserClient(userClient);
  }

  return device;
fail:
  if (key)
    key->release();
  if (device)
    device->release();
  return nullptr;
}

bool SoftU2FDriverClassName::destroyUserClientDevice(IOService *userClient) {
  SoftU2FDeviceClassName *device = nullptr;

  OSString *key = userClientKey(userClient);
  if (!key)
    goto fail;

  device = (SoftU2FDeviceClassName *)m_hid_devices->getObject(key);
  if (!device)
    goto fail;

  device->terminate();
  m_hid_devices->removeObject(key);
  key->release();
  device->release();

  return true;
fail:
  if (device)
    device->release();
  if (key)
    key->release();
  return false;
}

bool SoftU2FDriverClassName::userClientDeviceSend(IOService *userClient, U2FHID_FRAME *frame) {
  IOMemoryDescriptor *report = nullptr;
  SoftU2FDeviceClassName *device;
  bool ret = false;

  device = (SoftU2FDeviceClassName *)userClientDevice(userClient);
  if (!device)
    return false;

  report =
      IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, HID_RPT_SIZE);
  if (!report)
    return false;

  report->writeBytes(0, frame, HID_RPT_SIZE);

  if (device->handleReport(report, kIOHIDReportTypeInput) == kIOReturnSuccess) {
    ret = true;
  }

  report->release();

  return ret;
}
