//
//  SoftU2FClientInterface.c
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "SoftU2FClientInterface.h"

io_connect_t* softu2f_connection;

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

// Read a HID message from the device.
bool softu2f_hid_msg_read(U2F_HID_MESSAGE** msg_ptr) {
    kern_return_t ret;
    U2F_HID_MESSAGE* msg;
    U2FHID_FRAME frame;
    unsigned long frame_size = HID_RPT_SIZE;
    
    if (!softu2f_connection) return false;
    
    msg = (U2F_HID_MESSAGE*)calloc(1, sizeof(U2F_HID_MESSAGE));
    if (!msg) {
        fprintf(stderr, "No memory for new message.\n");
        return false;
    }
    
    while (1) {
        ret = IOConnectCallStructMethod(*softu2f_connection, kSoftU2FUserClientGetSetReport, NULL, 0, &frame, &frame_size);
        switch (ret) {
            case kIOReturnNoFrames:
                sleep(1); // poll interval shouldbe (0.005);
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
                fprintf(stderr, "error calling kSoftU2FUserClientGetSetReport: 0x%08x\n", ret);
                goto fail;
        }
        
        if (msg->data) {
            if (CFDataGetLength(msg->data) == msg->bcnt) {
                *msg_ptr = msg;
                return true;
            }
        }
    }
    
    return true;

fail:
    softu2f_hid_msg_free(msg);
    return false;
}

// Free memory for a HID message.
void softu2f_hid_msg_free(U2F_HID_MESSAGE* msg) {
    if (msg) {
        if (msg->data) CFRelease(msg->data);
        free(msg);
    }
}

// Read a frame into a message.
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
