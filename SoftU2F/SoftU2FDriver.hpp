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

class SoftU2FDriver : public IOService {
  OSDeclareDefaultStructors(SoftU2FDriver)

public :
  // IOService methods
  virtual bool init(OSDictionary *dictionary = 0) override;
  virtual void free(void) override;

  virtual bool start(IOService *provider) override;
  virtual void stop(IOService *provider) override;

  virtual OSString *userClientKey(IOService *userClient);
  virtual IOService *userClientDevice(IOService *userClient);
  virtual bool destroyUserClientDevice(IOService *userClient);
  virtual bool userClientDeviceSend(IOService *userClient, U2FHID_FRAME *frame);

private:
  OSDictionary *m_hid_devices;
};

#endif /* SoftU2F_hpp */
