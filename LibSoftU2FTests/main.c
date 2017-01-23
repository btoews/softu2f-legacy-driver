//
//  main.c
//  LibSoftU2FTests
//
//  Created by Benjamin P Toews on 1/20/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"
#include "SoftU2FClientInterface.h"
#include "u2f-host.h"
#include <pthread.h>

softu2f_ctx *ctx;
pthread_t runner;

void *run_device(void *args) {
  softu2f_run(ctx);
  return NULL;
}

static int setup() {
  u2fh_rc ret;
  int err;

  ctx = softu2f_init();
  if (!ctx) {
    printf("Error initializing softu2f.\n");
    return 1;
  }

  ret = u2fh_global_init(0);
  if (ret != U2FH_OK) {
    printf("Error initializing libu2f-host: %s\n", u2fh_strerror(ret));
    return 1;
  }

  err = pthread_create(&runner, NULL, run_device, NULL);
  if (err) {
    printf("Error creating runner thread: %d.\n", err);
    return 1;
  }

  sleep(1);

  return 0;
}

static int teardown() {
  ctx->shutdown = true;
  pthread_join(runner, NULL);

  softu2f_deinit(ctx);
  u2fh_global_done();
  return 0;
}

static void test_creates_device(void **state) {
  u2fh_rc rc;
  u2fh_devs *devs;

  rc = u2fh_devs_init (&devs);
  assert_string_equal(u2fh_strerror(rc), u2fh_strerror(U2FH_OK));

  rc = u2fh_devs_discover (devs, NULL);
  assert_string_equal(u2fh_strerror(rc), u2fh_strerror(U2FH_OK));

  u2fh_devs_done(devs);
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_creates_device),
  };

  return cmocka_run_group_tests(tests, setup, teardown);
}
