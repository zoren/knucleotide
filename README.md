# knucleotide
An implementation of knucleotide(http://benchmarksgame.alioth.debian.org/u64q/knucleotide-description.html#knucleotide) from the benchmark game.

Uses linear probing and a power-of-2 size hash-table. For a small benchmark it is faster than the currently fastest gcc implementation(#7).

# Compile
```
gcc -pthread -pipe -O3 -fomit-frame-pointer -march=native -std=c99 knucleotide.gcc-10.c -o k10
```
