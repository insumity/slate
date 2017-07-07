
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

2. **bodytrack** [results]()
```diff
+ Classifies mem for 10 and 20 threads.
- Classifies 61% (= 27 / 44 in both cases) as mem for 6 threads. Changes frequently between loc and mem.
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |  |     |   |    |        |  
| 10      |    |    |   |   |        |
| 20      |    |  |  |   |      |


['0', '50.355333', '48.469478', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['1', '47.027890', '49.046783', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['2', '48.162098', '48.687334', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['3', '48.327138', '48.282644', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['4', '35.939825', '35.612375', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['5', '34.367594', '36.583325', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['6', '36.891286', '36.220353', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['7', '36.423846', '36.339684', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['8', '23.879448', '21.985925', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['9', '24.747978', '24.722313', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['10', '24.966763', '21.604332', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['11', '25.706182', '22.517180', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['0', '50.192210', '48.581357', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['1', '47.119530', '48.669886', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['2', '48.564732', '48.796770', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['3', '47.819964', '49.050433', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['4', '36.211873', '35.916306', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['5', '35.047321', '35.685998', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['6', '36.984486', '36.038514', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['7', '36.415057', '36.343084', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['8', '23.837390', '23.193992', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['9', '25.027327', '24.391751', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['10', '26.468976', '25.250176', '0.000000', '0.428571', '2.000000', '0.000000', '']
===
['11', '26.138731', '23.164457', '0.000000', '0.428571', '2.000000', '0.000000', '']
kantonia@lpdquad:/localhome/kantonia/slate/experiments$ 


3. **canneal** [results](https://pastebin.com/mSA6J6x1)
```diff
+ Classifies loc for threads 6 and 10.  
- Classifies about 70% as loc for 20 threads.
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       | 35     |  28  | 41.5  |  35.5  |   41 |  
| 10      |  22.28|  18.15| 26.95 |  22.9  |  26.85     |
| 20      |  13.8 |   13.9 |  15.5   | 14.9    | 15.6 | 

Waits 3 seconds before it starts classifying since threads haven't spawned yet. This might explain why although it
classifies correctly for 6 and 10 threads performance is not as good.

4. **facesim** [results](https://pastebin.com/5R0ce20u)

This application only work with a specific number of threads (only 1, 2, 3, 4, 6, 8, 16, 32, 64)
```diff
+ Classifies loc for threads 4. 
- Execution time is 10s slower than the loc placement. Occurs because first second is placed under mem.
- Even if the 3rd second is the only one classifies as mem and we start with loc it takes 10s more!
+ Classifies mem for threads 8 and 16 (except the first second).
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 4       | 40.8   |  29.06 |  30.14 |  40.83  | 32.6 |  
| 8      |  25    |  19.14   |  21.95  | 22.06  |  22.4 |
| 16      |  16.79     | 18.25  |  18.75 | 18.37 |  20.8   |  


5. **ferret** [results](https://pastebin.com/jmBNtSt4)

Generates more threads than the ones provided: for n = 1 ->6, n=2 -> 10, n = 5 -> 22.
```diff
- Classifies 6 and 10 threads as loc 100%.
+ Classifies 22 threads as mem 100%.
```
| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |  96.8 |  73.23|  69.89|  97.07|  70.8 |  
| 10      | 44.05 |  38.58|  35.95 | 55.82  | 36.7    |
| 22      |   20  |  15.5  |   15.5 |  15.5   | 15.5   |  


6. **fluidanimate** [results]()

| threads | loc+  |  loc  |   mem | Slate | Linux  |
| --------|:-----:| -----:| -----:| -----:|-------:|
| 6       |     |       |       |       |        |  
| 10      |       |       |       |       |        |
| 20      |       |       |       |       |        |  


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
