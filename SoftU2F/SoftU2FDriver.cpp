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

// We override stop only to log that it has been called to make it easier to
// follow the driver's lifecycle.
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

// We override init only to log that it has been called to make it easier to
// follow the driver's lifecycle.
// Production drivers would only need to override init if they want to
// initialize data members.
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

// We override probe only to log that it has been called to make it easier to
// follow the driver's lifecycle.
// Production drivers can override probe if they need to make an active decision
// whether the driver is appropriate for
// the provider.
IOService *SoftU2FDriverClassName::probe(IOService *provider, SInt32 *score) {
  IOLog("%s[%p]::%s(%p, %p)\n", getName(), this, __FUNCTION__, provider, score);

  IOService *res = super::probe(provider, score);

  return res;
}

// We override willTerminate only to log that it has been called to make it
// easier to follow the driver's lifecycle.
//
// willTerminate is called at the beginning of the termination process. It is a
// notification
// that a provider has been terminated, sent before recursing up the stack, in
// root-to-leaf order.
//
// This is where any pending I/O should be terminated. At this point the user
// client has been marked
// inactive and any further requests from the user process should be returned
// with an error.
bool SoftU2FDriverClassName::willTerminate(IOService *provider,
                                           IOOptionBits options) {
  IOLog("%s[%p]::%s(%p, %ld)\n", getName(), this, __FUNCTION__, provider,
        (long)options);

  return super::willTerminate(provider, options);
}

// We override didTerminate only to log that it has been called to make it
// easier to follow the driver's lifecycle.
//
// didTerminate is called at the end of the termination process. It is a
// notification
// that a provider has been terminated, sent after recursing up the stack, in
// leaf-to-root order.
bool SoftU2FDriverClassName::didTerminate(IOService *provider,
                                          IOOptionBits options, bool *defer) {
  IOLog("%s[%p]::%s(%p, %ld, %p)\n", getName(), this, __FUNCTION__, provider,
        (long)options, defer);

  return super::didTerminate(provider, options, defer);
}

// We override terminate only to log that it has been called to make it easier
// to follow the driver's lifecycle.
// Production drivers will rarely need to override terminate. Termination
// processing should be done in
// willTerminate or didTerminate instead.
bool SoftU2FDriverClassName::terminate(IOOptionBits options) {
  bool success;

  IOLog("%s[%p]::%s(%ld)\n", getName(), this, __FUNCTION__, (long)options);

  success = super::terminate(options);

  return success;
}

// We override finalize only to log that it has been called to make it easier to
// follow the driver's lifecycle.
// Production drivers will rarely need to override finalize.
bool SoftU2FDriverClassName::finalize(IOOptionBits options) {
  bool success;

  IOLog("%s[%p]::%s(%ld)\n", getName(), this, __FUNCTION__, (long)options);

  success = super::finalize(options);

  return success;
}

OSString *SoftU2FDriverClassName::userClientKey(IOService *userClient) {
  const size_t ptrSize = sizeof(IOService *);
  const size_t hexPtrSize = 2 + (2 * ptrSize); // 0x...
  char cKey[hexPtrSize + 1];

  snprintf(cKey, hexPtrSize, "%p", userClient);
  cKey[hexPtrSize] = 0;

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
  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, userClient);

  SoftU2FDeviceClassName *device;

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

bool SoftU2FDriverClassName::userClientDeviceSend(IOService *userClient,
                                                  U2FHID_FRAME *frame) {
  IOLog("%s[%p]::%s(%p, %p)\n", getName(), this, __FUNCTION__, userClient,
        frame);

  IOMemoryDescriptor *report = nullptr;
  SoftU2FDeviceClassName *device;
  bool ret;

  device = (SoftU2FDeviceClassName *)userClientDevice(userClient);
  if (!device)
    return false;

  report =
      IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, HID_RPT_SIZE);
  if (!report)
    return false;

  report->writeBytes(0, frame, HID_RPT_SIZE);

  if (device->handleReport(report, kIOHIDReportTypeInput) == kIOReturnSuccess) {
    IOLog("Report correctly sent to device.");
    ret = true;
  } else {
    IOLog("Error while sending report to device.");
  }

  report->release();

  return ret;
}
