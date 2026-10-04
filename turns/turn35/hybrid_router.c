/* Turn35: every relation keeps the two-case memory; exactly ten relations,
   selected from source lives alone, keep four separately earned episodes.
   The frozen turn34 organ is included whole and supplies both controls and
   the unchanged laws. */
#include ".build/turn34_core.c"

#define HY_FINE_RECORDS 10
#define HY_ARCHIVE_BYTES 2112
#define HY_ARMS 6
enum { HY_DENSE10, HY_SPARSE10, HY_SEL4, HY_EARNED2, HY_POOLED, HY_COLD };
static const char *const hy_names[HY_ARMS] = {
    "dense10", "sparse10", "sel4", "earned2", "pooled", "cold"};

typedef struct { double h[SL_EPISODES]; } HYWealth;

typedef struct {
    /* Fine records use four episode books.  Coarse records use book 0 and 1
       as case A and B; their unused books are exact zeros. */
    Grammar book[SL_EPISODES];
    unsigned char *fine;
    size_t bytes, fine_count;
} HYBank;

static void hy_init(HYWealth *state) {
    for (int i = 0; i < SL_EPISODES; i++) state->h[i] = 0.0;
}

static void hy_check(const HYWealth *state, int fine) {
    int parts = fine ? 4 : 2;
    for (int i = 0; i < parts; i++)
        if (!isfinite(state->h[i]) || state->h[i] < -10.0 - 1e-12)
            fail("invalid hybrid residual wealth");
    for (int i = parts; i < 4; i++)
        if (state->h[i] != 0.0) fail("coarse hybrid has hidden episode wealth");
}

static int hy_silent(const HYWealth *state, int fine) {
    hy_check(state, fine);
    int parts = fine ? 4 : 2;
    for (int i = 0; i < parts; i++) if (state->h[i] > 0.0) return 0;
    return 1;
}

static void hy_source(const HYWealth *state, int fine, const double pooled[256],
                      const double *parts[SL_EPISODES], double out[256]) {
    if (fine) {
        SLWealth view;
        for (int i = 0; i < 4; i++) view.h[i] = state->h[i];
        sl_source(&view, pooled, parts, out);
    } else {
        ERWealth view = {state->h[0], state->h[1]};
        er_source(&view, pooled, parts[0], parts[1], out);
    }
}

static void hy_observe(HYWealth *state, int fine, double pooled,
                       const double part[SL_EPISODES]) {
    if (fine) {
        SLWealth view;
        for (int i = 0; i < 4; i++) view.h[i] = state->h[i];
        sl_observe(&view, pooled, part);
        for (int i = 0; i < 4; i++) state->h[i] = view.h[i];
    } else {
        ERWealth view = {state->h[0], state->h[1]};
        er_observe(&view, pooled, part[0], part[1]);
        state->h[0] = view.a;
        state->h[1] = view.b;
        state->h[2] = state->h[3] = 0.0;
    }
    hy_check(state, fine);
}

/* NETEH010: NETEB001 base records with reserved=1 for a fine record, followed
   in record order by E1 and E3 blocks (14 uint16 counters).  E2=A-E1 and
   E4=B-E3, so each fine record costs exactly 28 bytes. */
