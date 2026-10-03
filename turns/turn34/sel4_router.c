/* Turn34: four distinct episodes earn residual influence separately; a
   remembered episode speaks only where its own wealth is positive. The
   frozen turn33 organ is included whole: its two-case law IS this turn's
   earned2 control arm, and its helpers are reused unedited. */
/* .build/turn33_core.c is the frozen ../turn33/earned_router.c staged by the
   Makefile with exactly one mechanical change, checked in the recipe: its
   main is renamed turn33_router_main so this file can own the entry point. */
#include ".build/turn33_core.c"

#define SL_EPISODES 4
#define SL_CASE_ARCHIVE_BYTES 2112
#define SL_ARMS 5
enum { SL_SEL4, SL_EARNED2, SL_POOLED, SL_PERMUTED4, SL_COLD };
static const char *const sl_names[SL_ARMS] = {
    "sel4", "earned2", "pooled", "permuted4", "cold"};

typedef struct { double h[SL_EPISODES]; } SLWealth;

typedef struct {
    /* Four episode books over one shared rule set; book[0] owns the shared
       allocations, as in R3Bank. */
    Grammar book[SL_EPISODES];
    size_t bytes;
} SLBank;

static void sl_init(SLWealth *state) {
    for (int i = 0; i < SL_EPISODES; i++) state->h[i] = 0.0;
}

static void sl_check(const SLWealth *state) {
    const double floor_log = -10.0 - 1e-12;
    for (int i = 0; i < SL_EPISODES; i++)
        if (!isfinite(state->h[i]) || state->h[i] < floor_log)
            fail("invalid episode residual wealth");
}

static int sl_silent(const SLWealth *state) {
    for (int i = 0; i < SL_EPISODES; i++)
        if (state->h[i] > 0.0) return 0;
    return 1;
}

static void sl_source(const SLWealth *state, const double pooled[256],
                      const double *episode[SL_EPISODES], double out[256]) {
    sl_check(state);
    if (sl_silent(state)) {
        memcpy(out, pooled, 256 * sizeof(double));
        return;
    }
    double excess[SL_EPISODES], mix = -INFINITY;
    for (int i = 0; i < SL_EPISODES; i++) {
        excess[i] = er_log_excess(state->h[i]);
        mix = la(mix, excess[i]);
    }
    double normalizer = la(0.0, mix);
    for (int byte = 0; byte < 256; byte++) {
        int all_cold = 1;
        for (int i = 0; i < SL_EPISODES; i++)
            all_cold &= episode[i][byte] == pooled[byte];
        if (all_cold) { out[byte] = pooled[byte]; continue; }
        double residual = -INFINITY;
        for (int i = 0; i < SL_EPISODES; i++)
            residual = la(residual, excess[i] + episode[i][byte]);
        out[byte] = la(pooled[byte], residual) - normalizer;
    }
}

static void sl_observe(SLWealth *state, double pooled,
                       const double episode[SL_EPISODES]) {
    const double keep = log2(1.0 - R3_SHARE), share = log2(R3_SHARE);
    SLWealth next;
    for (int i = 0; i < SL_EPISODES; i++)
        next.h[i] = la(keep + state->h[i] + episode[i] - pooled, share);
    sl_check(&next);
    *state = next;
}

/* NETEB004: the NETEB001 shape widened to four books. 16-byte header,
   4-byte rules, then per record 4 bytes of address plus four 14-byte
   count blocks. */
