/* Turn30: distinct A/B histories against pooled counts on one address set.
   The turn13 predictor law, the turn28 NETEB001 bank format and the turn29
   binary router are reused; new here are the three-way local router and the
   life's single shared admission. */
#define main inherited_episode_main
#include ".build/turn13/episode.c"
#undef main

#define R3_ARMS 4
#define R3_SHARE 0x1p-10
#define R3_PRIOR 0.875

enum { R3_BANK3, R3_POOLED2, R3_PERMUTED2, R3_COLD };
static const char *const r3_names[R3_ARMS] = {"bank3", "pooled2", "permuted2", "cold"};
/* The turn29 prior mass, split equally between the two stored histories. */
static const double r3_prior3[3] = {1.0 - R3_PRIOR, R3_PRIOR / 2.0, R3_PRIOR / 2.0};

typedef struct { double weight[3]; } R3Three;
typedef struct { double memory; } R3Two;
typedef struct {
    /* Rules/expansions are shared by the two books; branch arrays are owned
       separately. Only book[0] owns the shared allocations. */
    Grammar book[2];
    size_t bytes;
} R3Bank;

static void r3_init3(R3Three *router) {
    memcpy(router->weight, r3_prior3, sizeof(r3_prior3));
}

static void r3_check3(const R3Three *router) {
    double sum = 0.0;
    for (int i = 0; i < 3; i++) {
        if (!isfinite(router->weight[i]) || router->weight[i] <= 0.0 ||
            router->weight[i] > 1.0) fail("invalid three-way router weight");
        sum += router->weight[i];
    }
    if (fabs(sum - 1.0) > 1e-10) fail("three-way router normalization");
}

static void r3_init2(R3Two *router) { router->memory = R3_PRIOR; }

static void r3_check2(const R3Two *router) {
    if (!isfinite(router->memory) || router->memory <= 0.0 || router->memory >= 1.0)
        fail("invalid binary router weight");
}

static void r3_quote3(const R3Three *router, const double cold[256],
                      const double a[256], const double b[256], double out[256]) {
    r3_check3(router);
    double lw[3];
    for (int i = 0; i < 3; i++) lw[i] = log2(router->weight[i]);
    for (int byte = 0; byte < 256; byte++)
        out[byte] = a[byte] == cold[byte] && b[byte] == cold[byte]
            ? cold[byte]
            : la(la(lw[0] + cold[byte], lw[1] + a[byte]), lw[2] + b[byte]);
}

static void r3_observe3(R3Three *router, double cold, double a, double b, double quoted) {
    double prices[3] = {cold, a, b}, posterior[3], total = 0.0;
    for (int i = 0; i < 3; i++) {
        posterior[i] = router->weight[i] * exp2(prices[i] - quoted);
        total += posterior[i];
    }
    if (!(total > 0.0) || !isfinite(total)) fail("invalid three-way posterior mass");
    R3Three next;
    for (int i = 0; i < 3; i++)
        next.weight[i] = (1.0 - R3_SHARE) * (posterior[i] / total) + R3_SHARE * r3_prior3[i];
    r3_check3(&next);
    *router = next;
}

static void r3_quote2(const R3Two *router, const double cold[256],
                      const double source[256], double out[256]) {
    r3_check2(router);
    double local = log2(1.0 - router->memory), memory = log2(router->memory);
    for (int byte = 0; byte < 256; byte++)
        out[byte] = source[byte] == cold[byte]
            ? cold[byte] : la(local + cold[byte], memory + source[byte]);
}

static void r3_observe2(R3Two *router, double source, double quoted) {
    double posterior = router->memory * exp2(source - quoted);
    router->memory = (1.0 - R3_SHARE) * posterior + R3_SHARE * R3_PRIOR;
    r3_check2(router);
}

