//
//  SoftU2FUserClient.hpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#ifndef SoftU2FUserClient_hpp
#define SoftU2FUserClient_hpp

#include "SoftU2FDriver.hpp"
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>

#define SoftU2FUserClientClassName com_github_SoftU2FUserClient

class SoftU2FUserClientClassName : public IOUserClient {
  OSDeclareDefaultStructors(com_github_SoftU2FUserClient)

protected:
  SoftU2FDriverClassName *fProvider;
  OSAsyncReference64 *fNotifyRef = nullptr;
  static const IOExternalMethodDispatch sMethods[kNumberOfMethods];

public:
  // IOUserClient methods
  virtual bool start(IOService *provider) override;

  virtual bool initWithTask(task_t owningTask, void *securityToken, UInt32 type, OSDictionary *properties) override;

  virtual IOReturn clientClose(void) override;

  virtual bool didTerminate(IOService *provider, IOOptionBits options, bool *defer) override;

  virtual void frameReceived(IOMemoryDescriptor *report);

protected:
  virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments *arguments, IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) override;

  // User client methods
  static IOReturn sSendFrame(SoftU2FUserClientClassName *target, void *reference, IOExternalMethodArguments *arguments);
  virtual IOReturn sendFrame(U2FHID_FRAME *frame, size_t frameSize);

  static IOReturn sNotifyFrame(SoftU2FUserClientClassName *target, void *reference, IOExternalMethodArguments *arguments);
  virtual IOReturn notifyFrame(io_user_reference_t *ref, uint32_t refCount);
};

#endif /* SoftU2FUserClient_hpp */
