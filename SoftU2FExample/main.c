//
//  main.c
//  SoftU2FExample
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include <stdio.h>
#include <IOKit/IOKitLib.h>
#include <mach/mach.h>
#include "UserKernelShared.h"
#include "SoftU2FClientInterface.h"

io_connect_t* u2fConnect;

void cleanup(int sig) {
    kern_return_t ret;
    
    // Close the user client.
    ret = IOConnectCallScalarMethod(*u2fConnect, kSoftU2FUserClientClose, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to close user client.\n");
    }
    
    // Close user client connection.
    IOServiceClose(*u2fConnect);
    
    exit(0);
}

// IOAsyncCallback0
void callback(void *context, IOReturn result) {
    fprintf(stderr, "callback called\n");
    CFRunLoopStop(CFRunLoopGetMain());
}

int main(int argc, const char * argv[]) {
    kern_return_t ret;
    io_service_t service;
    io_connect_t connect;
    io_async_ref64_t asyncRef;
    IONotificationPortRef port;
    CFRunLoopSourceRef runLoopSource;
    
    service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(kSoftU2FDriverClassName));
    if (!service) {
        fprintf(stderr, "SoftU2F.kext not loaded.\n");
        exit(1);
    }
    
    ret = IOServiceOpen(service, mach_task_self(), 0, &connect);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Error connecting to SoftU2F.kext: %d\n", ret);
        exit(1);
    }
    IOObjectRelease(service);
    u2fConnect = &connect;
    
    // Open the user client.
    ret = IOConnectCallScalarMethod(*u2fConnect, kSoftU2FUserClientOpen, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "Unable to open user client.\n");
        exit(1);
    }

    signal(SIGHUP, cleanup);
    signal(SIGINT, cleanup);
    signal(SIGQUIT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGKILL, cleanup);
    
    port = IONotificationPortCreate(kIOMasterPortDefault);
    runLoopSource = IONotificationPortGetRunLoopSource(port);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    
    asyncRef[kIOAsyncCalloutFuncIndex] = (uint64_t)callback;
    asyncRef[kIOAsyncCalloutRefconIndex] = (uint64_t)(void*)0xdeadbeef;
    
    // Register async handler
    ret = IOConnectCallAsyncScalarMethod(*u2fConnect, kSoftU2FClientRegisterAsync, IONotificationPortGetMachPort(port), asyncRef, kIOAsyncCalloutCount, NULL, 0, NULL, NULL);
    if (ret != kIOReturnSuccess) {
        fprintf(stderr, "error calling kSoftU2FClientRegisterAsync: %d\n", ret);
        cleanup(0);
        exit(1);
    }
    
    // Fire async handler
    ret = IOConnectCallScalarMethod(*u2fConnect, kSoftU2FClientFireAsync, NULL, 0, NULL, NULL);
    if (ret != KERN_SUCCESS) {
        fprintf(stderr, "error calling kSoftU2FClientFireAsync: %d\n", ret);
        cleanup(0);
        exit(1);
    }
    
    CFRunLoopRun();

    for(;;) {
        fprintf(stderr, "sleeping...\n");
        sleep(10);
    }
    
    return 0;
}
