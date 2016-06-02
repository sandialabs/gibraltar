BASE_DIR := $(HOME)
CEPH_DIR := $(BASE_DIR)/ceph/src

SRC=\
	src/gib_cpu_funcs.c		\
	src/gib_cuda_driver.c 		\
	src/gibraltar.c			\
	src/gib_galois.c		\
	src/gibraltar_cpu.c		\
	src/gibraltar_jerasure.c	\

TESTS=\
	examples/benchmark		\
	examples/benchmark-2		\
	examples/sweeping_test		
#	examples/GibraltarTest		\
#	examples/GibraltarTest-2	\
#	examples/GibraltarCephTest

# Expect CUDA library include directive to already be in CPPFLAGS,
# e.g. -I/usr/local/cuda/include
CPPFLAGS := -I/usr/local/cuda/include
CPPFLAGS += -Iinclude
CPPFLAGS += -I$(CEPH_DIR)
CPPFLAGS += -DGIB_USE_MMAP=1
CPPFLAGS += -DLARGE_ENOUGH=1024*4
CPPFLAGS += -pg

# Expect CUDA library link directive to already be in LDFLAGS,
# .e.g. -L/usr/local/cuda/lib
LDFLAGS := -L/usr/local/cuda/lib
LDFLAGS += -Llib/
LDFLAGS += -L$(CEPH_DIR)/.libs

STATICLIBS := $(CEPH_DIR)/.libs/libosd.a
STATICLIBS += $(CEPH_DIR)/.libs/libosdc.a
STATICLIBS += $(CEPH_DIR)/.libs/libcommon.a
STATICLIBS += $(CEPH_DIR)/.libs/libglobal.a

CFLAGS += -Wall
LDLIBS=-lcuda -ljerasure -lrados -lprofiler

all: lib/libjerasure.a src/libgibraltar.a $(TESTS)

src/libgibraltar.a: src/libgibraltar.a($(SRC:.c=.o))

$(TESTS): src/libgibraltar.a

examples/GibraltarCephTest: src/libgibraltar.a
	g++ $(CPPFLAGS) $(LDFLAGS) $(STATICLIBS) \
		examples/GibraltarCephTest.cc src/libgibraltar.a  $(LDLIBS) \
		 -o examples/GibraltarCephTest

lib/libjerasure.a:
	cd lib/Jerasure-1.2 && make
	ar rus lib/libjerasure.a lib/Jerasure-1.2/*.o

clean:
	rm -f lib/libjerasure.a src/libgibraltar.a
	rm -f $(TESTS)
