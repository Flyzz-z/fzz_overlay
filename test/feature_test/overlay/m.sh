#!/bin/bash
sudo umount merged
rm upper/*
sudo mount -t overlay overlay -olowerdir=./lower1:./lower2:./lower3,upperdir=./upper,workdir=./work ./merged

