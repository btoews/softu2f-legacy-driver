//
//  SoftU2FDevice.cpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include <IOKit/IOLib.h>
#include "SoftU2FDevice.hpp"
#include "SoftU2FUserClient.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(com_github_SoftU2FDevice, IOHIDDevice)

bool SoftU2FDeviceClassName::init(OSDictionary *dictionary) {
    if (!super::init(dictionary)) {
        return false;
    }
    
    // This IOLog must follow super::init because getName relies on the superclass initialization.
    IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, dictionary);
    
    return true;
}

bool SoftU2FDeviceClassName::start(IOService *provider) {
    IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);
    return super::start(provider);
}

void SoftU2FDeviceClassName::stop(IOService *provider) {
    IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, provider);
    super::stop(provider);
}

void SoftU2FDeviceClassName::free() {
    IOLog("%s[%p]::%s\n", getName(), this, __FUNCTION__);
    
    if (dUserClient) {
        dUserClient->release();
    }
    
    super::free();
}

IOReturn SoftU2FDeviceClassName::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
    IOLog("%s[%p]::%s(%p)\n", getName(), this, __FUNCTION__, descriptor);

    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withBytes(u2fhid_report_descriptor, sizeof(u2fhid_report_descriptor), kIODirectionNone);
    
    if (!buffer) {
        IOLog("Error while allocating new IOBufferMemoryDescriptor.");
        return kIOReturnNoResources;
    }

    *descriptor = buffer;
    
    return kIOReturnSuccess;
}

IOReturn SoftU2FDeviceClassName::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    IOLog("%s[%p]::%s(%p, %d, %d)\n", getName(), this, __FUNCTION__, report, reportType, options);

    if (dUserClient) {
        dUserClient->queueSetReport(report);
    }

    return kIOReturnSuccess;
}

OSString *SoftU2FDeviceClassName::newProductString() const {
    return OSString::withCString("SoftU2F");
}

OSString *SoftU2FDeviceClassName::newSerialNumberString() const {
    return OSString::withCString("123");
}

OSNumber *SoftU2FDeviceClassName::newVendorIDNumber() const {
    return OSNumber::withNumber(123, 32);
}

OSNumber *SoftU2FDeviceClassName::newProductIDNumber() const {
    return OSNumber::withNumber(123, 32);
}

bool SoftU2FDeviceClassName::setUserClient(IOService* userClient) {
    dUserClient = OSDynamicCast(SoftU2FUserClientClassName, userClient);

    if (dUserClient) {
        dUserClient->retain();
        return true;
    } else {
        return false;
    }
}
