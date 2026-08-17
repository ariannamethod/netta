#!/bin/bash
# Cross-reader agreement battery for the body-27 canonical language.
# The life (--cite) and the external hand (scripts/warrant_check.c) share one
# law and no code; on every record here they must both accept or both refuse,
# and a refused citation must leave state and biography byte-identical.
# One divergence is named and expected: S14 concatenates two complete sittings,
# which the external hand accepts as a transcript and the citation refuses as
# more than one sitting. Everything else that differs is a defect.
# Usage: battery.sh NETTA WCHECK WFIXTURE STATE BIO OUTDIR
set -u
export LC_ALL=C
[ $# -eq 6 ] || { echo "usage: $0 NETTA WCHECK WFIXTURE STATE BIO OUTDIR" >&2; exit 2; }
NETTA=$1; WC=$2; WFIX=$3; STATE=$4; BIO=$5; OUT=$6
HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE" || exit 2
LAW=$(sed -n '2s/.*law-digest=\([0-9a-f]*\)$/\1/p' base2.t)
mkdir -p "$OUT/cases" || exit 2
rm -f "$OUT"/cases/*.t
LAWLINE=$(sed -n 2p base2.t)
BANNER='NETTA ZERO'
T=$(mktemp -d) || exit 2

emit() { cat > "$OUT/cases/$1.t"; }

# recompute every court receipt under LAW and rewrite the close over the sitting
reseal() {
  cat > "$T/r1"
  { sed -n '1,3p' "$T/r1"
    sed -n '4,$p' "$T/r1" | grep -v '^court close: ' | while IFS= read -r l; do
      printf '%s\n' "${l% receipt=*}" | "$WFIX" receipt "$LAW"
    done
  } > "$T/r2"
  local d n
  d=$(sed -n '3,$p' "$T/r2" | "$WFIX" docket)
  n=$(sed -n '4,$p' "$T/r2" | wc -l | tr -d ' ')
  cat "$T/r2"; printf 'court close: verdicts=%s docket=%s\n' "$n" "$d"
}

mr() { sed -e "$2" base2.t | reseal | emit "$1"; }   # mutate base2, reseal
m0() { sed -e "$2" base2.t | emit "$1"; }            # mutate base2, raw
mp() { reseal < base2.t | sed -e "$2" | emit "$1"; } # reseal base2, then mutate

# court line constructor: id ctx bytes P gm covered denom changed shore verdict
mkcourt() {
  awk -v id="$1" -v ctx="$2" -v b="$3" -v P="$4" -v gm="$5" -v cov="$6" \
      -v den="$7" -v ch="$8" -v sh="$9" -v v="${10}" 'BEGIN{
    G=sprintf("%.6f", gm/1000000.0); Q=sprintf("%.6f", P+G);
    m=sprintf("%.4f", 100.0*cov/den);
    printf "court %s: digest=4e66ccf127a219ad context=%s bytes=%s P=%.6f Q=%s matched16=%s%% matched-bytes=%s/%s G=%s gap-micro=%s changed=%s/%s verdict=%s\n",
      id,ctx,b,P,Q,m,cov,den,G,gm,ch,sh,v }'
}
# one-shore sitting from a constructed court line: name bytes ctx courtline
mk1() {
  { printf '%s\n%s\n' "$BANNER" "$LAWLINE"
    printf 'court sitting: candidate-digest=b6fd16505adcaa7d bytes=%s context=%s shores=1 law-digest=%s\n' "$2" "$3" "$LAW"
    printf '%s\n' "$4"
  } | reseal | emit "$1"
}

# ---- P: positive controls
for b in base1 base2 base32 basectx; do cp $b.t "$OUT/cases/P_$b.t"; done
reseal < base2.t | emit P_base2_resealed

# ---- H: sitting header fields (docket resealed, so only the header is in question)
mr H01_bytes_lead0        '3s/bytes=228/bytes=0228/'
mr H02_bytes_plus         '3s/bytes=228/bytes=+228/'
mr H03_bytes_neg          '3s/bytes=228/bytes=-228/'
mr H04_bytes_float        '3s/bytes=228/bytes=228.0/'
mr H05_bytes_hex          '3s/bytes=228/bytes=0xe4/'
mr H06_bytes_empty        '3s/bytes=228/bytes=/'
mr H07_bytes_overflow     '3s/bytes=228/bytes=99999999999999999999/'
mr H08_bytes_u64max       '3s/bytes=228/bytes=18446744073709551615/'
mr H09_bytes_mismatch     '3s/bytes=228/bytes=227/'
mr H10_shores_lead0       '3s/shores=2/shores=02/'
mr H11_shores_plus        '3s/shores=2/shores=+2/'
mr H12_shores_neg         '3s/shores=2/shores=-2/'
mr H13_shores_zero        '3s/shores=2/shores=0/'
mr H14_shores_under       '3s/shores=2/shores=1/'
mr H15_shores_over        '3s/shores=2/shores=3/'
mr H16_shores_33          '3s/shores=2/shores=33/'
mr H17_ctx_upper          '3s/context=cold/context=COLD/'
mr H18_ctx_cap            '3s/context=cold/context=Cold/'
mr H19_ctx_warm           '3s/context=cold/context=warm/'
mr H20_ctx_hex3           '3s/context=cold/context=616/'
mr H21_ctx_hex5           '3s/context=cold/context=61622/'
mr H22_ctx_hexupper       '3s/context=cold/context=ABCD/'
mr H23_ctx_empty          '3s/context=cold/context=/'
mr H24_cand_upper         '3s/candidate-digest=a8bfae544bd5e579/candidate-digest=A8BFAE544BD5E579/'
mr H25_cand_15            '3s/candidate-digest=a8bfae544bd5e579/candidate-digest=a8bfae544bd5e57/'
mr H26_cand_17            '3s/candidate-digest=a8bfae544bd5e579/candidate-digest=a8bfae544bd5e5790/'
mr H27_cand_nonhex        '3s/candidate-digest=a8bfae544bd5e579/candidate-digest=g8bfae544bd5e579/'
mr H28_cand_empty         '3s/candidate-digest=a8bfae544bd5e579/candidate-digest=/'
mr H29_law_upper          "3s/law-digest=$LAW/law-digest=F2CC2274CB3C752B/"
mr H30_law_15             "3s/law-digest=$LAW/law-digest=f2cc2274cb3c752/"
mr H31_law_17             "3s/law-digest=$LAW/law-digest=f2cc2274cb3c752b0/"
mr H32_law_other          "3s/law-digest=$LAW/law-digest=0123456789abcdef/"
mr H33_prefix_cap         '3s/court sitting:/Court sitting:/'
mr H34_double_space       '3s/court sitting: /court sitting:  /'
mr H35_trailing_space     '3s/$/ /'
mr H36_extra_field        '3s/$/ extra=1/'
mr H37_tab_sep            '3s/ bytes=/	bytes=/'
mr H38_field_order        '3s/bytes=228 context=cold/context=cold bytes=228/'

# ---- B: bytes limits (constructed, one shore, witnesses re-derived)
mk1 B01_bytes_min2      2     cold "$(mkcourt 0 cold 2     5.390580 670993 1  2     1226 1800 replay)"
mk1 B02_bytes_max16384  16384 cold "$(mkcourt 0 cold 16384 5.390580 670993 84 16384 1226 1800 order)"
mk1 B03_bytes_16385     16385 cold "$(mkcourt 0 cold 16385 5.390580 670993 84 16385 1226 1800 order)"
mk1 B04_bytes_1         1     cold "$(mkcourt 0 cold 1     5.390580 670993 1  1     1226 1800 replay)"

# ---- C: court record fields (receipt resealed, so only grammar/law is in question)
mr C01_id_lead0           '4s/^court 0:/court 00:/'
mr C02_id_plus            '4s/^court 0:/court +0:/'
mr C03_id_neg0            '4s/^court 0:/court -0:/'
mr C04_id_first1          '4s/^court 0:/court 1:/'
{ sed -n '1,3p' base2.t; sed -n 5p base2.t; sed -n 4p base2.t; } | reseal | emit C05_id_swapped
{ sed -n '1,4p' base2.t; sed -n 4p base2.t; } | reseal | emit C06_id_dup0
mr C07_id_intmax1         '4s/^court 0:/court 2147483648:/'
mr C08_id_double_space    '4s/^court 0:/court  0:/'
mr C09_id_space_colon     '4s/^court 0:/court 0 :/'
mr C10_digest_upper       '4s/digest=4e66ccf127a219ad/digest=4E66CCF127A219AD/'
mr C11_digest_15          '4s/digest=4e66ccf127a219ad/digest=4e66ccf127a219a/'
mr C12_digest_17          '4s/digest=4e66ccf127a219ad/digest=4e66ccf127a219ad0/'
mr C13_digest_nonhex      '4s/digest=4e66ccf127a219ad/digest=4e66ccf127a219az/'
mr C14_ctx_mismatch       '4s/context=cold/context=6162/'
mr C15_ctx_upper          '4s/context=cold/context=COLD/'
mr C16_bytes_mismatch     '4s/bytes=228/bytes=227/'
mr C17_bytes_lead0        '4s/bytes=228/bytes=0228/'
mr C18_bytes_plus         '4s/bytes=228/bytes=+228/'
mr C19_P_plus             '4s/P=5.390580/P=+5.390580/'
mr C20_P_neg              '4s/P=5.390580/P=-5.390580/'
mr C21_P_5dec             '4s/P=5.390580/P=5.39058/'
mr C22_P_7dec             '4s/P=5.390580/P=5.3905800/'
mr C23_P_lead0            '4s/P=5.390580/P=05.390580/'
mr C24_P_exp              '4s/P=5.390580/P=5.390580e0/'
mr C25_P_nan              '4s/P=5.390580/P=nan/'
mr C26_P_inf              '4s/P=5.390580/P=inf/'
mr C27_P_negzero          '4s/P=5.390580/P=-0.000000/'
mr C28_P_nolead           '4s/P=5.390580/P=.390580/'
mr C29_Q_plus             '4s/Q=6.061573/Q=+6.061573/'
mr C30_Q_gap1e6           '4s/Q=6.061573/Q=6.061574/'
mr C31_Q_gap2e6           '4s/Q=6.061573/Q=6.061575/'
mr C32_Q_gap3e6           '4s/Q=6.061573/Q=6.061576/'
mr C33_m16_3dec           '4s/matched16=36.8421%/matched16=36.842%/'
mr C34_m16_5dec           '4s/matched16=36.8421%/matched16=36.84210%/'
mr C35_m16_wrong          '4s/matched16=36.8421%/matched16=36.8422%/'
mr C36_m16_plus           '4s/matched16=36.8421%/matched16=+36.8421%/'
mr C37_m16_nopct          '4s/matched16=36.8421%/matched16=36.8421/'
mr C38_m16_dblpct         '4s/matched16=36.8421%/matched16=36.8421%%/'
mr C39_mb_lead0           '4s|matched-bytes=84/228|matched-bytes=084/228|'
mr C40_mb_den_lead0       '4s|matched-bytes=84/228|matched-bytes=84/0228|'
mr C41_mb_plus            '4s|matched-bytes=84/228|matched-bytes=+84/228|'
mr C42_mb_over            '4s|matched-bytes=84/228|matched-bytes=229/228|'
mr C43_mb_den_mismatch    '4s|matched-bytes=84/228|matched-bytes=84/227|'
mr C44_mb_noden           '4s|matched-bytes=84/228|matched-bytes=84/|'
mr C45_mb_nocov           '4s|matched-bytes=84/228|matched-bytes=/228|'
mr C46_mb_neg             '4s|matched-bytes=84/228|matched-bytes=-84/228|'
mr C47_mb_triple          '4s|matched-bytes=84/228|matched-bytes=84/228/1|'
mr C48_G_plus             '4s/G=0.670993/G=+0.670993/'
mr C49_G_5dec             '4s/G=0.670993/G=0.67099/'
mr C50_G_7dec             '4s/G=0.670993/G=0.6709930/'
mr C51_G_vs_micro         '4s/G=0.670993/G=0.670994/'
mr C52_micro_vs_G         '4s/gap-micro=670993/gap-micro=670994/'
mr C53_micro_plus         '4s/gap-micro=670993/gap-micro=+670993/'
mr C54_micro_lead0        '4s/gap-micro=670993/gap-micro=0670993/'
mr C55_micro_float        '4s/gap-micro=670993/gap-micro=670993.0/'
mr C56_micro_llmax1       '4s/gap-micro=670993/gap-micro=9223372036854775808/'
mr C57_micro_llmin        '4s/G=0.670993 gap-micro=670993/G=-9223372036854.775808 gap-micro=-9223372036854775808/'
mr C58_ch_lead0           '4s|changed=1226/1800|changed=01226/1800|'
mr C59_ch_over            '4s|changed=1226/1800|changed=1801/1800|'
mr C60_ch_sh_lead0        '4s|changed=1226/1800|changed=1226/01800|'
mr C61_ch_nosh            '4s|changed=1226/1800|changed=1226/|'
mr C62_ch_neg             '4s|changed=1226/1800|changed=-1/1800|'
mr C63_ch_equal           '4s|changed=1226/1800|changed=1800/1800|'
mr C64_ch0_order          '4s|changed=1226/1800|changed=0/1800|'
mr C65_ch0_abstain        '4s|changed=1226/1800 verdict=order|changed=0/1800 verdict=abstain|'
mr C66_ch00_abstain       '4s|changed=1226/1800 verdict=order|changed=0/0 verdict=abstain|'
mr C67_v_cap              '4s/verdict=order/verdict=Order/'
mr C68_v_orders           '4s/verdict=order/verdict=orders/'
mr C69_v_replay           '4s/verdict=order/verdict=replay/'
mr C70_v_stranger         '4s/verdict=order/verdict=stranger/'
mr C71_v_abstain          '4s/verdict=order/verdict=abstain/'
mr C72_v_empty            '4s/verdict=order/verdict=/'
mr C73_v_ordering         '4s/verdict=order/verdict=ordering/'
mr C74_v_abstainer        '4s/verdict=order/verdict=abstainer/'
mr C75_v_trailing_space   '4s/verdict=order/verdict=order /'
mr C76_field_missing      '4s/ G=0.670993//'
mr C77_field_extra        '4s/ verdict=order/ extra=1 verdict=order/'
mr C78_tab_sep            '4s/ P=/	P=/'
mr C79_labels_swapped     '4s/P=5.390580 Q=6.061573/Q=5.390580 P=6.061573/'
mr C80_prefix_cap         '4s/^court 0:/Court 0:/'

# ---- L: law boundaries (constructed, witnesses re-derived)
mk1 L01_gm500000_order      228 cold "$(mkcourt 0 cold 228 5.390580 500000  84  228 1226 1800 order)"
mk1 L02_gm499999_stranger   228 cold "$(mkcourt 0 cold 228 5.390580 499999  84  228 1226 1800 stranger)"
mk1 L03_gm499999_order      228 cold "$(mkcourt 0 cold 228 5.390580 499999  84  228 1226 1800 order)"
mk1 L04_gm500000_stranger   228 cold "$(mkcourt 0 cold 228 5.390580 500000  84  228 1226 1800 stranger)"
mk1 L05_cov114_replay       228 cold "$(mkcourt 0 cold 228 5.390580 670993  114 228 1226 1800 replay)"
mk1 L06_cov113_order        228 cold "$(mkcourt 0 cold 228 5.390580 670993  113 228 1226 1800 order)"
mk1 L07_cov113_replay       228 cold "$(mkcourt 0 cold 228 5.390580 670993  113 228 1226 1800 replay)"
mk1 L08_cov114_order        228 cold "$(mkcourt 0 cold 228 5.390580 670993  114 228 1226 1800 order)"
mk1 L09_odd_cov114_replay   227 cold "$(mkcourt 0 cold 227 5.390580 670993  114 227 1226 1800 replay)"
mk1 L10_odd_cov113_order    227 cold "$(mkcourt 0 cold 227 5.390580 670993  113 227 1226 1800 order)"
mk1 L11_odd_cov114_order    227 cold "$(mkcourt 0 cold 227 5.390580 670993  114 227 1226 1800 order)"
mk1 L12_gm_neg500000        228 cold "$(mkcourt 0 cold 228 5.390580 -500000 84  228 1226 1800 stranger)"
mk1 L13_gm0_abstain         228 cold "$(mkcourt 0 cold 228 5.390580 0       84  228 1226 1800 abstain)"
mk1 L14_gm0_Gnegzero        228 cold "$(mkcourt 0 cold 228 5.390580 0 84 228 1226 1800 abstain | sed 's/G=0.000000/G=-0.000000/')"
mk1 L15_gmneg0              228 cold "$(mkcourt 0 cold 228 5.390580 0 84 228 1226 1800 abstain | sed 's/gap-micro=0 /gap-micro=-0 /')"
mk1 L16_gm0_stranger        228 cold "$(mkcourt 0 cold 228 5.390580 0       84  228 1226 1800 stranger)"
mk1 L17_gm1_stranger        228 cold "$(mkcourt 0 cold 228 5.390580 1       84  228 1226 1800 stranger)"
mk1 L18_ctx6162_ok          228 6162 "$(mkcourt 0 6162 228 5.390580 670993  84  228 1226 1800 order)"
mk1 L19_cov0_ch0_abstain    228 cold "$(mkcourt 0 cold 228 5.390580 670993  0   228 0    1800 abstain)"
mk1 L20_covfull_replay      228 cold "$(mkcourt 0 cold 228 5.390580 670993  228 228 1226 1800 replay)"
mk1 L21_P0_ok               228 cold "$(mkcourt 0 cold 228 0.000000 670993  84  228 1226 1800 order)"

# ---- R: receipt (post-reseal edits)
mp R01_receipt_upper      '4s/receipt=0f5593e745abe82e/receipt=0F5593E745ABE82E/'
mp R02_receipt_15         '4s/receipt=0f5593e745abe82e/receipt=0f5593e745abe82/'
mp R03_receipt_17         '4s/receipt=0f5593e745abe82e/receipt=0f5593e745abe82e0/'
mp R04_receipt_flip       '4s/receipt=0f5593e745abe82e/receipt=0f5593e745abe82f/'
{ sed -n '1,3p' base2.t; sed -n '4,5p' base2.t | while IFS= read -r l; do printf '%s\n' "${l% receipt=*}" | "$WFIX" receipt 0123456789abcdef; done; } > "$T/r5"
{ cat "$T/r5"; printf 'court close: verdicts=2 docket=%s\n' "$(sed -n '3,$p' "$T/r5" | "$WFIX" docket)"; } | emit R05_receipt_other_law
mp R06_receipt_empty      '4s/receipt=0f5593e745abe82e/receipt=/'
mp R07_receipt_missing    '4s/ receipt=0f5593e745abe82e//'
mp R08_receipt_trailing   '4s/receipt=0f5593e745abe82e/receipt=0f5593e745abe82e /'
mp R09_receipt_tab        '4s/ receipt=/	receipt=/'
mp R10_receipt_line5_flip '5s/receipt=f8a7761c79498f3c/receipt=f8a7761c79498f3d/'

# ---- X: close (post-reseal edits)
mp X01_verdicts_lead0     '6s/verdicts=2/verdicts=02/'
mp X02_verdicts_plus      '6s/verdicts=2/verdicts=+2/'
mp X03_verdicts_under     '6s/verdicts=2/verdicts=1/'
mp X04_verdicts_over      '6s/verdicts=2/verdicts=3/'
mp X05_docket_upper       '6s/docket=8ce3006a46a6bfe5/docket=8CE3006A46A6BFE5/'
mp X06_docket_flip        '6s/docket=8ce3006a46a6bfe5/docket=8ce3006a46a6bfe6/'
mp X07_docket_15          '6s/docket=8ce3006a46a6bfe5/docket=8ce3006a46a6bfe/'
mp X08_docket_17          '6s/docket=8ce3006a46a6bfe5/docket=8ce3006a46a6bfe50/'
mp X09_close_trailing     '6s/$/ /'
mp X10_close_double_space '6s/court close: /court close:  /'
mp X11_close_extra        '6s/$/ extra=1/'
mp X12_close_missing      '6d'
sed -n '1,4p' base2.t | sed '3s/shores=2/shores=1/' | reseal | emit X13b_shores1_1leaf_ok
sed -n '1,4p' base2.t | reseal | emit X13_close_after_1_of_2
{ reseal < base2.t; sed -n 5p base2.t; } | emit X14_court_after_close
{ reseal < base2.t; sed -n 6p base2.t; } | emit X15_close_twice
{ sed -n '1,3p' base2.t; printf 'court close: verdicts=0 docket=%s\n' "$(sed -n 3p base2.t | "$WFIX" docket)"; } | emit X16_close_no_courts
mp X17_close_cap          '6s/court close:/Court close:/'
mp X18_close_verdicts_neg '6s/verdicts=2/verdicts=-2/'

# ---- S: structure, transport, truncation
: | emit S01_empty
printf 'NETTA ZERO\n' | emit S02_banner_only
sed -n '1,2p' base2.t | emit S03_banner_law
sed -n '1,3p' base2.t | emit S04_no_leaves_no_close
sed -n '2,$p' base2.t | emit S05_no_banner
m0 S06_banner_lower       '1s/NETTA ZERO/netta zero/'
m0 S07_banner_trailing    '1s/$/ /'
m0 S08_banner_leading     '1s/^/ /'
m0 S09_law_v3             '2s/law v2/law v3/'
m0 S10_law_digest_wrong   "2s/law-digest=$LAW/law-digest=f2cc2274cb3c752c/"
m0 S11_law_digest_upper   "2s/law-digest=$LAW/law-digest=F2CC2274CB3C752B/"
{ sed -n 1p base2.t; printf '\n'; sed -n '2,$p' base2.t; } | emit S12_blank_after_banner
{ cat base2.t; printf '\n'; } | emit S13_blank_at_end
cat base1.t base2.t | emit S14_two_sittings
awk '{printf "%s\r\n",$0}' base2.t | emit S15_crlf_all
awk '{printf "%s\r",$0}' base2.t | emit S16_cr_only
awk 'NR==4{printf "%s\r\n",$0;next}{print}' base2.t | emit S17_crlf_one_line
awk 'NR==4{printf "%s\r\r\n",$0;next}{print}' base2.t | emit S18_crcrlf
{ head -c 300 base2.t; printf '\0'; tail -c +301 base2.t; } | emit S19_nul_mid_court
{ sed -n '1,3p' base2.t; printf '\0'; sed -n '4,$p' base2.t; } | emit S20_nul_line_start
awk -v pad="$(printf 'a%.0s' $(seq 1 1500))" 'NR==4{print $0 pad;next}{print}' base2.t | emit S21_overlong_court
{ printf '\xef\xbb\xbf'; cat base2.t; } | emit S22_bom
head -c $(( $(wc -c < base2.t) - 1 )) base2.t | emit S23_close_no_newline
sed -n '1,5p' base2.t | head -c $(( $(sed -n '1,5p' base2.t | wc -c) - 1 )) | emit S24_last_court_no_newline
sed -n '1,3p' base2.t | head -c $(( $(sed -n '1,3p' base2.t | wc -c) - 1 )) | emit S25_sitting_no_newline
printf 'NETTA ZERO' | emit S26_banner_no_newline
{ sed -n '1,2p' base2.t; sed -n 4p base2.t; sed -n 3p base2.t; sed -n '5,6p' base2.t; } | emit S27_court_before_sitting
{ sed -n '1,35p' base32.t; sed -n 35p base32.t | sed 's/^court 31:/court 32:/'; } | reseal | sed '3s/shores=32/shores=33/' | emit S28_shores33_33leaves
sed -n '1,34p' base32.t | reseal | emit S29_shores32_31leaves
sed -n '1,34p' base32.t | sed '3s/shores=32/shores=31/' | reseal | emit S30_shores31_31leaves_ok
{ sed -n '1,2p' base2.t; sed -n '2,$p' base2.t; } | emit S31_law_twice
{ sed -n '1,3p' base2.t; sed -n '3,$p' base2.t; } | emit S32_sitting_twice
m0 S33_banner_tab         '1s/NETTA ZERO/NETTA	ZERO/'
m0 S34_sitting_cap        '3s/court sitting/court Sitting/'
printf '\n' | emit S35_only_newline
{ cat base2.t; printf 'NETTA ZERO\n'; } | emit S36_second_banner_only
awk 'NR==4{printf "%s\n\n",$0;next}{print}' base2.t | emit S37_blank_between_courts
{ sed -n '1,3p' base2.t; sed -n '4,5p' base2.t | while IFS= read -r l; do printf '%s\n' "${l% receipt=*}" | "$WFIX" receipt "$LAW"; done; sed -n 6p base2.t; } | emit S38_reseal_receipts_keep_close

# ---- run
printf 'case\twc_rc\tcite_rc\tagree\ttrace\twc_reason\tcite_reason\n' > "$OUT/results.tsv"
n=0; dis=0; leak=0; acc=0; names=""
for f in "$OUT"/cases/*.t; do
  name=$(basename "$f" .t)
  "$WC" < "$f" > "$T/wo" 2>&1; rw=$?
  cp "$STATE" "$T/s.state"; cp "$BIO" "$T/s.bio"
  "$NETTA" --state "$T/s.state" --bio "$T/s.bio" --cite "$f" > /dev/null 2> "$T/co"; rc=$?
  tr=ok
  if [ $rc -ne 0 ]; then
    cmp -s "$STATE" "$T/s.state" && cmp -s "$BIO" "$T/s.bio" || { tr=TRACE-LEAK; leak=$((leak+1)); }
  fi
  if { [ $rw -eq 0 ] && [ $rc -eq 0 ]; } || { [ $rw -ne 0 ] && [ $rc -ne 0 ]; }; then ag=agree
  else ag=DISAGREE; dis=$((dis+1)); names="$names $name"; fi
  [ $rw -eq 0 ] && [ $rc -eq 0 ] && acc=$((acc+1))
  n=$((n+1))
  printf '%s\t%d\t%d\t%s\t%s\t%s\t%s\n' "$name" "$rw" "$rc" "$ag" "$tr" \
    "$(tail -1 "$T/wo")" "$(tail -1 "$T/co")" >> "$OUT/results.tsv"
done
rm -f "$T"/*; rmdir "$T"
printf 'transcripts=%d both-accept=%d disagree=%d [%s] trace-leaks=%d\n' \
  "$n" "$acc" "$dis" "${names# }" "$leak"
