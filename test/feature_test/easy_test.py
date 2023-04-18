import pytest
import time
import random
import timeit

#normal_dir="normal/"
#overlay_dir="overlay/merged/"
dir = "overlay/merged/"

def test_once():

    #400k
    print (" write 400k")

    pos = 4096 
    len = 4096 * 8
    bs = b'0'*len
    start = time.time()
    file = open(dir + "file_400k", "rb+")
    file.seek(pos)
    file.write(bs)
    end = time.time()
    print ("file.write(len) time: ", end - start)
    file.close()
    
    #8M
    print (" write 8M")
    pos = 4096 * 2
    len = 4096 * 8
    bs = b'0'*len
    start = time.time()
    file = open(dir + "file_8m", "rb+")
    file.seek(pos)
    file.write(bs)
    end = time.time()
    print ("file.write(len) time: ", end - start)
    file.close()

    #100M
    print (" write 100M")
    pos = 10800
    len = 4096 * 8
    bs = b'0'*len
    start = time.time()
    file = open(dir + "file_100m", "rb+")
    file.seek(pos)
    file.write(bs)
    end = time.time()
    print ("file.write(len) time: ", end - start)
    file.close()

    #1g
    print (" write 1g")
    pos = 10800
    len = 4096 * 8
    bs = b'0'*len
    start = time.perf_counter()
    file = open(dir + "file_1g", "rb+")
    file.seek(pos)
    file.write(bs)
    end = time.perf_counter()
    print ("file.write(len) time: ", end - start,end,start)
    file.close()

def test_write_once_small():

    #4k
    print (" write 4k")
    pos = 1024
    len = 2048
    bs = b'0'*len
    start = time.time()
    file = open(dir + "file_4k", "rb+")
    file.seek(pos)
    file.write(bs)
    end = time.time()
    print ("file.write(len) time: ", end - start)
    file.close()

    #16k
    print (" write 16k")
    pos = 4096
    len = 8192
    bs = b'0'*len
    start = time.time()
    file = open(dir + "file_16k", "rb+")
    file.seek(pos)
    file.write(bs)
    end = time.time()
    print ("file.write(len) time: ", end - start)
    file.close()

    #64k
    print (" write 64k")
    pos = 4096
    len = 4096 * 4
    bs = b'0'*len
    start = time.time()
    file = open(dir + "file_64k", "rb+")
    file.seek(pos)
    file.write(bs)
    end = time.time()
    print ("file.write(len) time: ", end - start)
    file.close()

def test_write_many():

    N = 20
    file = None
    s = 0
    first = 0

    #1g
    print ("many write 1g")
    for i in range(250000):
        pos = i*4096
        len = 400
        bs = b'a'*len
        start = time.time()
        if i==0:
            file = open(dir + "file_1g", "rb+")
        file.seek(pos)
        file.write(bs)
        end = time.time()
        s += end - start
        if i==0:
            first = end - start
    file.close()
    print ("many file.write(len) total time: ", s)
    print ("many file.write(len) avg time: ", s/250000)
    print ("first file.write(len) time: ", first)

def test_read_many_pre():
    print("pre read 1g")
    file = open(dir + "file_1g", "rb+")
    for i in range(250000):
        pos = i*4096
        len = 400
        bs = b'a'*len
        file.seek(pos)
        if i%2==1:
            file.write(bs)
    file.close()

def test_read_many():
    
    print ("many read 1g")
    file = open(dir + "file_1g", "rb+",buffering=0)
    s = 0
    n = 0
    for i in range(250000):
        pos = i*4096
        len = 800
        file.seek(pos)
        if i%2 != 1:
            n+=1
            start = time.time()
            file.read(len)
            end = time.time()
            s += end - start
    file.close()

    print ("many file.read(len) total time: ", s)
    print ("many file.read(len) avg time: ", s/n)

if __name__ == "__main__":
    #test_once()
    #test_write_once_small()
    #test_write_many()
    test_read_many()
    #test_read_many_pre()