static SLBank sl_load_bank(const char *path) {
    SLBank bank = {0};
    unsigned char *bytes = read_bytes(path, SL_CASE_ARCHIVE_BYTES, &bank.bytes);
    if (bank.bytes < 16 || memcmp(bytes, "NETEB004", 8)) fail("invalid NETEB004 header");
    size_t nr = le32(bytes + 8), nb = le32(bytes + 12);
    if (nr > MAX_RULES || nb > MAX_RECORDS ||
        bank.bytes != 16 + 4 * nr + (4 + 14 * SL_EPISODES) * nb)
        fail("invalid NETEB004 count/size");
    Grammar *g = &bank.book[0];
    g->count = nr;
    g->records = nb;
    g->bytes = bank.bytes;
    if (nr) {
        g->rule = calloc(nr, sizeof(*g->rule));
        g->forward = calloc(nr, sizeof(*g->forward));
        if (!g->rule || !g->forward) fail("allocate episode bank grammar");
    }
    for (size_t i = 0; i < nr; i++) {
        const unsigned char *p = bytes + 16 + 4 * i;
        g->rule[i] = (Rule){le16(p), le16(p + 2)};
        if (g->rule[i].left >= 7 + i || g->rule[i].right >= 7 + i)
            fail("invalid episode bank topology");
        append_child(&g->forward[i], g->rule[i].left, g->forward);
        append_child(&g->forward[i], g->rule[i].right, g->forward);
    }
    for (int c = 1; c < SL_EPISODES; c++) bank.book[c] = *g;
    for (int c = 0; c < SL_EPISODES; c++) {
        if (nb) {
            bank.book[c].branch = calloc(nb, sizeof(Branch));
            if (!bank.book[c].branch) fail("allocate episode bank records");
        }
    }
    for (size_t i = 0; i < nb; i++) {
        const unsigned char *p = bytes + 16 + 4 * nr + (4 + 14 * SL_EPISODES) * i;
        if (le16(p + 2)) fail("nonzero NETEB004 reserved bytes");
        for (int c = 0; c < SL_EPISODES; c++) {
            Branch *b = &bank.book[c].branch[i];
            b->rule_id = p[0];
            b->prefix_len = p[1];
            for (int j = 0; j < 7; j++) b->counts[j] = le16(p + 4 + 14 * c + 2 * j);
            if (b->rule_id >= nr || !b->prefix_len ||
                b->prefix_len >= g->forward[b->rule_id].length)
                fail("invalid episode bank record address");
        }
        const Branch *b = &g->branch[i];
        for (size_t j = 0; j < i; j++)
            if (g->branch[j].prefix_len == b->prefix_len &&
                !memcmp(g->forward[g->branch[j].rule_id].event,
                        g->forward[b->rule_id].event, b->prefix_len))
                fail("duplicate episode bank prefix content");
    }
    free(bytes);
    return bank;
}

static void sl_free_bank(SLBank *bank) {
    free(bank->book[0].rule);
    free(bank->book[0].forward);
    for (int c = 0; c < SL_EPISODES; c++) free(bank->book[c].branch);
}

/* E1+E2 must reconstruct case A, E3+E4 case B, A+B the pooled archive, and
   the permuted bank must be the episode bank with roles rotated per book. */
static void sl_check_addresses(const SLBank *bank4, const R3Bank *bank2,
                               const Grammar *pooled, const SLBank *permuted4) {
    const Grammar *e0 = &bank4->book[0];
    r3_same_rules(e0, &bank2->book[0]);
    r3_same_rules(e0, pooled);
    r3_same_rules(e0, &permuted4->book[0]);
    if (!e0->records || e0->records != pooled->records ||
        e0->records != bank2->book[0].records ||
        e0->records != permuted4->book[0].records ||
        bank4->bytes > SL_CASE_ARCHIVE_BYTES || bank2->bytes > ER_CASE_ARCHIVE_BYTES ||
        pooled->bytes > MAX_ARCHIVE_BYTES || permuted4->bytes != bank4->bytes)
        fail("episode/case/pooled archive projection differs");
    for (size_t i = 0; i < e0->records; i++) {
        const Branch *base = &e0->branch[i];
        if (base->rule_id != pooled->branch[i].rule_id ||
            base->prefix_len != pooled->branch[i].prefix_len ||
            base->rule_id != bank2->book[0].branch[i].rule_id ||
            base->prefix_len != bank2->book[0].branch[i].prefix_len)
            fail("episode/case/pooled selected address differs");
        for (int c = 0; c < SL_EPISODES; c++)
            if (base->rule_id != permuted4->book[c].branch[i].rule_id ||
                base->prefix_len != permuted4->book[c].branch[i].prefix_len)
                fail("episode/permuted selected address differs");
        for (int r = 0; r < 7; r++) {
            unsigned a = (unsigned)bank4->book[0].branch[i].counts[r] +
                         bank4->book[1].branch[i].counts[r];
            unsigned b = (unsigned)bank4->book[2].branch[i].counts[r] +
                         bank4->book[3].branch[i].counts[r];
            if (a > UINT16_MAX || a != bank2->book[0].branch[i].counts[r])
                fail("episodes 1+2 do not reconstruct case A");
            if (b > UINT16_MAX || b != bank2->book[1].branch[i].counts[r])
                fail("episodes 3+4 do not reconstruct case B");
            if (a + b > UINT16_MAX || a + b != pooled->branch[i].counts[r])
                fail("episode pooled count mismatch");
            int old = r ? r % 6 + 1 : 0;
            for (int c = 0; c < SL_EPISODES; c++)
                if (permuted4->book[c].branch[i].counts[r] !=
                    bank4->book[c].branch[i].counts[old])
                    fail("permuted episode book count mismatch");
        }
    }
}

