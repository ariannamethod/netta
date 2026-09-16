/* HEAD256-v1 source book. Each input life gets a fresh local frontend. */
#include "frontend.h"
#include "../portable_recurrence/recurrence.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message) {
    fprintf(stderr, "byte_collect: %s\n", message);
    exit(1);
}

static void put32(unsigned char *p, uint32_t v) {
    for (int i = 0; i < 4; i++) p[i] = (unsigned char)(v >> (8 * i));
}
static void put64(unsigned char *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (unsigned char)(v >> (8 * i));
}

int main(int argc, char **argv) {
    if (argc < 3) fail("usage: byte_collect NEW_ARCHIVE.bin SOURCE.bin ...");
    PRArchive archive = {0};
    for (int file = 2; file < argc; file++) {
        FILE *in = fopen(argv[file], "rb");
        BFState *state = bf_create();
        if (!in || !state) fail("open source or allocate frontend");
        for (;;) {
            BFQuote q;
            if (bf_quote(state, &q)) fail("frontend quote");
            int next = fgetc(in);
            if (next == EOF) break;
            if (q.context_len == BF_DEPTH) {
                int g = 0;
                while (g < q.repeat_classes && q.heads[g] != (uint8_t)next) g++;
                if (pr_archive_add(&archive, q.pattern, g)) fail("source count");
            }
            if (bf_observe(state, (uint8_t)next)) fail("frontend observe");
        }
        if (ferror(in) || fclose(in)) fail("source read");
        bf_destroy(state);
    }
    unsigned char bytes[PR_ARCHIVE_BYTES];
    memcpy(bytes, "NETHD256", 8);
    put32(bytes + 8, PR_DEPTH);
    put32(bytes + 12, PR_CELLS);
    for (int i = 0; i < PR_CELLS; i++)
        put64(bytes + 16 + 8 * i, archive.counts[i]);
    FILE *out = fopen(argv[1], "wx");
    if (!out) fail("create new archive (existing file is never overwritten)");
    size_t written = fwrite(bytes, 1, sizeof(bytes), out);
    int closed = fclose(out);
    if (written != sizeof(bytes) || closed)
        fail("archive write");
    return 0;
}
