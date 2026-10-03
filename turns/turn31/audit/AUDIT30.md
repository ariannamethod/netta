# Incoming turn30: численный аудит и предел вывода

2026-09-26. Входящий commit `bbd8dbb41f88d5a6697ca03533176bdbbfa6ee49`, checkout `/Users/ataeff/arianna/netta-don-turn18-20260921`, `turns/turn30`. Рабочая область этого аудита — только own `turns/turn31/audit`. Исходники, данные, архивы и sealed результаты Don не изменялись; новых миров, обучения и perturbation probes не было.

## Повтор и идентичность

Перед запуском прочитаны reader и его imports/write paths. Единственная запись reader — `--output` через `open('x')`; frozen helper turn28 не выполняет main при импорте. Запуск выполнен с `-B`, stdout/stderr сохранены прямо в log, exit code получен от `subprocess.run`, без pipe. Точная команда, время и результат находятся в [INCOMING_READER.receipt.json](INCOMING_READER.receipt.json), численный receipt — [INCOMING_READER.json](INCOMING_READER.json), log — [INCOMING_READER.log](INCOMING_READER.log). Разрешён один запуск.

**Завершение: rc=0 за 138.36 s; новый receipt побайтно идентичен sealed VERIFY.json.** Проверены 3,276,800 прогнозов и 2,688 source counters; maximum numeric error `3.392131020518718e-11`. `verification_pass=true`, `material_pass=false`, `gate_pass=false`; D6=true. Численного mismatch нет. Reader подтвердил manifest entries DATA280, EXTRACT344, MEMORY56, RESULTS160. Incoming git status после запуска остаётся clean.

Независимо проверены **28/28 freeze pins**, все совпадают. Основные hashes:

| Artifact | SHA256 |
|---|---|
| PROTOCOL.md | `fab5c1dc717c4ecbe76fab06d934852f999f187758b77a31a7e5f068edc2a47c` |
| FREEZE.json | `70df3daf990b4960a3cc8f99d07586e691711ccd71d8f461db69aa3fdf3afdda` |
| verify.py | `3d345a3ac2a44a47ae039aeb4a2a280b5c5650a93345f6a6947b5cb06a276734` |
| RESULT.json | `e8535c84ec71be9cfc09c166e7713d5c08934396473a609a96f128a47cc7db57` |
| sealed VERIFY.json | `6dd86effd6e0894440079831b6a240a575c847dc56ab33d248cde68a0d6eefe8` |

## Замороженный gate

**Материальный FAIL: D1 и D3.** Он сохраняется. D2 информационный и не входит в итоговый AND. D6 присваивается независимым reader; writer закономерно оставляет его false и pending=true.

| Условие | Итог | Сохранённое число |
|---|---|---|
| D1 partial early4096 | FAIL | bank3−pooled2 −31.4641046677 bits, wins 0/8 |
| D2 цена различения | выполнено | bank3 528 B, pooled2/permuted2 336 B, extra192 B |
| D3 recombined retention | FAIL | early .9361542374; whole .9436999821 при пороге .95 |
| D4 pooled null | PASS | ни в одном regime mean whole gain null не выше pooled2; max excess −100.2282335512 |
| D5 hygiene | PASS | общая admission четырёх gated arms; bounds и exactness соблюдены |
| D6 reader | закрывается отдельным receipt | writer не присваивает себе reader PASS |

Минимум prefix gain: −.9997905994, world287/moved_mid/bank3. Максимальный drawdown: 14.2468187440, world285/switched/pooled2. Max normalization error `1.3100631690576847e-14`.

D1 измеряет байты до смены закона: `early=4096`, seam=8192. Это правильно раскрыто Don в [REPORT.md:20](/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn30/REPORT.md:20). Дополнительно к reader самостоятельно сравнены декомпрессированные строки t0…8191: **все 24 пары** recombined↔partial/switched/moved_mid по восьми мирам побайтно одинаковы. Сам флаг reader `shared_prefix_identity` сравнивает early gains, а прямое сравнение здесь подтверждает и более сильную формулировку про весь trace prefix. После просмотра результата окно не переносилось.

## Полезные различия остались в данных

Все числа ниже — bank3−pooled2 в bits на одном мире; средние по восьми. Tail начинается с t8192. Они описывают существующий опыт и не заменяют D1.

| Regime | Early mean | Whole mean | Whole wins | Tail mean | Tail wins |
|---|---:|---:|---:|---:|---:|
| recombined | −31.464105 | −126.495615 | 1/8 | −76.255002 | 1/8 |
| partial | −31.464105 | −38.952428 | 3/8 | +11.288185 | 4/8 |
| switched | −31.464105 | +77.409275 | 6/8 | +127.649888 | 7/8 |
| moved_mid | −31.464105 | −101.152307 | 2/8 | −50.911694 | 2/8 |
| unrelated | +23.181445 | +94.727962 | 4/8 | +45.483870 | 4/8 |

Partial tail по worlds280…287: `[+3.213496, −53.673571, −50.838567, −21.593444, +106.423828, −22.396424, +7.269949, +121.900215]`. Поэтому его положительное среднее не является равномерным преимуществом.

