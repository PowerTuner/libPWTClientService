# PWTClientService

PowerTuner shared library for clients.

PowerTuner client service to send/receive to/from PowerTuner daemon/service.

Client service is run in its own thread.

## Stand-alone build

To build the library stand-alone, enable the option _DEV_BUILD_SETUP_.

Example command:
> cmake -B build -DDEV_BUILD_SETUP=ON

The required build components will be downloaded from github.
