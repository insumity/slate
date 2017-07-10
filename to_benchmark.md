
Metis
-------
1. **wc** (word count) *Used in part of the validation set.* 
+sfafdsdaa -sdaffdsa
2. **matrixmult** 
3. **wr** 
4. **wrmem** 


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


7. **raytrace** 

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       | 5    |       |       |       |        |  
| 10      |       |       |       |       |        |
| 20      |       |       |       |       |        |  


8. **streamcluster**

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       | 5    |       |       |       |        |  
| 10      |       |       |       |       |        |
| 20      |       |       |       |       |        |  


9. **swaptions** 

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       | 5    |       |       |       |        |  
| 10      |       |       |       |       |        |
| 20      |       |       |       |       |        |  


10. **vips**

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |    |       |       |       |        |  
| 10      |       |       |       |       |        |
| 20      |       |       |       |       |        |




11. **dedup**

Produces way too more threads at the beginning than the ones passed as parameter. The scheduler cannot take this into account.

12. **freqmine**

Doesn't work with pthreads.

13. **x264**

Throws *Error in `/localhome/kantonia/slate/benchmarks/parsec-3.0/bin/../pkgs/apps/x264/inst/amd64-linux.gcc-pthreads/bin/x264': double free or corruption (!prev): 0x0000000000d37020*
