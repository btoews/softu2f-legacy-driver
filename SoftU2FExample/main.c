//
//  main.c
//  SoftU2FExample
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "u2f_hid.h"
#include "softu2f.h"
#include <stdio.h>

softu2f_ctx *ctx;

void cleanup(int sig) {
  // Stop the run loop gracefully.
  if (ctx)
      softu2f_shutdown(ctx);
}

bool handle_u2f_msg(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  printf("Received U2F message from device.\n");

  // Send an error, since we don't handle higher level messages yet.
  return softu2f_hid_err_send(ctx, msg->cid, ERR_OTHER);
  ;
}

int main(int argc, const char *argv[]) {
  printf("Initializing connection to driver.\n");
  ctx = softu2f_init(false);

  if (!ctx) {
    printf("Error initializing connection to driver.\n");
    exit(1);
  }

  signal(SIGHUP, cleanup);
  signal(SIGINT, cleanup);
  signal(SIGQUIT, cleanup);
  signal(SIGTERM, cleanup);
  signal(SIGKILL, cleanup);

  printf("Registering handler for high-level U2F messages.\n");
  softu2f_hid_msg_handler_register(ctx, U2FHID_MSG, handle_u2f_msg);

  printf("Listening for HID messages.\n");
  softu2f_run(ctx);

  printf("Shutting down.\n");
  softu2f_deinit(ctx);
}