/* NETEB001 loader and record view, inherited from turn28 unchanged. */
static R3Bank r3_load_bank(const char *path) {
    R3Bank bank = {0};
    unsigned char *bytes = read_bytes(path, MAX_ARCHIVE_BYTES, &bank.bytes);
    if (bank.bytes < 16 || memcmp(bytes, "NETEB001", 8)) fail("invalid NETEB001 header");
    size_t nr = le32(bytes + 8), nb = le32(bytes + 12);
    if (nr > MAX_RULES || nb > MAX_RECORDS || bank.bytes != 16 + 4 * nr + 32 * nb)
        fail("invalid NETEB001 count/size");
    Grammar *g = &bank.book[0];
    g->count = nr;
    g->records = nb;
    g->bytes = bank.bytes;
    if (nr) {
        g->rule = calloc(nr, sizeof(*g->rule));
        g->forward = calloc(nr, sizeof(*g->forward));
        if (!g->rule || !g->forward) fail("allocate bank grammar");
    }
    for (size_t i = 0; i < nr; i++) {
        const unsigned char *p = bytes + 16 + 4 * i;
        g->rule[i] = (Rule){le16(p), le16(p + 2)};
        if (g->rule[i].left >= 7 + i || g->rule[i].right >= 7 + i)
            fail("invalid bank topology");
        append_child(&g->forward[i], g->rule[i].left, g->forward);
        append_child(&g->forward[i], g->rule[i].right, g->forward);
    }
    bank.book[1] = *g;
    for (int c = 0; c < 2; c++) {
        if (nb) {
            bank.book[c].branch = calloc(nb, sizeof(Branch));
            if (!bank.book[c].branch) fail("allocate bank records");
        }
    }
    for (size_t i = 0; i < nb; i++) {
        const unsigned char *p = bytes + 16 + 4 * nr + 32 * i;
        if (le16(p + 2)) fail("nonzero NETEB001 reserved bytes");
        for (int c = 0; c < 2; c++) {
            Branch *b = &bank.book[c].branch[i];
            b->rule_id = p[0];
            b->prefix_len = p[1];
            for (int j = 0; j < 7; j++) b->counts[j] = le16(p + 4 + 14 * c + 2 * j);
            if (b->rule_id >= nr || !b->prefix_len ||
                b->prefix_len >= g->forward[b->rule_id].length)
                fail("invalid bank record address");
        }
        const Branch *b = &g->branch[i];
        for (size_t j = 0; j < i; j++)
            if (g->branch[j].prefix_len == b->prefix_len &&
                !memcmp(g->forward[g->branch[j].rule_id].event,
                        g->forward[b->rule_id].event, b->prefix_len))
                fail("duplicate bank prefix content");
    }
    free(bytes);
    return bank;
}

static Match r3_record_match(const Grammar *g, int record) {
    Match match = {0};
    if (record < 0) return match;
    if ((size_t)record >= g->records) fail("record index outside bank");
    const Branch *b = &g->branch[record];
    match.length = b->prefix_len;
    match.nmatches = 1;
    match.matches[0] = record;
    for (int j = 0; j < 7; j++) match.votes[j] = b->counts[j];
    return match;
}

static void r3_same_rules(const Grammar *a, const Grammar *b) {
    if (a->count != b->count) fail("bank/pooled rule count differs");
    for (size_t i = 0; i < a->count; i++)
        if (a->rule[i].left != b->rule[i].left || a->rule[i].right != b->rule[i].right)
            fail("bank/pooled grammar differs");
}

/* One address set for all three archives: the same 12 records, A+B pooled
   exactly, and the pooled null carrying the rotated counts of that pool. */
static void r3_check_addresses(const R3Bank *bank, const Grammar *small,
                               const Grammar *permuted) {
    const Grammar *a = &bank->book[0], *b = &bank->book[1];
    r3_same_rules(a, small);
    r3_same_rules(a, permuted);
    size_t capacity = (MAX_ARCHIVE_BYTES - 16 - 4 * a->count) / 32;
    if (!a->records || a->records > capacity || small->records != a->records ||
        permuted->records != a->records) fail("bank/pooled record projection differs");
    for (size_t i = 0; i < a->records; i++) {
        if (a->branch[i].rule_id != small->branch[i].rule_id ||
            a->branch[i].prefix_len != small->branch[i].prefix_len ||
            a->branch[i].rule_id != permuted->branch[i].rule_id ||
            a->branch[i].prefix_len != permuted->branch[i].prefix_len)
            fail("bank/pooled selected address differs");
        for (int r = 0; r < 7; r++) {
            unsigned total = (unsigned)a->branch[i].counts[r] + b->branch[i].counts[r];
            if (total > UINT16_MAX || total != small->branch[i].counts[r])
                fail("source-case pooled count mismatch");
            int old = r ? r % 6 + 1 : 0;
            if (permuted->branch[i].counts[r] != small->branch[i].counts[old])
                fail("permuted pooled count mismatch");
        }
    }
}

/* The inherited outer law, with admission supplied by the life's single
   shared event instead of this arm's own shadow. */
