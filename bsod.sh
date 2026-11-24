#!/usr/bin/env bash
# bsod.sh — demo_buggy-г ажиллуулж, процесс унавал терминал дээр BSOD-ийн ДУУРАЙЛГА харуулна.
# Зөвхөн терминалын өнгө, текст өөрчилнө. Систем, kernel-д ямар ч нөлөөгүй.
BIN=${BIN:-./demo_buggy}
FILE=${1:-channel_file.txt}
[ -x "$BIN" ] || { echo "$BIN олдсонгүй — эхлээд: make buggy"; exit 1; }
LOG=$(mktemp /tmp/asan_log.XXXX)            # ASan-ийн stderr гаралтыг энд хадгална

"$BIN" "$FILE" 2>"$LOG"                      # stdout (rule 1..5) дэлгэцэнд харагдана
status=$?

# Программ амжилттай дууссан бол BSOD харуулахгүй
if [ "$status" -eq 0 ]; then
    echo "(унасангүй — BSOD харуулахгүй)"
    exit 0
fi

# Зөвхөн ASan heap/stack buffer overflow READ илэрсэн үед BSOD харуулна
if ! grep -qE 'ERROR: AddressSanitizer: (heap-buffer-overflow|stack-buffer-overflow)' "$LOG" ||
   ! grep -qE 'READ of size [0-9]+' "$LOG"; then

    echo "BSOD simulation skipped: ASan out-of-bounds READ илрээгүй."
    echo "Програмын exit code: $status"
    echo "ASan log: $LOG"
    exit "$status"
fi
sleep 1.5                                     # үзэгчид "rule 5" хүртэлх мөрийг харах завсар

# ASan log-оос алдааны төрөл, байршлыг авна (жинхэнэ дээр "What failed: csagent.sys" байсан)
KIND=$(grep -m1 -oE 'heap-buffer-overflow|SEGV|stack-buffer-overflow' "$LOG")
WHERE=$(grep -m1 -oE '[a-z_]+\.c:[0-9]+' "$LOG")

restore() { printf '\033[0m\033[2J\033[H'; tput cnorm 2>/dev/null; }
trap restore EXIT
trap 'exit 130' INT

tput civis 2>/dev/null                        # курсорыг нууна
printf '\033[44;97m\033[2J\033[H'             # 44 = цэнхэр дэвсгэр, 97 = цагаан үсэг, 2J = дэлгэц цэвэрлэх

draw() {                                      # $1 = хувь (0..100)
  printf '\033[H\n\n\n'
  printf '      \033[1m:(\033[22m\n\n'
  printf '      Your PC ran into a problem and needs to restart. We'"'"'re just\n'
  printf '      collecting some error info, and then we'"'"'ll restart for you.\n\n'
  printf '      %3d%% complete          \n\n\n' "$1"
  printf '      Stop code: PAGE_FAULT_IN_NONPAGED_AREA\n'
  printf '      What failed: %s  (%s)\n\n\n' "${WHERE:-demo_buggy.c}" "${KIND:-crash}"
  printf '      [ SIMULATION — CrowdStrike Channel File 291 demo, MUIS ]\n'
}

for p in 0 0 10 20 20 35 50 70 85 100; do draw "$p"; sleep 0.6; done

printf '\n      Press any key to return...'
read -r -n1 -s 2>/dev/null </dev/tty || sleep 3   # товч дартал хүлээнэ
restore; trap - EXIT
echo "ASan-ийн бүтэн гаралт: $LOG"
