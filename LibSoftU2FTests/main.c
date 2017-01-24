//
//  main.c
//  LibSoftU2FTests
//
//  Created by Benjamin P Toews on 1/20/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <unistd.h>
#include "u2f-host.h"
#include "u2f_hid.h"

u2fh_devs *devs = NULL;

// Test basic PING request/response.
static void test_ping(void **state) {
  unsigned char data[] = "hello";
  unsigned char resp[1024];
  size_t resplen = sizeof(resp);
  u2fh_sendrecv(devs, 0, U2FHID_PING, data, sizeof(data), resp, &resplen);

  assert_int_equal(sizeof(data), resplen);
  assert_string_equal(data, resp);
}

// Test long PING (message fragmentation).
static void test_long_ping(void **state) {
  unsigned char data[] = "9dac044c027bf00e1505b32b19a42053dee08f7a8e971e17e447a86d393745591ab720559cb65b0c";
  unsigned char resp[1024];
  size_t resplen = sizeof(resp);
  u2fh_sendrecv(devs, 0, U2FHID_PING, data, sizeof(data), resp, &resplen);

  assert_int_equal(sizeof(data), resplen);
  assert_string_equal(data, resp);
}



static int setup(void **state) {
  int rc;
  unsigned int max_dev_idx = 0;

  // Init libu2f-host.
  rc = u2fh_global_init(0); // U2FH_DEBUG for debugging.
  if (rc != U2FH_OK) {
    printf("Error initializing libu2f-host: %s\n", u2fh_strerror(rc));
    return -1;
  }

  rc = u2fh_devs_init(&devs);
  if (rc) {
    printf("Error initializing libu2f-host devs.\n");
    return -1;
  }

  while (u2fh_devs_discover(devs, &max_dev_idx)) {
    u2fh_devs_done(devs);

    rc = u2fh_devs_init(&devs);
    if (rc) {
      printf("Error initializing libu2f-host devs.\n");
      return -1;
    }

    printf("libu2f-host couldn't find soft u2f device. Trying again.\n");
    sleep(1);
  }

  if (max_dev_idx != 0) {
    printf("libu2f-host found multiple devices.\n");
    return -1;
  }

  return 0;
}

static int teardown(void **state) {
  if (devs) u2fh_devs_done(devs);
  devs = NULL;

  u2fh_global_done();
  
  return 0;
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_ping),
    cmocka_unit_test(test_long_ping),
  };

  return cmocka_run_group_tests(tests, setup, teardown);
}
