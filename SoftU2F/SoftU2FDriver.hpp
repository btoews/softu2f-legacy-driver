//
//  SoftU2FDriver.hpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.

#ifndef SoftU2FDriver_hpp
#define SoftU2FDriver_hpp

#include "UserKernelShared.h"
#include "u2f_hid.h"
#include <IOKit/IOService.h>

class SoftU2FDriverClassName : public IOService {
  OSDeclareDefaultStructors(com_github_SoftU2FDriver)

public :
  // IOService methods
  virtual bool start(IOService *provider) override;
  virtual void stop(IOService *provider) override;
  
  virtual IOReturn newUserClient(task_t owningTask, void * securityID, UInt32 type, OSDictionary * properties, IOUserClient ** handler) override;

  // Our methods
  virtual bool destroyUserClientDevice(IOService *userClient);
  virtual bool userClientDeviceSend(IOService *userClient, U2FHID_FRAME *frame);
};

#endif /* SoftU2F_hpp */
