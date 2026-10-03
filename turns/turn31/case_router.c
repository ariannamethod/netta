/* Turn31: separate memory permission u from conditional source choice v.
   Flat3, binary, loader and shared-admission outer helpers below are copied
   without changes from the frozen turn30/router3.c. */
#define main inherited_episode_main
#include ".build/turn13/episode.c"
#undef main

#define R3_SHARE 0x1p-10
#define R3_PRIOR 0.875

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

#define CR_ARMS 6
enum { CR_FACTORED, CR_FLAT3, CR_BALANCED, CR_POOLED, CR_PERMUTED, CR_COLD };
static const char *const cr_names[CR_ARMS] = {
    "factored", "flat3", "balanced", "pooled", "permuted", "cold"};
typedef struct { double u, v; } CRFactored;

static void cr_init(CRFactored *router) {
    router->u = R3_PRIOR;
    router->v = 0.5;
}

static void cr_check(const CRFactored *router) {
    if (!isfinite(router->u) || !isfinite(router->v) ||
        router->u <= 0.0 || router->u >= 1.0 || router->v <= 0.0 || router->v >= 1.0)
        fail("invalid factored router state");
}

static void cr_conditional(double v, const double cold[256],
                            const double a[256], const double b[256], double out[256]) {
    double wa = log2(v), wb = log2(1.0 - v);
    for (int byte = 0; byte < 256; byte++)
        out[byte] = a[byte] == cold[byte] && b[byte] == cold[byte]
            ? cold[byte] : la(wa + a[byte], wb + b[byte]);
}

static void cr_quote(const CRFactored *router, const double cold[256],
                       const double a[256], const double b[256],
                       double conditional[256], double out[256]) {
    cr_check(router);
    cr_conditional(router->v, cold, a, b, conditional);
    R3Two permission = {router->u};
    r3_quote2(&permission, cold, conditional, out);
}

static void cr_observe(CRFactored *router, double a, double conditional, double quoted) {
    /* Both posterior numerators use OLD state. Conditional choice is learned
       from S, even when memory permission u is tiny; no new u enters v. */
    CRFactored next = {
        (1.0 - R3_SHARE) * router->u * exp2(conditional - quoted) + R3_SHARE * R3_PRIOR,
        (1.0 - R3_SHARE) * router->v * exp2(a - conditional) + R3_SHARE * 0.5
    };
    cr_check(&next);
    *router = next;
}

static void cr_check_addresses(const R3Bank *bank, const Grammar *small,
                                const R3Bank *permuted) {
    const Grammar *a = &bank->book[0], *b = &bank->book[1];
    r3_same_rules(a, small);
    r3_same_rules(a, &permuted->book[0]);
    size_t capacity = (MAX_ARCHIVE_BYTES - 16 - 4 * a->count) / 32;
    if (!a->records || a->records > capacity || small->records != a->records ||
        permuted->book[0].records != a->records || permuted->bytes != bank->bytes)
        fail("bank/pooled/permuted record projection differs");
    for (size_t i = 0; i < a->records; i++) {
        if (a->branch[i].rule_id != small->branch[i].rule_id ||
            a->branch[i].prefix_len != small->branch[i].prefix_len)
            fail("bank/pooled selected address differs");
        for (int c = 0; c < 2; c++)
            if (a->branch[i].rule_id != permuted->book[c].branch[i].rule_id ||
                a->branch[i].prefix_len != permuted->book[c].branch[i].prefix_len)
                fail("bank/permuted selected address differs");
        for (int r = 0; r < 7; r++) {
            unsigned total = (unsigned)a->branch[i].counts[r] + b->branch[i].counts[r];
            if (total > UINT16_MAX || total != small->branch[i].counts[r])
                fail("source-case pooled count mismatch");
            int old = r ? r % 6 + 1 : 0;
            for (int c = 0; c < 2; c++)
                if (permuted->book[c].branch[i].counts[r] != bank->book[c].branch[i].counts[old])
                    fail("permuted book count mismatch");
        }
    }
}