static HYBank hy_load_bank(const char *path) {
    HYBank bank = {0};
    unsigned char *bytes = read_bytes(path, HY_ARCHIVE_BYTES, &bank.bytes);
    if (bank.bytes < 16 || memcmp(bytes, "NETEH010", 8)) fail("invalid NETEH010 header");
    size_t nr = le32(bytes + 8), nb = le32(bytes + 12);
    size_t base_end = 16 + 4 * nr + 32 * nb;
    if (nr > MAX_RULES || nb > MAX_RECORDS || base_end > bank.bytes)
        fail("invalid NETEH010 count/size");
    bank.fine = calloc(nb, 1);
    if (nb && !bank.fine) fail("allocate hybrid tags");
    Grammar *g = &bank.book[0];
    g->count = nr; g->records = nb; g->bytes = bank.bytes;
    if (nr) {
        g->rule = calloc(nr, sizeof(*g->rule));
        g->forward = calloc(nr, sizeof(*g->forward));
        if (!g->rule || !g->forward) fail("allocate hybrid grammar");
    }
    for (size_t i = 0; i < nr; i++) {
        const unsigned char *p = bytes + 16 + 4 * i;
        g->rule[i] = (Rule){le16(p), le16(p + 2)};
        if (g->rule[i].left >= 7 + i || g->rule[i].right >= 7 + i)
            fail("invalid hybrid topology");
        append_child(&g->forward[i], g->rule[i].left, g->forward);
        append_child(&g->forward[i], g->rule[i].right, g->forward);
    }
    for (int c = 1; c < 4; c++) bank.book[c] = *g;
    for (int c = 0; c < 4; c++) {
        if (nb) bank.book[c].branch = calloc(nb, sizeof(Branch));
        if (nb && !bank.book[c].branch) fail("allocate hybrid records");
    }
    for (size_t i = 0; i < nb; i++) {
        const unsigned char *p = bytes + 16 + 4 * nr + 32 * i;
        unsigned tag = le16(p + 2);
        if (tag > 1) fail("invalid hybrid fine tag");
        bank.fine[i] = (unsigned char)tag;
        bank.fine_count += tag;
    }
    if (bank.fine_count != HY_FINE_RECORDS ||
        bank.bytes != base_end + 28 * bank.fine_count)
        fail("invalid hybrid residual size");
    const unsigned char *extra = bytes + base_end;
    for (size_t i = 0; i < nb; i++) {
        const unsigned char *p = bytes + 16 + 4 * nr + 32 * i;
        for (int c = 0; c < 4; c++) {
            Branch *b = &bank.book[c].branch[i];
            b->rule_id = p[0]; b->prefix_len = p[1];
            if (b->rule_id >= nr || !b->prefix_len ||
                b->prefix_len >= g->forward[b->rule_id].length)
                fail("invalid hybrid record address");
        }
        for (int r = 0; r < 7; r++) {
            unsigned a = le16(p + 4 + 2 * r), b = le16(p + 18 + 2 * r);
            if (bank.fine[i]) {
                unsigned e1 = le16(extra + 2 * r), e3 = le16(extra + 14 + 2 * r);
                if (e1 > a || e3 > b) fail("hybrid residual exceeds coarse parent");
                bank.book[0].branch[i].counts[r] = (uint16_t)e1;
                bank.book[1].branch[i].counts[r] = (uint16_t)(a - e1);
                bank.book[2].branch[i].counts[r] = (uint16_t)e3;
                bank.book[3].branch[i].counts[r] = (uint16_t)(b - e3);
            } else {
                bank.book[0].branch[i].counts[r] = (uint16_t)a;
                bank.book[1].branch[i].counts[r] = (uint16_t)b;
            }
        }
        if (bank.fine[i]) extra += 28;
        const Branch *b = &g->branch[i];
        for (size_t j = 0; j < i; j++)
            if (g->branch[j].prefix_len == b->prefix_len &&
                !memcmp(g->forward[g->branch[j].rule_id].event,
                        g->forward[b->rule_id].event, b->prefix_len))
                fail("duplicate hybrid prefix content");
    }
    if (extra != bytes + bank.bytes) fail("hybrid residual parse drift");
    free(bytes);
    return bank;
}

static void hy_free_bank(HYBank *bank) {
    free(bank->book[0].rule); free(bank->book[0].forward); free(bank->fine);
    for (int c = 0; c < 4; c++) free(bank->book[c].branch);
}