static int r3_observe_outer(Outer *s, size_t t, double cold, double cand,
                            double live, int admit) {
    double delta = cand - cold;
    s->gain += live - cold;
    s->shadow += delta;
    int activated = 0;
    if (s->active) {
        double z = s->odds + delta;
        s->odds = log1p(-PR_HMM_HAZARD) / log(2.0) - la(-z, log2(PR_HMM_HAZARD));
    } else if (admit) {
        s->active = activated = 1;
        s->odds = 0;
        s->activation = t + 1;
    }
    if (!isfinite(s->odds) || !isfinite(s->shadow) || !isfinite(s->gain))
        fail("nonfinite outer state");
    if (s->gain < s->minimum) s->minimum = s->gain;
    if (s->gain > s->peak) s->peak = s->gain;
    if (s->peak - s->gain > s->drawdown) s->drawdown = s->peak - s->gain;
    if (s->minimum < -1.0 - 1e-7 || s->drawdown > 16.0 + 1e-7)
        fail("outer one-bit/sixteen-bit loss bound");
    return activated;
}

static void r3_print3(const R3Three *router) {
    printf("%.17g,%.17g,%.17g", router->weight[0], router->weight[1], router->weight[2]);
}

static void r3_free_bank(R3Bank *bank) {
    free(bank->book[0].rule);
    free(bank->book[0].forward);
    free(bank->book[0].branch);
    free(bank->book[1].branch);
}

static void r3_free_grammar(Grammar *g) {
    free(g->rule);
    free(g->forward);
    free(g->branch);
}

