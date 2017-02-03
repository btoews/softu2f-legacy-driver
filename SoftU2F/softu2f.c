//
//  SoftU2FClientInterface.c
//  SoftU2F
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include "softu2f.h"
#include "internal.h"
#include <sys/time.h>

// Initialize libSoftU2F before usage.
softu2f_ctx *softu2f_init(softu2f_init_flags flags) {
  softu2f_ctx *ctx = NULL;
  io_service_t service = IO_OBJECT_NULL;
  kern_return_t ret;

  // Allocate a new context.
  ctx = (softu2f_ctx *)calloc(1, sizeof(softu2f_ctx));
  if (!ctx)
    return NULL;

  // Apply init flags.
  ctx->debug = (flags & SOFTU2F_DEBUG) == 1;

  // Find driver.
  service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(kSoftU2FDriverClassName));
  if (!service) {
    softu2f_log(ctx, "SoftU2F.kext not loaded.\n");
    return NULL;
  }

  // Open connection to user client.
  ret = IOServiceOpen(service, mach_task_self(), 0, &ctx->con);
  if (ret != KERN_SUCCESS) {
    softu2f_log(ctx, "Error connecting to SoftU2F.kext: %d\n", ret);
    goto fail;
  }
  IOObjectRelease(service);
  service = IO_OBJECT_NULL;

  return ctx;

fail:
  if (service)
    IOObjectRelease(service);
  if (ctx)
    softu2f_deinit(ctx);
  return NULL;
}

// Shutdown the run loop.
void softu2f_shutdown(softu2f_ctx *ctx) {
  ctx->shutdown = true;
}

// Cleanup after using libSoftU2F.
void softu2f_deinit(softu2f_ctx *ctx) {
  kern_return_t ret;

  // Close user client connection.
  if (ctx->con) {
    ret = IOServiceClose(ctx->con);
    if (ret != KERN_SUCCESS)
      softu2f_log(ctx, "Error closing connection to SoftU2F.kext: %d.\n", ret);
  }

  // Cleanup
  free(ctx);
}

// Read HID messages from device in loop.
void softu2f_run(softu2f_ctx *ctx) {
  struct timespec duration;
  duration.tv_sec = 0;
  duration.tv_nsec = 10000000L; // 0.01 seconds

  while (true) {
    // Stop the run loop.
    if (ctx->shutdown)
      break;

    softu2f_hid_read(ctx);
    softu2f_hid_handle_messages(ctx);
    nanosleep(&duration, NULL);
  }
}

// Send a HID message to the device.
bool softu2f_hid_msg_send(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  uint8_t *src;
  uint8_t *src_end;
  uint8_t *dst;
  uint8_t *dst_end;
  uint8_t seq = 0x00;
  U2FHID_FRAME frame;
  kern_return_t ret;

  memset(&frame, 0, HID_RPT_SIZE);

  // Init frame.
  frame.cid = msg->cid;
  frame.type |= TYPE_INIT;
  frame.init.cmd |= msg->cmd;
  frame.init.bcnth = CFDataGetLength(msg->data) >> 8;
  frame.init.bcntl = CFDataGetLength(msg->data) & 0xff;

  src = (uint8_t *)CFDataGetBytePtr(msg->data);
  src_end = src + CFDataGetLength(msg->data);
  dst = frame.init.data;
  dst_end = dst + sizeof(frame.init.data);

  while (1) {
    if (src_end - src > dst_end - dst) {
      memcpy(dst, src, dst_end - dst);
      src += dst_end - dst;
    } else {
      memcpy(dst, src, src_end - src);
      src += src_end - src;
    }

    // Send frame.
    softu2f_debug_frame(ctx, &frame, false);
    ret = IOConnectCallStructMethod(ctx->con, kSoftU2FUserClientSendFrame, &frame, HID_RPT_SIZE, NULL, NULL);
    if (ret != kIOReturnSuccess) {
      softu2f_log(ctx, "Error calling kSoftU2FUserClientSendFrame: 0x%08x\n", ret);
      return false;
    }

    // No more frames.
    if (src >= src_end)
      break;

    // Sleep for a bit.
    nanosleep(&softu2f_poll_interval, NULL);

    // Cont frame.
    dst = frame.cont.data;
    dst_end = dst + sizeof(frame.cont.data);
    frame.cont.seq = seq++;
    memset(frame.cont.data, 0, sizeof(frame.cont.data));
  }

  return true;
}

