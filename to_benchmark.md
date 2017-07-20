

The actual execution times (from the average of 5 runs as well as the standard deviation) can be found
in the "execution_times" file (root directory on github).

Furthermore, we introduce the remaining Metis benchmarks (pca, linear_regression, string_match, kmeans, and hist).
pca and linear_regression (I think) were taken from the newest Metis benchmark suite, while all the others are from the old.
I started using the Metis (from MCTOP) which was the old version, but then when running pca and kmeans I was getting seg. faults.
Only the new version worked. Also, I was getting messed up results with the new metis benchmark suite. E.g., wc was better under "mem" than with "loc" etc.


Metis
-------
1. **wc** [results](https://pastebin.com/6cPfZkkd)

*Used in part of the validation set, so it doesn't count.*

```diff
+ Correctly classified loc 100% for 6 threads.
- Classified 87.5% as loc for 10 threads and and 86% as loc for 20 threads.
```
| threads       | loc+          |  loc |      mem | Slate | Linux |
| ------------- |:-------------:| -----:| -----:| -----:|-----:|
| 6      |  9.57 |  8.36 |  10.6 |  9.85 |  11.8 |
| 10      |  9.15 |  8.32 |  12.8 |  9.63 | 10 |
| 20      |  10.8 | 15.7 |  19 | 16.9 |  12* |

*has high variance.

2. **matrixmult** [results](https://pastebin.com/fB4aVzb6)
```diff
Classsifies as mem 100% for 6, 10, and 20 threads.

```
| threads       | loc+          |  loc |      mem | Slate | Linux |
| ------------- |:-------------:| -----:| -----:| -----:|-----:|
| 6      |  13.98 |  8.15 |  8 |  8.01 |  7.99|
| 10      |  8.74 |  5.02 |  5.04 |  5.04 | 5.02 |
| 20      |  4.63 |  2.85 | 2.99 | 3.05 |  2.80 |

3. **wr** [results](https://pastebin.com/0FQ89u4i  )
```diff
+ Classifies as loc 100% for 6 threads.
+ Classifies 82% as loc for 10 threads.
+ Classifies 86% as loc for 20 threads.
```
| threads       | loc+          |  loc |      mem | Slate | Linux |
| ------------- |:-------------:| -----:| -----:| -----:|-----:|
| 6      |    13.56   | 11.02 |   17 | 13.11 |   14 | 
| 10      |  12.34 | 11.19 | 17.6 | 13.4 | 12.8 |
| 20      |  11.6 |  16.2 | 19.5 |  17.2 | 13 |


4. **wrmem** [results](https://pastebin.com/Ue49qjfZ)
```diff
+ Classified as loc 100% for 6 threads.
+ Classified 16% as loc for 10 threads and 22% as loc for 20 threads.
```
| threads       | loc+          |  loc |      mem | Slate | Linux |
| ------------- |:-------------:| -----:| -----:| -----:|-----:|
| 6      |  29.6 |  22.14  | 22.3 | 22.5 |  24.5 | 
| 10      |  20.3 | 15.7 |  16.7 | 16.6 |  17.5 |
| 20      |  13.8 |  12. 4| 13.1 | 13.8 | 15 |



5. **pca** 
```diff
foo
```
| threads       | loc+          |  loc |      mem | Slate | Linux |
| ------------- |:-------------:| -----:| -----:| -----:|-----:|
| 6      |  x |  x  | x | x | 5  | 
| 10      |  x | x |  x | x| 5  |
| 20      |  x |  x| x | x | 5.4 |
  
  
  
6. **stringmatch** 
```diff
foo
```
| threads       | loc+          |  loc |      mem | Slate | Linux |
| ------------- |:-------------:| -----:| -----:| -----:|-----:|
| 6      |  x |  x  | x | x | 27  | 
| 10      |  x | x |  x | x| X  |
| 20      |  x |  x| x | x | X|

PARSEC
-------

1. **blackscholes** [results](https://pastebin.com/PfZWTKnA)
```diff
+ Correctly classified mem (all instances were 1). 
```

| threads       | loc+          |  loc |      mem | Slate | Linux |
| ------------- |:-------------:| -----:| -----:| -----:|-----:|
| 6      | 5.87 | 5.01 | 4.83 | 4.52   | 4.5 |  
| 10      | 4.47 | 3.88 | 3.85 | 3.84   | 3.83 |   
| 20      |  3.50 | 3.21 |  3.20|  3.17  | 3.01 |  

2. **bodytrack** [results](https://pastebin.com/PTVWbE5R)
```diff
+ Classifies mem for 10 and 20 threads.
- Classifies 61% (= 27 / 44 in both cases) as mem for 6 threads. Changes frequently between loc and mem.
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |  50 |   47  |  48 |  48  |  48   |  
| 10      | 35.9   |  34.36   |  36.89 | 36.42  |  36.4      |
| 20      |  23.87  | 24.74 | 24.966 | 25.7  |   22.5   |


3. **canneal** [results](https://pastebin.com/0tYbJsUg)
```diff
+ Classifies loc for threads 6 and 10.  
- Classifies about 22% as mem for 20 threads.
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       | 35    |   28.4|  41.85|  29.2 | 41.2   |
| 10      | 22.7  | 18.8  |  27.0 | 19.6  |  26.5  |
| 20      |  13.5 | 13.85 | 15.45 | 15.4  |  15.7 |

Waits 3 seconds before it starts classifying since threads haven't spawned yet. This might explain why although it
classifies correctly for 6 and 10 threads performance is not as good.

4. **facesim** [results](https://pastebin.com/Dcb8xKEn)

This application only work with a specific number of threads (only 1, 2, 3, 4, 6, 8, 16, 32, 64)
```diff
+ Classifies 77%  as loc for threads 4. Classifies 94% as mem for threads 8. Classifies 94% as mem for threads 16.
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 4       |  40.5 |  29.08|  30.3 |  30.2 | 32      |
| 8      |  24.9  |   19.12| 22.3 | 22.0  | 22.1  |
| 16      |  16.9 |  18.3 |  18.5 | 18.6 |    21 |


5. **ferret** [results](https://pastebin.com/C0mgMbwf)
Generates more threads than the ones provided: for n = 1 ->6, n=2 -> 10, n = 5 -> 22.
```diff
- Classifies 6 and 10 threads as loc 100%.
+ Classifies 22 threads as mem 100%.
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |  84   |  73   |  70   |  73   |   71.5 |
| 10      | 49.5  |  38.5 |   36 |   38.4 |  36.5  |
| 22      |  19.5 |  16.1 |  15.6 |  15.7 |   15.6 |


6. **fluidanimate** [results](https://pastebin.com/ShcJWji2)
Number of threads must be a power of 2.
```diff
+ 37% are classified as mem for 4 threads.
+ 100% are classified as mem for 8 and 16 threads.
```

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 4      |   55.55  | 38.26   |  35. |   37.5 | 48 |  
| 8      |   32.5  |   22.1 |  21.2  |  21.6  |   29     |
| 16      |  18.7  |  13.4|  13.5 |  13.5   |  17.9    |  


7. **raytrace** [results](https://pastebin.com/Ti6sZh8s)
```diff
+ Classifies everything as mem (100%).
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       | 102.4 |    92   |  91   |  91.1     |  89      |  
| 10      |   87   |   79.7 |    80   |   80.2    |    77   |
| 20      |   74.9   |  69   |   70.5    |  70.5     |   65     |  


8. **streamcluster** [results](https://pastebin.com/MEaAFwLH)

*is part of the validation set ... doesn't count.*
```diff
+For 6 and 10 threads classifies 100% as loc.
+For 20 threads classifies 100% as mem.
```

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       | 29.49|   24.7 |   33  |  26.5 |  32   |  
| 10      |  18.5 | 15.7  |   18.6  | 17.1    |  21.      |
| 20      |  11.2 |    14  |   11    |  10.8     |  14.5      |  


9. **swaptions** [results](https://pastebin.com/HQpS7Bdw)
```diff
+ Classifies everything (100%) as mem.
```

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |  17.6 |  11.68|  10.8 |   10.6 | 10.9       |  
| 10      |  11.48| 7.5   | 7.2  |   7.2    |  7.1    |
| 20      |   6.7| 4.5    | 4.4   |  4.38   |  4.25   |  


10. **vips** [results](https://pastebin.com/i05jMjLr)

```diff
+ Classifies everything as mem.
```

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |    28 |   20    |  19     |  19     |    20.2   |  
| 10      |    19   |  12.8|  12.7  |    12.8 |   13  |
| 20      |   10.5 |  7.5     | 7.4   |  7.3     |  7.6      |




11. **dedup**

Produces way too more threads at the beginning than the ones passed as parameter. The scheduler cannot take this into account.

12. **freqmine**

Doesn't work with pthreads.

13. **x264**

Throws *Error in `/localhome/kantonia/slate/benchmarks/parsec-3.0/bin/../pkgs/apps/x264/inst/amd64-linux.gcc-pthreads/bin/x264': double free or corruption (!prev): 0x0000000000d37020*
