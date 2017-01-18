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

extern io_connect_t* softu2f_connection;

typedef struct U2F_HID_MESSAGE {
    uint8_t   cmd;
    uint16_t  bcnt;
    CFMutableDataRef data;
    uint8_t   lastSeq;
} U2F_HID_MESSAGE;

// Initialization
bool softu2f_init();

bool softu2f_hid_msg_read(U2F_HID_MESSAGE** msgPtr);
void softu2f_hid_msg_free(U2F_HID_MESSAGE* msg);
bool softu2f_hid_msg_read_frame(U2F_HID_MESSAGE* msg, U2FHID_FRAME* frame);

// Deinitialization
bool softu2f_deinit();

#endif /* SoftU2FClientInterface_h */