// Send a HID error to the device.
bool softu2f_hid_err_send(softu2f_ctx *ctx, uint32_t cid, uint8_t code) {
  softu2f_hid_message msg;

  msg.cmd = U2FHID_ERROR;
  msg.cid = cid;
  msg.bcnt = 1;
  msg.data = CFDataCreateWithBytesNoCopy(NULL, &code, 1, NULL);

  return softu2f_hid_msg_send(ctx, &msg);
}

// Read HID frames from the device until there aren't any more.
void softu2f_hid_read(softu2f_ctx *ctx) {
  kern_return_t ret = kIOReturnSuccess;
  U2FHID_FRAME frame = {0};
  unsigned long frame_size = HID_RPT_SIZE;

  while (1) {
    // Stop the run loop.
    if (ctx->shutdown)
      return;

    ret = IOConnectCallStructMethod(ctx->con, kSoftU2FUserClientGetFrame, NULL, 0, &frame, &frame_size);

    if (ret == kIOReturnNoFrames)
      return;

    if (ret != kIOReturnSuccess) {
      softu2f_log(ctx, "error calling kSoftU2FUserClientGetFrame: 0x%08x\n", ret);
      return;
    }

    softu2f_debug_frame(ctx, &frame, true);

    softu2f_hid_frame_read(ctx, &frame);
  }
}

// Read an individual HID frame from the device into a HID message.
void softu2f_hid_frame_read(softu2f_ctx *ctx, U2FHID_FRAME *frame) {
  uint8_t *data;
  unsigned int ndata;
  softu2f_hid_message *msg;

  // See if there's already a message in progress for this channel.
  msg = softu2f_hid_msg_list_find(ctx, frame->cid);

  if (frame->cid == 0x00000000) {
    softu2f_log(ctx, "Frame with CID 0.\n");
    softu2f_hid_err_send(ctx, frame->cid, ERR_INVALID_CID);
    return;
  }

  switch (FRAME_TYPE(*frame)) {
  case TYPE_INIT:
    if (msg) {
      if (frame->init.cmd == U2FHID_INIT) {
        softu2f_log(ctx, "U2FHID_INIT while waiting for CONT. Resetting.\n");
        softu2f_hid_msg_list_remove(ctx, msg);
      } else {
        softu2f_log(ctx, "INIT frame out of order. Bailing.\n");
        softu2f_hid_err_send(ctx, frame->cid, ERR_INVALID_SEQ);
        softu2f_hid_msg_list_remove(ctx, msg);
        return;
      }
    } else if (frame->init.cmd == U2FHID_SYNC) {
      softu2f_log(ctx, "SYNC frame out of order. Bailing.\n");
      softu2f_hid_err_send(ctx, frame->cid, ERR_INVALID_CMD);
      return;
    } else if (frame->init.cmd != U2FHID_INIT && softu2f_hid_msg_list_count(ctx) > 0) {
      softu2f_log(ctx, "INIT frame while waiting for CONT on other CID.\n");
      softu2f_hid_err_send(ctx, frame->cid, ERR_CHANNEL_BUSY);
      return;
    }

    if (frame->cid == CID_BROADCAST && frame->init.cmd != U2FHID_INIT) {
      softu2f_log(ctx, "Non U2FHID_INIT message on broadcast CID.\n");
      softu2f_hid_err_send(ctx, frame->cid, ERR_INVALID_CID);
      return;
    }

    msg = softu2f_hid_msg_list_create(ctx);
    if (!msg)
      return;

    msg->cmd = frame->init.cmd;
    msg->cid = frame->cid;
    msg->bcnt = MSG_LEN(*frame);

    // From the spec: With a packet size of 64 bytes (max for full-speed
    // devices), this means that the maximum message payload length is
    // 64 - 7 + 128 * (64 - 5) = 7609 bytes.
    if (msg->bcnt > 7609) {
      softu2f_log(ctx, "BCNT too large (%u). Bailing.\n", msg->bcnt);
      softu2f_hid_err_send(ctx, msg->cid, ERR_INVALID_LEN);
      softu2f_hid_msg_list_remove(ctx, msg);
      return;
    }

    msg->buf = CFDataCreateMutable(NULL, msg->bcnt);

    data = frame->init.data;

    if (msg->bcnt > sizeof(frame->init.data)) {
      ndata = sizeof(frame->init.data);
    } else {
      ndata = msg->bcnt;
    }

    break;
  case TYPE_CONT:
    if (!msg) {
      softu2f_log(ctx, "CONT frame out of order. Ignoring\n");
      return;
    }

    if (FRAME_SEQ(*frame) != msg->lastSeq++) {
      softu2f_log(ctx, "Bad SEQ in CONT frame (%d). Bailing\n", FRAME_SEQ(*frame));
      softu2f_hid_msg_list_remove(ctx, msg);
      softu2f_hid_err_send(ctx, frame->cid, ERR_INVALID_SEQ);
      return;
    }

    data = frame->cont.data;

    if (CFDataGetLength(msg->buf) + sizeof(frame->cont.data) > msg->bcnt) {
      ndata = msg->bcnt - (uint16_t)CFDataGetLength(msg->buf);
    } else {
      ndata = sizeof(frame->cont.data);
    }

    break;
  default:
    softu2f_log(ctx, "Unknown frame type: 0x%08x\n", FRAME_TYPE(*frame));
    return;
  }

  CFDataAppendBytes(msg->buf, data, ndata);
}

