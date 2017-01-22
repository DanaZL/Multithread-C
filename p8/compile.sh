#!/bin/bash

g++ crawler.cpp extract_urls.cpp -std=c++14 libtask/libtask.a -I libtask/ -I gumbo-parser-master/ `pkg-config --cflags --libs gumbo` -o crawler