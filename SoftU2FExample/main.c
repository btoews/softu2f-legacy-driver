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

int main(int argc, const char * argv[]) {
    CFDataRef msg;

    if (!softu2f_init()) {
        cleanup(0);
        exit(1);
    }

    signal(SIGHUP, cleanup);
    signal(SIGINT, cleanup);
    signal(SIGQUIT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGKILL, cleanup);
    
    while ((msg = softu2f_u2f_msg_read())) {
        fprintf(stderr, "Received U2F message from device.\n");
        CFRelease(msg);
    }

    cleanup(0);
}
