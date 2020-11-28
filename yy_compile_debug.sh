#!/bin/bash

gcc -g -o dbg $(find -name "*.c")
gdb ./dbg