// Handle complete messages. Abort messages that timed out.
void softu2f_hid_handle_messages(softu2f_ctx *ctx) {
  softu2f_hid_message *msg = NULL;
  softu2f_hid_message *nextMsg = ctx->msg_list;
  softu2f_hid_message_handler handler = NULL;

  while (nextMsg) {
    msg = nextMsg;
    nextMsg = msg->next;

    if (softu2f_hid_msg_is_complete(ctx, msg)) {
      softu2f_hid_msg_finalize(ctx, msg);
      handler = softu2f_hid_msg_handler(ctx, msg);

      if (handler) {
        if (!handler(ctx, msg)) {
          softu2f_log(ctx, "Error handling HID message\n");
        }
      } else {
        softu2f_log(ctx, "No handler for HID message\n");
        softu2f_hid_err_send(ctx, msg->cid, ERR_INVALID_CMD);
      }

      softu2f_hid_msg_list_remove(ctx, msg);
    } else if (softu2f_hid_msg_is_timed_out(ctx, msg)) {
      softu2f_log(ctx, "Message timeout on CID: 0x%08x\n", msg->cid);
      softu2f_hid_err_send(ctx, msg->cid, ERR_MSG_TIMEOUT);
      softu2f_hid_msg_list_remove(ctx, msg);
    }

    handler = NULL;
  }
}

// Register a handler for a message type.
void softu2f_hid_msg_handler_register(softu2f_ctx *ctx, uint8_t type, softu2f_hid_message_handler handler) {
  switch (type) {
  case U2FHID_PING:
    ctx->ping_handler = handler;
    break;
  case U2FHID_MSG:
    ctx->msg_handler = handler;
    break;
  case U2FHID_INIT:
    ctx->init_handler = handler;
    break;
  case U2FHID_WINK:
    ctx->wink_handler = handler;
    break;
  case U2FHID_SYNC:
    ctx->sync_handler = handler;
    break;
  }
}

// Find a message handler for a message.
softu2f_hid_message_handler softu2f_hid_msg_handler(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  switch (msg->cmd) {
  case U2FHID_PING:
    if (ctx->ping_handler)
      return ctx->ping_handler;
    break;
  case U2FHID_MSG:
    if (ctx->msg_handler)
      return ctx->msg_handler;
    break;
  case U2FHID_INIT:
    if (ctx->init_handler)
      return ctx->init_handler;
    break;
  case U2FHID_WINK:
    if (ctx->wink_handler)
      return ctx->wink_handler;
    break;
  case U2FHID_SYNC:
    if (ctx->sync_handler)
      return ctx->sync_handler;
    break;
  }

  return softu2f_hid_msg_handler_default(ctx, msg);
}

