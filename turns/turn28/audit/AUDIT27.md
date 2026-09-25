# Входящий turn27: независимый повтор reader

2026-09-25. Incoming checkout: `/Users/ataeff/arianna/netta-don-turn18-20260921`, commit `48e36938f887ec57b26e4ac20b83f58dd5d149ab`. Чтение исходников и evidence; единственный запуск сохранённого `verify_repair.py`. Incoming дерево не изменялось; новых миров и повторного обучения нет.

## Результат

**Численная проверка PASS; материальный опыт FAIL.** Reader завершился с rc=0 за 79.94 s, проверил 3,145,728 прогнозов; максимальное численное расхождение `1.1948486644541845e-10`. Новый `INCOMING_READER.json` побайтно совпадает с сохранённым `turns/turn27/VERIFY.json`: SHA256 `f8ede0c01f9516ee8e0d585cb02a18cd48ba0170c8df6100acc2ec2ca61bb4ab` (718,393 bytes).

Точная команда, время, прямой exit code и hashes находятся в [INCOMING_READER.receipt.json](INCOMING_READER.receipt.json), stdout/stderr — в [INCOMING_READER.log](INCOMING_READER.log). Запуск использовал `python3.14 -B`, абсолютный новый output в этом audit и `open('x')`; reader предварительно проверен на записи. Других write paths у него нет.

## Freeze и repair

Все **39/39** freeze pins совпали. Проверены manifest entries: DATA 248, EXTRACT 312, MEMORY 64, LABELS 1, RESULTS 128.

| Artifact | SHA256 |
|---|---|
| FREEZE.json | `7faab5b4a2c5db53d797fe219f3af72d6611e06aec8f9a02d6700dff1e83be1d` |
| PROTOCOL.md | `9264908b35d0ce89025d0bce6d9cecc6e25e8cf28c5d761072154ce6e3ca6df1` |
| frozen verify.py | `da7e1ad97d6a235580881d6ec06aa0d498166992dfe9366bea9c232db9f7d435` |
| actual verify_repair.py | `be63f64452e60d11a65abdedf2a34d4c2ed176a281a1b43ff8a650b3fb78d2b8` |
| RESULT.json | `ee72e7890bf4b14707b0ef00c8aafc1fb6431f8bea8d8c9a59660eda1da42d49` |
| original VERIFY.stderr | `96e0cda6807cce35baaffe9216a8eac1f4116f7c1e36fdf4dc4e15339f16bdc4` |

Полный diff reader содержит только disclosure docstring и добавление `c7=dict(hygiene)` в rebuilt details. Frozen reader раньше заканчивался `KeyError: 'c7'`: hygiene уже вычислен и сравнен, но отсутствовал в details, по которым затем шёл общий цикл. Исходные `verify.py`, freeze, `VERIFY.rc` с `VERIFY_RC=1` и stderr сохранены. Prediction law, числа, thresholds и материальный verdict не менялись. Actual repair hash отдельно закреплён нашим receipt; freeze продолжает проверять исходный reader.

## C1–C8 и вред

| Gate | Итог | Основное число |
|---|---|---|
| C1 law tail | FAIL | cusum−fast −2.9402072411 bits; 4/8 миров не хуже −1 |
| C2 law whole | PASS | cusum−fast +6.4392539828 bits |
| C3 moved | FAIL | vs fast +1.2500773398, wins 6/8; vs slow −6.2272087241 при требовании ≥−3 |
| C4 recombined | PASS | early ratio .9994946492; full ratio .9973896923; full positive 8/8 |
| C5 incumbent | PASS | law tail лучше hysteresis на +2.0481222674 bits |
| C6 mechanism | FAIL | 13/24 false latches; настоящих post-seam срабатываний в срок 2/8 |
| C7 hygiene | PASS | все девять subtests |
| C8 reader | PASS | отдельный disclosed repair, воспроизведён |

Law-tail cusum−fast по worlds256…263: `[-5.9343671175, 0, .0005419653, -5.8764338640, -5.8687890006, .1329624931, -5.9755724049, 0]`.

