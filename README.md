# Slash

- [Slash](#slash)
  - [Architecture](#architecture)
  - [Authors](#authors)

## Architecture
* [`Makefile`](Makefile)
* [`slash.c`](slash.c)
  * `slash_read_input()`
    * Installation of libreadline-dev
    * Regarding the memory leak for read line, see [this](https://stackoverflow.com/questions/55196451/gnu-readline-enormous-memory-**leak**).
  * `slash_interpret_input()`
  * `slash_exit()`
  * `slash_exec()`

## Authors
See [AUTHORS.md](AUTHORS.md)