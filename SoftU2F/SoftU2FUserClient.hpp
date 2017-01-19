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

      protected : SoftU2FDriverClassName *fProvider;
  task_t fTask;
  OSArray *fQueuedSetReports;
  static const IOExternalMethodDispatch sMethods[kNumberOfMethods];

public:
  // IOUserClient methods
  virtual void stop(IOService *provider) override;
  virtual bool start(IOService *provider) override;

  virtual bool initWithTask(task_t owningTask, void *securityToken, UInt32 type,
                            OSDictionary *properties) override;

  virtual IOReturn clientClose(void) override;
  virtual IOReturn clientDied(void) override;

  virtual bool willTerminate(IOService *provider,
                             IOOptionBits options) override;
  virtual bool didTerminate(IOService *provider, IOOptionBits options,
                            bool *defer) override;

  virtual bool terminate(IOOptionBits options = 0) override;
  virtual bool finalize(IOOptionBits options) override;

  virtual bool queueFrame(IOMemoryDescriptor *report);

protected:
  virtual IOReturn externalMethod(uint32_t selector,
                                  IOExternalMethodArguments *arguments,
                                  IOExternalMethodDispatch *dispatch,
                                  OSObject *target, void *reference) override;

  // User client methods
  static IOReturn sOpenUserClient(SoftU2FUserClientClassName *target,
                                  void *reference,
                                  IOExternalMethodArguments *arguments);
  virtual IOReturn openUserClient(void);

  static IOReturn sCloseUserClient(SoftU2FUserClientClassName *target,
                                   void *reference,
                                   IOExternalMethodArguments *arguments);
  virtual IOReturn closeUserClient(void);

  static IOReturn sGetFrame(SoftU2FUserClientClassName *target, void *reference,
                            IOExternalMethodArguments *arguments);
  virtual IOReturn getFrame(U2FHID_FRAME *frame, size_t *frameSize);

  static IOReturn sSendFrame(SoftU2FUserClientClassName *target,
                             void *reference,
                             IOExternalMethodArguments *arguments);
  virtual IOReturn sendFrame(U2FHID_FRAME *frame, size_t frameSize);
};

#endif /* SoftU2FUserClient_hpp */
