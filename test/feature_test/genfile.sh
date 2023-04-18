#!/bin/bash
head -c200 /dev/urandom > s_file
head -c39848 /dev/urandom > m_file1
head -c178929 /dev/urandom > m_file2
head -c8900909 /dev/urandom > m_file3
head -c19290387 /dev/urandom > m_file4
head -c100000000 /dev/urandom > l_file1
head -c435002300 /dev/urandom > l_file2