// Find the default message handler for a message.
softu2f_hid_message_handler softu2f_hid_msg_handler_default(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  switch (msg->cmd) {
  case U2FHID_PING:
    return softu2f_hid_msg_handle_ping;
  case U2FHID_MSG:
    return NULL;
  case U2FHID_INIT:
    return softu2f_hid_msg_handle_init;
  case U2FHID_WINK:
    return softu2f_hid_msg_handle_wink;
  case U2FHID_SYNC:
    return softu2f_hid_msg_handle_sync;
  default:
    return NULL;
  }
}

// Send an INIT response for a given request.
bool softu2f_hid_msg_handle_init(softu2f_ctx *ctx, softu2f_hid_message *req) {
  softu2f_hid_message resp;
  U2FHID_INIT_RESP resp_data = {0};
  U2FHID_INIT_REQ *req_data;

  req_data = (U2FHID_INIT_REQ *)CFDataGetBytePtr(req->data);

  resp.cmd = U2FHID_INIT;
  resp.bcnt = sizeof(U2FHID_INIT_RESP);
  resp.data = CFDataCreateWithBytesNoCopy(NULL, (uint8_t *)&resp_data, resp.bcnt, NULL);

  if (req->cid == CID_BROADCAST) {
    // Allocate a new CID for the client and tell them about it.
    resp.cid = CID_BROADCAST;
    resp_data.cid = ++ctx->next_cid;
  } else {
    // Use whatever CID they wanted.
    resp.cid = req->cid;
    resp_data.cid = req->cid;
  }

  memcpy(resp_data.nonce, req_data->nonce, INIT_NONCE_SIZE);
  resp_data.versionInterface = U2FHID_IF_VERSION;
  resp_data.versionMajor = 0;
  resp_data.versionMinor = 0;
  resp_data.versionBuild = 0;
  resp_data.capFlags |= CAPFLAG_WINK;

  return softu2f_hid_msg_send(ctx, &resp);
}

// Send a PING response for a given request.
bool softu2f_hid_msg_handle_ping(softu2f_ctx *ctx, softu2f_hid_message *req) {
  softu2f_hid_message resp;

  resp.cid = req->cid;
  resp.cmd = U2FHID_PING;
  resp.bcnt = req->bcnt;
  resp.data = req->data;

  return softu2f_hid_msg_send(ctx, &resp);
}

// Send a WINK response for a given request.
bool softu2f_hid_msg_handle_wink(softu2f_ctx *ctx, softu2f_hid_message *req) {
  softu2f_hid_message resp;

  resp.cid = req->cid;
  resp.cmd = U2FHID_WINK;
  resp.bcnt = req->bcnt;
  resp.data = req->data;

  return softu2f_hid_msg_send(ctx, &resp);
}

// Send a SYNC response for a given request.
bool softu2f_hid_msg_handle_sync(softu2f_ctx *ctx, softu2f_hid_message *req) {
  softu2f_hid_message resp;

  resp.cid = req->cid;
  resp.cmd = U2FHID_SYNC;
  resp.bcnt = req->bcnt;
  resp.data = req->data;

  return softu2f_hid_msg_send(ctx, &resp);
}

// Create a new message and add it to the list.
softu2f_hid_message *softu2f_hid_msg_list_create(softu2f_ctx *ctx) {
  softu2f_hid_message *msg;
  softu2f_hid_message *last_msg = NULL;

  msg = softu2f_hid_msg_alloc(ctx);
  if (!msg)
    return NULL;

  // No messages in list. Start a new list.
  if (!ctx->msg_list) {
    ctx->msg_list = msg;
    return msg;
  }

  // Add new message to end of list.
  last_msg = ctx->msg_list;
  while (last_msg->next) {
    last_msg = last_msg->next;
  }
  last_msg->next = msg;

  return msg;
}

// Find a message with the given cid.
softu2f_hid_message *softu2f_hid_msg_list_find(softu2f_ctx *ctx, uint32_t cid) {
  softu2f_hid_message *msg = ctx->msg_list;

  while (msg) {
    if (msg->cid == cid)
      break;

    msg = msg->next;
  }

  return msg;
}

