
# minialign

Minialign is a little bit fast and moderately accurate nucleotide sequence alignment tool designed for PacBio and Nanopore long reads. It is built on three key algorithms, minimizer-based index of the [minimap](https://github.com/lh3/minimap) overlapper, array-based seed chaining, and SIMD-parallel Smith-Waterman-Gotoh extension.

## Getting started

C99 compiler (gcc / clang / icc) is required to build the program.

```
$ make && make install	# PREFIX=/usr/local by default
$ minialign <reference.fa> <reads.fq> > out.sam		# read-to-ref alignment
```

Reference sequence index can be stored in separate. Using prebuilt index saves around a minute per run for a human haploid (~3G) genome.

```
$ minialign -d index.mai <reference.fa>	# build index
$ minialign -l index.mai <reads.fq> > out.sam	# mapping on prebuilt index
```

Frequently used options are: scoring parameters, minimum score cut-offs, and number of threads.

```
$ minialign -a1 -b2 -p2 -q1		# match, mismatch, gap-open and gap-extend
$ minialign -s1000	# set minimum score threshold to 1000
$ minialign -m0.8	# set report threshold at 0.8 of the highest score for every read
$ minialign -t10	# minialign is now 10x faster!!!
```

## Benchmarks

All the benchmarks were took on Intel i5-6260U (Skylake, 2C4T, 2.8GHz, 4MBL3) with 32GB (DDR4, 2133MHz) RAM.

### Speed

|                      Time (sec.)                     |  minialign  |   DALIGNER  |   BWA-MEM   |
|:----------------------------------------------------:|:-----------:|:-----------:|:-----------:|
| E.coli (MG1655) x100 simulated read (460Mb) to ref.  |        16.6 |        39.5 |        6272 |
| S.cerevisiae (sacCer3) x100 sim. (1.2Gb) to ref.     |        45.7 |       1134* |       10869 |
| D.melanogaster (dm6) x20 sim. (2.75Gb) to ref.       |         140 |           - |       31924 |
| Human (hg38) x3 sim. (9.2Gb) to ref.                 |        1074 |           - |           - |

Notes: Execution time was measured with the unix `time` command, shown in seconds. Dashes denote untested conditions. Program version information: minialign-0.4.0, DALIGNER-ca167d3 (commit on 2016/9/27), and BWA-MEM-0.7.15-r1142-dirty. All the programs were compiled with gcc/g++-5.4.0 providing the optimization flag `-O3`. PBSIM (PacBio long-read simulator), [modified version based on 1.0.3](https://github.com/ocxtal/pbsim/tree/nfree) not to generate reads containing N's, was used to generate read sets. Parameters (len-mean, len-SD, acc-mean, acc-SD) were fixed at (20k, 2k, 0.88, 0.07) in all the samples. Minialign and DALIGNER were run with default parameters except for the thread count flags, `-t4` and `-T4` respectively. BWA-MEM was run with `-t4 -A1 -B2 -O2 -E1 -L0`, where scoring (mismatch and gap-open) parameters were equalized to the defaults of the minialign. Index construction (minialign and BWA-MEM) and format conversion time (DALIGNER: fasta -> DB, las -> sam) are excluded from the measurements. Peak RAM usage of minialign was around 12GB in human read-to-ref mapping with four threads. Starred sample, S.cerevisiae on DALIGNER, was splitted into five subsamples since the whole concatenated fastq resulted in an out-of-memory error. Calculation time of the subsamples were 61.6, 914.2, 56.8, 50.3, and 51.5 seconds, where the second trial behaved a bit strangely with too long calculation on one (out of four) threads.

### Sensitivity-specificity trend (ROC curve)

![ROC curve (D.melanogaster)](https://github.com/ocxtal/minialign/blob/master/fig/roc.dm6.png)

(a) D.melanogaster (dm6) x10 (1.4Gb)

![ROC curve (Human)](https://github.com/ocxtal/minialign/blob/master/fig/roc.hg38.png)

(b) Human (hg38) x0.3 (1Gb)

Notes: Sensitivity is defined as: the number of reads whose originating locations are correctly identified (**including secondary mappings**) / the total number of reads. Specificity is: the number of correct alignments / the total number of alignments. The sensitivity and specificity pairs were calculated from the output sam files, filtered with different mapping quality thresholds between 0 and 60. Program version information: minialign-0.4.0, GraphMap-v0.3.1 with [a patch](https://github.com/ocxtal/graphmap/tree/edlib_segv_patch), BLASR-0014a57 (the last commit with the SAM output), and BWA-MEM-0.7.15-r1142-dirty. Read set was generated by PBSIM with the parameters (len-mean, len-SD, acc-mean, acc-SD) set to (10k, 10k, 0.8, 0.2) without ALT / random contigs. Reads were mapped onto the corresponding reference genomes including ALT / random contigs with the default parameters for the BLASR and GraphMap, `-a1 -b1 -p1 -q1` for the minialign, and `-xpacbio` for the BWA-MEM, and the additional four-thread multithreading directions. Calculation time and peak RAM usages are shown in the table below.

|     Time (sec.) / Peak RAM (GB)       |  minialign  |   GraphMap  |    BLASR    |   BWA-MEM   |
|:-------------------------------------:|:-----------:|:-----------:|:-----------:|:-----------:|
| D.melanogaster (dm6) x10 sim. (1.4Gb) |  68.8 / 2.2 |  6460 / 4.3 | 30081 / 1.0 | 37292 / 0.5 |
| Human (hg38) x0.3 sim. (1Gb)          |    119 / 12 |           - |           - | 34529 / 5.5 |

### Effect of read length and identity on recall

Since the minimizer-based seeding and the rough chaining algorithm of the software affects on its recall ratio (sensitivity) when the query sequences are short and errorneous. The experiment, estimating recall ratio on the PacBio simulated read sets, showed that the software tends to fail recovering the correct locations at a relatively high rate when the query length is shorter than 1000 bases and the identity is less than 0.75.

|  Recall |     500 |    1000 |    2000 |    5000 |   10000 |   20000 |   50000 |  100000 |
|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|:-------:|
|    0.65 |   0.070 |   0.195 |   0.471 |   0.871 |   0.965 |   0.980 |   0.985 |   0.986 |
|    0.70 |   0.191 |   0.447 |   0.776 |   0.963 |   0.979 |   0.983 |   0.987 |   0.989 |
|    0.75 |   0.433 |   0.754 |   0.939 |   0.977 |   0.984 |   0.987 |   0.991 |   0.993 |
|    0.80 |   0.732 |   0.930 |   0.973 |   0.983 |   0.988 |   0.993 |   0.997 |   0.997 |
|    0.85 |   0.913 |   0.972 |   0.981 |   0.988 |   0.994 |   0.997 |   0.999 |   0.999 |
|    0.90 |   0.963 |   0.983 |   0.988 |   0.994 |   0.998 |   0.999 |   1.000 |   0.999 |
|    0.95 |   0.978 |   0.991 |   0.994 |   0.998 |   0.999 |   0.999 |   1.000 |   1.000 |

Notes: Row: identity, column: length in bases. Sensitivity (recall) is defined as above. Reads are generated from hg38 without ALT / random contigs using PBSIM with the (len-SD, acc-SD) pairs set to (len-mean / 10, 0.01). Reads were mapped onto the reference with ALT / random contigs included. Minialign was run with the same parameters as in the speed benchmark except for the score filter ratio `-r` set at 0.5.

## Algorithm overview

### Minimizer-based index structure

Indexing routines: minimizer calculation, hash table construction, and hash table retrieval are roughly diverted from the original minimap program except that the position of sequence direction flag in hash table is moved from the least significant bit to the int32_t sign bit. See descriptions in the minimap [repository](https://github.com/lh3/minimap) and [paper](http://bioinformatics.oxfordjournals.org/content/32/14/2103) for the details of the invertible hash function used to generate minimizers.

### Seed chaining

Collected seeds (minimizers) were first sorted by its (rpos - 2*qpos) value, resulting in lining up along with 15-degree leaned lines from the diagonal. Then (rpos - qpos/2) values are evaluated from head to tail on the sorted elements, and chained when the current seed is inside a 30-degree-angled parallelogram window at the right-bottom direction of the previous one. Chaining is iteratively performed on the remaining seed array and terminated when no seed is left in it. Finally, collected chains are sorted by their rough path lengths: (re - rs + qe - qs).

### Smith-Waterman-Gotoh extension

The second head seed of each chain is extended upward (3' on the reference side) then downward from the maximum score position found. If the resulted path is shorter than the seed chain span, similar extension is repeatedly performed on the next one (three times at the maximum). Each extension is carried out by the [GABA library](https://github.com/ocxtal/libgaba), which implements the adaptive-banded semi-global Smith-Waterman-Gotoh algorithm with difference recurrences.

## Notes, issues and limitations

* k-mer length (`k`) and minimizer window size (`w`) cannot be changed when the index is loaded from file. If you frequently adjust the two parameters, please prepare indices for each value or use the on-the-fly index construction mode.
* The default score filter threshold `-s200` tends to drop reads shorter than 1k bases. To collect these short alignments, please set lower `-s` values such as `-s50`. (The `-s50` setting is especially effective on typical PacBio read sets.)
* Large gap open penalty (> 5) and large X-drop penalty (> 64) are disallowed due to the limitation of the GABA library.
* SDUST masking is removed from the original minimap implementation.
* Repetitive seed-hit region detection is also removed.
* Index file format is incompatible with of the minimap.

## Updates

* 2016/12/5 (0.4.2) Add splitted alignment rescuing algorithm.
* 2016/12/1 (0.4.1) Fix bug in sam output (broken CIGAR with both reverse-complemented and secondary flags).
* 2016/11/27 (0.4.0) Added mapping quality output, fix bug in chaining, and change output threshold measure from length to score (note: `-s` flag is changed to minimum score, `-r` is interpreted as score ratio).
* 2016/11/24 (0.3.3) Fix bugs in index load / dump functions.
* 2016/11/24 (0.3.2) Fix bugs in the chaining routine, make minimum score threshold option deprecated.
* 2016/11/1 (0.3.1) Changed minimum path length threshold option from '-M' to '-s'.
* 2016/11/1 (0.3.0) First release of 0.3 series, with a better chaining algorithm.
* 2016/10/5 (0.2.1) Last tagged commit of the version 0.2 series.
* 2016/9/13 (0.2.0) First tagged commit (unstable).

## Gallery

#### *Fast and Accurate* logo

![metcha hayaiyo](https://github.com/ocxtal/minialign/blob/master/pic/hayai.png)

#### Intel nuc, my main development machine

![he is also powerful](https://github.com/ocxtal/minialign/blob/master/pic/nuc.png)

## Copyright and license

The original source codes of the minimap program were developed by Heng Li and licensed under MIT, modified by Hajime Suzuki. The other codes, libgaba and ptask, were added by Hajime Suzuki. The whole repository except for the pictures in the gallery section (contents of pic directory) is licensed under MIT, following that of the original repository.
