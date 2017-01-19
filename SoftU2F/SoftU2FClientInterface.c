//
//  SoftU2FClientInterface.c
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "SoftU2FClientInterface.h"

io_connect_t* softu2f_connection;
uint32_t next_cid;

// Initialize libSoftU2F before usage.
bool softu2f_init() {
    kern_return_t ret;
    io_service_t service;
    
    service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(kSoftU2FDriverClassName));
    if (!service) {
        fprintf(stderr, "SoftU2F.kext not loaded.\n");
        return false;
    }
    
    // Open connection to user client.
    softu2f_connection = malloc(sizeof(io_connect_t));
    ret = IOServiceOpen(service, mach_task_self(), 0, softu2f_connection);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Error connecting to SoftU2F.kext: %d\n", ret);
        return false;
    }
    IOObjectRetain(*softu2f_connection);
    IOObjectRelease(service);
    
    // Initialize connection to user client.
    ret = IOConnectCallScalarMethod(*softu2f_connection, kSoftU2FUserClientOpen, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to open user client: %d.\n", ret);
        return false;
    }

    return true;
}

// Read a U2F message from the device.
CFDataRef softu2f_u2f_msg_read() {
    CFDataRef u2fmsg;
    U2F_HID_MESSAGE *hidmsg;
    U2F_HID_MESSAGE_HANDLER handler;
    
    while ((hidmsg = softu2f_hid_msg_read())) {
        if (hidmsg->cmd == U2FHID_MSG) {
            u2fmsg = hidmsg->data;
            free(hidmsg);
            return u2fmsg;
        }
        
        if ((handler = soft_u2f_hid_msg_handler(hidmsg))) {
            if (!handler(hidmsg)) {
                fprintf(stderr, "Error handling HID message\n");
            }
        } else {
            fprintf(stderr, "No handler for HID message\n");
        }
        
        softu2f_hid_msg_free(hidmsg);
    }
    
    return NULL;
}

// Send a HID message to the device.
bool softu2f_hid_msg_send(U2F_HID_MESSAGE* msg) {
    uint8_t *src;
    uint8_t *src_end;
    uint8_t *dst;
    uint8_t *dst_end;
    uint8_t seq;
    U2FHID_FRAME frame;
    kern_return_t ret;

    memset(&frame, 0, HID_RPT_SIZE);
    
    // Init frame.
    frame.cid = msg->cid;
    frame.type |= TYPE_INIT;
    frame.init.cmd |= msg->cmd;
    frame.init.bcnth = CFDataGetLength(msg->data) << 8;
    frame.init.bcntl = CFDataGetLength(msg->data) & 0xff;
    
    src = (uint8_t*)CFDataGetBytePtr(msg->data);
    src_end = src + CFDataGetLength(msg->data);
    dst = frame.init.data;
    dst_end = dst + sizeof(frame.init.data);
    
    while (1) {
        if (src_end - src > dst_end - dst) {
            memcpy(dst, src, dst_end - dst);
            src += dst_end - dst;
        } else {
            memcpy(dst, src, src_end - src);
            src += src_end - src;
        }
        
        // Send frame.
        ret = IOConnectCallStructMethod(*softu2f_connection, kSoftU2FUserClientSendFrame, &frame, HID_RPT_SIZE, NULL, NULL);
        if (ret != kIOReturnSuccess) {
            fprintf(stderr, "Error calling kSoftU2FUserClientSendFrame: 0x%08x\n", ret);
            return false;
        }
        
        // No more frames.
        if (src <= src_end) break;
        
        // Polling interval is 5ms.
        sleep(0.005);
        
        // Cont frame.
        dst = frame.cont.data;
        dst_end = dst + sizeof(frame.cont.data);
        frame.cont.seq = seq++;
        memset(frame.cont.data, 0, sizeof(frame.cont.data));
    }
    
    return true;
}

// Read a HID message from the device.
U2F_HID_MESSAGE* softu2f_hid_msg_read() {
    kern_return_t ret;
    U2F_HID_MESSAGE* msg;
    U2FHID_FRAME frame;
    unsigned long frame_size = HID_RPT_SIZE;
    
    if (!softu2f_connection) return false;

    msg = (U2F_HID_MESSAGE*)calloc(1, sizeof(U2F_HID_MESSAGE));
    if (!msg) {
        fprintf(stderr, "No memory for new message.\n");
        return NULL;
    }
    
    while (1) {
        ret = IOConnectCallStructMethod(*softu2f_connection, kSoftU2FUserClientGetFrame, NULL, 0, &frame, &frame_size);
        switch (ret) {
            case kIOReturnNoFrames:
                // poll interval is 5ms.
                sleep(1);
                break;

            case kIOReturnSuccess:
                if (frame_size != HID_RPT_SIZE) {
                    fprintf(stderr, "bad frame\n");
                    goto fail;
                }
                
                if (!softu2f_hid_msg_read_frame(msg, &frame)) {
                    goto fail;
                }
                
                break;

            default:
                fprintf(stderr, "error calling kSoftU2FUserClientGetFrame: 0x%08x\n", ret);
                goto fail;
        }
        
        if (msg->data) {
            if (CFDataGetLength(msg->data) == msg->bcnt) {
                return msg;
            }
        }
    }
    
    return NULL;

fail:
    softu2f_hid_msg_free(msg);
    return false;
}

