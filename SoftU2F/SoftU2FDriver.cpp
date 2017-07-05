//
//  SoftU2FDriver.cpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.

#include <IOKit/IOLib.h>
#include "SoftU2FDriver.hpp"

#define super IOService
OSDefineMetaClassAndStructors(SoftU2FDriver, IOService);

bool SoftU2FDriver::start(IOService *provider) {
  IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);

  if (!super::start(provider))
    return false;

  registerService();

  return true;
}