static void hy_check_addresses(const HYBank *dense, const HYBank *sparse,
                               const SLBank *full, const R3Bank *two,
                               const Grammar *pooled) {
    const Grammar *g = &full->book[0];
    r3_same_rules(g, &dense->book[0]); r3_same_rules(g, &sparse->book[0]);
    r3_same_rules(g, &two->book[0]); r3_same_rules(g, pooled);
    if (!g->records || g->records != dense->book[0].records ||
        g->records != sparse->book[0].records || g->records != two->book[0].records ||
        g->records != pooled->records || dense->bytes != sparse->bytes)
        fail("hybrid parent archive projection differs");
    for (size_t i = 0; i < g->records; i++) {
        const Grammar *hy[2] = {&dense->book[0], &sparse->book[0]};
        const unsigned char tag[2] = {dense->fine[i], sparse->fine[i]};
        for (int h = 0; h < 2; h++) {
            if (hy[h]->branch[i].rule_id != g->branch[i].rule_id ||
                hy[h]->branch[i].prefix_len != g->branch[i].prefix_len)
                fail("hybrid selected address differs");
            for (int r = 0; r < 7; r++) {
                unsigned a, b;
                if (tag[h]) {
                    a = (unsigned)hy[h][0].branch[i].counts[r] + hy[h][1].branch[i].counts[r];
                    b = (unsigned)hy[h][2].branch[i].counts[r] + hy[h][3].branch[i].counts[r];
                    for (int c = 0; c < 4; c++)
                        if (hy[h][c].branch[i].counts[r] != full->book[c].branch[i].counts[r])
                            fail("hybrid fine counts differ from full episodes");
                } else {
                    a = hy[h][0].branch[i].counts[r];
                    b = hy[h][1].branch[i].counts[r];
                }
                if (a != two->book[0].branch[i].counts[r] ||
                    b != two->book[1].branch[i].counts[r] ||
                    a + b != pooled->branch[i].counts[r])
                    fail("hybrid coarse projection differs");
            }
        }
    }
}

