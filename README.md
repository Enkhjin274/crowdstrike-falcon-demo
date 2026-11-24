# CrowdStrike Falcon Channel File 291 — Out-of-Bounds Read Demo

МУИС, "Компьютерын системийн зарчим I", нэмэлт даалгавар.

## Demo юуг харуулдаг вэ

2024 оны 7-р сарын 19-нд CrowdStrike Falcon sensor-ийн content interpreter
Channel File 291-ээс дүрэм уншихдаа 21 дэх оролтын талбарт хандсан. Гэтэл
sensor-ийн код зөвхөн 20 талбар дамжуулдаг байсан тул массивын хязгаараас гадуур
уншсан (out-of-bounds read)[1]. Content validator template-ийн 21 талбартай
харьцуулдаг байсан тул энэ зөрүүг барьж чадаагүй[1]. Энэ demo тухайн механизмыг
энгийн user-space C програм дээр давтан харуулна: validator алдаатай файлыг
зөвшөөрч, interpreter 20 элементтэй heap массиваас `fields[20]`-ыг уншина.

**Яагаад 5 сар барьсангүй вэ — wildcard.** Template Type 21 2024 оны 2-р сараас
хэрэглэгдсэн боловч өмнөх бүх дүрэм 21 дэх талбартаа wildcard (`*`, "ямар ч утга
таарна") ашигладаг байсан тул interpreter тэр талбарыг огт уншдаггүй байв. Алдаа
кодонд байсан ч өгөгдөл түүнийг өдөөгөөгүй (латент алдаа). 7-р сарын 19-ний дүрэм
21 дэх талбарт анх удаа тодорхой утга тавьснаар уншилт бодитоор хийгдсэн[1]. Demo-д
дүрэм 6 (`6 20 *`) нь index 20-той ч унахгүй, дүрэм 7 (`7 20 200`) дээр унана.

| Жинхэнэ осол | Demo |
|---|---|
| Template Type: 21 талбар | `TEMPLATE_FIELDS 21` |
| Sensor 20 талбар өгдөг | `malloc(20 * sizeof(int))` |
| Өмнөх дүрмүүд (21 дэх талбар wildcard) | дүрэм 6: `6 20 *` |
| Channel File 291 (тодорхой утга) | дүрэм 7: `7 20 200` |
| Content Validator | `validate()` — 21-тэй харьцуулдаг |
| Content Interpreter | `interpret()` |

## Суулгах, ажиллуулах

```bash
sudo apt install build-essential   # gcc, make (ASan gcc-д хамт ирдэг)
make buggy     # алдаатай хувилбарыг ASan-тай compile хийнэ
make fixed     # зассан хувилбарыг ASan-тай compile хийнэ
make run       # хоёуланг нь дараалан ажиллуулна
make clean
```

**BSOD дуурайлга (илтгэлд):** `make bsod` — алдаатай хувилбарыг ажиллуулж,
дүрэм 1–6-ын гаралтыг харуулаад 1.5 секундын дараа терминалыг цэнхэр болгож
Windows-ийн BSOD-тэй төстэй дэлгэц (`:(`, хувь тоологч, Stop code, "What failed")
харуулна. "What failed" мөрөнд ASan-ийн log-оос авсан `demo_buggy.c:55` гарна.
Товч дарахад терминал хэвэндээ орно. ASan-ийн бүтэн гаралт `/tmp/asan_log.*`-д
хадгалагдана. Энэ нь зөвхөн терминалын өнгө, текст (ANSI escape code) бөгөөд
систем огт унахгүй. Терминалаа fullscreen (F11) болгоод үзүүлбэл илүү бодитой.

Гараар: `gcc -g -fsanitize=address demo_buggy.c -o demo_buggy && ./demo_buggy channel_file.txt`

## Хүлээгдэж буй гаралт

**Алдаатай хувилбар (`demo_buggy`)** — validator зөвшөөрнө, дүрэм 1–6 хэвийн (6 нь wildcard тул уншаагүй), дүрэм 7 дээр унана:

```
[validator] OK: 7 rules accepted (limit = 21 fields)
[interp] rule 1: fields[0]=0 vs 0 -> MATCH
...
[interp] rule 5: fields[19]=190 vs 190 -> MATCH
[interp] rule 6: fields[20] = *  -> MATCH (wildcard, not read)
==356==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x5070000000e0 ...
READ of size 4 at 0x5070000000e0 thread T0
    #0 ... in interpret demo_buggy.c:55
0x5070000000e0 is located 0 bytes after 80-byte region [...]
SUMMARY: AddressSanitizer: heap-buffer-overflow demo_buggy.c:55 in interpret
==356==ABORTING
```

Унших гол мөрүүд: `READ of size 4` (нэг `int`), `0 bytes after 80-byte region`
(20 × 4 = 80 байтын яг араас), `demo_buggy.c:55` (алдаатай мөр).

**Зассан хувилбар (`demo_fixed`)** — алдаатай дүрмийг алгасаад ажиллаад дуусна:

```
[validator] OK: 7 rules accepted (limit = 21 fields)
[interp] rule 1: fields[0]=0 vs 0 -> MATCH
...
[interp] rule 5: fields[19]=190 vs 190 -> MATCH
[interp] WARNING: rule 6 wants fields[20], but only 20 fields exist -> rule skipped
[interp] WARNING: rule 7 wants fields[20], but only 20 fields exist -> rule skipped
[sensor] done — still running
```

Зассан хувилбарт bounds check wildcard шалгалтаас **өмнө** хийгддэг тул буруу
index-тэй дүрэм wildcard байсан ч (дүрэм 6) анхааруулга өгнө: латент алдааг ч илрүүлнэ. CrowdStrike мөн interpreter-д bounds check нэмж, validator-д нэмэлт шалгалт оруулсан[1][2].

**Сонирхолтой нь: ASan-гүй compile хийвэл** (`gcc -g demo_buggy.c`) алдаатай
хувилбар ихэвчлэн унахгүй, `rule 7: fields[20]=540705489` гэх мэт хогийн утга чимээгүй
уншаад дуусна. Энэ бол undefined behavior: алдаа байгаа ч харагдахгүй байж
болно. Тиймээс ASan шиг багаж, bounds check хоёул хэрэгтэй.

## Аюулгүй байдал
- Demo зөвхөн өөрийн компьютер эсвэл VM дээр ажиллана.
- Сүлжээ, socket, system()/exec, driver, kernel код огт байхгүй.
- Алдаа нь зөвхөн унших (out-of-bounds read) төрлийн: програм өөрийн heap-ийн хилийн 
яг дараах 4 байтыг уншина. Санах ойд бичихгүй, хяналтын урсгалыг (control flow) 
өөрчлөхгүй, exploit биш. (CrowdStrike-ийн өөрийн шинжилгээ ба гуравдагч талын хяналтаар 
ч жинхэнэ алдаа нь халдагчид ашиглах боломжгүй гэж дүгнэсэн[2].)
- channel_file.txt бол өөрсдийн зохиосон дуурайлга. Жинхэнэ C-00000291-*.sys файл, csagent.sys driver ашиглаагүй.
- BSOD дэлгэц зөвхөн терминалын өнгө/текст бөгөөд "SIMULATION" гэж тодорхой бичигдсэн.
- Бусдын компьютер, бодит систем, сүлжээний эсрэг ажиллуулж болохгүй.

## Жинхэнэ осол ба demo-гийн ялгаа

- **Kernel vs user space:** Falcon sensor `csagent.sys` kernel driver байсан.
  Kernel дотор буруу санах ойд хандахад Windows бүхэлдээ унадаг (BSOD), дахин
  асахад driver дахин ачаалагдаж унадаг давталт үүссэн. Энд зөвхөн нэг процесс
  зогсоно, үйлдлийн систем хэвийн ажиллана.
- **Хэн барьдаг вэ:** Kernel-д ASan байхгүй. CPU page fault өгч, Windows
  `PAGE_FAULT_IN_NONPAGED_AREA` алдаагаар зогссон. Энд ASan heap-ийн "redzone"
  ашиглан алдааг тодорхой мөрөөр нь мэдээлнэ.
- **Уншсан утгыг юунд ашигласан:** Жинхэнэ ослын үед crash dump-ийн шинжилгээгээр
  массивын гаднаас уншсан утгыг pointer мэт ашиглаж, хүчингүй хаяг руу хандсан
  нь харагдсан. Энд зөвхөн `int` уншиж харьцуулна.
- **Хэмжээ:** Ойролцоогоор 8.5 сая Windows машин нөлөөлөлд өртсөн[3]. Энэ demo нь
  ямар ч систем, driver, kernel-д нөлөөлөхгүй.
- **Хуваарилалт:** Жинхэнэ channel file автоматаар бүх клиент рүү нэг дор
  түгээгдсэн (staged rollout байгаагүй). Энд гараар ажиллуулна.

## Файлууд

- `demo_buggy.c` — алдаатай validator + bounds check-гүй interpreter
- `demo_fixed.c` — interpreter массивын уртыг авч, унших бүрийн өмнө шалгана
- `channel_file.txt` — 5 хэвийн дүрэм + index 20 дээр wildcard дүрэм + index 20-ыг бодитоор унших 1 дүрэм
- `bsod.sh` — `demo_buggy` унавал терминал дээр BSOD-ийн дуурайлга харуулдаг wrapper
- `Makefile` — `make buggy`, `make fixed`, `make run`, `make bsod`, `make clean`

## Эх сурвалж

-  CrowdStrike, *External Technical Root Cause Analysis — Channel File 291*, 2024-08-06.
   https://www.crowdstrike.com/wp-content/uploads/2024/08/Channel-File-291-Incident-Root-Cause-Analysis-08.06.2024.pdf
-  CrowdStrike, *Executive Summary: Root Cause Analysis — Channel File 291*, 2024-08-06.
   https://crowdstrike.com/wp-content/uploads/2024/08/Executive-Summary_Root-Cause-Analysis_Channel-File-291.pdf
-  TechCrunch, *Microsoft says 8.5M Windows devices were affected by CrowdStrike outage*, 2024-07-20
   (Microsoft-ийн David Weston-ын blog-ийг иш татсан).
   https://techcrunch.com/2024/07/20/microsoft-says-8.5m-windows-devices-were-affected-by-crowdstrike-outage/