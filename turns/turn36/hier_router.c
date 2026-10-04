/* Turn36: fine detail earns against its own coarse parent.
   The frozen controls, archive readers, P0 and outer law remain included. */
#include ".build/turn34_core.c"

#define H_ARMS 6
enum { H_HIER, H_COARSE, H_FINE, H_NULL, H_POOL, H_COLD };
static const char *const h_names[H_ARMS] = {"hier", "coarse", "fine", "null", "pooled", "cold"};
typedef struct {
    ERWealth coarse, detail[2], scrambled[2];
    SLWealth fine;
    R3Two permission;
    uint64_t visits;
} HState;

static void h_init(HState *s) {
    memset(s, 0, sizeof(*s));
    r3_init2(&s->permission);
}

/* Source order: P,A,B,E1..4,N1..4. Corrected order: hier,coarse,fine,null. */
static void h_sources(const HState *s, double src[11][256], double out[4][256]) {
    double detail[2][256], scrambled[2][256];
    for (int p = 0; p < 2; p++) {
        er_source(&s->detail[p], src[1+p], src[3+2*p], src[4+2*p], detail[p]);
        er_source(&s->scrambled[p], src[1+p], src[7+2*p], src[8+2*p], scrambled[p]);
    }
    er_source(&s->coarse, src[0], detail[0], detail[1], out[0]);
    er_source(&s->coarse, src[0], src[1], src[2], out[1]);
    const double *episodes[4] = {src[3], src[4], src[5], src[6]};
    sl_source(&s->fine, src[0], episodes, out[2]);
    er_source(&s->coarse, src[0], scrambled[0], scrambled[1], out[3]);
    int silent = 1;
    for (int p = 0; p < 2; p++) silent &= s->detail[p].a <= 0 && s->detail[p].b <= 0;
    for (int b = 0; b < 256; b++) {
        if (silent && out[0][b] != out[1][b]) fail("hier detail spoke without wealth");
        if (s->coarse.a <= 0 && s->coarse.b <= 0 && out[0][b] != src[0][b])
            fail("hier case spoke without wealth");
    }
}

static void h_observe(HState *s, const double src[11], double pooled_quote) {
    for (int p = 0; p < 2; p++) {
        er_observe(&s->detail[p], src[1+p], src[3+2*p], src[4+2*p]);
        er_observe(&s->scrambled[p], src[1+p], src[7+2*p], src[8+2*p]);
    }
    er_observe(&s->coarse, src[0], src[1], src[2]);
    sl_observe(&s->fine, src[0], src+3);
    r3_observe2(&s->permission, src[0], pooled_quote);
    s->visits++;
}

static void h_print_values(const double *x, int n) {
    for (int i = 0; i < n; i++) printf("%s%.17g", i ? "," : "", x[i]);
}

static void h_print_state(const HState *s) {
    double v[15] = {s->permission.memory, s->coarse.a, s->coarse.b,
        s->detail[0].a, s->detail[0].b, s->detail[1].a, s->detail[1].b,
        s->scrambled[0].a, s->scrambled[0].b, s->scrambled[1].a, s->scrambled[1].b,
        s->fine.h[0], s->fine.h[1], s->fine.h[2], s->fine.h[3]};
    h_print_values(v, 15);
}

static void h_archive_check(const SLBank *bank, const R3Bank *two,
                            const Grammar *pool, const SLBank *null) {
    r3_same_rules(&bank->book[0], pool);
    r3_same_rules(&bank->book[0], &two->book[0]);
    r3_same_rules(&bank->book[0], &null->book[0]);
    if (pool->records != 24 || bank->bytes != 1584 || two->bytes != 912 ||
        pool->bytes != 528 || null->bytes != bank->bytes) fail("fixed archive dimensions");
    for (size_t j = 0; j < pool->records; j++) {
        const Branch *base = &pool->branch[j];
        for (int e = 0; e < 4; e++) {
            const Branch *b = &bank->book[e].branch[j], *n = &null->book[e].branch[j];
            if (b->rule_id != base->rule_id || b->prefix_len != base->prefix_len ||
                n->rule_id != base->rule_id || n->prefix_len != base->prefix_len)
                fail("hier archive address mismatch");
            for (int r = 0; r < 7; r++)
                if (n->counts[r] != bank->book[(r & 1) ? (e^1) : e].branch[j].counts[r])
                    fail("hier null parent-preserving exchange");
        }
        for (int p = 0; p < 2; p++) {
            const Branch *c = &two->book[p].branch[j];
            if (c->rule_id != base->rule_id || c->prefix_len != base->prefix_len)
                fail("hier coarse address mismatch");
            for (int r = 0; r < 7; r++)
                if ((unsigned)c->counts[r] != (unsigned)bank->book[2*p].branch[j].counts[r]
                    + bank->book[2*p+1].branch[j].counts[r]) fail("hier parent projection");
        }
        for (int r = 0; r < 7; r++)
            if ((unsigned)base->counts[r] != (unsigned)two->book[0].branch[j].counts[r]
                + two->book[1].branch[j].counts[r]) fail("hier pooled projection");
    }
}

