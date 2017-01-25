//
//  internal.h
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/25/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#ifndef internal_h
#define internal_h

#include "UserKernelShared.h"
#include "u2f_hid.h"
#include <IOKit/IOKitLib.h>

// Lock held by application.
struct softu2f_hid_lock {
    uint32_t cid;
    time_t expiration;
};

// Context includes lock, cid counter, connection.
struct softu2f_ctx {
    io_connect_t con;
    softu2f_hid_lock *lock;
    uint32_t next_cid;

    // Stop the run loop.
    bool shutdown;

    // Handlers registered for HID msg types.
    softu2f_hid_message_handler ping_handler;
    softu2f_hid_message_handler msg_handler;
    softu2f_hid_message_handler lock_handler;
    softu2f_hid_message_handler init_handler;
    softu2f_hid_message_handler wink_handler;
};

// Is this client allowed to start a transaction (not locked by another client)?
bool softu2f_hid_is_unlocked_for_client(softu2f_ctx *ctx, uint32_t cid);

// Read a HID message from the device.
softu2f_hid_message *softu2f_hid_msg_read(softu2f_ctx *ctx);

// Read an individual HID frame from the device into a HID message.
bool softu2f_hid_msg_frame_read(softu2f_ctx *ctx, softu2f_hid_message *msg, U2FHID_FRAME *frame);

// Handle a SYNC packet.
bool softu2f_hid_msg_frame_handle_sync(softu2f_ctx *ctx, U2FHID_FRAME *frame);

// Find a message handler for a message.
softu2f_hid_message_handler softu2f_hid_msg_handler(softu2f_ctx *ctx, softu2f_hid_message *msg);

// Send an INIT response for a given request.
bool softu2f_hid_msg_handle_init(softu2f_ctx *ctx, softu2f_hid_message *req);

// Send a PING response for a given request.
bool softu2f_hid_msg_handle_ping(softu2f_ctx *ctx, softu2f_hid_message *req);

// Send a WINK response for a given request.
bool softu2f_hid_msg_handle_wink(softu2f_ctx *ctx, softu2f_hid_message *req);

// Send a LOCK response for a given request.
bool softu2f_hid_msg_handle_lock(softu2f_ctx *ctx, softu2f_hid_message *req);

// Free a HID message and associated data.
void softu2f_hid_msg_free(softu2f_hid_message *msg);

// Callback called when a setReport is received by the driver.
void _softu2f_async_callback(void *refcon, IOReturn result);

// Block until setReport is called on the device.
void softu2f_wait_for_set_report(softu2f_ctx *ctx);

void debug_frame(U2FHID_FRAME *frame, bool recv);


#endif /* internal_h */
