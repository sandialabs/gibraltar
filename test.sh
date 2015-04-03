#!/bin/bash

g++  -I/usr/local/cuda/include -Iexamples/ -Iinclude/ -L/usr/local/cuda/lib  -L/home/whaddock/gibraltar/lib -Llib/  examples/GibraltarTest.cc src/libgibraltar.a  -lcuda -o examples/GibraltarTest