static void r3_predict(const char *bank_path, const char *small_path,
                       const char *permuted_path) {
    R3Bank bank = r3_load_bank(bank_path);
    Grammar small = load_grammar(small_path), permuted = load_grammar(permuted_path);
    r3_check_addresses(&bank, &small, &permuted);
    size_t records = small.records;
    R3Three *three = calloc(records, sizeof(*three));
    R3Two *two_pooled = calloc(records, sizeof(*two_pooled));
    R3Two *two_perm = calloc(records, sizeof(*two_perm));
    if (!three || !two_pooled || !two_perm) fail("allocate local routers");
    for (size_t i = 0; i < records; i++) {
        r3_init3(&three[i]);
        r3_init2(&two_pooled[i]);
        r3_init2(&two_perm[i]);
    }
    R3Three prior3;
    R3Two prior2;
    r3_init3(&prior3);
    r3_init2(&prior2);
    BFState *front = bf_create();
    PRLife life;
    pr_life_init(&life);
    if (!front) fail("allocate turn30 frontend");
    History history = {{0}, 0};
    Outer outer[R3_ARMS] = {{0}};
    double overall_norm = 0.0, admission_shadow = 0.0;
    int admitted = 0;
    size_t admission_step = 0;

    fputs("t\tk\theads\tcold_heads\ttruth\trank\thistory\tlogcold\tmatched\trecord"
          "\tsource_pooled\tsource_a\tsource_b\tsource_permuted\tmax_norm_error\tnew_exact"
          "\tadmission_shadow_before\tadmitted_before"
          "\tbank3_w_before\tbank3_w_after\tpooled2_w_before\tpooled2_w_after"
          "\tpermuted2_w_before\tpermuted2_w_after", stdout);
    for (int arm = 0; arm < R3_ARMS; arm++)
        printf("\t%s_candidate\t%s_live\t%s_shadow_before\t%s_odds_before"
               "\t%s_active_before\t%s_activated_after\t%s_gain_after",
               r3_names[arm], r3_names[arm], r3_names[arm], r3_names[arm],
               r3_names[arm], r3_names[arm], r3_names[arm]);
    putchar('\n');

    for (;;) {
        Quote q;
        quote_cold(front, &life, &q);
        Match match = branch_match(&small, &history);
        Match bank_match = branch_match(&bank.book[0], &history);
        Match perm_match = branch_match(&permuted, &history);
        if (match.length != bank_match.length || match.nmatches != bank_match.nmatches ||
            match.length != perm_match.length || match.nmatches != perm_match.nmatches ||
            (match.nmatches && (match.matches[0] != bank_match.matches[0] ||
                                match.matches[0] != perm_match.matches[0])))
            fail("bank/pooled/permuted match differs");
        int record = match.nmatches ? match.matches[0] : -1;
        Match book_b = r3_record_match(&bank.book[1], record);
        double pooled[256], source_a[256], source_b[256], source_perm[256];
        double candidates[R3_ARMS][256], live[R3_ARMS][256];
        candidate(&q, &match, pooled);
        candidate(&q, &bank_match, source_a);
        candidate(&q, &book_b, source_b);
        candidate(&q, &perm_match, source_perm);
        R3Three three_before = record < 0 ? prior3 : three[record];
        R3Two pooled_before = record < 0 ? prior2 : two_pooled[record];
        R3Two perm_before = record < 0 ? prior2 : two_perm[record];
        r3_quote3(&three_before, q.cold, source_a, source_b, candidates[R3_BANK3]);
        r3_quote2(&pooled_before, q.cold, pooled, candidates[R3_POOLED2]);
        r3_quote2(&perm_before, q.cold, source_perm, candidates[R3_PERMUTED2]);
        memcpy(candidates[R3_COLD], q.cold, sizeof(q.cold));
        double norm = fmax(fmax(validate(q.bf.logp_base), validate(q.cold)),
                           fmax(fmax(validate(pooled), validate(source_a)),
                                fmax(validate(source_b), validate(source_perm))));
        for (int arm = 0; arm < R3_ARMS; arm++) {
            for (int byte = 0; byte < 256; byte++) {
                live[arm][byte] = !outer[arm].active || candidates[arm][byte] == q.cold[byte]
                    ? q.cold[byte]
                    : la(q.cold[byte], outer[arm].odds + candidates[arm][byte])
                      - la(0, outer[arm].odds);
                if (!q.rank[byte] &&
                    (candidates[arm][byte] != q.cold[byte] || live[arm][byte] != q.cold[byte]))
                    fail("protected NEW changed in turn30 arm");
            }
            norm = fmax(norm, validate(candidates[arm]));
            norm = fmax(norm, validate(live[arm]));
        }

        /* All source, router, candidate and live vectors precede truth. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        overall_norm = fmax(overall_norm, norm);
        double shadow_before = admission_shadow;
        int admitted_before = admitted;
        admission_shadow += candidates[R3_POOLED2][truth] - q.cold[truth];
        int admit = !admitted && admission_shadow >= 32.0;
        if (admit) {
            admitted = 1;
            admission_step = q.bf.t + 1;
        }
        Outer outer_before[R3_ARMS];
        int activated[R3_ARMS];
        for (int arm = 0; arm < R3_ARMS; arm++) {
            outer_before[arm] = outer[arm];
            activated[arm] = r3_observe_outer(&outer[arm], q.bf.t, q.cold[truth],
                                              candidates[arm][truth], live[arm][truth], admit);
        }
        if (record >= 0) {
            r3_observe3(&three[record], q.cold[truth], source_a[truth], source_b[truth],
                        candidates[R3_BANK3][truth]);
            r3_observe2(&two_pooled[record], pooled[truth], candidates[R3_POOLED2][truth]);
            r3_observe2(&two_perm[record], source_perm[truth], candidates[R3_PERMUTED2][truth]);
        }
        R3Three three_after = record < 0 ? prior3 : three[record];
        R3Two pooled_after = record < 0 ? prior2 : two_pooled[record];
        R3Two perm_after = record < 0 ? prior2 : two_perm[record];

        printf("%zu\t%d\t", q.bf.t, q.k);
        print_heads(&q, 0);
        putchar('\t');
        print_heads(&q, 1);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-');
        for (size_t i = 0; i < history.length; i++) putchar('0' + history.event[i]);
        printf("\t%.17g\t%d\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t1\t%.17g\t%d\t",
               q.cold[truth], match.length, record, pooled[truth], source_a[truth],
               source_b[truth], source_perm[truth], norm, shadow_before, admitted_before);
        r3_print3(&three_before);
        putchar('\t');
        r3_print3(&three_after);
        printf("\t%.17g\t%.17g\t%.17g\t%.17g", pooled_before.memory, pooled_after.memory,
               perm_before.memory, perm_after.memory);
        for (int arm = 0; arm < R3_ARMS; arm++)
            printf("\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g",
                   candidates[arm][truth], live[arm][truth], outer_before[arm].shadow,
                   outer_before[arm].odds, outer_before[arm].active,
                   activated[arm], outer[arm].gain);
        putchar('\n');
        observe_cold(front, &life, &q, truth);
        if (history.length == HORIZON) {
            memmove(history.event, history.event + 1, HORIZON - 1);
            history.length--;
        }
        history.event[history.length++] = q.rank[truth];
    }
    stream_end();
    fprintf(stderr, "router3 predict: bytes=%" PRIu64 " bank_bytes=%zu pooled_small_bytes=%zu "
                    "permuted_small_bytes=%zu rules=%zu records=%zu three_struct_bytes=%zu "
                    "two_struct_bytes=%zu router_total_bytes=%zu outer_state_bytes=%zu "
                    "history_struct_bytes=%zu forecast_vector_bytes=%zu "
                    "admission_shadow=%.17g admission_step=%zu max_norm_error=%.17g\n",
            life.events, bank.bytes, small.bytes, permuted.bytes, small.count, records,
            sizeof(R3Three), sizeof(R3Two),
            records * (sizeof(R3Three) + 2 * sizeof(R3Two)), sizeof(outer), sizeof(history),
            (4 + 2 * R3_ARMS) * 256 * sizeof(double), admission_shadow, admission_step,
            overall_norm);
    for (int arm = 0; arm < R3_ARMS; arm++)
        fprintf(stderr, "arm=%s gain=%.17g minimum=%.17g drawdown=%.17g active=%d activation=%zu\n",
                r3_names[arm], outer[arm].gain, outer[arm].minimum, outer[arm].drawdown,
                outer[arm].active, outer[arm].activation);
    free(three);
    free(two_pooled);
    free(two_perm);
    bf_destroy(front);
    r3_free_bank(&bank);
    r3_free_grammar(&small);
    r3_free_grammar(&permuted);
}

/* Handwritten prices, two separate records, one no-match visit, one
   all-cold visit, NEW truths and a shared admission. No world, archive or
   frontend is touched. */
static void r3_fixture(void) {
    Quote q = {0};
    q.k = 3;
    q.heads[0] = 3; q.heads[1] = 7; q.heads[2] = 11;
    for (int byte = 0; byte < 256; byte++) q.cold[byte] = -8.0;
    for (int j = 0; j < 3; j++) q.rank[q.heads[j]] = (uint8_t)(j + 1);
    Match ma = {0}, mb = {0}, mp = {0}, ms = {0};
    ma.length = mb.length = mp.length = ms.length = 1;
    ma.votes[1] = 300; ma.votes[2] = 1; ma.votes[3] = 1;
    mb.votes[1] = 100; mb.votes[2] = 2; mb.votes[3] = 2;
    for (int j = 0; j < 7; j++) ms.votes[j] = ma.votes[j] + mb.votes[j];
    for (int r = 0; r < 7; r++) mp.votes[r] = ms.votes[r ? r % 6 + 1 : 0];
    R3Three three[2];
    R3Two two_pooled[2], two_perm[2];
    R3Three prior3;
    R3Two prior2;
    r3_init3(&prior3);
    r3_init2(&prior2);
    for (int i = 0; i < 2; i++) {
        r3_init3(&three[i]);
        r3_init2(&two_pooled[i]);
        r3_init2(&two_perm[i]);
    }
    Outer outer[R3_ARMS] = {{0}};
    double admission_shadow = 0.0;
    int admitted = 0;
    for (size_t t = 0; t < 48; t++) {
        int record = t == 3 ? -1 : (int)(t % 3 == 1);
        int all_cold = t == 7;
        uint8_t truth = t == 5 ? 7 : t == 11 ? 11 : (t == 13 || t == 19) ? 23 : 3;
        double pooled[256], source_a[256], source_b[256], source_perm[256];
        double candidates[R3_ARMS][256], live[R3_ARMS][256];
        if (record < 0 || all_cold) {
            memcpy(pooled, q.cold, sizeof(pooled));
            memcpy(source_a, q.cold, sizeof(source_a));
            memcpy(source_b, q.cold, sizeof(source_b));
            memcpy(source_perm, q.cold, sizeof(source_perm));
        } else {
            candidate(&q, &ms, pooled);
            candidate(&q, &ma, source_a);
            candidate(&q, &mb, source_b);
            candidate(&q, &mp, source_perm);
        }
        R3Three three_before = record < 0 ? prior3 : three[record];
        R3Two pooled_before = record < 0 ? prior2 : two_pooled[record];
        R3Two perm_before = record < 0 ? prior2 : two_perm[record];
        r3_quote3(&three_before, q.cold, source_a, source_b, candidates[R3_BANK3]);
        r3_quote2(&pooled_before, q.cold, pooled, candidates[R3_POOLED2]);
        r3_quote2(&perm_before, q.cold, source_perm, candidates[R3_PERMUTED2]);
        memcpy(candidates[R3_COLD], q.cold, sizeof(q.cold));
        double norm = fmax(fmax(validate(pooled), validate(source_a)),
                           fmax(validate(source_b), validate(source_perm)));
        for (int arm = 0; arm < R3_ARMS; arm++) {
            for (int byte = 0; byte < 256; byte++)
                live[arm][byte] = !outer[arm].active || candidates[arm][byte] == q.cold[byte]
                    ? q.cold[byte]
                    : la(q.cold[byte], outer[arm].odds + candidates[arm][byte])
                      - la(0, outer[arm].odds);
            norm = fmax(norm, validate(candidates[arm]));
            norm = fmax(norm, validate(live[arm]));
        }
        double shadow_before = admission_shadow;
        int admitted_before = admitted;
        admission_shadow += candidates[R3_POOLED2][truth] - q.cold[truth];
        int admit = !admitted && admission_shadow >= 32.0;
        if (admit) admitted = 1;
        double live_price[R3_ARMS];
        int activated[R3_ARMS];
        for (int arm = 0; arm < R3_ARMS; arm++) {
            live_price[arm] = live[arm][truth];
            activated[arm] = r3_observe_outer(&outer[arm], t, q.cold[truth],
                                              candidates[arm][truth], live[arm][truth], admit);
        }
        if (record >= 0) {
            r3_observe3(&three[record], q.cold[truth], source_a[truth], source_b[truth],
                        candidates[R3_BANK3][truth]);
            r3_observe2(&two_pooled[record], pooled[truth], candidates[R3_POOLED2][truth]);
            r3_observe2(&two_perm[record], source_perm[truth], candidates[R3_PERMUTED2][truth]);
        }
        R3Three three_after = record < 0 ? prior3 : three[record];
        R3Two pooled_after = record < 0 ? prior2 : two_pooled[record];
        R3Two perm_after = record < 0 ? prior2 : two_perm[record];
        printf("{\"t\":%zu,\"record\":%d,\"truth\":%u,\"rank\":%u,\"cold\":%.17g,"
               "\"pooled\":%.17g,\"a\":%.17g,\"b\":%.17g,\"perm\":%.17g,"
               "\"admission_shadow_before\":%.17g,\"admitted_before\":%d,"
               "\"admitted_after\":%d,\"bank3_w_before\":[",
               t, record, (unsigned)truth, (unsigned)q.rank[truth], q.cold[truth],
               pooled[truth], source_a[truth], source_b[truth], source_perm[truth],
               shadow_before, admitted_before, admitted);
        r3_print3(&three_before);
        fputs("],\"bank3_w_after\":[", stdout);
        r3_print3(&three_after);
        printf("],\"pooled2_w_before\":%.17g,\"pooled2_w_after\":%.17g,"
               "\"permuted2_w_before\":%.17g,\"permuted2_w_after\":%.17g",
               pooled_before.memory, pooled_after.memory,
               perm_before.memory, perm_after.memory);
        for (int arm = 0; arm < R3_ARMS; arm++)
            printf(",\"%s_candidate\":%.17g,\"%s_live\":%.17g,\"%s_activated\":%d,"
                   "\"%s_gain\":%.17g",
                   r3_names[arm], candidates[arm][truth], r3_names[arm], live_price[arm],
                   r3_names[arm], activated[arm], r3_names[arm], outer[arm].gain);
        printf(",\"max_norm_error\":%.17g}\n", norm);
    }
    stream_end();
}

int main(int argc, char **argv) {
    if (argc >= 2 && (!strcmp(argv[1], "emit") || !strcmp(argv[1], "trace")))
        return inherited_episode_main(argc, argv);
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    if (argc == 5 && !strcmp(argv[1], "predict")) r3_predict(argv[2], argv[3], argv[4]);
    else if (argc == 2 && !strcmp(argv[1], "--fixture")) r3_fixture();
    else {
        fputs("usage: router3 predict BANK POOLED_SMALL PERMUTED_SMALL < RAW\n"
              "       router3 emit ALPHABET NONZERO_SEED < COMMANDS\n"
              "       router3 trace [compact] < RAW\n"
              "       router3 --fixture\n", stderr);
        return 2;
    }
    return 0;
}
