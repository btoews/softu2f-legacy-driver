//
//  main.c
//  SoftU2FExample
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "SoftU2FClientInterface.h"
#include "UserKernelShared.h"
#include <IOKit/IOKitLib.h>
#include <stdio.h>

softu2f_ctx *ctx;

void cleanup(int sig) {
  softu2f_deinit(ctx);
  exit(0);
}

int main(int argc, const char *argv[]) {
  CFDataRef msg;

  ctx = softu2f_init();
  if (!ctx)
    exit(1);

  signal(SIGHUP, cleanup);
  signal(SIGINT, cleanup);
  signal(SIGQUIT, cleanup);
  signal(SIGTERM, cleanup);
  signal(SIGKILL, cleanup);

  while ((msg = softu2f_u2f_msg_read(ctx))) {
    fprintf(stderr, "Received U2F message from device.\n");
    CFRelease(msg);
  }

  cleanup(0);
}
