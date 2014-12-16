# Makefile: Makefile.
# Copyright (C) University of Alabama at Birmingham and Sandia
# National Laboratories, 2010, written by Matthew L. Curry
# <mlcurry@sandia.gov>
#
# Changes:
#
#

# Author:  Matthew L. Curry
# Email:   mlcurry@sandia.gov
#
# Use "make cuda=1" to build for CUDA, "make jerasure=1" to build for Jerasure,
# and "make cpu=1" to use the low-performance CPU implementation.

CC=gcc
CFLAGS=-Wall -Llib -Iinc -g -O0 
LFLAGS=-lgibraltar
CUDAINC=-I $(CUDA_INC_PATH)
CUDALIB=-L $(CUDA_LIB_PATH)
min_test=2
max_test=16
test_range=`seq $(min_test) $(max_test)`

.PHONY: clean lib/libgibraltar.a obj/gibraltar.o jerasure examples

################################################################################
# Depending on the variable set, different versions of the Gibraltar library
# is built.

ifneq ($(all),)
# Compile against all three (cuda, cpu, jerasure)
GIB_IMP=src/gib_cuda_driver.c
CFLAGS+=$(CUDAINC)
LFLAGS+=$(CUDALIB)
LFLAGS+=-lcudart -lcuda -ljerasure
GIB_OBJ+=obj/gib_galois.o obj/gib_cpu_funcs.o obj/dynamic_gibraltar.o
GIB_DEP+=cache lib/libjerasure.a
chosen+=1
endif

ifneq ($(cuda),)
# Just for the sake of having someplace to put ptx/cubin files.
GIB_IMP=src/gib_cuda_driver.c
CFLAGS+=$(CUDAINC)
LFLAGS+=$(CUDALIB)
LFLAGS+=-lcudart -lcuda
GIB_OBJ+=obj/gib_galois.o obj/gib_cpu_funcs.o obj/dynamic_gibraltar.o
GIB_DEP+=cache
chosen+=1
endif

ifneq ($(cpu),)
GIB_IMP=src/gibraltar_cpu.c
GIB_OBJ+=obj/gib_galois.o obj/gib_cpu_funcs.o obj/dynamic_gibraltar.o
chosen+=1
endif

ifneq ($(jerasure),)
GIB_IMP=src/gibraltar_jerasure.c
GIB_DEP+=lib/libjerasure.a
LFLAGS+=-ljerasure
chosen+=1
endif

################################################################################

all:
	if [ "$(chosen)" = "1" ]; then \
		echo "Good"; \
	else \
		echo "Please choose a single target as follows"; \
		echo "(In order of preference)"; \
		echo "make all=1"; \
		echo "make cuda=1"; \
		echo "make jerasure=1"; \
		echo "make cpu=1"; \
		exit 1; \
	fi
	echo $(LFLAGS) > LFLAGS
	make examples

examples: lib/libgibraltar.a
ifeq ($(all),1)
	$(CXX) -Dmin_test=$(min_test) -Dmax_test=$(max_test) $(CFLAGS) \
		examples/benchmark_d.cc -o examples/benchmark_d $(LFLAGS)
	$(CXX) -Dmin_test=$(min_test) -Dmax_test=$(max_test) $(CFLAGS) \
	examples/sweeping_test_d.cc -o examples/sweeping_test_d $(LFLAGS)
else
	$(CXX) -Dmin_test=$(min_test) -Dmax_test=$(max_test) $(CFLAGS) \
		examples/benchmark.cc -o examples/benchmark $(LFLAGS)
	$(CXX) -Dmin_test=$(min_test) -Dmax_test=$(max_test) $(CFLAGS) \
		examples/sweeping_test.cc -o examples/sweeping_test $(LFLAGS)
endif

obj/gibraltar.o: obj
ifeq ($(all),1)
	$(CC) $(CFLAGS) -c $(GIB_IMP) -o obj/cuda.o
	$(CC) $(CFLAGS) -c src/gibraltar_cpu.c -o obj/cpu.o
	$(CC) $(CFLAGS) -c src/gibraltar_jerasure.c -o obj/jer.o
	ld -r obj/cuda.o obj/cpu.o obj/jer.o -o obj/gibraltar.o
else	
	$(CC) $(CFLAGS) -c $(GIB_IMP) -o obj/gibraltar.o
endif

lib/libgibraltar.a: obj/gibraltar.o $(GIB_OBJ) $(GIB_DEP)
	ar rus lib/libgibraltar.a $(GIB_OBJ) obj/gibraltar.o

lib/libjerasure.a:
	cd lib/Jerasure-1.2 && make
	ar rus lib/libjerasure.a lib/Jerasure-1.2/*.o

obj:
	mkdir -p obj

obj/%.o: src/%.c obj
	$(CC) $(CFLAGS) -c src/$*.c -o obj/$*.o

# A special kind of rule:  These files don't need to be remade if they're
# out of date, just destroyed.
cache:  src/gib_cuda_checksum.cu
	rm -rf cache
	mkdir cache

clean:
	rm -rf obj cache LFLAGS ptx
	rm -f lib/*.a
	rm -f examples/benchmark_d
