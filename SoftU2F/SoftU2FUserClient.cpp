//
//  SoftU2FUserClient.cpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "SoftU2FUserClient.hpp"
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOLib.h>

#define super IOUserClient

OSDefineMetaClassAndStructors(com_github_SoftU2FUserClient, IOUserClient)

    /**
     * A dispatch table for this User Client interface, used by
     * 'SoftU2FUserClientClassName::externalMethod()'.
     * The fields of the IOExternalMethodDispatch type follows:
     *
     *  struct IOExternalMethodDispatch
     *  {
     *      IOExternalMethodAction function;
     *      uint32_t		   checkScalarInputCount;
     *      uint32_t		   checkStructureInputSize;
     *      uint32_t		   checkScalarOutputCount;
     *      uint32_t		   checkStructureOutputSize;
     *  };
     */
    const IOExternalMethodDispatch
    SoftU2FUserClientClassName::sMethods[kNumberOfMethods] = {
        {(IOExternalMethodAction)&SoftU2FUserClientClassName::sOpenUserClient,
         0, 0, 0, 0},
        {(IOExternalMethodAction)&SoftU2FUserClientClassName::sCloseUserClient,
         0, 0, 0, 0},
        {(IOExternalMethodAction)&SoftU2FUserClientClassName::sGetFrame, 0, 0,
         0, sizeof(U2FHID_FRAME)},
        {(IOExternalMethodAction)&SoftU2FUserClientClassName::sSendFrame, 0,
         sizeof(U2FHID_FRAME), 0, 0},
};

IOReturn SoftU2FUserClientClassName::externalMethod(
    uint32_t selector, IOExternalMethodArguments *arguments,
    IOExternalMethodDispatch *dispatch, OSObject *target, void *reference)

{
  IOLog("%s[%p]::%s(%d, %p, %p, %p, %p)\n", getName(), this, __FUNCTION__,
        selector, arguments, dispatch, target, reference);

  if (selector < (uint32_t)kNumberOfMethods) {
    dispatch = (IOExternalMethodDispatch *)&sMethods[selector];

    if (!target) {
      target = this;
    }
  }

  return super::externalMethod(selector, arguments, dispatch, target,
                               reference);
}

// There are two forms of IOUserClient::initWithTask, the second of which
// accepts an additional OSDictionary* parameter.
// If your user client needs to modify its behavior when it's being used by a
// process running using Rosetta,
// you need to implement the form of initWithTask with this additional
// parameter.
//
// initWithTask is called as a result of the user process calling IOServiceOpen.
bool SoftU2FUserClientClassName::initWithTask(task_t owningTask,
                                              void *securityToken, UInt32 type,
                                              OSDictionary *properties) {
  bool success;

  success = super::initWithTask(owningTask, securityToken, type, properties);

  // This IOLog must follow super::initWithTask because getName relies on the
  // superclass initialization.
  IOLog("%s[%p]::%s(%p, %p, %u, %p)\n", getName(), this, __FUNCTION__,
        owningTask, securityToken, (unsigned int)type, properties);

  fTask = owningTask;
  fProvider = NULL;
  fQueuedSetReports = OSArray::withCapacity(1);

  return success;
}

// start is called after initWithTask as a result of the user process calling
// IOServiceOpen.
bool SoftU2FUserClientClassName::start(IOService *provider) {
  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  // Verify that this user client is being started with a provider that it knows
  // how to communicate with.
  fProvider = OSDynamicCast(SoftU2FDriverClassName, provider);
  if (!fProvider)
    return false;

  return super::start(provider);
}

// We override stop only to log that it has been called to make it easier to
// follow the user client's lifecycle.
void SoftU2FUserClientClassName::stop(IOService *provider) {
  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  super::stop(provider);
}

