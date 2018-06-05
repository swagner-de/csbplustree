CC      = /usr/bin/g++
CFLAGS  = --std=c++17 -Wall -Wextra -Weffc++ -O3 -march=native

perftree:
	$(CC) $(CFLAGS) -o perf_all_idx_struc.o tests/perf_all_idx_struc.cpp  src/CsvWriter_t.cxx art_src/art.c

perfstack:
	$(CC) $(CFLAGS) -o perf_FatStack.o tests/perf_FatStack.cpp


allperf: perftree perfstack