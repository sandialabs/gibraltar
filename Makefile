SRC=\
	src/gib_cpu_funcs.c		\
	src/gib_cuda_driver.c 		\
	src/gibraltar.c			\
	src/gib_galois.c		\
	src/gibraltar_cpu.c		\
	src/gibraltar_jerasure.c	\

TESTS=\
	examples/benchmark		\
	examples/sweeping_test		\

# Expect CUDA library include directive to already be in CPPFLAGS,
# e.g. -I/usr/local/cuda/include
CPPFLAGS += -Iinc/ -I/usr/local/cuda-6.5/include

# Expect CUDA library link directive to already be in LDFLAGS,
# .e.g. -L/usr/local/cuda/lib
LDFLAGS += -Llib/ -L/usr/local/cuda-6.5/lib64

CFLAGS += -Wall
LDLIBS=-lcuda -ljerasure

all: lib/libjerasure.a src/libgibraltar.a $(TESTS)

src/libgibraltar.a: src/libgibraltar.a($(SRC:.c=.o))

$(TESTS): src/libgibraltar.a

lib/libjerasure.a:
	cd lib/Jerasure-1.2 && make
	ar rus lib/libjerasure.a lib/Jerasure-1.2/*.o

clean:
	rm -f lib/libjerasure.a src/libgibraltar.a
	rm -f $(TESTS)
	rm -f src/*.o