// clientClose is called as a result of the user process calling IOServiceClose.
IOReturn SoftU2FUserClientClassName::clientClose(void) {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  // Defensive coding in case the user process called IOServiceClose
  // without calling closeUserClient first.
  (void)closeUserClient();

  // Inform the user process that this user client is no longer available. This
  // will also cause the
  // user client instance to be destroyed.
  //
  // terminate would return false if the user process still had this user client
  // open.
  // This should never happen in our case because this code path is only reached
  // if the user process
  // explicitly requests closing the connection to the user client.
  bool success = terminate();
  if (!success) {
    IOLog("%s[%p]::%s(): terminate() failed.\n", getName(), this, __FUNCTION__);
  }

  // DON'T call super::clientClose, which just returns kIOReturnUnsupported.

  return kIOReturnSuccess;
}

// clientDied is called if the client user process terminates unexpectedly
// (crashes).
// We override clientDied only to log that it has been called to make it easier
// to follow the user client's lifecycle.
// Production user clients need to override clientDied only if they need to take
// some alternate action if the user process
// crashes instead of exiting normally.
IOReturn SoftU2FUserClientClassName::clientDied(void) {
  IOReturn result = kIOReturnSuccess;

  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  // The default implementation of clientDied just calls clientClose.
  result = super::clientDied();

  return result;
}

// willTerminate is called at the beginning of the termination process. It is a
// notification
// that a provider has been terminated, sent before recursing up the stack, in
// root-to-leaf order.
//
// This is where any pending I/O should be terminated. At this point the user
// client has been marked
// inactive and any further requests from the user process should be returned
// with an error.
bool SoftU2FUserClientClassName::willTerminate(IOService *provider,
                                               IOOptionBits options) {
  IOLog("%s[%p]::%s(%p, %ld)\n", getName(), this, __FUNCTION__, provider,
        (long)options);

  return super::willTerminate(provider, options);
}

// didTerminate is called at the end of the termination process. It is a
// notification
// that a provider has been terminated, sent after recursing up the stack, in
// leaf-to-root order.
bool SoftU2FUserClientClassName::didTerminate(IOService *provider,
                                              IOOptionBits options,
                                              bool *defer) {
  IOLog("%s[%p]::%s(%p, %ld, %p)\n", getName(), this, __FUNCTION__, provider,
        (long)options, defer);

  // If all pending I/O has been terminated, close our provider. If I/O is still
  // outstanding, set defer to true
  // and the user client will not have stop called on it.
  closeUserClient();
  *defer = false;

  return super::didTerminate(provider, options, defer);
}

// We override terminate only to log that it has been called to make it easier
// to follow the user client's lifecycle.
// Production user clients will rarely need to override terminate. Termination
// processing should be done in
// willTerminate or didTerminate instead.
bool SoftU2FUserClientClassName::terminate(IOOptionBits options) {
  bool success;

  IOLog("%s[%p]::%s(%ld)\n", getName(), this, __FUNCTION__, (long)options);

  success = super::terminate(options);

  return success;
}

// We override finalize only to log that it has been called to make it easier to
// follow the user client's lifecycle.
// Production user clients will rarely need to override finalize.
bool SoftU2FUserClientClassName::finalize(IOOptionBits options) {
  bool success;

  IOLog("%s[%p]::%s(%ld)\n", getName(), this, __FUNCTION__, (long)options);

  success = super::finalize(options);

  return success;
}

bool SoftU2FUserClientClassName::queueFrame(IOMemoryDescriptor *report) {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (!fQueuedSetReports)
    return false;

  OSData *userReport;
  IOMemoryMap *reportMap;

  report->prepare();

  reportMap = report->map();
  userReport = OSData::withBytes((void *)reportMap->getAddress(),
                                 (unsigned int)reportMap->getLength());

  report->complete();
  reportMap->release();

  return fQueuedSetReports->setObject(userReport);
}

IOReturn SoftU2FUserClientClassName::sOpenUserClient(
    SoftU2FUserClientClassName *target, void *reference,
    IOExternalMethodArguments *arguments) {
  return target->openUserClient();
}

