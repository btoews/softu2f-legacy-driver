# SoftU2F

This is a toolset for implementing HID U2F tokens in software. This includes an OSX driver that emulates HID U2F devices as well as a library for sending/receiving messages using emulated device. See the [included example](SoftU2FExample/main.c) for a demonstration of basic usage. See [SoftU2FTool](https://github.com/mastahyeti/SoftU2FTool) for an example of a fully functional U2F authenticator.

## Building

You must have Xcode Command Line Tools installed to build this project.

```bash
# Install Commaned Line Tools
xcode-select --install

# Build softu2f.kext and libsoftu2f.a.
script/build
```

## Loading kernel extension

I'm waiting on Apple to get a certificate for signing kernel extension. In the meantime, you'll have to [disable System Integrity Protection](https://developer.apple.com/library/content/documentation/Security/Conceptual/System_Integrity_Protection_Guide/ConfiguringSystemIntegrityProtection/ConfiguringSystemIntegrityProtection.html#//apple_ref/doc/uid/TP40016462-CH5-SW1) before trying to load `softu2f.kext`.

```bash
# Build and load softu2f.kext (requires sudo)
script/load
```


## Usage

### Initialize/deinitialize the library

```c
#include "softu2f.h"

void main() {
  softu2f_ctx *ctx = softu2f_init(SOFTU2F_DEBUG);

  // do stuff...

  softu2f_deinit(ctx);  
}
```

### Handle HID messages from clients

```c
#include "softu2f.h"

// Called with HID messages while the main run loop is going.
bool handle_message(softu2f_ctx *ctx, softu2f_hid_message *req) {
  printf("Received U2F message (code %d) on channel %d.\n", req->cmd, req->cid);

  // Stop the main run loop once we receive a message.
  softu2f_shutdown(ctx);

  // Indicate that we didn't actually "handle" the message.
  return false;
}

void main() {
  // initialize...

  // Register callback for any U2F-level messages.
  softu2f_hid_msg_handler_register(ctx, U2FHID_MSG, handle_message);

  // Wait for messages.
  softu2f_run(ctx);

  // deinitialize...
}
```

### Send HID messages to clients

```c
#include "softu2f.h"

bool handle_message(softu2f_ctx *ctx, softu2f_hid_message *req) {
  bool ret;
  softu2f_hid_message *resp;

  // Build a response to send.
  resp = build_response(req);
  if (!resp) {
    printf("Error processing request.\n");
    return false;
  }

  // Send the response to the client.
  ret = softu2f_hid_msg_send(ctx, resp);
  if (!ret) {
    printf("Error sending response.\n");
  }

  // Deallocate memory from response.
  softu2f_hid_msg_free(resp);

  // Indicate whether we were able to handle the message.
  return ret;
}

softu2f_hid_message *build_response(softu2f_hid_message *req) {
  // Process U2F level message and build response...
}

void main() {
  // initialize, register message/signal handlers, deinitialize...
}
```
