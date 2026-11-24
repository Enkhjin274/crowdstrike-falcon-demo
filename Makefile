# Makefile — CrowdStrike Channel File 291 demo
CC     = gcc
CFLAGS = -g -O0 -Wall -fsanitize=address -fno-omit-frame-pointer

.PHONY: all buggy fixed run bsod clean

all: buggy fixed

buggy: demo_buggy
fixed: demo_fixed

demo_buggy: demo_buggy.c
	$(CC) $(CFLAGS) $< -o $@

demo_fixed: demo_fixed.c
	$(CC) $(CFLAGS) $< -o $@

# Хоёуланг нь дараалан ажиллуулна. "-" нь buggy унасан ч make зогсохгүй байх зориулалттай.
run: demo_buggy demo_fixed
	@echo "=============== BUGGY (validator + interpreter, no bounds check) ==============="
	-./demo_buggy channel_file.txt
	@echo
	@echo "=============== FIXED (interpreter bounds check) ==============="
	./demo_fixed channel_file.txt

# Алдаатай хувилбарыг ажиллуулж, унавал терминал дээр BSOD дуурайлга харуулна
bsod: demo_buggy
	@bash ./bsod.sh channel_file.txt

clean:
	rm -f demo_buggy demo_fixed /tmp/asan_log.*
