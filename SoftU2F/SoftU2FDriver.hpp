//
//  SoftU2FDriver.hpp
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.

#ifndef SoftU2FDriver_hpp
#define SoftU2FDriver_hpp

#include <IOKit/IOService.h>
#include "UserKernelShared.h"

class SoftU2FDriverClassName : public IOService
{
    OSDeclareDefaultStructors(com_github_SoftU2FDriver)
    
public:
    // IOService methods
    virtual bool init(OSDictionary* dictionary = 0) override;
    virtual void free(void) override;
    
    virtual IOService* probe(IOService* provider, SInt32* score) override;
    
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
    
    virtual bool willTerminate(IOService* provider, IOOptionBits options) override;
    virtual bool didTerminate(IOService* provider, IOOptionBits options, bool* defer) override;
    
    virtual bool terminate(IOOptionBits options = 0) override;
    virtual bool finalize(IOOptionBits options) override;
    
    virtual OSString* userClientKey(IOService* userClient);
    virtual IOService* userClientDevice(IOService* userClient);
    virtual bool destroyUserClientDevice(IOService* userClient);
    virtual bool userClientDeviceSend(IOService* userClient, U2FHID_FRAME* frame);

private:
    OSDictionary *m_hid_devices;
};

#endif /* SoftU2F_hpp */