// Get size of message list.
unsigned int softu2f_hid_msg_list_count(softu2f_ctx *ctx) {
  softu2f_hid_message *msg = ctx->msg_list;
  unsigned int count;

  while (msg) {
    count++;
    msg = msg->next;
  }

  return count;
}

// Remove a message from the list and free it.
void softu2f_hid_msg_list_remove(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  softu2f_hid_message *previous;

  // msg is first.
  if (msg == ctx->msg_list) {
    ctx->msg_list = msg->next;
    softu2f_hid_msg_free(msg);
    return;
  }

  // find previous msg.
  previous = ctx->msg_list;
  while (previous->next != msg) {
    previous = previous->next;
  }

  // Remove msg from list.
  previous->next = msg->next;
  softu2f_hid_msg_free(msg);
}

// Allocate memory for a new message.
softu2f_hid_message *softu2f_hid_msg_alloc(softu2f_ctx *ctx) {
  softu2f_hid_message *msg;

  msg = (softu2f_hid_message *)calloc(1, sizeof(softu2f_hid_message));

  if (!msg) {
    softu2f_log(ctx, "No memory for new message.\n");
    return NULL;
  }

  // Make note of when message was created.
  gettimeofday(&msg->start, NULL);

  return msg;
}

// Check if the message has timed out.
bool softu2f_hid_msg_is_timed_out(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  struct timeval now, delta;
  gettimeofday(&now, NULL);

  timersub(&now, &msg->start, &delta);

  // Spec says 3 seconds (U2FHID_TRANS_TIMEOUT)
  // Conformance test expects 0.5 seconds though.
  return delta.tv_usec > 500000L;
}

// Check if we've read the whole message.
bool softu2f_hid_msg_is_complete(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  if (msg && msg->buf) {
    if (CFDataGetLength(msg->buf) == msg->bcnt) {
      return true;
    }
  }

  return false;
}

// Initialize the message's data with the contents of its read buffer.
void softu2f_hid_msg_finalize(softu2f_ctx *ctx, softu2f_hid_message *msg) {
  msg->data = CFDataCreateCopy(NULL, msg->buf);
  CFRelease(msg->buf);
  msg->buf = NULL;
}

// Free a HID message and associated data.
void softu2f_hid_msg_free(softu2f_hid_message *msg) {
  if (msg) {
    if (msg->data)
      CFRelease(msg->data);
    if (msg->buf)
      CFRelease(msg->buf);
    free(msg);
  }
}

void softu2f_log(softu2f_ctx *ctx, char *fmt, ...) {
  if (ctx->debug) {
    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
  }
}

void softu2f_debug_frame(softu2f_ctx *ctx, U2FHID_FRAME *frame, bool recv) {
  uint8_t *data = NULL;
  uint16_t dlen = 0;

  if (recv) {
    softu2f_log(ctx, "Received frame:\n");
  } else {
    softu2f_log(ctx, "Sending frame:\n");
  }

  softu2f_log(ctx, "\tCID: 0x%08x\n", frame->cid);

  switch (FRAME_TYPE(*frame)) {
  case TYPE_INIT:
    softu2f_log(ctx, "\tTYPE: INIT\n");
    softu2f_log(ctx, "\tCMD: 0x%02x\n", frame->init.cmd & ~TYPE_MASK);
    softu2f_log(ctx, "\tBCNTH: 0x%02x\n", frame->init.bcnth);
    softu2f_log(ctx, "\tBCNTL: 0x%02x\n", frame->init.bcntl);
    data = frame->init.data;
    dlen = HID_RPT_SIZE - 7;

    break;

  case TYPE_CONT:
    softu2f_log(ctx, "\tTYPE: CONT\n");
    softu2f_log(ctx, "\tSEQ: 0x%02x\n", frame->cont.seq);
    data = frame->cont.data;
    dlen = HID_RPT_SIZE - 5;

    break;
  }

  softu2f_log(ctx, "\tDATA:");
  for (int i = 0; i < dlen; i++) {
    softu2f_log(ctx, " %02x", data[i]);
  }

  softu2f_log(ctx, "\n\n");
}
