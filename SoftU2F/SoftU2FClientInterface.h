//
//  SoftU2FClientInterface.h
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#ifndef SoftU2FClientInterface_h
#define SoftU2FClientInterface_h

#include "UserKernelShared.h"
#include <IOKit/IOKitLib.h>

io_connect_t* libSoftU2FConnection;

bool libSoftU2FInit();
bool libSoftU2FInitConnection();
bool libSoftU2FOpenUserClient();
bool libSoftU2FDeinit();
bool libSoftU2FCloseUserClient();
bool libSoftU2FDeinitConnection();

#endif /* SoftU2FClientInterface_h */
