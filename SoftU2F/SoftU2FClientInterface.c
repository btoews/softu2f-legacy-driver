//
//  SoftU2FClientInterface.c
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "SoftU2FClientInterface.h"

io_connect_t* libSoftU2FInit() {
    kern_return_t ret;
    io_service_t service;
    io_iterator_t iterator;
    io_connect_t *connect;
    bool driverFound = false;
    
    ret = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(kSoftU2FDriverClassName), &iterator);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "IOServiceGetMatchingServices returned 0x%08x\n\n", ret);
        return NULL;
    }
    
    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        ret = IOServiceOpen(service, mach_task_self(), 0, connect);
        if (ret == KERN_SUCCESS) {
            driverFound = true;
            break;
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);
    
    if (driverFound == false) {
        fprintf(stderr, "No matching drivers found.\n");
        return NULL;
    }
    
    // Open the user client.
    ret = IOConnectCallScalarMethod(*connect, kSoftU2FUserClientOpen, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to open user client.\n");
        return NULL;
    }
    
    return connect;
}

bool libSoftU2FDeinit(io_connect_t connect) {
    kern_return_t ret;
    
    // Close the user client.
    ret = IOConnectCallScalarMethod(connect, kSoftU2FUserClientClose, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to close user client.\n");
        return false;
    }
    
    // Close user client connection.
    IOServiceClose(connect);
    
    return true;
}
