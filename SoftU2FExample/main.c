//
//  main.c
//  SoftU2FExample
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include <stdio.h>
#include <IOKit/IOKitLib.h>
#include "SoftU2FClientInterface.h"
#include "UserKernelShared.h"

void cleanup(int sig) {
    softu2f_deinit();
    exit(0);
}

// IOAsyncCallback0
void callback(void *context, IOReturn result, void** args, int numArgs) {
    fprintf(stderr, "callback called with %d args:\n", numArgs);
    CFRunLoopStop(CFRunLoopGetMain());
}

int main(int argc, const char * argv[]) {
    U2F_HID_MESSAGE* msg;

    if (!softu2f_init()) {
        cleanup(0);
        exit(1);
    }

    signal(SIGHUP, cleanup);
    signal(SIGINT, cleanup);
    signal(SIGQUIT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGKILL, cleanup);
    
    while (1) {
        if (!softu2f_hid_msg_read(&msg)) break;
        
        switch (msg->cmd) {
            case U2FHID_PING:
                fprintf(stderr, "U2FHID_PING\n");
                break;
            case U2FHID_MSG:
                fprintf(stderr, "U2FHID_MSG\n");
                break;
            case U2FHID_LOCK:
                fprintf(stderr, "U2FHID_LOCK\n");
                break;
            case U2FHID_INIT:
                fprintf(stderr, "U2FHID_INIT\n");
                break;
            case U2FHID_WINK:
                fprintf(stderr, "U2FHID_WINK\n");
                break;
            case U2FHID_SYNC:
                fprintf(stderr, "U2FHID_SYNC\n");
                break;
            case U2FHID_ERROR:
                fprintf(stderr, "U2FHID_ERROR\n");
                break;
            default:
                fprintf(stderr, "UNKOWN: 0x%08x\n", msg->cmd);
                break;
        }
        
        softu2f_hid_msg_free(msg);
    }
    
    softu2f_hid_msg_free(msg);
    cleanup(0);
}
