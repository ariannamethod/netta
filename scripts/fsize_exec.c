/* Test hand for publication failures. Run COMMAND under an exact per-file
   size ceiling; "ignore" turns SIGXFSZ into ordinary write errors, while
   "die" leaves the signal fatal. Never linked into Netta. */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 4) return 2;
    char *end = NULL;
    errno = 0;
    unsigned long long n = strtoull(argv[1], &end, 10);
    if (errno || !end || *end) return 2;
    struct rlimit limit = {(rlim_t)n, (rlim_t)n};
    if (setrlimit(RLIMIT_FSIZE, &limit) != 0) return 2;
    if (!strcmp(argv[2], "ignore")) {
        if (signal(SIGXFSZ, SIG_IGN) == SIG_ERR) return 2;
    } else if (strcmp(argv[2], "die")) {
        return 2;
    }
    execv(argv[3], &argv[3]);
    return 2;
}