static void h_predict(char **paths) {
    SLBank bank = sl_load_bank(paths[0]), null = sl_load_bank(paths[3]);
    R3Bank two = r3_load_bank(paths[1]);
    Grammar pool = load_grammar(paths[2]);
    h_archive_check(&bank, &two, &pool, &null);
    HState states[MAX_RECORDS], prior;
    h_init(&prior);
    for (size_t r = 0; r < pool.records; r++) h_init(&states[r]);
    BFState *front = bf_create();
    if (!front) fail("hier frontend allocation");
    PRLife life; pr_life_init(&life);
    History history = {{0}, 0};
    Outer outer[H_ARMS] = {{0}};
    double admission_shadow = 0;
    int admitted = 0;
    fputs("t\tk\theads\tcold_heads\ttruth\trank\thistory\tlogcold\tmatched\trecord\tvisits"
          "\tsources\tcorrected\tstate_before\tstate_after\tnorm\tadmission_shadow\tadmitted", stdout);
    for (int a = 0; a < H_ARMS; a++) printf("\t%s_candidate\t%s_live\t%s_odds\t%s_active\t%s_gain",
        h_names[a], h_names[a], h_names[a], h_names[a], h_names[a]);
    putchar('\n');
    for (;;) {
        Quote q; quote_cold(front, &life, &q);
        Match m = branch_match(&pool, &history);
        int record = m.nmatches ? m.matches[0] : -1;
        HState before = record < 0 ? prior : states[record];
        double src[11][256], corr[4][256], candidate_q[H_ARMS][256], live[H_ARMS][256];
        candidate(&q, &m, src[0]);
        for (int p = 0; p < 2; p++) {
            Match x = r3_record_match(&two.book[p], record);
            candidate(&q, &x, src[1+p]);
        }
        for (int e = 0; e < 4; e++) {
            Match x = r3_record_match(&bank.book[e], record), n = r3_record_match(&null.book[e], record);
            candidate(&q, &x, src[3+e]); candidate(&q, &n, src[7+e]);
        }
        h_sources(&before, src, corr);
        for (int a = 0; a < 4; a++) r3_quote2(&before.permission, q.cold, corr[a], candidate_q[a]);
        r3_quote2(&before.permission, q.cold, src[0], candidate_q[H_POOL]);
        memcpy(candidate_q[H_COLD], q.cold, sizeof(q.cold));
        double norm = validate(q.cold);
        for (int i = 0; i < 11; i++) norm = fmax(norm, validate(src[i]));
        for (int i = 0; i < 4; i++) norm = fmax(norm, validate(corr[i]));
        for (int a = 0; a < H_ARMS; a++) {
            if (outer[a].active != admitted) fail("hier shared admission");
            for (int b = 0; b < 256; b++) {
                live[a][b] = !admitted || candidate_q[a][b] == q.cold[b] ? q.cold[b]
                    : la(q.cold[b], outer[a].odds+candidate_q[a][b])-la(0, outer[a].odds);
                if (!q.rank[b] && (candidate_q[a][b] != q.cold[b] || live[a][b] != q.cold[b]))
                    fail("hier protected NEW");
            }
            norm = fmax(norm, validate(candidate_q[a])); norm = fmax(norm, validate(live[a]));
        }
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        double source_truth[11], corrected_truth[4];
        for (int i = 0; i < 11; i++) source_truth[i] = src[i][truth];
        for (int i = 0; i < 4; i++) corrected_truth[i] = corr[i][truth];
        Outer ob[H_ARMS]; memcpy(ob, outer, sizeof(ob));
        double shadow = admission_shadow;
        int admitted_before = admitted;
        admission_shadow += candidate_q[H_POOL][truth]-q.cold[truth];
        int admit = !admitted && admission_shadow >= 32;
        if (admit) admitted = 1;
        for (int a = 0; a < H_ARMS; a++)
            r3_observe_outer(&outer[a], q.bf.t, q.cold[truth], candidate_q[a][truth], live[a][truth], admit);
        if (record >= 0) h_observe(&states[record], source_truth, candidate_q[H_POOL][truth]);
        const HState *after = record < 0 ? &prior : &states[record];
        printf("%zu\t%d\t", q.bf.t, q.k);
        print_heads(&q, 0); putchar('\t'); print_heads(&q, 1);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-');
        for (size_t j = 0; j < history.length; j++) putchar('0'+history.event[j]);
        printf("\t%.17g\t%d\t%d\t%" PRIu64 "\t", q.cold[truth], m.length, record, before.visits);
        h_print_values(source_truth, 11); putchar('\t'); h_print_values(corrected_truth, 4);
        putchar('\t'); h_print_state(&before); putchar('\t'); h_print_state(after);
        printf("\t%.17g\t%.17g\t%d", norm, shadow, admitted_before);
        for (int a = 0; a < H_ARMS; a++) printf("\t%.17g\t%.17g\t%.17g\t%d\t%.17g",
            candidate_q[a][truth], live[a][truth], ob[a].odds, ob[a].active, outer[a].gain);
        putchar('\n');
        observe_cold(front, &life, &q, truth);
        if (history.length == HORIZON) { memmove(history.event, history.event+1, HORIZON-1); history.length--; }
        history.event[history.length++] = q.rank[truth];
    }
    stream_end();
    bf_destroy(front); sl_free_bank(&bank); sl_free_bank(&null); r3_free_bank(&two); r3_free_grammar(&pool);
}

