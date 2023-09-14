# Simple 8-bit machine simulator
Based on the architecture of [Ben Eater's 8-bit computer](https://www.youtube.com/watch?v=HyznrdDSSGM), and come up with my own code word.
*(Pss.. If you have not check [his playlist on 8-bit computer](https://www.youtube.com/playlist?list=PLowKtXNTBypGqImE405J2565dvjafglHU), please check them!! It is really, really, reallyy well-explained!!)*

<p align="center">
    <img src="img/ben-eater-machine" alt="Ben Eater's Project" style="zoom:50%;" />
</p>

## Files

- `parser.cpp`: compile my own home-brew assembly syntax into *(also my own home-brew)* machine code *(not all of them, but some are used)* that could only be understood by `run.cpp`.
- `run.cpp`: run the machine code produced by c, emulating it in an interactive console *(can see the program state)*.

## Compiling

### Windows

Compiling them with **Dev-C++**, in Windows environment, and you're ready to go!
~~(Sorry, not for *linux-heads* :() (is there even such a word?, hmm)~~

### Linux

**Hahaha** I'm using Linux now *(such a different time back then...)*!! 

And just use `g++` for compiling `.cpp` files :) The code requires `libncurses` to be installed.

```bash
g++ parser.cpp -o parser
g++ run.cpp -o run -lncurses
```

## How to use
You can find the files ending with `.su` *(Mist**"su"**u, surpriseee)* in the `example-su-asms` folder. They are my own test assembly babies. You could compile them with the `parser` file. It will drop out a similar file name with `.out` ending, and you use `run` file to run that!

```bash
./parser examples-su-asms/Fibonacci.su
./run examples-su-asms/Fibonacci.out
```

Here is the output image:

<p align="center">
    <img src="img/run-run"/>
    <p align="center">
        <i>Not really user-friendly tho...</i>
    </p>
</p>
