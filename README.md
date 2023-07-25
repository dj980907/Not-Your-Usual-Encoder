# Not-Your-Usual-Encoder

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


## I/O redirection
Sometimes, a user would read the input to a program from a file rather than the keyboard, or send the output of a program to a file rather than the screen. <br/>
This shell program redirects the **standard input** (`STDIN`) and the **standard output** (`STDOUT`). <br/>
For simplicity, tis program does not redirect the **standard error** (`STDERR`).

### Input Redirection
Input redirection is achieved by a `<` symbol followed by a filename. For Example:
```
[nyush]$ cat < input.txt
```
If the file does not exist, this program prints the following error message to `STDERR` and prompt for the next command.<br/>
```
Error: invalid file
```
### Output Redirection
Output redirection is achieved by `>` or `>>` followed by a filename. For example:
```
[nyush]$ ls -l > output.txt
[nyush]$ ls -l >> output.txt
```
If the file does not exist, a new file should be created.<br/>
If the file already exists, redirecting with `>` overwrites the file (after truncating it), whereas redirecting with `>>` appends to the existing file.

### Pipe
The user may invoke n programs chained through (n - 1) pipes. <br/>
Each pipe connects the output of the program immediately before the pipe to the input of the program immediately after the pipe. For Example:
```
[nyush]$ cat shell.c | grep main | less
```

## Built-in Commands
There are four built-in commands in this program: `cd`, `jobs`, `fg`, and `exit`. 

### `cd <dir>`
This command changes the current working directory of the shell. <br/>
It takes exactly one argument: the directory, which may be an absolute or relative path. For Example:
```
[nyush local]$ cd bin
[nyush bin]$ █
```
If `cd` is called with 0 or 2+ arguments, your shell should print the following error message to `STDERR` and prompt for the next command.
```
Error: invalid command
```
If the directory does not exist, your shell should print the following error message to `STDERR` and prompt for the next command.
```
Error: invalid directory
```

### `jobs`
This command prints a list of currently suspended jobs to `STDOUT`, one job per line.<br/>
Each line has the following format: `[index] command`. For example:
```
[nyush]$ jobs
[1] ./hello
[2] /usr/bin/top -c
[3] cat > output.txt
[nyush]$ █
```
(If there are no currently suspended jobs, this command doees not print anything.)

A job is the whole command, including any arguments and I/O redirections.<br/>
A job may be suspended by `Ctrl-Z`, the `SIGTSTP` signal, or the `SIGSTOP` signal.<br/>
This list is sorted by the time each job is suspended (oldest first), and the index starts from 1.

The `jobs` command takes no arguments. <br/>
If it is called with any arguments, this program prints the following error message to `STDERR` and prompt for the next command.
```
Error: invalid command
```

### `fg <index>`
This command resumes a job in the foreground.<br/>
It takes exactly one argument: the job index, which is the number inside the bracket printed by the `jobs` command. For example:
```
[nyush]$ jobs
[1] ./hello
[2] /usr/bin/top -c
[3] cat > output.txt
[nyush]$ fg 2
```
The last command would resume `/usr/bin/top -c` in the foreground. <br/>

If `fg` is called with 0 or 2+ arguments, this program prints the following error message to `STDERR` and prompt for the next command.
```
Error: invalid command
```

If the job `index` does not exist in the list of currently suspended jobs, this program prints the following error message to `STDERR` and prompt for the next command.
```
Error: invalid job
```

### `exit`

This command terminates the shell.<br/>
However, if there are currently suspended jobs, this program will not terminate. <br/>
Instead, it prints the following error message to `STDERR` and prompt for the next command.
```
Error: there are suspended jobs

```
The `exit` command takes no arguments. <br/>
If it is called with any arguments, this program prints the following error message to `STDERR` and prompt for the next command.
```
Error: invalid command
```
If the `STDIN` of this program is closed (e.g., by pressing `Ctrl-D` at the prompt), this program terminates regardless of whether there are suspended jobs!
