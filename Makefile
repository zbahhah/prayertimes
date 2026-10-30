CC ?= cc
CXX ?= c++
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra -Werror
LDLIBS = -lm

TESTS = out/test_sun out/test_rules out/test_jpl out/test_muis out/test_cxx

all: $(TESTS) out/prayertimes_cli

out:
	mkdir -p out

out/test_sun: tests/test_sun.c prayertimes.h | out
	$(CC) $(CFLAGS) -I. $< -o $@ $(LDLIBS)

out/test_rules: tests/test_rules.c prayertimes.h | out
	$(CC) $(CFLAGS) -I. $< -o $@ $(LDLIBS)

out/test_jpl: tests/test_jpl.c prayertimes.h | out
	$(CC) $(CFLAGS) -I. $< -o $@ $(LDLIBS)

out/test_muis: tests/test_muis.c prayertimes.h | out
	$(CC) $(CFLAGS) -I. $< -o $@ $(LDLIBS)

out/test_cxx: tests/test_cxx.cpp prayertimes.h | out
	$(CXX) $(CXXFLAGS) -I. $< -o $@ $(LDLIBS)

out/prayertimes_cli: examples/prayertimes_cli.c prayertimes.h | out
	$(CC) $(CFLAGS) -I. $< -o $@ $(LDLIBS)

test: all
	out/test_sun tests/data
	out/test_rules tests/data
	out/test_jpl tests/data
	out/test_muis tests/data
	out/test_cxx tests/data

clean:
	rm -rf out