static void hy_predict(const char *dense_path, const char *sparse_path,
                       const char *full_path, const char *two_path,
                       const char *pooled_path) {
    HYBank dense = hy_load_bank(dense_path), sparse = hy_load_bank(sparse_path);
    SLBank full = sl_load_bank(full_path);
    R3Bank two = r3_load_bank(two_path);
    Grammar pooled = load_grammar(pooled_path);
    hy_check_addresses(&dense, &sparse, &full, &two, &pooled);
    size_t records = pooled.records;
    HYWealth *dh = calloc(records, sizeof(*dh)), *sh = calloc(records, sizeof(*sh));
    SLWealth *all = calloc(records, sizeof(*all));
    ERWealth *coarse = calloc(records, sizeof(*coarse));
    R3Two *permission = calloc(records, sizeof(*permission));
    uint64_t *visits = calloc(records, sizeof(*visits));
    if (!dh || !sh || !all || !coarse || !permission || !visits)
        fail("allocate turn35 routers");
    for (size_t i = 0; i < records; i++) {
        hy_init(&dh[i]); hy_init(&sh[i]); sl_init(&all[i]); er_init(&coarse[i]);
        r3_init2(&permission[i]);
    }
    HYWealth priorh; SLWealth priors; ERWealth priorw; R3Two prior2;
    hy_init(&priorh); sl_init(&priors); er_init(&priorw); r3_init2(&prior2);
    BFState *front = bf_create(); PRLife life; pr_life_init(&life);
    if (!front) fail("allocate turn35 frontend");
    History history = {{0}, 0}; Outer outer[HY_ARMS] = {{0}};
    double admission_shadow = 0.0, overall_norm = 0.0;
    int admitted = 0; size_t admission_step = 0;

    fputs("t\tk\theads\tcold_heads\ttruth\trank\thistory\tlogcold\tmatched\trecord"
          "\trecord_visits_before\tdense_fine\tsparse_fine\tsource_pooled"
          "\tsource_e1\tsource_e2\tsource_e3\tsource_e4\tsource_a2\tsource_b2"
          "\tsource_d1\tsource_d2\tsource_d3\tsource_d4"
          "\tsource_s1\tsource_s2\tsource_s3\tsource_s4"
          "\tcorrected_dense10\tcorrected_sparse10\tcorrected_sel4"
          "\tcorrected_earned2\tmax_norm_error\tnew_exact"
          "\tadmission_shadow_before\tadmitted_before\tpermission_u_before"
          "\tpermission_u_after", stdout);
    for (int i = 1; i <= 4; i++) printf("\tdense10_h%d_before", i);
    for (int i = 1; i <= 4; i++) printf("\tdense10_h%d_after", i);
    for (int i = 1; i <= 4; i++) printf("\tsparse10_h%d_before", i);
    for (int i = 1; i <= 4; i++) printf("\tsparse10_h%d_after", i);
    for (int i = 1; i <= 4; i++) printf("\tsel4_h%d_before", i);
    for (int i = 1; i <= 4; i++) printf("\tsel4_h%d_after", i);
    fputs("\tearned2_ha_before\tearned2_hb_before\tearned2_ha_after\tearned2_hb_after", stdout);
    for (int arm = 0; arm < HY_ARMS; arm++)
        printf("\t%s_candidate\t%s_live\t%s_shadow_before\t%s_odds_before"
               "\t%s_active_before\t%s_activated_after\t%s_gain_after",
               hy_names[arm], hy_names[arm], hy_names[arm], hy_names[arm],
               hy_names[arm], hy_names[arm], hy_names[arm]);
    putchar('\n');

    for (;;) {
        Quote q; quote_cold(front, &life, &q);
        Match pm = branch_match(&pooled, &history), fm = branch_match(&full.book[0], &history);
        Match tm = branch_match(&two.book[0], &history), dm = branch_match(&dense.book[0], &history);
        Match sm = branch_match(&sparse.book[0], &history);
        if (pm.length != fm.length || pm.nmatches != fm.nmatches ||
            pm.length != tm.length || pm.nmatches != tm.nmatches ||
            pm.length != dm.length || pm.nmatches != dm.nmatches ||
            pm.length != sm.length || pm.nmatches != sm.nmatches ||
            (pm.nmatches && (pm.matches[0] != fm.matches[0] || pm.matches[0] != tm.matches[0] ||
                             pm.matches[0] != dm.matches[0] || pm.matches[0] != sm.matches[0])))
            fail("hybrid/full/coarse/pooled match differs");
        int record = pm.nmatches ? pm.matches[0] : -1;
        int df = record >= 0 ? dense.fine[record] : 0;
        int sf = record >= 0 ? sparse.fine[record] : 0;
        double sources[15][256], corrected[4][256];
        double candidates[HY_ARMS][256], live[HY_ARMS][256];
        candidate(&q, &pm, sources[0]);
        candidate(&q, &fm, sources[1]);
        for (int c = 1; c < 4; c++) { Match m = r3_record_match(&full.book[c], record); candidate(&q, &m, sources[1+c]); }
        candidate(&q, &tm, sources[5]);
        { Match m = r3_record_match(&two.book[1], record); candidate(&q, &m, sources[6]); }
        candidate(&q, &dm, sources[7]);
        for (int c = 1; c < 4; c++) { Match m = r3_record_match(&dense.book[c], record); candidate(&q, &m, sources[7+c]); }
        candidate(&q, &sm, sources[11]);
        for (int c = 1; c < 4; c++) { Match m = r3_record_match(&sparse.book[c], record); candidate(&q, &m, sources[11+c]); }
        HYWealth db = record < 0 ? priorh : dh[record], xb = record < 0 ? priorh : sh[record];
        SLWealth fb = record < 0 ? priors : all[record];
        ERWealth cb = record < 0 ? priorw : coarse[record];
        R3Two ub = record < 0 ? prior2 : permission[record];
        uint64_t visits_before = record < 0 ? 0 : visits[record];
        const double *ep[4] = {sources[1], sources[2], sources[3], sources[4]};
        const double *dp[4] = {sources[7], sources[8], sources[9], sources[10]};
        const double *sp[4] = {sources[11], sources[12], sources[13], sources[14]};
        hy_source(&db, df, sources[0], dp, corrected[0]);
        hy_source(&xb, sf, sources[0], sp, corrected[1]);
        sl_source(&fb, sources[0], ep, corrected[2]);
        er_source(&cb, sources[0], sources[5], sources[6], corrected[3]);
        r3_quote2(&ub, q.cold, corrected[0], candidates[HY_DENSE10]);
        r3_quote2(&ub, q.cold, corrected[1], candidates[HY_SPARSE10]);
        r3_quote2(&ub, q.cold, corrected[2], candidates[HY_SEL4]);
        r3_quote2(&ub, q.cold, corrected[3], candidates[HY_EARNED2]);
        r3_quote2(&ub, q.cold, sources[0], candidates[HY_POOLED]);
        memcpy(candidates[HY_COLD], q.cold, sizeof(q.cold));
        if (record >= 0) {
            int silent[4] = {hy_silent(&db, df), hy_silent(&xb, sf), sl_silent(&fb),
                             cb.a <= 0.0 && cb.b <= 0.0};
            for (int arm = 0; arm < 4; arm++) if (!visits_before || silent[arm])
                for (int byte = 0; byte < 256; byte++)
                    if (candidates[arm][byte] != candidates[HY_POOLED][byte])
                        fail("hybrid residual spoke without positive wealth");
        }
        double norm = fmax(validate(q.bf.logp_base), validate(q.cold));
        for (int i = 0; i < 15; i++) norm = fmax(norm, validate(sources[i]));
        for (int i = 0; i < 4; i++) norm = fmax(norm, validate(corrected[i]));
        for (int arm = 0; arm < HY_ARMS; arm++) {
            if (outer[arm].active != admitted) fail("shared admission before quote");
            for (int byte = 0; byte < 256; byte++) {
                live[arm][byte] = !outer[arm].active || candidates[arm][byte] == q.cold[byte]
                    ? q.cold[byte] : la(q.cold[byte], outer[arm].odds + candidates[arm][byte])
                                      - la(0, outer[arm].odds);
                if (!q.rank[byte] && (candidates[arm][byte] != q.cold[byte] ||
                                      live[arm][byte] != q.cold[byte]))
                    fail("protected NEW changed in turn35 arm");
            }
            norm = fmax(norm, validate(candidates[arm])); norm = fmax(norm, validate(live[arm]));
        }
        int next = fgetc(stdin); if (next == EOF) break;
        uint8_t truth = (uint8_t)next; overall_norm = fmax(overall_norm, norm);
        double shadow_before = admission_shadow; int admitted_before = admitted;
        admission_shadow += candidates[HY_POOLED][truth] - q.cold[truth];
        if (!isfinite(admission_shadow)) fail("nonfinite common admission shadow");
        int admit = !admitted && admission_shadow >= 32.0;
        if (admit) { admitted = 1; admission_step = q.bf.t + 1; }
        Outer ob[HY_ARMS]; int activated[HY_ARMS];
        for (int arm = 0; arm < HY_ARMS; arm++) {
            ob[arm] = outer[arm];
            activated[arm] = r3_observe_outer(&outer[arm], q.bf.t, q.cold[truth],
                                              candidates[arm][truth], live[arm][truth], admit);
            if (activated[arm] != admit || outer[arm].active != admitted ||
                outer[arm].activation != admission_step) fail("shared admission after observation");
        }
        if (record >= 0) {
            double et[4], dt[4], st[4];
            for (int i = 0; i < 4; i++) { et[i]=ep[i][truth]; dt[i]=dp[i][truth]; st[i]=sp[i][truth]; }
            hy_observe(&dh[record], df, sources[0][truth], dt);
            hy_observe(&sh[record], sf, sources[0][truth], st);
            sl_observe(&all[record], sources[0][truth], et);
            er_observe(&coarse[record], sources[0][truth], sources[5][truth], sources[6][truth]);
            r3_observe2(&permission[record], sources[0][truth], candidates[HY_POOLED][truth]);
            visits[record]++;
        }
        HYWealth da = record < 0 ? priorh : dh[record], xa = record < 0 ? priorh : sh[record];
        SLWealth fa = record < 0 ? priors : all[record];
        ERWealth ca = record < 0 ? priorw : coarse[record];
        R3Two ua = record < 0 ? prior2 : permission[record];

        printf("%zu\t%d\t", q.bf.t, q.k); print_heads(&q, 0); putchar('\t'); print_heads(&q, 1);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-'); else for (size_t i=0;i<history.length;i++) putchar('0'+history.event[i]);
        printf("\t%.17g\t%d\t%d\t%" PRIu64 "\t%d\t%d", q.cold[truth], pm.length, record,
               visits_before, df, sf);
        for (int i = 0; i < 15; i++) printf("\t%.17g", sources[i][truth]);
        for (int i = 0; i < 4; i++) printf("\t%.17g", corrected[i][truth]);
        printf("\t%.17g\t1\t%.17g\t%d\t%.17g\t%.17g", norm, shadow_before,
               admitted_before, ub.memory, ua.memory);
        for (int i=0;i<4;i++) printf("\t%.17g", db.h[i]);
        for (int i=0;i<4;i++) printf("\t%.17g", da.h[i]);
        for (int i=0;i<4;i++) printf("\t%.17g", xb.h[i]);
        for (int i=0;i<4;i++) printf("\t%.17g", xa.h[i]);
        for (int i=0;i<4;i++) printf("\t%.17g", fb.h[i]);
        for (int i=0;i<4;i++) printf("\t%.17g", fa.h[i]);
        printf("\t%.17g\t%.17g\t%.17g\t%.17g", cb.a, cb.b, ca.a, ca.b);
        for (int arm=0;arm<HY_ARMS;arm++)
            printf("\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g",
                   candidates[arm][truth], live[arm][truth], ob[arm].shadow, ob[arm].odds,
                   ob[arm].active, activated[arm], outer[arm].gain);
        putchar('\n');
        observe_cold(front, &life, &q, truth);
        if (history.length == sizeof(history.event)) { memmove(history.event, history.event+1, history.length-1); history.length--; }
        history.event[history.length++] = q.rank[truth];
    }
    stream_end();
    fprintf(stderr, "hybrid_router predict: bytes=%" PRIu64
                    " dense_bytes=%zu sparse_bytes=%zu full_bytes=%zu coarse_bytes=%zu pooled_bytes=%zu"
                    " records=%zu hybrid_struct_bytes=%zu admission_shadow=%.17g admission_step=%zu"
                    " max_norm_error=%.17g\n", life.events, dense.bytes, sparse.bytes, full.bytes,
            two.bytes, pooled.bytes, records, sizeof(HYWealth), admission_shadow,
            admission_step, overall_norm);
    for (int arm=0;arm<HY_ARMS;arm++)
        fprintf(stderr, "arm=%s gain=%.17g minimum=%.17g drawdown=%.17g active=%d activation=%zu\n",
                hy_names[arm], outer[arm].gain, outer[arm].minimum, outer[arm].drawdown,
                outer[arm].active, outer[arm].activation);
    free(dh); free(sh); free(all); free(coarse); free(permission); free(visits); bf_destroy(front);
    hy_free_bank(&dense); hy_free_bank(&sparse); sl_free_bank(&full); r3_free_bank(&two);
    r3_free_grammar(&pooled);
}