static void h_fixture(void) {
    HState state[2], prior;
    h_init(&prior); h_init(&state[0]); h_init(&state[1]);
    const double votes[4][3] = {{300,1,1}, {2,4,100}, {1,200,2}, {1,1,50}};
    puts("t\trecord\ttruth\tsources\tcorrected\tstate_before\tstate_after\tcold\tpooled_quote");
    for (int t = 0; t < 80; t++) {
        int r = t == 4 || t == 17 ? -1 : (t == 1 || t == 18 || t == 22 ? 1 : 0);
        int truth = t == 2 ? 23 : t < 14 ? 0 : t < 28 ? 1 : t < 42 ? 2 : t < 56 ? 0 : t < 68 ? 2 : 1;
        double src[11][256], out[4][256], cold[256], pooledq[256];
        for (int b = 0; b < 256; b++) {
            cold[b] = -8;
            for (int i = 0; i < 11; i++) src[i][b] = -8;
        }
        if (r >= 0 && t != 3) for (int b = 0; b < 3; b++) {
            double total = 0, hit = 0;
            for (int e = 0; e < 4; e++) {
                double sum = votes[e][0]+votes[e][1]+votes[e][2];
                src[3+e][b] = log2((3.0/256)*(votes[e][b]+.5)/(sum+1.5));
                int other = e^1;
                double nsum = votes[other][0]+votes[e][1]+votes[other][2];
                double nvote = votes[b == 1 ? e : other][b];
                src[7+e][b] = log2((3.0/256)*(nvote+.5)/(nsum+1.5));
                total += sum; hit += votes[e][b];
            }
            src[0][b] = log2((3.0/256)*(hit+.5)/(total+1.5));
            for (int p = 0; p < 2; p++) {
                double sum = 0;
                for (int j = 0; j < 3; j++) sum += votes[2*p][j]+votes[2*p+1][j];
                src[1+p][b] = log2((3.0/256)*(votes[2*p][b]+votes[2*p+1][b]+.5)/(sum+1.5));
            }
        }
        HState before = r < 0 ? prior : state[r];
        h_sources(&before, src, out);
        r3_quote2(&before.permission, cold, src[0], pooledq);
        for (int i = 0; i < 11; i++) validate(src[i]);
        for (int i = 0; i < 4; i++) validate(out[i]);
        double prices[11], corr[4];
        for (int i = 0; i < 11; i++) prices[i] = src[i][truth];
        for (int i = 0; i < 4; i++) corr[i] = out[i][truth];
        if (r >= 0) h_observe(&state[r], prices, pooledq[truth]);
        printf("%d\t%d\t%d\t", t, r, truth); h_print_values(prices, 11);
        putchar('\t'); h_print_values(corr, 4); putchar('\t'); h_print_state(&before);
        putchar('\t'); h_print_state(r < 0 ? &prior : &state[r]);
        printf("\t-8\t%.17g\n", pooledq[truth]);
    }
}

int main(int argc, char **argv) {
    if (argc >= 2 && (!strcmp(argv[1], "emit") || !strcmp(argv[1], "trace")))
        return inherited_episode_main(argc, argv);
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("hier stdout buffering");
    if (argc == 6 && !strcmp(argv[1], "predict")) h_predict(argv+2);
    else if (argc == 2 && !strcmp(argv[1], "--fixture")) h_fixture();
    else return 2;
    return 0;
}
