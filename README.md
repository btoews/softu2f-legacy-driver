# SoftU2F

This is a toolset for implementing HID U2F tokens in software. This includes an OSX driver that emulates HID U2F devices as well as a library for sending/receiving messages using emulated device. See the [included example](SoftU2FExample/main.c) for a demonstration of basic usage.

## TODO

* [ ] Async message from kernel->userland when HID report is received.
* [ ] Upstream driver improvements to [foohid](https://github.com/unbit/foohid) instead of maintaining it here.
