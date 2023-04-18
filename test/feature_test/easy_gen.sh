#!/bin/bash
head -c409600 /dev/urandom > file_400k
head -c8000000 /dev/urandom >file_8m
head -c102400000 /dev/urandom > file_100m
head -c1024000000 /dev/urandom > file_1g
head -c4096 /dev/urandom > file_4k
#16k
head -c16384 /dev/urandom > file_16k
#64k
head -c65536 /dev/urandom > file_64k