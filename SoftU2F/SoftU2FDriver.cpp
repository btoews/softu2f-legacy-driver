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

  if (( success = super::start(provider) ))
    registerService();

  return success;
}

void SoftU2FDriverClassName::stop(IOService *provider) {
  OSIterator *iter;
  OSObject *obj;
  SoftU2FUserClientClassName *userClient;
  SoftU2FDeviceClassName *device;

  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  // iterate through user clients and terminate their devices.
  iter = this->getClientIterator();
  if (!iter) {
    IOLog("Coulnd't get client iterator");
    goto done;
  }

  while (( obj = iter->getNextObject() )) {
    userClient = OSDynamicCast(SoftU2FUserClientClassName, obj);
    if (!userClient) {
      IOLog("Userclient isn't a SoftU2FUserClientClassName");
      continue;
    }

    device = OSDynamicCast(SoftU2FDeviceClassName, userClient->getDevice());
    if (!device) {
      IOLog("Couldn't get user client device");
      continue;
    }

    if (!device->isInactive() && !device->terminate())
      IOLog("Error terminating device.");

    device->release();
    userClient->setDevice(nullptr);
  }

  iter->release();

done:
  super::stop(provider);
}

IOReturn SoftU2FDriverClassName::newUserClient(task_t owningTask, void *securityID, UInt32 type, OSDictionary *properties, IOUserClient **handler) {
  SoftU2FUserClientClassName *userClient = nullptr;
  SoftU2FDeviceClassName *device = nullptr;
  
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);
  
  IOReturn ret = super::newUserClient(owningTask, securityID, type, properties, handler);
  if (ret != kIOReturnSuccess) {
    IOLog("Error creating user client");
    return ret;
  }
  
  if (*handler == nullptr) {
    IOLog("Null user client");
    return kIOReturnError;
  }
  
  userClient = OSDynamicCast(SoftU2FUserClientClassName, *handler);
  if (!userClient) {
    IOLog("User client isn't a SoftU2FUserClientClassName");
    return kIOReturnError;
  }

  device = OSTypeAlloc(SoftU2FDeviceClassName);
  if (!device) {
    IOLog("Error allocating memory for user client device");
    return kIOReturnNoMemory;
  }
  
  if (!device->init(nullptr)) {
    IOLog("Error initializing user client device");
    device->free();
    return kIOReturnError;
  }

  if (!device->attach(this)) {
    IOLog("Error attaching device");
    device->free();
    return kIOReturnError;
  }
  
  if (!device->start(this)) {
    IOLog("Error starting device");
    device->free();
    return kIOReturnError;
  }
  
  device->setUserClient(userClient);
  userClient->setDevice(device);
  
  return kIOReturnSuccess;
}

bool SoftU2FDriverClassName::destroyUserClientDevice(IOService *service) {
  SoftU2FUserClientClassName *userClient = nullptr;
  SoftU2FDeviceClassName *device = nullptr;
  
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);
  
  userClient = OSDynamicCast(SoftU2FUserClientClassName, service);
  if (!userClient) {
    IOLog("Service isn't a SoftU2FUserClientClassName");
    return false;
  }

  device = OSDynamicCast(SoftU2FDeviceClassName, userClient->getDevice());
  if (!device) {
    IOLog("Error getting user client device");
    return false;
  }

  if (!device->isInactive()) {
    IOLog("Terminating device");
    if (!device->terminate()) {
      IOLog("Error terminating device. Client attached?");
    }
  }
  
  userClient->setDevice(nullptr);
  device->release();

  return true;
}

bool SoftU2FDriverClassName::userClientDeviceSend(IOService *service, U2FHID_FRAME *frame) {
  SoftU2FUserClientClassName *userClient = nullptr;
  SoftU2FDeviceClassName *device = nullptr;
  IOMemoryDescriptor *report = nullptr;
  bool ret = false;
  
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);
  
  userClient = OSDynamicCast(SoftU2FUserClientClassName, service);
  if (!userClient) {
    IOLog("Service isn't a SoftU2FUserClientClassName");
    return false;
  }

  device = OSDynamicCast(SoftU2FDeviceClassName, userClient->getDevice());
  if (!device) {
    IOLog("Error getting user client device");
    return false;
  }

  report = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, HID_RPT_SIZE);
  if (!report) {
    IOLog("Error initializing IOBufferMemoryDescriptor");
    return false;
  }

  report->writeBytes(0, frame, HID_RPT_SIZE);

  ret = device->handleReport(report, kIOHIDReportTypeInput) == kIOReturnSuccess;
  if (!ret)
    IOLog("Error sending report to device");

  report->release();

  return ret;
}