IOReturn SoftU2FUserClientClassName::openUserClient() {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (fProvider == NULL || isInactive()) {
    // Return an error if we don't have a provider. This could happen if the
    // user process
    // called openUserClient without calling IOServiceOpen first. Or, the user
    // client could be
    // in the process of being terminated and is thus inactive.
    IOLog("%s[%p]::%s()->kIOReturnNotAttached\n", getName(), this,
          __FUNCTION__);
    return kIOReturnNotAttached;
  }

  if (!fProvider->open(this)) {
    // The most common reason this open call will fail is because the provider
    // is already open
    // and it doesn't support being opened by more than one client at a time.
    IOLog("%s[%p]::%s()->kIOReturnExclusiveAccess\n", getName(), this,
          __FUNCTION__);
    return kIOReturnExclusiveAccess;
  }

  IOService *device = fProvider->userClientDevice(this);
  if (!device) {
    IOLog("%s[%p]::%s()->kIOReturnError\n", getName(), this, __FUNCTION__);
    return kIOReturnError;
  }

  IOLog("%s[%p]::%s()->kIOReturnSuccess\n", getName(), this, __FUNCTION__);
  return kIOReturnSuccess;
}

IOReturn SoftU2FUserClientClassName::sCloseUserClient(
    SoftU2FUserClientClassName *target, void *reference,
    IOExternalMethodArguments *arguments) {
  return target->closeUserClient();
}

IOReturn SoftU2FUserClientClassName::closeUserClient() {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (fQueuedSetReports) {
    while (fQueuedSetReports->getCount()) {
      fQueuedSetReports->removeObject(0);
    }

    fQueuedSetReports->release();
    fQueuedSetReports = nullptr;
  }

  if (!fProvider) {
    // Return an error if we don't have a provider. This could happen if the
    // user process
    // called closeUserClient without calling IOServiceOpen first.
    IOLog("%s[%p]::%s(): returning kIOReturnNotAttached.\n", getName(), this,
          __FUNCTION__);
    return kIOReturnNotAttached;
  }

  if (!fProvider->isOpen(this)) {
    IOLog("%s[%p]::%s(): returning kIOReturnNotOpen.\n", getName(), this,
          __FUNCTION__);
    return kIOReturnNotOpen;
  }

  if (!fProvider->destroyUserClientDevice(this)) {
    IOLog("%s[%p]::%s(): returning kIOReturnError.\n", getName(), this,
          __FUNCTION__);
    return kIOReturnError;
  }

  // Make sure we're the one who opened our provider before we tell it to close.
  fProvider->close(this);
  return kIOReturnSuccess;
}

IOReturn
SoftU2FUserClientClassName::sGetFrame(SoftU2FUserClientClassName *target,
                                      void *reference,
                                      IOExternalMethodArguments *arguments) {
  return target->getFrame((U2FHID_FRAME *)arguments->structureOutput,
                          (size_t *)&arguments->structureOutputSize);
}

IOReturn SoftU2FUserClientClassName::getFrame(U2FHID_FRAME *frame,
                                              size_t *frameSize) {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (!fProvider)
    return kIOReturnNotAttached;
  if (!fProvider->isOpen(this))
    return kIOReturnNotOpen;

  OSData *report;
  OSObject *obj;

  if (!fQueuedSetReports || !fTask)
    return kIOReturnNotOpen;

  obj = fQueuedSetReports->getObject(0);
  if (!obj)
    return kIOReturnNoFrames;

  report = OSDynamicCast(OSData, obj);
  if (!report)
    return kIOReturnError;

  memcpy(frame, report->getBytesNoCopy(), report->getLength());
  *frameSize = report->getLength();

  fQueuedSetReports->removeObject(0);

  return kIOReturnSuccess;
}

IOReturn
SoftU2FUserClientClassName::sSendFrame(SoftU2FUserClientClassName *target,
                                       void *reference,
                                       IOExternalMethodArguments *arguments) {
  return target->sendFrame((U2FHID_FRAME *)arguments->structureInput,
                           arguments->structureInputSize);
}

IOReturn SoftU2FUserClientClassName::sendFrame(U2FHID_FRAME *frame,
                                               size_t frameSize) {
  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (!fProvider)
    return kIOReturnNotAttached;
  if (!fProvider->isOpen(this))
    return kIOReturnNotOpen;
  if (frameSize != HID_RPT_SIZE)
    return kIOReturnBadArgument;

  if (fProvider->userClientDeviceSend(this, frame)) {
    return kIOReturnSuccess;
  } else {
    return kIOReturnError;
  }
}
