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

typedef struct {
    uint8_t   cmd;
    uint16_t  bcnt;
    uint32_t  cid;
    CFDataRef data;
    CFMutableDataRef buf;
    uint8_t   lastSeq;
} U2F_HID_MESSAGE;

typedef struct {
    uint32_t cid;
    time_t expiration;
} U2F_HID_LOCK;

typedef bool (*U2F_HID_MESSAGE_HANDLER)(U2F_HID_MESSAGE *req);

// Initialization
bool softu2f_init();

// Read a U2F message from the device.
CFDataRef softu2f_u2f_msg_read();

// Is this client allowed to start a transaction (not locked by another client)?
bool softu2f_hid_is_unlocked_for_client(uint32_t cid);

// Send a HID message to the device.
bool softu2f_hid_msg_send(U2F_HID_MESSAGE* msg);

// Read a HID message from the device.
U2F_HID_MESSAGE* softu2f_hid_msg_read();

// Read an individual HID frame from the device into a HID message.
bool softu2f_hid_msg_frame_read(U2F_HID_MESSAGE* msg, U2FHID_FRAME* frame);

// Handle a SYNC packet.
bool softu2f_hid_msg_frame_handle_sync(U2FHID_FRAME* frame);

// Find a message handler for a message.
U2F_HID_MESSAGE_HANDLER soft_u2f_hid_msg_handler(U2F_HID_MESSAGE *msg);

// Send the appropriate HID message response for a received HID message.
bool softu2f_hid_msg_handle(U2F_HID_MESSAGE* msg);

// Send an INIT response for a given request.
bool softu2f_hid_msg_handle_init(U2F_HID_MESSAGE* req);

// Send a PING response for a given request.
bool softu2f_hid_msg_handle_ping(U2F_HID_MESSAGE* req);

// Send a WINK response for a given request.
bool softu2f_hid_msg_handle_wink(U2F_HID_MESSAGE* req);

// Send a LOCK response for a given request.
bool softu2f_hid_msg_handle_lock(U2F_HID_MESSAGE* req);

// Free a HID message and associated data.
void softu2f_hid_msg_free(U2F_HID_MESSAGE* msg);

// Deinitialization
bool softu2f_deinit();

#endif /* SoftU2FClientInterface_h */
