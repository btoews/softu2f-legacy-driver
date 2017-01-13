//
//  main.c
//  SoftU2FExample
//
//  Created by Benjamin P Toews on 1/12/17.
//  Copyright © 2017 GitHub. All rights reserved.
//

#include <stdio.h>
#include "SoftU2FClientInterface.h"

io_connect_t* u2fConnect;

void cleanup(int sig) {
    libSoftU2FDeinit(*u2fConnect);
    exit(0);
}

int main(int argc, const char * argv[]) {
    u2fConnect = libSoftU2FInit();
    if (u2fConnect) {
        signal(SIGHUP, cleanup);
        signal(SIGINT, cleanup);
        signal(SIGQUIT, cleanup);
        signal(SIGTERM, cleanup);

        for(;;) {
            printf("sleeping...\n");
            sleep(10);
        }
    }
    
    return 0;
}
