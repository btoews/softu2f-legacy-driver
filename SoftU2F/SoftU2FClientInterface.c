//
//  SoftU2FClientInterface.c
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "SoftU2FClientInterface.h"

// Initialize libSoftU2F before usage.
bool libSoftU2FInit() {
    if (!libSoftU2FInitConnection()) return false;
    if (!libSoftU2FOpenUserClient()) return false;

    return true;
}

// Open connection to user client.
bool libSoftU2FInitConnection() {
    kern_return_t ret;
    io_service_t service;
    
    service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(kSoftU2FDriverClassName));
    if (!service) {
        fprintf(stderr, "SoftU2F.kext not loaded.\n");
        return false;
    }

    ret = IOServiceOpen(service, mach_task_self(), 0, libSoftU2FConnection);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Error connecting to SoftU2F.kext: %d\n", ret);
        return false;
    }
    IOObjectRetain(*libSoftU2FConnection);
    IOObjectRelease(service);

    return true;
}

// Initialize connection to user client.
bool libSoftU2FOpenUserClient() {
    kern_return_t ret;
    
    ret = IOConnectCallScalarMethod(*libSoftU2FConnection, kSoftU2FUserClientOpen, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to open user client: %d.\n", ret);
        return false;
    }
    
    return true;
}

// Cleanup after using libSoftU2F.
bool libSoftU2FDeinit() {
    if (!libSoftU2FCloseUserClient()) return false;
    if (!libSoftU2FDeinitConnection()) return false;
    
    return true;
}

// Deinitialize connection to user client.
bool libSoftU2FCloseUserClient() {
    kern_return_t ret;

    ret = IOConnectCallScalarMethod(*libSoftU2FConnection, kSoftU2FUserClientClose, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to close user client: %d.\n", ret);
        return false;
    }

    return true;
}

// Close user client connection.
bool libSoftU2FDeinitConnection() {
    kern_return_t ret;
    
    ret = IOServiceClose(*libSoftU2FConnection);
    
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Error closing connection to SoftU2F.kext: %d.\n", ret);
        return false;
    }
    
    IOObjectRelease(*libSoftU2FConnection);
    
    return true;
}