/* A deterministic no-world fixture.  Record 0 is fine, record 1 coarse. */
static void hy_fixture(void) {
    Quote q = {0}; q.k=3; q.heads[0]=3; q.heads[1]=7; q.heads[2]=11;
    for (int byte=0;byte<256;byte++) q.cold[byte]=-8.0;
    for (int j=0;j<3;j++) q.rank[q.heads[j]]=(uint8_t)(j+1);
    Match me[4]={{0},{0},{0},{0}}, mp={0}, ma={0}, mb={0};
    static const uint16_t votes[4][4]={{0,300,1,1},{0,2,4,100},{0,1,200,2},{0,1,1,50}};
    for (int c=0;c<4;c++) { me[c].length=1; for (int j=1;j<4;j++) me[c].votes[j]=votes[c][j]; }
    mp.length=ma.length=mb.length=1;
    for (int j=0;j<7;j++) {
        ma.votes[j]=me[0].votes[j]+me[1].votes[j];
        mb.votes[j]=me[2].votes[j]+me[3].votes[j];
        mp.votes[j]=ma.votes[j]+mb.votes[j];
    }
    HYWealth wealth[2], priorh; R3Two permission[2], prior2; uint64_t visits[2]={0,0};
    hy_init(&priorh); r3_init2(&prior2);
    for (int i=0;i<2;i++) { hy_init(&wealth[i]); r3_init2(&permission[i]); }
    for (int t=0;t<40;t++) {
        int record = t==4 || t==21 ? -1 : (t%2); int fine = record==0;
        uint8_t truth = t==4 || t==21 ? 23 : t<12 ? 3 : t<26 ? 7 : 11;
        double ep[4][256], part[4][256], pool[256], source[256], hq[256], pq[256];
        const double *parts[4];
        if (record<0 || t==3) {
            for (int c=0;c<4;c++) memcpy(ep[c],q.cold,sizeof(ep[c])); memcpy(pool,q.cold,sizeof(pool));
        } else {
            for (int c=0;c<4;c++) candidate(&q,&me[c],ep[c]); candidate(&q,&mp,pool);
        }
        if (fine) for (int c=0;c<4;c++) memcpy(part[c],ep[c],sizeof(part[c]));
        else {
            if (record<0 || t==3) { memcpy(part[0],q.cold,sizeof(part[0])); memcpy(part[1],q.cold,sizeof(part[1])); }
            else { candidate(&q,&ma,part[0]); candidate(&q,&mb,part[1]); }
            memcpy(part[2],q.cold,sizeof(part[2])); memcpy(part[3],q.cold,sizeof(part[3]));
        }
        for (int c=0;c<4;c++) parts[c]=part[c];
        HYWealth before=record<0?priorh:wealth[record]; R3Two ub=record<0?prior2:permission[record];
        uint64_t vb=record<0?0:visits[record]; hy_source(&before,fine,pool,parts,source);
        r3_quote2(&ub,q.cold,source,hq); r3_quote2(&ub,q.cold,pool,pq);
        double norm=fmax(validate(q.cold),fmax(validate(pool),fmax(validate(source),validate(hq))));
        if (record>=0 && (!vb || hy_silent(&before,fine)))
            for (int byte=0;byte<256;byte++) if (hq[byte]!=pq[byte]) fail("hybrid fixture silence");
        if (record>=0) {
            double pt[4]; for(int c=0;c<4;c++) pt[c]=part[c][truth];
            hy_observe(&wealth[record],fine,pool[truth],pt);
            r3_observe2(&permission[record],pool[truth],pq[truth]); visits[record]++;
        }
        HYWealth after=record<0?priorh:wealth[record]; R3Two ua=record<0?prior2:permission[record];
        printf("{\"t\":%d,\"record\":%d,\"fine\":%d,\"visits_before\":%" PRIu64
               ",\"truth\":%u,\"rank\":%u,\"cold\":%.17g,\"parts\":[%.17g,%.17g,%.17g,%.17g]"
               ",\"pooled\":%.17g,\"corrected\":%.17g,\"permission_before\":%.17g"
               ",\"permission_after\":%.17g,\"wealth_before\":[%.17g,%.17g,%.17g,%.17g]"
               ",\"wealth_after\":[%.17g,%.17g,%.17g,%.17g],\"hybrid\":%.17g"
               ",\"pooled_quote\":%.17g,\"max_norm_error\":%.17g,\"wealth_struct_bytes\":%zu}\n",
               t,record,fine,vb,(unsigned)truth,(unsigned)q.rank[truth],q.cold[truth],
               part[0][truth],part[1][truth],part[2][truth],part[3][truth],pool[truth],source[truth],
               ub.memory,ua.memory,before.h[0],before.h[1],before.h[2],before.h[3],
               after.h[0],after.h[1],after.h[2],after.h[3],hq[truth],pq[truth],norm,sizeof(HYWealth));
    }
    stream_end();
}

