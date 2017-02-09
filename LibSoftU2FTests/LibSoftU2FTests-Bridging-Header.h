#import "softu2f.h"
#import "u2f-host.h"
#import "u2f_hid.h"
#import "hidapi.h"

typedef struct u2fdevice {
    struct u2fdevice *next;
    hid_device *devh;
    unsigned id;
    uint32_t cid;
    char *device_string;
    char *device_path;
    int skipped;
    uint8_t versionInterface; // Interface version
    uint8_t versionMajor;     // Major version number
    uint8_t versionMinor;     // Minor version number
    uint8_t versionBuild;     // Build version number
    uint8_t capFlags;         // Capabilities flags
} u2fdevice;

struct u2fh_devs {
    unsigned max_id;
    struct u2fdevice *first;
};
