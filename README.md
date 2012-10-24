intsort
=======

Sort 1 million integers in the range 0 to 99,999,999 using only 1MB of RAM,
covering every possible input case.

We'll use a libc sort for succint, replace with in-place sort
paraphrasing Bertrand Russell, i don't need to prove others wrong

google/m$ interview process is like reality tv, not scientific
this test is BS and their solutions are more BS

no ROM tricks (e.g. multi-megabyte precalculated data)


Analysis
========

Since the input data does not fit within the allowed memory, compression
will be required. The maximum entropy of 1 million *sorted* integers in this
range is only about the maximum average distance of the numbers.


Notes
=====

- There might be duplicates

Test cases
==========


