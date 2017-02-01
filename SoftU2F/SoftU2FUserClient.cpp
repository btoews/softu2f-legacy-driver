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
      {(IOExternalMethodAction)&SoftU2FUserClientClassName::sGetFrame, 0, 0, 0, sizeof(U2FHID_FRAME)},
      {(IOExternalMethodAction)&SoftU2FUserClientClassName::sSendFrame, 0, sizeof(U2FHID_FRAME), 0, 0},
      {(IOExternalMethodAction)&SoftU2FUserClientClassName::sNotifyFrame, 0, 0, 0, 0},
};

IOReturn SoftU2FUserClientClassName::externalMethod(uint32_t selector, IOExternalMethodArguments *arguments, IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) {
//  IOLog("%s[%p]::%s(%d, %p, %p, %p, %p)\n", getName(), this, __FUNCTION__, selector, arguments, dispatch, target, reference);

  if (selector < (uint32_t)kNumberOfMethods) {
    dispatch = (IOExternalMethodDispatch *)&sMethods[selector];

    if (!target) {
      target = this;
    }
  }

  return super::externalMethod(selector, arguments, dispatch, target, reference);
}

// There are two forms of IOUserClient::initWithTask, the second of which
// accepts an additional OSDictionary* parameter.
// If your user client needs to modify its behavior when it's being used by a
// process running using Rosetta,
// you need to implement the form of initWithTask with this additional
// parameter.
//
// initWithTask is called as a result of the user process calling IOServiceOpen.
bool SoftU2FUserClientClassName::initWithTask(task_t owningTask, void *securityToken, UInt32 type, OSDictionary *properties) {
  bool success;

  success = super::initWithTask(owningTask, securityToken, type, properties);

  // This IOLog must follow super::initWithTask because getName relies on the
  // superclass initialization.
//  IOLog("%s[%p]::%s(%p, %p, %u, %p)\n", getName(), this, __FUNCTION__, owningTask, securityToken, (unsigned int)type, properties);

  fTask = owningTask;
  fProvider = NULL;
  fQueuedSetReports = OSArray::withCapacity(1);

  return success;
}

// start is called after initWithTask as a result of the user process calling
// IOServiceOpen.
bool SoftU2FUserClientClassName::start(IOService *provider) {
//  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  // Verify that this user client is being started with a provider that it knows
  // how to communicate with.
  fProvider = OSDynamicCast(SoftU2FDriverClassName, provider);
  if (!fProvider)
    return false;

  IOService *device = fProvider->userClientDevice(this);
  if (!device) {
      return false;
  }

  return super::start(provider);
}

// We override stop only to log that it has been called to make it easier to
// follow the user client's lifecycle.
void SoftU2FUserClientClassName::stop(IOService *provider) {
//  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  super::stop(provider);
}

// clientClose is called as a result of the user process calling IOServiceClose.
IOReturn SoftU2FUserClientClassName::clientClose(void) {
//  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (fNotifyRef) {
    IOFree(fNotifyRef, sizeof(OSAsyncReference64));
    fNotifyRef = nullptr;
  }

  if (fQueuedSetReports) {
    while (fQueuedSetReports->getCount()) {
      fQueuedSetReports->removeObject(0);
    }

    fQueuedSetReports->release();
    fQueuedSetReports = nullptr;
  }

  if (!fProvider->destroyUserClientDevice(this)) {
    return kIOReturnError;
  }

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
//    IOLog("%s[%p]::%s(): terminate() failed.\n", getName(), this, __FUNCTION__);
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

//  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

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
bool SoftU2FUserClientClassName::willTerminate(IOService *provider, IOOptionBits options) {
//  IOLog("%s[%p]::%s(%p, %ld)\n", getName(), this, __FUNCTION__, provider, (long)options);

  return super::willTerminate(provider, options);
}