// Read an individual HID frame from the device into a HID message.
bool softu2f_hid_msg_read_frame(U2F_HID_MESSAGE* msg, U2FHID_FRAME* frame) {
    uint8_t* data;
    unsigned int ndata;

    switch (FRAME_TYPE(*frame)) {
        case TYPE_INIT:
            if (msg->data) {
                // TODO: respond busy
                fprintf(stderr, "init frame out of order. ignoring.\n");
                return true;
            }

            msg->cmd = frame->init.cmd;
            msg->cid = frame->cid;
            msg->bcnt = MSG_LEN(*frame);
            msg->data = CFDataCreateMutable(NULL, msg->bcnt);
            
            data = frame->init.data;
            
            if (msg->bcnt > sizeof(frame->init.data)) {
                ndata = sizeof(frame->init.data);
            } else {
                ndata = msg->bcnt;
            }
            
            break;
        
        case TYPE_CONT:
            if (!msg->data) {
                fprintf(stderr, "cont frame out of order. ignoring\n");
                return true;
            }
            
            if (FRAME_SEQ(*frame) != ++msg->lastSeq) {
                fprintf(stderr, "bad seq in cont frame. bailing\n");
                return false;
            }

            data = frame->cont.data;
            
            if (CFDataGetLength(msg->data) + sizeof(frame->cont.data) > msg->bcnt) {
                ndata = msg->bcnt - (uint16_t)CFDataGetLength(msg->data);
            } else {
                ndata = sizeof(frame->cont.data);
            }
            
            break;
            
        default:
            fprintf(stderr, "unknown frame type: 0x%08x\n", FRAME_TYPE(*frame));
            return false;
    }
    
    CFDataAppendBytes(msg->data, data, ndata);
    
    return true;
}

bool softu2f_hid_msg_handle(U2F_HID_MESSAGE *msg) {
    switch (msg->cmd) {
        case U2FHID_PING:
            fprintf(stderr, "Handling Request: U2FHID_PING\n");
            return false;
        case U2FHID_MSG:
            fprintf(stderr, "Handling Request: U2FHID_MSG\n");
            return false;
        case U2FHID_LOCK:
            fprintf(stderr, "Handling Request: U2FHID_LOCK\n");
            return false;
        case U2FHID_INIT:
            fprintf(stderr, "Handling Request: U2FHID_INIT\n");
            return softu2f_hid_msg_handle_init(msg);
        case U2FHID_WINK:
            fprintf(stderr, "Handling Request: U2FHID_WINK\n");
            return false;
        case U2FHID_SYNC:
            fprintf(stderr, "Handling Request: U2FHID_SYNC\n");
            return false;
        case U2FHID_ERROR:
            fprintf(stderr, "Handling Request: U2FHID_ERROR\n");
            return false;
        default:
            fprintf(stderr, "Handling Request: UNKOWN 0x%08x\n", msg->cmd);
            return false;
    }
}

U2F_HID_MESSAGE_HANDLER soft_u2f_hid_msg_handler(U2F_HID_MESSAGE *msg) {
    switch (msg->cmd) {
        case U2FHID_PING:
            return NULL;
        case U2FHID_MSG:
            return NULL;
        case U2FHID_LOCK:
            return NULL;
        case U2FHID_INIT:
            return softu2f_hid_msg_handle_init;
        case U2FHID_WINK:
            return NULL;
        case U2FHID_SYNC:
            return NULL;
        default:
            return NULL;
    }
}

// Send an INIT response for a given request.
bool softu2f_hid_msg_handle_init(U2F_HID_MESSAGE* req) {
    fprintf(stderr, "Sending Response: U2FHID_INIT\n");

    U2F_HID_MESSAGE resp;
    U2FHID_INIT_RESP resp_data;
    U2FHID_INIT_REQ *req_data;
    
    req_data = (U2FHID_INIT_REQ*)CFDataGetBytePtr(req->data);
    
    resp.cmd = U2FHID_INIT;
    resp.bcnt = sizeof(U2FHID_INIT_RESP);
    // TODO: make mutable copy.
    resp.data = CFDataCreateWithBytesNoCopy(NULL, (uint8_t*)&resp_data, resp.bcnt, NULL);
    
    if (req->cid == CID_BROADCAST) {
        // Allocate a new CID for the client and tell them about it.
        resp.cid = CID_BROADCAST;
        resp_data.cid = next_cid++;
    } else {
        // Use whatever CID they wanted.
        resp.cid = req->cid;
        resp_data.cid = req->cid;
    }
    
    memcpy(resp_data.nonce, req_data->nonce, INIT_NONCE_SIZE);
    resp_data.versionInterface = U2FHID_IF_VERSION;
    resp_data.versionMajor = 0;
    resp_data.versionMinor = 0;
    resp_data.versionBuild = 0;
    resp_data.capFlags |= CAPFLAG_WINK;
    
    return softu2f_hid_msg_send(&resp);
}

// Free a HID message and associated data.
void softu2f_hid_msg_free(U2F_HID_MESSAGE* msg) {
    if (msg) {
        if (msg->data) CFRelease(msg->data);
        free(msg);
    }
}

// Cleanup after using libSoftU2F.
bool softu2f_deinit() {
    kern_return_t ret;

    if (!softu2f_connection) return false;
    
    // Deinitialize connection to user client.
    ret = IOConnectCallScalarMethod(*softu2f_connection, kSoftU2FUserClientClose, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to close user client: %d.\n", ret);
        return false;
    }
    
    // Close user client connection.
    ret = IOServiceClose(*softu2f_connection);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Error closing connection to SoftU2F.kext: %d.\n", ret);
        return false;
    }
    
    // Cleanup.
    IOObjectRelease(*softu2f_connection);
    free(softu2f_connection);
    
    return true;
}