В unrelated gated arms допущены в четырёх мирах: 282 (t1913), 283 (t272), 284 (t558), 287 (t918). Bank3 выигрывает у pooled2 во всех четырёх; остальные четыре дают ties0, а не отрицательные случаи. У full24 дополнительно есть admission world281/t2270, всего пять admitted unrelated lives в объединении arm sets. Поэтому фраза отчёта «five unrelated-life admissions» относится к объединению с yardstick, не к bank3. У permuted2 на этих четырёх мирах whole gain соответственно −.989323, −.970776, −.586627, −.972025; null admission навязан общим clock, а не заработан null stream.

96/96 bank records действительно имеют разные A/B count vectors. Дополнительная проверка нормированных KT repeat laws при k=6 также дала различие в 96/96, TV от .0012139056 до .9441722435. Это подтверждает наличие различий в переносимом содержимом; их уместность в очередном recipient определяется дальнейшим наблюдением.

## Два сырых экстремума и причинная интерпретация

Примеры извлечены непосредственно из gzip TSV; совпадают с RESULT.raw_help/raw_harm.

### Help: world282/moved_mid, t14914, record1, truth235/rank4

- До truth веса bank3 `(P0,A,B)=(.9005453257,.0537330542,.0457216202)`; pooled2 memory weight `.9834948381`.
- Log prices: P0 −6.0975492625, A −14.0607694795, B −14.4210163780, pooled source −15.2517451494.
- Paid live: bank3 −6.2471947583, pooled2 −11.7520801783; разница **+5.5048854200 bits**.
- После truth P0 weight становится `.9987484701`. Этот последующий вес не причиняет уже уплаченную цену. До события bank3 уже ставил 90.05% на P0, а не 99.87%.
- Raw окно: `098bebeb8bebebd7b7d7ebe8b7ebc9c9ebc9c9c978c9eb25c9eb7676ebd376afaf`.
- Trace SHA256: `d223da32a9fe95c38062b714f723a79aad44fb2116c5df7bda9b0bbb5e18afef`.

Этот конкретный выигрыш объясняется главным образом уже произошедшим отходом к P0; выбор B вместо A его не объясняет.

### Harm: world285/switched, t9092, record5, truth207/rank2

- До truth `(P0,A,B)=(.0001937931,.9993350010,.0004712058)`.
- Log prices: P0 −1.4794428756, A −6.1248204776, B −1.3284410509, pooled source −1.4761776950.
- Paid live: bank3 −6.0929878000, pooled2 −1.4793356886; разница **−4.6136521114 bits**.
- Raw окно: `2efd2e0404fd08babafd08bababacf08cfcf0f0f0fb0b08cb00f03030f03b00303`.
- Trace SHA256: `1a5a8ccd68a12989298aed22e985e34598bd3df165d29297d75a9dd2ebcbde66`.

На этом событии память ошибочно почти целиком выбрала A, хотя B лучше прогнозировал наблюдение. Эти два выбранных экстремума устанавливают механизмы именно этих событий; они не разлагают агрегат switched/unrelated на discrimination и retreat. Сам Don дальше верно называет это открытым следующим вопросом, хотя заголовок [REPORT.md:47](/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn30/REPORT.md:47) уже утверждает «mechanism is not discrimination».

## Что значит «нет residual value» здесь

Формулировка «не выполнил frozen gate» поддержана точно. D1 измерил общий префикс, D3 также провален; условие протокольного branch-closing рассуждения «D1 fails while D3…D6 pass» фактически не наступило. Положительные whole/tail контрасты в switched и unrelated сохранены в том же опыте. Поэтому итог не исключает ценность различения как семейства конструкций или в post-seam partial.

Кроме того, стартовый общий вес памяти одинаков: `.4375+.4375=.875`. Пусть A и B предсказывают одну и ту же S, а `m=wA+wB`. Тогда трёхчастный forecast равен `(1−m)P0+mS`, posterior sum равна `mS/Q`, а после share `m_next=(1−rho)*mS/Q+rho*.875`: **это ровно бинарный закон** с тем же total prior. Следовательно, третье имя состояния само по себе не создаёт неизбежного налога при совпадающих распределениях. В актуальных архивах законы различаются; здесь меняются predictive shape, сила KT smoothing и история веса, и это нельзя свести к одному числу «.4375 вместо .875».

Предложенный pooled-half с total prior .4375 изменил бы также общий prior памяти, сохранив один pooled law. Это информативный отдельный контроль, но его совпадение с bank3 на выбранных показателях само по себе не докажет, что «distinctness fully dead», а превосходство bank3 не установит единственную причину discrimination без дальнейшей атрибуции. Нового опыта в этом audit не запускалось.

## Предел reader

Reader самостоятельно пересчитывает source continuations, точные bank/pooled/permuted bytes на двух покрытиях, три- и двухчастные router trajectories, общий incumbent-driven admission, cross-trace chronology, пять outer trajectories, все horizons и gate. Он наследует grammar selection и HEAD256 P0/bindings как закреплённые inputs; full frontend заново не строится. Full24 имеет собственный admission и только descriptive статус.

Проверка воспроизводит материальный FAIL, сохраняя различие между ним и численной корректностью. Обобщающие формулировки выше уточнены по исходным формулам и raw событиям; incoming report и evidence не переписаны.
