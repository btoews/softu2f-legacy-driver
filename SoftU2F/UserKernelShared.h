//
//  UserKernelShared.h
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#ifndef UserKernelShared_h
#define UserKernelShared_h

#define SoftU2FDriverClassName com_github_SoftU2FDriver
#define kSoftU2FDriverClassName "com_github_SoftU2FDriver"

// User client method dispatch selectors.
enum {
  kSoftU2FUserClientSendFrame,
  kSoftU2FUserClientNotifyFrame,
  kNumberOfMethods // Must be last
};

#endif /* UserKernelShared_h */
