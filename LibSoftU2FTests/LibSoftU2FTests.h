//
//  LibSoftU2FTests.h
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/24/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#ifndef LibSoftU2FTests_h
#define LibSoftU2FTests_h

#include <hidapi.h>

typedef struct u2fdevice
{
  struct u2fdevice *next;
  hid_device *devh;
  unsigned id;
  uint32_t cid;
  char *device_string;
  char *device_path;
  int skipped;
  uint8_t versionInterface;	// Interface version
  uint8_t versionMajor;		// Major version number
  uint8_t versionMinor;		// Minor version number
  uint8_t versionBuild;		// Build version number
  uint8_t capFlags;		// Capabilities flags
} u2fdevice;

struct u2fh_devs
{
  unsigned max_id;
  struct u2fdevice *first;
};

// Test INIT request/response.
void test_init(void **state);

// Test CID gets incremented between clients.
void test_cid_increment(void **state);

// Test basic PING request/response.
void test_ping(void **state);

// Test long PING (message fragmentation).
void test_long_ping(void **state);

// Test WINK request/response.
void test_wink(void **state);

// Test LOCK request/response.
void test_lock(void **state);

u2fdevice *get_device();

int setup(void **state);

int teardown(void **state);


#endif /* LibSoftU2FTests_h */
