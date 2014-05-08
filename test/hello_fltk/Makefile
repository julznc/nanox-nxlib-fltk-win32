
NXFLTK_DIR = ../..

INCLUDES = -I$(NXFLTK_DIR)/fltk

LIBS = \
	$(NXFLTK_DIR)/fltk/lib/libfltk.a \
	$(NXFLTK_DIR)/fltk/lib/libfltk_forms.a \
	$(NXFLTK_DIR)/nxlib/lib/libnx.a \
	$(NXFLTK_DIR)/nanox/lib/libnanox.a \
	$(NXFLTK_DIR)/freetype/objs/.libs/libfreetype.a
	
all:
	g++ $(INCLUDES) -c main.cpp -o main.o
	g++ -o test.exe main.o $(LIBS) -lgdi32 -lws2_32
