/* 
 * Copyright (c) 2015-2016, Gregory M. Kurtzer. All rights reserved.
 * 
 * “Singularity” Copyright (c) 2016, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * This software is licensed under a customized 3-clause BSD license.  Please
 * consult LICENSE file distributed with the sources of this project regarding
 * your rights to use or distribute this software.
 * 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
*/

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>

#include "file.h"
#include "util.h"
#include "message.h"
#include "config_parser.h"
#include "privilege.h"


int singularity_ns_mnt_unshare(void) {
    config_rewind();
    int slave;

    slave = config_get_key_bool("mount slave", 0);

    priv_escalate();
#ifdef NS_CLONE_FS
    message(DEBUG, "Virtualizing FS namespace\n");
    if ( unshare(CLONE_FS) < 0 ) {
        message(ERROR, "Could not virtualize file system namespace: %s\n", strerror(errno));
        ABORT(255);
    }
#endif

    message(DEBUG, "Virtualizing mount namespace\n");
    if ( unshare(CLONE_NEWNS) < 0 ) {
        message(ERROR, "Could not virtualize mount namespace: %s\n", strerror(errno));
        ABORT(255);
    }

    // Privatize the mount namespaces
    //
#ifdef SINGULARITY_MS_SLAVE
    message(DEBUG, "Making mounts %s\n", (slave ? "slave" : "private"));
    if ( mount(NULL, "/", NULL, (slave ? MS_SLAVE : MS_PRIVATE)|MS_REC, NULL) < 0 ) {
        message(ERROR, "Could not make mountspaces %s: %s\n", (slave ? "slave" : "private"), strerror(errno));
        ABORT(255);
    }
#else
    if ( slave > 0 ) {
        message(WARNING, "Requested option 'mount slave' is not available on this host, using private\n");
    }
    message(DEBUG, "Making mounts private\n");
    if ( mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL) < 0 ) {
        message(ERROR, "Could not make mountspaces %s: %s\n", (slave ? "slave" : "private"), strerror(errno));
        ABORT(255);
    }
#endif

    priv_drop();
    return(0);
}

