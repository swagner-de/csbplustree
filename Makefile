CC      = /usr/bin/g++
CFLAGS  = --std=c++17 -Wall -Wextra -Weffc++ -O3 -march=native

perftree:
	$(CC) $(CFLAGS) -o perf_all_idx_struc.o tests/perf_all_idx_struc.cpp  src/CsvWriter_t.cxx art_src/art.c

perfstack:
	$(CC) $(CFLAGS) -o perf_FatStack.o tests/perf_FatStack.cpp

perfmove:
	$(CC) $(CFLAGS) -o perf_move.o tests/perf_move.cpp

perfsimd:
	$(CC) $(CFLAGS) -o perf_simd.o tests/perf_simd.cxx

perffill:
	$(CC) $(CFLAGS) -o perf_mem_fill.o tests/perf_mem_fill.cpp

allperf: perftree perfstack