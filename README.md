intsort
=======

Sort 1 million integers in the range 0 to 99,999,999 using only 1MB of RAM,
covering every possible input case.

For now, the solutions uses a libc sort, but it can and should eventually be
replaced with another in-place sort function.


Analysis
========

Since the input data does not fit within the allowed memory, compression
will be required. The maximum entropy of 1 million *sorted* integers in this
range is only about the maximum average distance of the numbers.


Notes
=====

- There might be duplicates
- No ROM tricks (e.g. multi-megabyte precalculated data or models)


Test cases
==========

- Random ints in the range
- Series with huge gaps close to the whole range


Opinion
=======

Google/m$ interview process is like reality tv, not scientific
this test is BS and their solutions are more BS


Other "solutions"
=================

Like in Russell's teapot, the programatic burden of proof lies on those
making vague claims. In particular those who look like they don't grasp
the concept of entropy.

