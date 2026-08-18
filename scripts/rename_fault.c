/* Link-time red hand for state publication. netta.c is compiled with
   -Drename=netta_test_rename; "fail" returns EXDEV and "stop" suspends at
   the exact pre-rename point so the test can deliver SIGKILL. */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int netta_test_rename(const char *from, const char *to) {
    (void)from;
    (void)to;
    const char *mode = getenv("NETTA_RENAME_FAULT");
    if (mode && !strcmp(mode, "stop")) {
        const char *marker = getenv("NETTA_RENAME_MARK");
        if (marker) {
            int fd = open(marker, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd < 0 || write(fd, "stop\n", 5) != 5 ||
                    close(fd) != 0)
                _exit(124);
        }
        if (raise(SIGSTOP) != 0) _exit(125);
        for (;;) pause();
    }
    errno = EXDEV;
    return -1;
}
