/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#define LOG_TAG "su"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <unistd.h>
#include <time.h>

#include <pwd.h>

#include <private/android_filesystem_config.h>

/*
 * SU can be given a specific command to exec. UID _must_ be
 * specified for this (ie argc => 3).
 *
 * Usage:
 * su 1000
 * su 1000 ls -l
 */
int main(int argc, char **argv)
{
    struct passwd *pw;
    int uid, gid, myuid;

    if(argc < 2) {
        uid = gid = 0;
    } else {
        pw = getpwnam(argv[1]);

        if(pw == 0) {
            uid = gid = atoi(argv[1]);
        } else {
            uid = pw->pw_uid;
            gid = pw->pw_gid;
        }
    }

    /* Until we have something better, only root and the shell can use su. */
    myuid = getuid();
    if (myuid != AID_ROOT && myuid != AID_SHELL) {
        fprintf(stderr,"su: uid %d not allowed to su\n", myuid);
        return 1;
    }
    
    if(setgid(gid) || setuid(uid)) {
        fprintf(stderr,"su: permission denied\n");
        return 1;
    }

    /* User specified command for exec. */
    if (argc == 3 ) {
        if (execlp(argv[2], argv[2], NULL) < 0) {
            fprintf(stderr, "su: exec failed for %s Error:%s\n", argv[2],
                    strerror(errno));
            return -errno;
        }
    } else if (argc > 3) {
        int first_arg = 2;

        /*
         * try to be compatible with POSIX su's that needs "-c"
         * which causes an SH to interpret the rest!
         */
        if(argv[first_arg][0]=='-' && argv[first_arg][1]=='c') {
            /* just become sh, with the same arguments, offset by one! */

            argv[1]=argv[0];
            if(execvp("/system/bin/sh", argv+1) < 0) {
                fprintf(stderr, "su: exec(sh) failed for %s Error:%s\n", argv[2],
                        strerror(errno));
                return -errno;
            }

        } else {
            /* Copy the rest of the args from main. */
            char *exec_args[argc - first_arg + 1];
            memset(exec_args, 0, sizeof(exec_args));
            memcpy(exec_args, &argv[first_arg], sizeof(exec_args));
            if (execvp(argv[first_arg], exec_args) < 0) {
                fprintf(stderr, "su: exec failed for %s Error:%s\n", argv[2],
                        strerror(errno));
                return -errno;
            }
        }
    }

    /* Default exec shell. */
    execlp("/system/bin/sh", "sh", NULL);

    fprintf(stderr, "su: exec failed\n");
    return 1;
}