Ложные срабатывания: recombined 5/8, moved_mid 8/8, unrelated 0/8. Все unrelated остаются без admission и с нулевым live gain. Замороженный счётчик switched «успел к deadline» равен 5/8, но три из них произошли ещё до seam. Don правильно отделяет их от двух настоящих post-seam detections; C6 провален в обоих прочтениях.

| World | Recombined latch t | Switched latch t (от seam) | Moved latch t |
|---|---:|---:|---:|
|256|10996|8749 (+557)|10005|
|257|4369|4369 (−3823)|4369|
|258|7578|7578 (−614)|7578|
|259|8949|8294 (+102)|8950|
|260|—|8454 (+262)|9557|
|261|—|8285 (+93)|9283|
|262|—|9252 (+1060)|10276|
|263|2092|2092 (−6100)|2092|

Здесь t — индекс observation, изменившего latch; новая hazard действует со следующего байта. По всему набору минимум prefix gain −.8690790876 (world258/moved_mid/cusum), максимальный drawdown 14.1490547023 (world263/moved_mid/slow). Проверенные bounds не нарушены.

## Две поправки к причинному объяснению

**1. У этого конкретного episode predictor отдельный delta имеет конечный нижний предел.** В [REPORT.md:21](/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn27/REPORT.md:21) он назван неограниченным снизу. Код [episode.c:311](/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn13/episode.c:311) для repeat rank r задаёт `Q_r = R * (v_r+.5)/(V+.5*k)`, где `P0_r <= R`. Matching выбирает один record, его счётчики сериализованы как uint16 ([verify.py:348](/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn13/verify.py:348)), k≤6. Поэтому `delta >= log2(.5/(V+.5*k)) >= -log2(786426) = -19.5849514938 bits`. NEW и fallback дают delta=0.

Это консервативная граница формата, а не оценка типичной ошибки; она значительно слабее 1 bit. Don верно установил, что cumulative mixture floor −1 нельзя применять к отдельному delta. Утверждение о его математической неограниченности всё же сильнее реализации. Из этого конечного опыта также не следует пересечение любого фиксированного threshold при достаточно длинной жизни без дополнительных предпосылок о процессе. Наблюдённые false alarms и material FAIL остаются в силе.

**2. Raw world263 пересекает threshold после двух из трёх процитированных больших ошибок.** Из сохранённых `results/world263/recombined.tsv.gz` и `results/authority/world263/recombined.tsv.gz`:

| t | delta | S до | S после | latch до→после | slow_used |
|---|---:|---:|---:|---|---:|
|2086|−4.1251424317|1.5527245035|5.1778669352|0→0|1|
|2092|−3.9445830910|4.7345101946|8.1790932856|0→1|1|
|2093|+.4617877829|8.1790932856|7.2173055027|1→1|0|
|2098|−4.2081481547|4.5738669030|8.2820150577|1→1|0|

Третья ошибка t2098 происходит уже после latch, поэтому не является причиной первого crossing t2092. Наличие промежуточных matched наблюдений объясняет изменения S между приведёнными строками. Ценообразование t2092 использует старую slow hazard, t2093 — новую fast: причинный порядок соблюдён.

## Точный предел проверки

Reader самостоятельно пересчитывает шесть Decimal probability trajectories, quotes/state/odds, matched clock, CUSUM/latch, chronology, labels, metrics, gates и false-alarm census; проверяет все hashes, точный размер потоков и полный grid 8×4. **P0, candidate и matching/rank поступают из закреплённого tape.** Он не пересобирает source books, learned-unit frontend или candidate distribution из первичного source опыта; normalization здесь подтверждена сохранённым C receipt. Это заявленная граница проверки, которую новый запуск не расширяет.

Численного mismatch между сохранённым RESULT и воспроизведённым reader нет. Разрешённый audit завершён; исходный FAIL сохранён. Успех выбранного CUSUM относительно выбранного hysteresis по C5 не доказывает пригодность всего семейства статистик или невозможность других механизмов.
