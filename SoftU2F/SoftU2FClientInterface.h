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

// U2FHID message.
typedef struct {
    uint8_t   cmd;
    uint16_t  bcnt;
    uint32_t  cid;
    CFDataRef data;
    CFMutableDataRef buf;
    uint8_t   lastSeq;
} softu2f_hid_message;

// Lock held by application.
typedef struct {
    uint32_t cid;
    time_t expiration;
} softu2f_hid_lock;

// Context includes lock, cid counter, connection.
typedef struct {
    io_connect_t con;
    softu2f_hid_lock* lock;
    uint32_t next_cid;
} softu2f_ctx;

// Handler function for HID message.
typedef bool (*softu2f_hid_message_handler)(softu2f_ctx* ctx, softu2f_hid_message *req);

// Initialization
softu2f_ctx* softu2f_init();

// Deinitialization
void softu2f_deinit(softu2f_ctx* ctx);

// Read a U2F message from the device.
CFDataRef softu2f_u2f_msg_read(softu2f_ctx* ctx);

// Is this client allowed to start a transaction (not locked by another client)?
bool softu2f_hid_is_unlocked_for_client(softu2f_ctx* ctx, uint32_t cid);

// Send a HID message to the device.
bool softu2f_hid_msg_send(softu2f_ctx* ctx, softu2f_hid_message* msg);

// Send a HID error to the device.
bool softu2f_hid_err_send(softu2f_ctx* ctx, uint32_t cid, uint8_t code);

// Read a HID message from the device.
softu2f_hid_message* softu2f_hid_msg_read(softu2f_ctx* ctx);

// Read an individual HID frame from the device into a HID message.
bool softu2f_hid_msg_frame_read(softu2f_ctx* ctx, softu2f_hid_message* msg, U2FHID_FRAME* frame);

// Handle a SYNC packet.
bool softu2f_hid_msg_frame_handle_sync(softu2f_ctx* ctx, U2FHID_FRAME* frame);

// Find a message handler for a message.
softu2f_hid_message_handler soft_u2f_hid_msg_handler(softu2f_ctx* ctx, softu2f_hid_message *msg);

// Send an INIT response for a given request.
bool softu2f_hid_msg_handle_init(softu2f_ctx* ctx, softu2f_hid_message* req);

// Send a PING response for a given request.
bool softu2f_hid_msg_handle_ping(softu2f_ctx* ctx, softu2f_hid_message* req);

// Send a WINK response for a given request.
bool softu2f_hid_msg_handle_wink(softu2f_ctx* ctx, softu2f_hid_message* req);

// Send a LOCK response for a given request.
bool softu2f_hid_msg_handle_lock(softu2f_ctx* ctx, softu2f_hid_message* req);

// Free a HID message and associated data.
void softu2f_hid_msg_free(softu2f_hid_message* msg);

#endif /* SoftU2FClientInterface_h */
