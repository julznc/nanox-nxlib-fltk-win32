
NXFLTK_DIR = ../..

INCLUDES = -I$(NXFLTK_DIR)/nanox/include

LIBS = \
	$(NXFLTK_DIR)/nanox/lib/libnanox.a \
	$(NXFLTK_DIR)/freetype/objs/.libs/libfreetype.a \
	$(NXFLTK_DIR)/harfbuzz/lib/libharfbuzz.a

all:
	gcc $(INCLUDES) -c main.c -o main.o
	g++ -o test.exe main.o $(LIBS) -lgdi32