int main(int argc, char **argv) {
    if (argc>=2 && (!strcmp(argv[1],"emit") || !strcmp(argv[1],"trace")))
        return inherited_episode_main(argc,argv);
    if (setvbuf(stdout,NULL,_IOFBF,1u<<20)) fail("stdout buffering");
    if (argc==7 && !strcmp(argv[1],"predict")) hy_predict(argv[2],argv[3],argv[4],argv[5],argv[6]);
    else if (argc==6 && !strcmp(argv[1],"predict34")) sl_predict(argv[2],argv[3],argv[4],argv[5]);
    else if (argc==2 && !strcmp(argv[1],"--fixture")) hy_fixture();
    else if (argc==2 && !strcmp(argv[1],"--fixture34")) sl_fixture();
    else {
        fputs("usage: hybrid_router predict DENSE10 SPARSE10 BANK4 BANK2 POOLED < RAW\n"
              "       hybrid_router predict34 BANK4 BANK2 POOLED PERMUTED4 < RAW\n"
              "       hybrid_router emit ALPHABET NONZERO_SEED < COMMANDS\n"
              "       hybrid_router trace [compact] < RAW\n"
              "       hybrid_router --fixture\n       hybrid_router --fixture34\n",stderr);
        return 2;
    }
    return 0;
}