static void cr_predict(const char *bank_path, const char *small_path, const char *permuted_path) {
    R3Bank bank = r3_load_bank(bank_path), permuted = r3_load_bank(permuted_path);
    Grammar small = load_grammar(small_path);
    cr_check_addresses(&bank, &small, &permuted);
    size_t records = small.records;
    CRFactored *factored = calloc(records, sizeof(*factored));
    CRFactored *perm = calloc(records, sizeof(*perm));
    R3Three *flat = calloc(records, sizeof(*flat));
    R3Two *balanced = calloc(records, sizeof(*balanced));
    R3Two *pooled = calloc(records, sizeof(*pooled));
    if (!factored || !perm || !flat || !balanced || !pooled) fail("allocate turn31 routers");
    for (size_t i = 0; i < records; i++) {
        cr_init(&factored[i]); cr_init(&perm[i]); r3_init3(&flat[i]);
        r3_init2(&balanced[i]); r3_init2(&pooled[i]);
    }
    CRFactored priorf;
    R3Three prior3;
    R3Two prior2;
    cr_init(&priorf); r3_init3(&prior3); r3_init2(&prior2);
    BFState *front = bf_create();
    PRLife life;
    pr_life_init(&life);
    if (!front) fail("allocate turn31 frontend");
    History history = {{0}, 0};
    Outer outer[CR_ARMS] = {{0}};
    double admission_shadow = 0.0, overall_norm = 0.0;
    int admitted = 0;
    size_t admission_step = 0;
    fputs("t\tk\theads\tcold_heads\ttruth\trank\thistory\tlogcold\tmatched\trecord"
          "\tsource_pooled\tsource_a\tsource_b\tsource_permuted_a\tsource_permuted_b"
          "\tmax_norm_error\tnew_exact\tadmission_shadow_before\tadmitted_before"
          "\tfactored_u_before\tfactored_v_before\tfactored_u_after\tfactored_v_after"
          "\tflat3_w_before\tflat3_w_after\tbalanced_u_before\tbalanced_u_after"
          "\tpooled_u_before\tpooled_u_after\tpermuted_u_before\tpermuted_v_before"
          "\tpermuted_u_after\tpermuted_v_after", stdout);
    for (int arm = 0; arm < CR_ARMS; arm++)
        printf("\t%s_candidate\t%s_live\t%s_shadow_before\t%s_odds_before"
               "\t%s_active_before\t%s_activated_after\t%s_gain_after",
               cr_names[arm], cr_names[arm], cr_names[arm], cr_names[arm],
               cr_names[arm], cr_names[arm], cr_names[arm]);
    putchar('\n');
    for (;;) {
        Quote q;
        quote_cold(front, &life, &q);
        Match match = branch_match(&small, &history);
        Match bank_match = branch_match(&bank.book[0], &history);
        Match perm_match = branch_match(&permuted.book[0], &history);
        if (match.length != bank_match.length || match.nmatches != bank_match.nmatches ||
            match.length != perm_match.length || match.nmatches != perm_match.nmatches ||
            (match.nmatches && (match.matches[0] != bank_match.matches[0] ||
                               match.matches[0] != perm_match.matches[0])))
            fail("bank/pooled/permuted match differs");
        int record = match.nmatches ? match.matches[0] : -1;
        Match book_b = r3_record_match(&bank.book[1], record);
        Match perm_b = r3_record_match(&permuted.book[1], record);
        /* source order: pooled, A, B, permuted A, permuted B. Conditional
           order: factored, balanced, permuted. */
        double sources[5][256], conditional[3][256];
        double candidates[CR_ARMS][256], live[CR_ARMS][256];
        candidate(&q, &match, sources[0]);
        candidate(&q, &bank_match, sources[1]);
        candidate(&q, &book_b, sources[2]);
        candidate(&q, &perm_match, sources[3]);
        candidate(&q, &perm_b, sources[4]);
        CRFactored fb = record < 0 ? priorf : factored[record];
        CRFactored pb = record < 0 ? priorf : perm[record];
        R3Three flatb = record < 0 ? prior3 : flat[record];
        R3Two bb = record < 0 ? prior2 : balanced[record];
        R3Two cb = record < 0 ? prior2 : pooled[record];
        cr_quote(&fb, q.cold, sources[1], sources[2], conditional[0], candidates[CR_FACTORED]);
        r3_quote3(&flatb, q.cold, sources[1], sources[2], candidates[CR_FLAT3]);
        cr_conditional(0.5, q.cold, sources[1], sources[2], conditional[1]);
        r3_quote2(&bb, q.cold, conditional[1], candidates[CR_BALANCED]);
        r3_quote2(&cb, q.cold, sources[0], candidates[CR_POOLED]);
        cr_quote(&pb, q.cold, sources[3], sources[4], conditional[2], candidates[CR_PERMUTED]);
        memcpy(candidates[CR_COLD], q.cold, sizeof(q.cold));
        double norm = fmax(validate(q.bf.logp_base), validate(q.cold));
        for (int i = 0; i < 5; i++) norm = fmax(norm, validate(sources[i]));
        for (int i = 0; i < 3; i++) norm = fmax(norm, validate(conditional[i]));
        for (int arm = 0; arm < CR_ARMS; arm++) {
            if (outer[arm].active != admitted) fail("shared admission before quote");
            for (int byte = 0; byte < 256; byte++) {
                live[arm][byte] = !outer[arm].active || candidates[arm][byte] == q.cold[byte]
                    ? q.cold[byte] : la(q.cold[byte], outer[arm].odds + candidates[arm][byte]) - la(0, outer[arm].odds);
                if (!q.rank[byte] && (candidates[arm][byte] != q.cold[byte] || live[arm][byte] != q.cold[byte]))
                    fail("protected NEW changed in turn31 arm");
            }
            norm = fmax(norm, validate(candidates[arm]));
            norm = fmax(norm, validate(live[arm]));
        }
        /* All source, conditional, candidate and live vectors precede truth. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        overall_norm = fmax(overall_norm, norm);
        double shadow_before = admission_shadow;
        int admitted_before = admitted;
        admission_shadow += candidates[CR_POOLED][truth] - q.cold[truth];
        if (!isfinite(admission_shadow)) fail("nonfinite common admission shadow");
        int admit = !admitted && admission_shadow >= 32.0;
        if (admit) { admitted = 1; admission_step = q.bf.t + 1; }
        Outer outer_before[CR_ARMS];
        int activated[CR_ARMS];
        for (int arm = 0; arm < CR_ARMS; arm++) {
            outer_before[arm] = outer[arm];
            activated[arm] = r3_observe_outer(&outer[arm], q.bf.t, q.cold[truth],
                                             candidates[arm][truth], live[arm][truth], admit);
            if (activated[arm] != admit || outer[arm].active != admitted ||
                outer[arm].activation != admission_step) fail("shared admission after observation");
        }
        if (record >= 0) {
            cr_observe(&factored[record], sources[1][truth], conditional[0][truth], candidates[CR_FACTORED][truth]);
            r3_observe3(&flat[record], q.cold[truth], sources[1][truth], sources[2][truth], candidates[CR_FLAT3][truth]);
            r3_observe2(&balanced[record], conditional[1][truth], candidates[CR_BALANCED][truth]);
            r3_observe2(&pooled[record], sources[0][truth], candidates[CR_POOLED][truth]);
            cr_observe(&perm[record], sources[3][truth], conditional[2][truth], candidates[CR_PERMUTED][truth]);
        }
        CRFactored fa = record < 0 ? priorf : factored[record];
        CRFactored pa = record < 0 ? priorf : perm[record];
        R3Three flata = record < 0 ? prior3 : flat[record];
        R3Two ba = record < 0 ? prior2 : balanced[record];
        R3Two ca = record < 0 ? prior2 : pooled[record];
        printf("%zu\t%d\t", q.bf.t, q.k);
        print_heads(&q, 0); putchar('\t'); print_heads(&q, 1);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-');
        for (size_t i = 0; i < history.length; i++) putchar('0' + history.event[i]);
        printf("\t%.17g\t%d\t%d", q.cold[truth], match.length, record);
        for (int i = 0; i < 5; i++) printf("\t%.17g", sources[i][truth]);
        printf("\t%.17g\t1\t%.17g\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t",
               norm, shadow_before, admitted_before, fb.u, fb.v, fa.u, fa.v);
        r3_print3(&flatb); putchar('\t'); r3_print3(&flata);
        printf("\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g",
               bb.memory, ba.memory, cb.memory, ca.memory, pb.u, pb.v, pa.u, pa.v);
        for (int arm = 0; arm < CR_ARMS; arm++)
            printf("\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g",
                   candidates[arm][truth], live[arm][truth], outer_before[arm].shadow,
                   outer_before[arm].odds, outer_before[arm].active, activated[arm], outer[arm].gain);
        putchar('\n');
        observe_cold(front, &life, &q, truth);
        if (history.length == HORIZON) {
            memmove(history.event, history.event + 1, HORIZON - 1); history.length--;
        }
        history.event[history.length++] = q.rank[truth];
    }
    stream_end();
    fprintf(stderr, "case_router predict: bytes=%" PRIu64 " bank_bytes=%zu pooled_bytes=%zu permuted_bytes=%zu "
                    "rules=%zu records=%zu factored_struct_bytes=%zu flat_struct_bytes=%zu binary_struct_bytes=%zu "
                    "factored_router_bytes=%zu flat3_router_bytes=%zu balanced_router_bytes=%zu pooled_router_bytes=%zu "
                    "permuted_router_bytes=%zu cold_router_bytes=0 router_total_bytes=%zu outer_state_bytes=%zu "
                    "history_struct_bytes=%zu forecast_vector_bytes=%zu admission_shadow=%.17g admission_step=%zu max_norm_error=%.17g\n",
            life.events, bank.bytes, small.bytes, permuted.bytes, small.count, records,
            sizeof(CRFactored), sizeof(R3Three), sizeof(R3Two), records * sizeof(CRFactored),
            records * sizeof(R3Three), records * sizeof(R3Two), records * sizeof(R3Two), records * sizeof(CRFactored),
            records * (2 * sizeof(CRFactored) + sizeof(R3Three) + 2 * sizeof(R3Two)),
            sizeof(outer), sizeof(history), (8 + 2 * CR_ARMS) * 256 * sizeof(double),
            admission_shadow, admission_step, overall_norm);
    for (int arm = 0; arm < CR_ARMS; arm++)
        fprintf(stderr, "arm=%s gain=%.17g minimum=%.17g drawdown=%.17g active=%d activation=%zu\n",
                cr_names[arm], outer[arm].gain, outer[arm].minimum, outer[arm].drawdown,
                outer[arm].active, outer[arm].activation);
    free(factored); free(perm); free(flat); free(balanced); free(pooled);
    bf_destroy(front); r3_free_bank(&bank); r3_free_bank(&permuted); r3_free_grammar(&small);
}

static void cr_fixture(void) {
    /* Twenty hand-priced observations from normalized full-byte forecasts.
       Two record identities; no archive, frontend, generator or target data. */
    Quote q = {0};
    q.k = 3;
    q.heads[0] = 3; q.heads[1] = 7; q.heads[2] = 11;
    for (int byte = 0; byte < 256; byte++) q.cold[byte] = -8.0;
    for (int j = 0; j < 3; j++) q.rank[q.heads[j]] = (uint8_t)(j + 1);
    Match ma = {0}, mb = {0}, mp = {0};
    ma.length = mb.length = mp.length = 1;
    ma.votes[1] = 300; ma.votes[2] = 1; ma.votes[3] = 1;
    mb.votes[1] = 2; mb.votes[2] = 4; mb.votes[3] = 100;
    for (int j = 0; j < 7; j++) mp.votes[j] = ma.votes[j] + mb.votes[j];
    CRFactored factored[2], pf;
    R3Three flat[2], p3;
    R3Two balanced[2], pooled[2], p2;
    cr_init(&pf); r3_init3(&p3); r3_init2(&p2);
    for (int i = 0; i < 2; i++) {
        cr_init(&factored[i]); r3_init3(&flat[i]); r3_init2(&balanced[i]); r3_init2(&pooled[i]);
    }
    for (int t = 0; t < 20; t++) {
        int record = t == 4 ? -1 : (t == 1 || t == 18 ? 1 : 0);
        uint8_t truth = t == 1 ? 11 : t == 2 || t == 19 ? 23 : t >= 5 && t <= 12 ? 7 : 3;
        double a[256], b[256], pool[256], s[256], equal[256], cq[256], fq[256], bq[256], pq[256];
        if (record < 0 || t == 3) {
            memcpy(a, q.cold, sizeof(a)); memcpy(b, q.cold, sizeof(b)); memcpy(pool, q.cold, sizeof(pool));
        } else {
            candidate(&q, &ma, a); candidate(&q, &mb, b); candidate(&q, &mp, pool);
        }
        CRFactored before = record < 0 ? pf : factored[record];
        R3Three flatb = record < 0 ? p3 : flat[record];
        R3Two bb = record < 0 ? p2 : balanced[record], pb = record < 0 ? p2 : pooled[record];
        cr_quote(&before, q.cold, a, b, s, cq);
        r3_quote3(&flatb, q.cold, a, b, fq);
        cr_conditional(0.5, q.cold, a, b, equal);
        r3_quote2(&bb, q.cold, equal, bq); r3_quote2(&pb, q.cold, pool, pq);
        double norm = validate(q.cold);
        const double *vectors[9] = {a, b, pool, s, equal, cq, fq, bq, pq};
        for (int i = 0; i < 9; i++) norm = fmax(norm, validate(vectors[i]));
        for (int byte = 0; byte < 256; byte++)
            if ((!q.rank[byte] || (a[byte] == q.cold[byte] && b[byte] == q.cold[byte])) &&
                (cq[byte] != q.cold[byte] || fq[byte] != q.cold[byte] || bq[byte] != q.cold[byte]))
                fail("fixture exact common forecast");
        if (record >= 0) {
            cr_observe(&factored[record], a[truth], s[truth], cq[truth]);
            r3_observe3(&flat[record], q.cold[truth], a[truth], b[truth], fq[truth]);
            r3_observe2(&balanced[record], equal[truth], bq[truth]);
            r3_observe2(&pooled[record], pool[truth], pq[truth]);
        }
        CRFactored after = record < 0 ? pf : factored[record];
        R3Three flata = record < 0 ? p3 : flat[record];
        R3Two ba = record < 0 ? p2 : balanced[record], pa = record < 0 ? p2 : pooled[record];
        printf("{\"t\":%d,\"record\":%d,\"truth\":%u,\"rank\":%u,\"cold\":%.17g,\"a\":%.17g,\"b\":%.17g,\"pooled\":%.17g,"
               "\"factored_before\":[%.17g,%.17g],\"factored_after\":[%.17g,%.17g],\"flat_before\":[",
               t, record, (unsigned)truth, (unsigned)q.rank[truth], q.cold[truth], a[truth], b[truth], pool[truth],
               before.u, before.v, after.u, after.v);
        r3_print3(&flatb); fputs("],\"flat_after\":[", stdout); r3_print3(&flata);
        printf("],\"balanced_before\":%.17g,\"balanced_after\":%.17g,\"pooled_before\":%.17g,\"pooled_after\":%.17g,"
               "\"factored\":%.17g,\"flat3\":%.17g,\"balanced\":%.17g,\"pooled2\":%.17g,\"max_norm_error\":%.17g,"
               "\"factored_struct_bytes\":%zu,\"flat_struct_bytes\":%zu,\"binary_struct_bytes\":%zu}\n",
               bb.memory, ba.memory, pb.memory, pa.memory, cq[truth], fq[truth], bq[truth], pq[truth], norm,
               sizeof(CRFactored), sizeof(R3Three), sizeof(R3Two));
    }
    stream_end();
}

int main(int argc, char **argv) {
    if (argc >= 2 && (!strcmp(argv[1], "emit") || !strcmp(argv[1], "trace")))
        return inherited_episode_main(argc, argv);
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    if (argc == 5 && !strcmp(argv[1], "predict")) cr_predict(argv[2], argv[3], argv[4]);
    else if (argc == 2 && !strcmp(argv[1], "--fixture")) cr_fixture();
    else {
        fputs("usage: case_router predict BANK POOLED_SMALL PERMUTED_BANK < RAW\n"
              "       case_router emit ALPHABET NONZERO_SEED < COMMANDS\n"
              "       case_router trace [compact] < RAW\n"
              "       case_router --fixture\n", stderr);
        return 2;
    }
    return 0;
}