// didTerminate is called at the end of the termination process. It is a
// notification
// that a provider has been terminated, sent after recursing up the stack, in
// leaf-to-root order.
bool SoftU2FUserClientClassName::didTerminate(IOService *provider, IOOptionBits options, bool *defer) {
//  IOLog("%s[%p]::%s(%p, %ld, %p)\n", getName(), this, __FUNCTION__, provider, (long)options, defer);

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

//  IOLog("%s[%p]::%s(%ld)\n", getName(), this, __FUNCTION__, (long)options);

  success = super::terminate(options);

  return success;
}

// We override finalize only to log that it has been called to make it easier to
// follow the user client's lifecycle.
// Production user clients will rarely need to override finalize.
bool SoftU2FUserClientClassName::finalize(IOOptionBits options) {
  bool success;

//  IOLog("%s[%p]::%s(%ld)\n", getName(), this, __FUNCTION__, (long)options);

  success = super::finalize(options);

  return success;
}

bool SoftU2FUserClientClassName::queueFrame(IOMemoryDescriptor *report) {
//  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (!fQueuedSetReports)
    return false;

  bool ret = false;
  OSData *userReport;
  IOMemoryMap *reportMap;

  report->prepare();

  reportMap = report->map();
  userReport = OSData::withBytes((void *)reportMap->getAddress(), (unsigned int)reportMap->getLength());

  report->complete();
  reportMap->release();

  ret = fQueuedSetReports->setObject(userReport);

  // Notify userland that we got a report.
  if (fNotifyRef) {
    sendAsyncResult64(*fNotifyRef, kIOReturnSuccess, NULL, 0);
    IOFree(fNotifyRef, sizeof(OSAsyncReference64));
    fNotifyRef = nullptr;
  }

  return ret;
}

IOReturn SoftU2FUserClientClassName::sGetFrame(SoftU2FUserClientClassName *target, void *reference, IOExternalMethodArguments *arguments) {
  return target->getFrame((U2FHID_FRAME *)arguments->structureOutput, (size_t *)&arguments->structureOutputSize);
}

IOReturn SoftU2FUserClientClassName::getFrame(U2FHID_FRAME *frame, size_t *frameSize) {
//  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (!fProvider)
    return kIOReturnNotAttached;

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

IOReturn SoftU2FUserClientClassName::sSendFrame(SoftU2FUserClientClassName *target, void *reference, IOExternalMethodArguments *arguments) {
  return target->sendFrame((U2FHID_FRAME *)arguments->structureInput, arguments->structureInputSize);
}

IOReturn SoftU2FUserClientClassName::sendFrame(U2FHID_FRAME *frame, size_t frameSize) {
//  IOLog("%s[%p]::%s()\n", getName(), this, __FUNCTION__);

  if (!fProvider)
    return kIOReturnNotAttached;
  if (frameSize != HID_RPT_SIZE)
    return kIOReturnBadArgument;

  if (fProvider->userClientDeviceSend(this, frame)) {
    return kIOReturnSuccess;
  } else {
    return kIOReturnError;
  }
}

IOReturn SoftU2FUserClientClassName::sNotifyFrame(SoftU2FUserClientClassName *target, void *reference, IOExternalMethodArguments *arguments) {
  return target->notifyFrame(arguments->asyncReference);
}

IOReturn SoftU2FUserClientClassName::notifyFrame(io_user_reference_t *ref) {
//  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, ref);

  if (!fProvider)
    return kIOReturnNotAttached;

  if (fNotifyRef) {
    IOFree(fNotifyRef, sizeof(OSAsyncReference64));
    fNotifyRef = nullptr;
  }

  fNotifyRef = (OSAsyncReference64 *)IOMalloc(sizeof(OSAsyncReference64));
  if (!fNotifyRef)
    return kIOReturnNoMemory;

  memcpy(fNotifyRef, ref, sizeof(OSAsyncReference64));

  return kIOReturnSuccess;
}
