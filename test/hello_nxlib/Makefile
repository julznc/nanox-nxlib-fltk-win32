
NANOX_DIR = ../../nanox
NXLIB_DIR = ../../nxlib
FT2_DIR = ../../freetype

INCLUDES = -I$(NANOX_DIR)/include -I$(NXLIB_DIR)

all:
	gcc $(INCLUDES) -c main.c -o main.o
	gcc -o test.exe main.o $(NXLIB_DIR)/lib/libnx.a $(NANOX_DIR)/lib/libnanox.a $(FT2_DIR)/objs/.libs/libfreetype.a -lgdi32
