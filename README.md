# Top K URLs from a 100GB file



## Problem Description

Use 1GB of memory to find the top 100 frequent-appeared URLs in a 100GB file. Each line in the file is one URL, lines are unordered, and length of the URLs are not fixed.



## General Approach 

1. Use hash functions to put URLs in the 100 GB **input file** into *f* different **intermideate files** (see below for calculating *f*), to make sure that the same URLs will be in the same file, in the mean time the entire file can be fitted into the memory.
2. (Use multi-core here) For each **intermediate file**, use hash map to calculate the appearance count of each URL. For each core, we have a fixed-size min heap as the **intermediate Top-100 URLs**. After processing each file, we traverse the hash map, and update elements in the min heap, so each heap on each core will host the Top-100 URLs of the files this core has processed.
3. Use multi-merge sort to get the **overall Top-100 URLs** from all the intermediate top 100 URLs.



## 0. Calculate how many files

Step 2 is limited by the memory size, it need to host the intermediate file, the max heap, and the hash map, at the same time.

1. min heap: it always hosts at most 100 elements, so neglectable.
2. hash map: We do not know about the URL count distribution, so it would be hard to estimate how much entries the hash table would have. I would like to assume it takes 15 times the memory of the original dataset. I estimated a high value, because wasting memory seems better than using more memory than we have.

Also, I would like to use multiple cores in this step, therefore each core will handle their own file and have their own hash map.

I plan to have one thread to run on each core (of a total of *N* cores). Then the size of one file would be (1GB / (N * (1+15)), and the number of intermediate files *f* would be 100GB / (1GB / (N * (1+15)). The program will calculate *f* based on how many cores the machine has (not fixed).

For instance, on my computer, *N* = 6:

* Average size of one intermediate file = 1GB / (6 * 16) = 10.67 MB
* Number of intermediate files *f* = 100GB / 10.67MB = 9,600

(one issue here is Linux's default restriction of file descriptors for one application is 1024, so it is best to do`ulimit -u unlimited` before running the experiment.)

According to [Average length of a URL](http://www.supermind.org/blog/740/average-length-of-a-url-part-2), the mean length of a URL is about 77 characters (so 77 bytes in ASCII). So one intermidate file would have around 145,000 URLs. Again, it's hard to do this estimation, if the possible URL distribution is unknown.



## 1. Spliting files

Splitting files using hash function is to gaurantee that the same URL will always be in the same file, so the count of that URL would be correct. Therefore, I used a single thread to split the input file into intermediate files. That proves to be the bottleneck of the whole system.

Therefore one optimization point would be using multiple threads to do this splitting. Then MapReduce-like structures would be very helpful: the shuffling will feed the reducer with all the intermediate files within that range, so the reducer won't miss some of the counting.



## 2. Counting the Intermediate files

Here, the program will create 1 thread for each core, and binded the thread onto cores. Each core runs a `Counter` , which will get a file from the `\tmp` directory, count the URLs using `unordered_map`, and sees if the URLs goes into the per-core min-heap.

The min-heap here is per-core instead of per-file, which would save a lot of memory. If a URL's count is smaller than the top of min-heap, then it would certainly smaller than all the other elements in the heap.



## 3. K-way Merge min-heaps

Here, I first ported the data from min-heaps (`std::priority_queue`)to vectors, because priority_queue does not support random access. I put the elements into the vector in ascending order, so (1) insert the top of min_heap is only append, and (2) when merging, pop the largest is only pop_back. Both of them will not need to move the whole vector.

Anyway, this part is actually O(1), so the time spent here is several microseconds, compared with previous tens (or hundreds) seconds.



## How to run

```bash
ulimit -u unlimited # for opening more than 1024 files
cd topk
make
./generator -i input.txt -M 1024 # will generate a 1G input file; not necessary
./main -k 
# -k: keep /tmp files (if you want to reuse them on the second time)
# -s: on the second time, skip the split, directly use /tmp(because so slow)
```



## Results

My testing environment:

Intel Core i5 8500 (6 cores, 6 threads, 3,00GHz) / 32GB DDR4-2666MHz Memory / Ubuntu 18.04, Kernel 4.15.0-46-generic

Because I don't have 200G space on my SSD, and my server in the lab (which do have 200GB space) runs on NFS, so I have to settle for 1G and 10G tests.

For 1GB:  splitting takes 146 seconds, counting takes 11 seconds, and merging takes 69 **micro**seconds. 

For 10GB: splitting takes 1697 seconds, couting takes 96 seconds, and merging takes 61 **micro**seconds. 

Due to limited time, I didn't do much optimization on splitting, which should be the real bottleneck here.