static void sl_predict(const char *bank4_path, const char *bank2_path,
                       const char *pooled_path, const char *permuted4_path) {
    SLBank bank4 = sl_load_bank(bank4_path), permuted4 = sl_load_bank(permuted4_path);
    R3Bank bank2 = r3_load_bank(bank2_path);
    Grammar pooled = load_grammar(pooled_path);
    sl_check_addresses(&bank4, &bank2, &pooled, &permuted4);
    size_t records = pooled.records;
    SLWealth *sel = calloc(records, sizeof(*sel));
    SLWealth *perm = calloc(records, sizeof(*perm));
    ERWealth *two = calloc(records, sizeof(*two));
    R3Two *permission = calloc(records, sizeof(*permission));
    uint64_t *visits = calloc(records, sizeof(*visits));
    if (!sel || !perm || !two || !permission || !visits)
        fail("allocate turn34 routers");
    for (size_t i = 0; i < records; i++) {
        sl_init(&sel[i]); sl_init(&perm[i]);
        er_init(&two[i]); r3_init2(&permission[i]);
    }
    SLWealth priors;
    ERWealth priorw;
    R3Two prior2;
    sl_init(&priors); er_init(&priorw); r3_init2(&prior2);
    BFState *front = bf_create();
    PRLife life;
    pr_life_init(&life);
    if (!front) fail("allocate turn34 frontend");
    History history = {{0}, 0};
    Outer outer[SL_ARMS] = {{0}};
    double admission_shadow = 0.0, overall_norm = 0.0;
    int admitted = 0;
    size_t admission_step = 0, first_quotes = 0, silent_quotes = 0, active_quotes = 0;

    fputs("t\tk\theads\tcold_heads\ttruth\trank\thistory\tlogcold\tmatched\trecord"
          "\trecord_visits_before\tsource_pooled\tsource_e1\tsource_e2\tsource_e3"
          "\tsource_e4\tsource_a2\tsource_b2\tsource_pe1\tsource_pe2\tsource_pe3"
          "\tsource_pe4\tcorrected_sel4\tcorrected_earned2\tcorrected_permuted4"
          "\tmax_norm_error\tnew_exact\tadmission_shadow_before\tadmitted_before"
          "\tpermission_u_before\tpermission_u_after", stdout);
    for (int i = 1; i <= SL_EPISODES; i++) printf("\tsel4_h%d_before", i);
    for (int i = 1; i <= SL_EPISODES; i++) printf("\tsel4_h%d_after", i);
    for (int i = 1; i <= SL_EPISODES; i++) printf("\tpermuted4_h%d_before", i);
    for (int i = 1; i <= SL_EPISODES; i++) printf("\tpermuted4_h%d_after", i);
    fputs("\tearned2_ha_before\tearned2_hb_before\tearned2_ha_after\tearned2_hb_after",
          stdout);
    for (int arm = 0; arm < SL_ARMS; arm++)
        printf("\t%s_candidate\t%s_live\t%s_shadow_before\t%s_odds_before"
               "\t%s_active_before\t%s_activated_after\t%s_gain_after",
               sl_names[arm], sl_names[arm], sl_names[arm], sl_names[arm],
               sl_names[arm], sl_names[arm], sl_names[arm]);
    putchar('\n');

    for (;;) {
        Quote q;
        quote_cold(front, &life, &q);
        Match pmatch = branch_match(&pooled, &history);
        Match ematch = branch_match(&bank4.book[0], &history);
        Match amatch = branch_match(&bank2.book[0], &history);
        Match nmatch = branch_match(&permuted4.book[0], &history);
        if (pmatch.length != ematch.length || pmatch.nmatches != ematch.nmatches ||
            pmatch.length != amatch.length || pmatch.nmatches != amatch.nmatches ||
            pmatch.length != nmatch.length || pmatch.nmatches != nmatch.nmatches ||
            (pmatch.nmatches && (pmatch.matches[0] != ematch.matches[0] ||
                                 pmatch.matches[0] != amatch.matches[0] ||
                                 pmatch.matches[0] != nmatch.matches[0])))
            fail("episode/case/pooled/permuted match differs");
        int record = pmatch.nmatches ? pmatch.matches[0] : -1;
        double sources[11][256], corrected[3][256];
        double candidates[SL_ARMS][256], live[SL_ARMS][256];
        candidate(&q, &pmatch, sources[0]);
        candidate(&q, &ematch, sources[1]);
        for (int c = 1; c < SL_EPISODES; c++) {
            Match m = r3_record_match(&bank4.book[c], record);
            candidate(&q, &m, sources[1 + c]);
        }
        candidate(&q, &amatch, sources[5]);
        {
            Match m = r3_record_match(&bank2.book[1], record);
            candidate(&q, &m, sources[6]);
        }
        candidate(&q, &nmatch, sources[7]);
        for (int c = 1; c < SL_EPISODES; c++) {
            Match m = r3_record_match(&permuted4.book[c], record);
            candidate(&q, &m, sources[7 + c]);
        }
        SLWealth sb = record < 0 ? priors : sel[record];
        SLWealth nb = record < 0 ? priors : perm[record];
        ERWealth wb = record < 0 ? priorw : two[record];
        R3Two ub = record < 0 ? prior2 : permission[record];
        uint64_t visits_before = record < 0 ? 0 : visits[record];
        const double *episodes[SL_EPISODES] =
            {sources[1], sources[2], sources[3], sources[4]};
        const double *permuted_eps[SL_EPISODES] =
            {sources[7], sources[8], sources[9], sources[10]};
        sl_source(&sb, sources[0], episodes, corrected[0]);
        er_source(&wb, sources[0], sources[5], sources[6], corrected[1]);
        sl_source(&nb, sources[0], permuted_eps, corrected[2]);
        r3_quote2(&ub, q.cold, corrected[0], candidates[SL_SEL4]);
        r3_quote2(&ub, q.cold, corrected[1], candidates[SL_EARNED2]);
        r3_quote2(&ub, q.cold, sources[0], candidates[SL_POOLED]);
        r3_quote2(&ub, q.cold, corrected[2], candidates[SL_PERMUTED4]);
        memcpy(candidates[SL_COLD], q.cold, sizeof(q.cold));
        if (record >= 0 && (!visits_before || sl_silent(&sb))) {
            for (int byte = 0; byte < 256; byte++)
                if (candidates[SL_SEL4][byte] != candidates[SL_POOLED][byte])
                    fail("episode residual spoke without positive wealth");
            if (!visits_before) first_quotes++;
            silent_quotes++;
        } else if (record >= 0) active_quotes++;
        if (record >= 0 && (!visits_before || sl_silent(&nb)))
            for (int byte = 0; byte < 256; byte++)
                if (candidates[SL_PERMUTED4][byte] != candidates[SL_POOLED][byte])
                    fail("permuted episode residual spoke without positive wealth");
        if (record >= 0 && (!visits_before || (wb.a <= 0.0 && wb.b <= 0.0)))
            for (int byte = 0; byte < 256; byte++)
                if (candidates[SL_EARNED2][byte] != candidates[SL_POOLED][byte])
                    fail("earned2 residual spoke without positive wealth");

        double norm = fmax(validate(q.bf.logp_base), validate(q.cold));
        for (int i = 0; i < 11; i++) norm = fmax(norm, validate(sources[i]));
        for (int i = 0; i < 3; i++) norm = fmax(norm, validate(corrected[i]));
        for (int arm = 0; arm < SL_ARMS; arm++) {
            if (outer[arm].active != admitted) fail("shared admission before quote");
            for (int byte = 0; byte < 256; byte++) {
                live[arm][byte] = !outer[arm].active || candidates[arm][byte] == q.cold[byte]
                    ? q.cold[byte] : la(q.cold[byte], outer[arm].odds + candidates[arm][byte])
                                      - la(0, outer[arm].odds);
                if (!q.rank[byte] &&
                    (candidates[arm][byte] != q.cold[byte] || live[arm][byte] != q.cold[byte]))
                    fail("protected NEW changed in turn34 arm");
            }
            norm = fmax(norm, validate(candidates[arm]));
            norm = fmax(norm, validate(live[arm]));
        }

        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        overall_norm = fmax(overall_norm, norm);
        double shadow_before = admission_shadow;
        int admitted_before = admitted;
        admission_shadow += candidates[SL_POOLED][truth] - q.cold[truth];
        if (!isfinite(admission_shadow)) fail("nonfinite common admission shadow");
        int admit = !admitted && admission_shadow >= 32.0;
        if (admit) { admitted = 1; admission_step = q.bf.t + 1; }
        Outer outer_before[SL_ARMS];
        int activated[SL_ARMS];
        for (int arm = 0; arm < SL_ARMS; arm++) {
            outer_before[arm] = outer[arm];
            activated[arm] = r3_observe_outer(&outer[arm], q.bf.t, q.cold[truth],
                                              candidates[arm][truth], live[arm][truth], admit);
            if (activated[arm] != admit || outer[arm].active != admitted ||
                outer[arm].activation != admission_step)
                fail("shared admission after observation");
        }
        if (record >= 0) {
            double etruth[SL_EPISODES], ptruth[SL_EPISODES];
            for (int i = 0; i < SL_EPISODES; i++) {
                etruth[i] = episodes[i][truth];
                ptruth[i] = permuted_eps[i][truth];
            }
            sl_observe(&sel[record], sources[0][truth], etruth);
            sl_observe(&perm[record], sources[0][truth], ptruth);
            er_observe(&two[record], sources[0][truth], sources[5][truth], sources[6][truth]);
            r3_observe2(&permission[record], sources[0][truth], candidates[SL_POOLED][truth]);
            visits[record]++;
        }
        SLWealth sa = record < 0 ? priors : sel[record];
        SLWealth na = record < 0 ? priors : perm[record];
        ERWealth wa = record < 0 ? priorw : two[record];
        R3Two ua = record < 0 ? prior2 : permission[record];

        printf("%zu\t%d\t", q.bf.t, q.k);
        print_heads(&q, 0); putchar('\t'); print_heads(&q, 1);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-');
        for (size_t i = 0; i < history.length; i++) putchar('0' + history.event[i]);
        printf("\t%.17g\t%d\t%d\t%" PRIu64, q.cold[truth], pmatch.length, record,
               visits_before);
        for (int i = 0; i < 11; i++) printf("\t%.17g", sources[i][truth]);
        printf("\t%.17g\t%.17g\t%.17g\t%.17g\t1\t%.17g\t%d\t%.17g\t%.17g",
               corrected[0][truth], corrected[1][truth], corrected[2][truth],
               norm, shadow_before, admitted_before, ub.memory, ua.memory);
        for (int i = 0; i < SL_EPISODES; i++) printf("\t%.17g", sb.h[i]);
        for (int i = 0; i < SL_EPISODES; i++) printf("\t%.17g", sa.h[i]);
        for (int i = 0; i < SL_EPISODES; i++) printf("\t%.17g", nb.h[i]);
        for (int i = 0; i < SL_EPISODES; i++) printf("\t%.17g", na.h[i]);
        printf("\t%.17g\t%.17g\t%.17g\t%.17g", wb.a, wb.b, wa.a, wa.b);
        for (int arm = 0; arm < SL_ARMS; arm++)
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
    fprintf(stderr, "sel4_router predict: bytes=%" PRIu64
                    " bank4_bytes=%zu bank2_bytes=%zu pooled_bytes=%zu permuted4_bytes=%zu"
                    " rules=%zu records=%zu sel_struct_bytes=%zu first_quotes=%zu"
                    " silent_quotes=%zu active_quotes=%zu admission_shadow=%.17g"
                    " admission_step=%zu max_norm_error=%.17g\n",
            life.events, bank4.bytes, bank2.bytes, pooled.bytes, permuted4.bytes,
            pooled.count, records, sizeof(SLWealth), first_quotes, silent_quotes,
            active_quotes, admission_shadow, admission_step, overall_norm);
    for (int arm = 0; arm < SL_ARMS; arm++)
        fprintf(stderr, "arm=%s gain=%.17g minimum=%.17g drawdown=%.17g active=%d activation=%zu\n",
                sl_names[arm], outer[arm].gain, outer[arm].minimum, outer[arm].drawdown,
                outer[arm].active, outer[arm].activation);
    free(sel); free(perm); free(two); free(permission); free(visits);
    bf_destroy(front); sl_free_bank(&bank4); sl_free_bank(&permuted4);
    r3_free_bank(&bank2); r3_free_grammar(&pooled);
}

/* Frozen handcrafted sequence: first visits, per-episode positive and
   negative wealth, recovery, two records, exact equality, NEW and no-match.
   fixture_check.py reconstructs every row with independent Decimal state. */
static void sl_fixture(void) {
    Quote q = {0};
    q.k = 3;
    q.heads[0] = 3; q.heads[1] = 7; q.heads[2] = 11;
    for (int byte = 0; byte < 256; byte++) q.cold[byte] = -8.0;
    for (int j = 0; j < 3; j++) q.rank[q.heads[j]] = (uint8_t)(j + 1);
    Match me[SL_EPISODES] = {{0}, {0}, {0}, {0}}, mp = {0};
    static const uint16_t votes[SL_EPISODES][4] = {
        {0, 300, 1, 1}, {0, 2, 4, 100}, {0, 1, 200, 2}, {0, 1, 1, 50}};
    for (int c = 0; c < SL_EPISODES; c++) {
        me[c].length = 1;
        for (int j = 1; j < 4; j++) me[c].votes[j] = votes[c][j];
    }
    mp.length = 1;
    for (int j = 0; j < 7; j++) {
        unsigned total = 0;
        for (int c = 0; c < SL_EPISODES; c++) total += me[c].votes[j];
        mp.votes[j] = (uint16_t)total;
    }
    SLWealth wealth[2], priors;
    R3Two permission[2], prior2;
    uint64_t visits[2] = {0, 0};
    sl_init(&priors); r3_init2(&prior2);
    for (int i = 0; i < 2; i++) { sl_init(&wealth[i]); r3_init2(&permission[i]); }
    for (int t = 0; t < 28; t++) {
        int record = t == 4 || t == 17 ? -1 : (t == 1 || t == 18 || t == 22 ? 1 : 0);
        uint8_t truth = t == 0 ? 7 : t == 1 ? 11 : t == 2 || t == 17 ? 23
                      : t >= 5 && t <= 11 ? 3 : t >= 12 && t <= 16 ? 7
                      : t >= 23 && t <= 25 ? 11 : 3;
        double e[SL_EPISODES][256], pool[256], source[256], selq[256], pooledq[256];
        const double *episodes[SL_EPISODES];
        if (record < 0 || t == 3) {
            for (int c = 0; c < SL_EPISODES; c++) memcpy(e[c], q.cold, sizeof(e[c]));
            memcpy(pool, q.cold, sizeof(pool));
        } else {
            for (int c = 0; c < SL_EPISODES; c++) candidate(&q, &me[c], e[c]);
            candidate(&q, &mp, pool);
        }
        for (int c = 0; c < SL_EPISODES; c++) episodes[c] = e[c];
        SLWealth before = record < 0 ? priors : wealth[record];
        R3Two ub = record < 0 ? prior2 : permission[record];
        uint64_t visits_before = record < 0 ? 0 : visits[record];
        sl_source(&before, pool, episodes, source);
        r3_quote2(&ub, q.cold, source, selq);
        r3_quote2(&ub, q.cold, pool, pooledq);
        double norm = validate(q.cold);
        norm = fmax(norm, validate(pool));
        norm = fmax(norm, validate(source));
        norm = fmax(norm, validate(selq));
        norm = fmax(norm, validate(pooledq));
        for (int c = 0; c < SL_EPISODES; c++) norm = fmax(norm, validate(e[c]));
        if (record >= 0 && (!visits_before || sl_silent(&before)))
            for (int byte = 0; byte < 256; byte++)
                if (selq[byte] != pooledq[byte]) fail("fixture silent equality");
        if (record >= 0) {
            double etruth[SL_EPISODES];
            for (int c = 0; c < SL_EPISODES; c++) etruth[c] = e[c][truth];
            sl_observe(&wealth[record], pool[truth], etruth);
            r3_observe2(&permission[record], pool[truth], pooledq[truth]);
            visits[record]++;
        }
        SLWealth after = record < 0 ? priors : wealth[record];
        R3Two ua = record < 0 ? prior2 : permission[record];
        printf("{\"t\":%d,\"record\":%d,\"visits_before\":%" PRIu64
               ",\"truth\":%u,\"rank\":%u,\"cold\":%.17g,\"episodes\":[%.17g,%.17g,%.17g,%.17g]"
               ",\"pooled\":%.17g,\"corrected\":%.17g,\"permission_before\":%.17g"
               ",\"permission_after\":%.17g,\"wealth_before\":[%.17g,%.17g,%.17g,%.17g]"
               ",\"wealth_after\":[%.17g,%.17g,%.17g,%.17g],\"sel4\":%.17g"
               ",\"pooled_quote\":%.17g,\"max_norm_error\":%.17g"
               ",\"wealth_struct_bytes\":%zu}\n",
               t, record, visits_before, (unsigned)truth, (unsigned)q.rank[truth],
               q.cold[truth], e[0][truth], e[1][truth], e[2][truth], e[3][truth],
               pool[truth], source[truth], ub.memory, ua.memory,
               before.h[0], before.h[1], before.h[2], before.h[3],
               after.h[0], after.h[1], after.h[2], after.h[3],
               selq[truth], pooledq[truth], norm, sizeof(SLWealth));
    }
    stream_end();
}

int main(int argc, char **argv) {
    if (argc >= 2 && (!strcmp(argv[1], "emit") || !strcmp(argv[1], "trace")))
        return inherited_episode_main(argc, argv);
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    if (argc == 6 && !strcmp(argv[1], "predict"))
        sl_predict(argv[2], argv[3], argv[4], argv[5]);
    else if (argc == 5 && !strcmp(argv[1], "predict33"))
        er_predict(argv[2], argv[3], argv[4]);  /* frozen turn33 organ, provenance probe */
    else if (argc == 2 && !strcmp(argv[1], "--fixture"))
        sl_fixture();
    else if (argc == 2 && !strcmp(argv[1], "--fixture33"))
        er_fixture();                           /* frozen turn33 fixture, provenance probe */
    else {
        fputs("usage: sel4_router predict BANK4_FULL BANK2_FULL POOLED_FULL PERMUTED4_FULL < RAW\n"
              "       sel4_router predict33 BANK_FULL POOLED_FULL PERMUTED_FULL < RAW\n"
              "       sel4_router emit ALPHABET NONZERO_SEED < COMMANDS\n"
              "       sel4_router trace [compact] < RAW\n"
              "       sel4_router --fixture\n"
              "       sel4_router --fixture33\n", stderr);
        return 2;
    }
    return 0;
}
