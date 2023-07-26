# Not-Your-Usual-Encoder

This program is from CS202 of Prof. Yang Tang @ NYU. <br />
This is a program that demonstrates my familiarity of multithreaded programming using POSIX threads using C programming language.<br>
This program implements a thread pool using mutexes and condition variables to parallelize a encoding process. <br/>

## Run-length encoding
Run-length encoding (RLE) is when you encounter n characters of the same type in a row, the encoder will turn that into a single instance of the character followed by the count n.

For example, a file with the following content:
```
aaaaaabbbbbbbbba
```
would be encoded (logically, as the numbers would be in binary format instead of ASCII) as:
```
a6b9a1
```

Note that the exact format of the encoded file is important. The encoder stores the character in ASCII and the count as a 1-byte unsigned integer in binary format. 

In this example, the original file is 16 bytes, and the encoded file is 6 bytes.

For simplicity, this program assumes that no character will appear more than 255 times in a row. In other words, it safely stores the count in one byte.

## Sequential RLE
The first version if this encoder is a single-threaded program. The encoder reads from one or more files specified as command-line arguments and writes to STDOUT. Thus, the typical usage of this encoder would use shell redirection to write the encoded output to a file.

For example, let’s encode the aforementioned file.

```
$ echo -n "aaaaaabbbbbbbbba" > file.txt
$ xxd file.txt
0000000: 6161 6161 6161 6262 6262 6262 6262 6261  aaaaaabbbbbbbbba
$ ./nyuenc file.txt > file.enc
$ xxd file.enc
0000000: 6106 6209 6101                           a.b.a.
```

If multiple files are passed to the encoder, they will be concatenated and encoded into a single compressed output. For example:

```
$ echo -n "aaaaaabbbbbbbbba" > file.txt
$ xxd file.txt
0000000: 6161 6161 6161 6262 6262 6262 6262 6261  aaaaaabbbbbbbbba
$ ./nyuenc file.txt file.txt > file2.enc
$ xxd file2.enc
0000000: 6106 6209 6107 6209 6101                 a.b.a.b.a.
```

Note that the last a in the first file and the leading a’s in the second file are merged!

## Parallel RLE
Next, i implemented a parallelized version of the encoder using POSIX threads. In particular, this program implements a thread pool for executing encoding tasks.

This program uses mutexes, condition variables to realize proper synchronization among threads. 

This program takes an optional command-line option -j jobs, which specifies the number of worker threads. (If no such option is provided, it runs sequentially (version 1))

For example:
```
$ time ./nyuenc file.txt > /dev/null
real    0m0.527s
user    0m0.475s
sys     0m0.233s
$ time ./nyuenc -j 3 file.txt > /dev/null
real    0m0.191s
user    0m0.443s
sys     0m0.179s
```

At the beginning of the program, I created a pool of worker threads. The number of threads is specified by the command-line argument -j jobs.

The main thread divides the input data logically into fixed-size 4KB (i.e., 4,096-byte) chunks and submit the tasks to the task queue, where each task would encode a chunk. Whenever a worker thread becomes available, it executes the next task in the task queue.

For simplicity, I assumed that the task queue is unbounded.

After submitting all tasks, the main thread collects the results and write them to `STDOUT`. For example, if the previous chunk ends with `aaaaa`, and the next chunk starts with `aaa`, instead of writing `a5a3`, the program writes `a8`.

It is important to note that the program synchronizes the threads so that there are no deadlocks or race conditions. 
Two important things to note:

- The worker thread waits until there is a task to do.
- The main thread waits until a task has been completed so that it can collect the result.

## Testing
I have example files (file.txt, file1.txt, file2.txt, file3.txt) that can be tested. Feel free to try it!
