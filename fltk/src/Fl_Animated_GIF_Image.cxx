//
// "$Id: Fl_GIF_Image.cxx 5845 2007-05-20 00:01:06Z mike $"
//
// Fl_GIF_Image routines.
//
// Copyright 1997-2005 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//
// Contents:
//
//

//
// Include necessary header files...
//

#include <FL/Fl.H>
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_Widget.H>
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <vector>

// Read a .gif file and convert it to a "xpm" format (actually my
// modified one with compressed colormaps).

// Modifications for animated GIF support by Marc Brevoort, 2010

// Extensively modified from original code for gif2ras by
// Patrick J. Naughton of Sun Microsystems.  The original
// copyright notice follows:

/* gif2ras.c - Converts from a Compuserve GIF (tm) image to a Sun Raster image.
 *
 * Copyright (c) 1988 by Patrick J. Naughton
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the author:
 *
 *                     Patrick J. Naughton
 *                     Sun Microsystems, Inc.
 *                     2550 Garcia Ave, MS 14-40
 *                     Mountain View, CA 94043
 *                     (415) 336-1080
 */

typedef unsigned char uchar;

int Fl_GIF_Image::nextbyte()
{
	int x=getc(GifFile);
	//LOGPRINT("Read  0x%02x from offset %d (0x%02x)\n",x,foffset,foffset);
	foffset++;
	return x;

}

int Fl_GIF_Image::getshort()
{
	int x=nextbyte();
	int y=nextbyte();
	if (x<0) return 0;
	if (y<0) return 0;
	return x+(y*256);
}

unsigned int Fl_GIF_Image::framecount()
{
	return this->FrameCount;
}

unsigned int Fl_GIF_Image::framedelay()
{
	/* returns delay of current frame in msec */
        unsigned int currdelay=(*FrameDelay)[currframe]; // in 1/100 sec.
	return 10*currdelay;
}


void Fl_GIF_Image::framedelay(unsigned int newdelay)
{
	unsigned int mynewdelay=(unsigned int)(newdelay/10);
	(*FrameDelay)[currframe]=mynewdelay;
}

int Fl_GIF_Image::read_image_data(int Left,int top,int frameWidth,int frameHeight,int BitsPerPixel)
{
  //LOGPRINT("Reading image data.\n");
  int ColorMapSize = 1 << BitsPerPixel;
  if (BitsPerPixel >= CodeSize)
  {
    // Workaround for broken GIF files...
    BitsPerPixel = CodeSize - 1;
    ColorMapSize = 1 << BitsPerPixel;
  }

  Image = new uchar[canvasWidth*canvasHeight]; // cleaned up by convert_to_xpm**
  uchar tp=transparent_pixel;
  if (!has_transparent) tp=0;

  for (int i=0;i<(canvasWidth*canvasHeight);i++) { Image[i]=tp; }

  int YC = top, Pass = 0; /* Used to de-interlace the picture */
  uchar *p = Image+Left+(top*canvasWidth); /* where to start drawing */
  uchar *eol = p+frameWidth;

  int InitCodeSize = CodeSize;
  int ClearCode = (1 << (CodeSize-1));
  int EOFCode = ClearCode + 1;
  int FirstFree = ClearCode + 2;
  int FinChar = 0;
  int ReadMask = (1<<CodeSize) - 1;
  int FreeCode = FirstFree;
  int OldCode = ClearCode;


  blocklen = nextbyte();
  //LOGPRINT("Blocklen=%d\n",blocklen);
  uchar thisbyte = nextbyte(); blocklen--;
  int frombit = 0;

  for (;;) {

/* Fetch the next code from the raster data stream.  The codes can be
 * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
 * maintain our location as a pointer and a bit offset.
 * In addition, gif adds totally useless and annoying block counts
 * that must be correctly skipped over. */
    int CurCode = thisbyte;
    if (frombit+CodeSize > 7) {
      if (blocklen <= 0) {
	blocklen = nextbyte();
	if (blocklen <= 0) break;
      }
      thisbyte = nextbyte(); blocklen--;
      CurCode |= thisbyte<<8;
    }
    if (frombit+CodeSize > 15) {
      if (blocklen <= 0) {
	blocklen = nextbyte();
	if (blocklen <= 0) break;
      }
      thisbyte = nextbyte(); blocklen--;
      CurCode |= thisbyte<<16;
    }
    CurCode = (CurCode>>frombit)&ReadMask;
    frombit = (frombit+CodeSize)%8;

    //LOGPRINT("CurCode=%d, Clearcode=%d\n",CurCode,ClearCode);
    if (CurCode == ClearCode) {
      //LOGPRINT("CurCode==Clearcode\n");
      CodeSize = InitCodeSize;
      ReadMask = (1<<CodeSize) - 1;
      FreeCode = FirstFree;
      OldCode = ClearCode;
      continue;
    }

    if (CurCode == EOFCode) break;

    uchar OutCode[1025]; // temporary array for reversing codes
    uchar *tp = OutCode;
    int i;
    if (CurCode < FreeCode) 
    { i = CurCode; }
    else if (CurCode == FreeCode) {*tp++ = (uchar)FinChar; i = OldCode;}
    else { 
        //LOGPRINT("Error reading GIF");
	return 1; /* error reading GIF */ 
    }

    while (i >= ColorMapSize) {*tp++ = Suffix[i]; i = Prefix[i];}
    *tp++ = FinChar = i;
    do {
      *p++ = *--tp;
      if (p >= eol) {
	if (!Interlace) YC++;
	else switch (Pass) {
	case 0: YC += 8; if (YC >= (frameHeight+top)) {Pass++; YC = top+4;} break;
	case 1: YC += 8; if (YC >= (frameHeight+top)) {Pass++; YC = top+2;} break;
	case 2: YC += 4; if (YC >= (frameHeight+top)) {Pass++; YC = top+1;} break;
	case 3: YC += 2; break;
	}
	if (YC>=frameHeight+top) YC=top; /* cheap bug fix when excess data */
	p = Image + YC*canvasWidth + Left;
	eol = p+frameWidth;
      }
    } while (tp > OutCode);

    if (OldCode != ClearCode) {
      Prefix[FreeCode] = (short)OldCode;
      Suffix[FreeCode] = FinChar;
      FreeCode++;
      if (FreeCode > ReadMask) {
	if (CodeSize < 12) {
	  CodeSize++;
	  ReadMask = (1 << CodeSize) - 1;
	}
	else FreeCode--;
      }
    }
    OldCode = CurCode;
  }

  /* read block terminator byte. Should always be 0 
     so if this is not the case, that's an error condition. */
//  int returning=nextbyte();
  //LOGPRINT("Finished reading image data, file cursor is now at %d\n",foffset);
//  //LOGPRINT("Byte at that point should be zero and is %d\n",returning);
  return 0; //returning;
}

void Fl_GIF_Image::convert_to_xpm(int Left,int Top,int frameWidth,int frameHeight,int BitsPerPixel)
{
  //... actually, probably this image is already canvas size.
  int ColorMapSize = 1 << BitsPerPixel;
  // allocate line pointer arrays:
  w(canvasWidth);
  h(canvasHeight);
  d(1);
  
  new_data = new char*[canvasHeight+2];
  Frame->push_back(new_data);
  FrameHeight->push_back(canvasHeight);
  // transparent pixel must be zero, swap if it isn't
  // But only if it's a local colormap OR the first frame.

  if (has_transparent && (transparent_pixel != 0)) {
    p = Image+canvasWidth*canvasHeight;
    while (p-- > Image) {
      if (*p==transparent_pixel) *p = 0;
      else if (*p==0) *p = transparent_pixel;
    }

    if ((localcolormap==1) || 
       ((localcolormap==0) && (FrameCount==1))
    ) {
    //LOGPRINT("swap transparent pixel with zero\n");
    
    uchar t;
    t                        = Red[0];
    Red[0]                   = Red[transparent_pixel];
    Red[transparent_pixel]   = t;

    t                        = Green[0];
    Green[0]                 = Green[transparent_pixel];
    Green[transparent_pixel] = t;

    t                        = Blue[0];
    Blue[0]                  = Blue[transparent_pixel];
    Blue[transparent_pixel]  = t;
    }
  }

  // find out what colors are actually used:
  uchar used[256]; uchar remap[256];
  int i;
  for (i = 0; i < ColorMapSize; i++) used[i] = 0;
  p = Image+canvasWidth*canvasHeight;
  while (p-- > Image) used[*p] = 1;

  // remap them to start with printing characters:
  int base = has_transparent && used[0] ? ' ' : ' '+1;
  int numcolors = 0;
  for (i = 0; i < ColorMapSize; i++) if (used[i]) {
    remap[i] = (uchar)(base++);
    numcolors++;
  }

  // write the first line of xpm data (use suffix as temp array):
  int length = sprintf((char*)(Suffix),
		       "%d %d %d %d",canvasWidth,canvasHeight,-numcolors,1);
  new_data[0] = new char[length+1];
  strcpy(new_data[0], (char*)Suffix);

  // write the colormap
  new_data[1] = (char*)(p = new uchar[4*numcolors]);
  for (i = 0; i < ColorMapSize; i++) if (used[i]) {
    *p++ = remap[i];
    *p++ = Red[i];
    *p++ = Green[i];
    *p++ = Blue[i];
  }

  // remap the image data:
  p = Image+canvasWidth*canvasHeight;
  while (p-- > Image) *p = remap[*p];

  // split the image data into lines:
  for (i=0; i<canvasHeight; i++) {
    new_data[i+2] = new char[canvasWidth+1];
    memcpy(new_data[i + 2], (char*)(Image + i*canvasWidth), canvasWidth);
    new_data[i + 2][canvasWidth] = 0;
  }
  
  data((const char **)new_data, canvasHeight + 2);
  alloc_data = 1;

  delete[] Image; // ** allocated in read_image_data

}

Fl_GIF_Image::~Fl_GIF_Image()
{
	/* stop using the data about to disappear */
	Fl::remove_timeout(this->animate); 

	// TODO: Clean up contents of vector
	while (Frame->size()>0)
	{	
		/* This results in heap corruption: */

		//int index=Frame->size()-1;
		//delete (*Frame)[index];

		/*
		char** data=(*Frame)[index]; 
		int fheight=(*FrameHeight)[index];
		
		for (int j=0;j<fheight+2;j++)
		{
			delete data[j];
		}
		delete data; */

		Frame->pop_back();
	}

	delete Frame; Frame=0;

	FrameHeight->clear();
	delete FrameHeight; FrameHeight=0;

	FrameDelay->clear();
	delete FrameDelay; FrameDelay=0;
}

void Fl_GIF_Image::animating(int yesno)
{
	/* Set animation for running: 1=yes, 0=no
           Lacking a standardised bool type, the 
           checks are to make sure only valid values
	   are interpreted */
	if (yesno==1) 
	{
		run_animation=1; 
		animate(this);
	}
	else
	{
		if (yesno==0) 
		{ 
			run_animation=0; 
			Fl::remove_timeout(this->animate);
		}
	}
	return;
}

int Fl_GIF_Image::animating()
{
	return run_animation;
}

void Fl_GIF_Image::select_frame(unsigned int framenum)
{

  if (framenum>=FrameCount) return;
  int fheight=(*FrameHeight)[framenum];
  ((Fl_Pixmap*)this)->Fl_Pixmap::uncache();
  data((*Frame)[framenum], 2+ fheight);
  alloc_data = 1;

}

void Fl_GIF_Image::animate(void* instance)
{
	Fl_GIF_Image* mythis=(Fl_GIF_Image*)instance;
//	//LOGPRINT("animate, currframe=%d\n",mythis->currframe);

	mythis->currframe++;
	int stopanimate=0;
	if (mythis->currframe>=mythis->FrameCount) 
	{
		if (mythis->repcount>0)
		{
			mythis->repcount--;
			if (mythis->repcount==0)
			{
				stopanimate=1; /* end of animation. */
			} else {
				mythis->currframe=0;
			}
		} else {
			mythis->currframe=0;
		}
	}
	mythis->select_frame(mythis->currframe);
	Fl_Widget* parent=mythis->parent();
	if (parent!=NULL) { parent->damage(); parent->redraw(); }
	if (stopanimate==0)
	{
		// frame delay is specified in 100ths of seconds
		double framedelay=(double)(((*(mythis->FrameDelay))[mythis->currframe])*10);
	        
		Fl::add_timeout(framedelay/1000,
			mythis->animate,
			instance);
	}
	return;
}

unsigned int Fl_GIF_Image::repeatcount()
{
	return this->repcount;
}

void Fl_GIF_Image::repeatcount(unsigned int newcount)
{
	this->repcount=newcount;
}

Fl_Widget* Fl_GIF_Image::parent()
{
	return this->_parent;
}

void Fl_GIF_Image::parent(Fl_Widget* p_parent)
{
	this->_parent=p_parent;
}

Fl_GIF_Image::Fl_GIF_Image(const char *infname) : Fl_Pixmap((char *const*)0) {
  //LOGPRINT("Fl_GIF_Image::Fl_GIF_Image(\"%s\")\n",infname);
  int delay; // not used for now
  _parent=0;
  FrameCount=0;
  foffset=0;
  Red=(uchar*)0;
  Green=(uchar*)0;
  Blue=(uchar*)0;
  Frame=new std::vector<char**>();
  FrameHeight=new std::vector<unsigned int>();
  FrameDelay=new std::vector<unsigned int>();
  this->repcount=0; /* By default loop forever */
  // tables used by LZW decompresser:
  short int _Prefix[4096];
  uchar _Suffix[4096];

  Prefix=_Prefix;
  Suffix=_Suffix;
  
  if ((GifFile = fopen(infname, "rb")) == NULL) {
    Fl::error("Fl_GIF_Image: Unable to open %s!", infname);
    return;
  }

  {char b[6];
  if (fread(b,1,6,GifFile)<6) {
    fclose(GifFile);
    return; /* quit on eof */
  }
  if (b[0]!='G' || b[1]!='I' || b[2] != 'F') {
    fclose(GifFile);
    Fl::error("Fl_GIF_Image: %s is not a GIF file.\n", infname);
    return;
  }
  if (b[3]!='8' || b[4]>'9' || b[5]!= 'a')
    Fl::warning("%s is version %c%c%c.",infname,b[3],b[4],b[5]);
  }
  foffset=6;
  canvasWidth=getshort();
  //LOGPRINT("Canvas width is %d\n",canvasWidth);
  canvasHeight=getshort();
  //LOGPRINT("Canvas height is %d\n",canvasHeight);

  uchar ch = nextbyte();
  char HasColormap = ((ch & 0x80) != 0);
  int BitsPerPixel = (ch & 7) + 1;
  int ColorMapSize = 1 << BitsPerPixel;
  // int OriginalResolution = ((ch>>4)&7)+1;
  // int SortedTable = (ch&8)!=0;
  /* ch = */ nextbyte(); // Background Color index
  /* ch = */ nextbyte(); // Aspect ratio is N/64

  // Read in global colormap:
  transparent_pixel = 0;
  has_transparent = 0;

  uchar R[256], G[256], B[256]; /* color map */

  Red=R; Green=G; Blue=B; /* Point class color map to stack color map */

  if (HasColormap) {
    //LOGPRINT("Reading colormap.\n");
    for (int i=0; i < ColorMapSize; i++) {	
      Red[i] = (uchar)nextbyte();
      Green[i] = (uchar)nextbyte();
      Blue[i] = (uchar)nextbyte();
    }
  } else {
    Fl::warning("%s does not have a colormap.", infname);
    for (int i = 0; i < ColorMapSize; i++)
      Red[i] = Green[i] = Blue[i] = (uchar)(255 * i / (ColorMapSize-1));
  }
  int is_animation=0;
  for (;;) {
    int i=nextbyte();
    if (i==0) {
       if (is_animation==1) {
          is_animation=0;
	  i=nextbyte();
       }
    }

    if (i < 0) {
      fclose(GifFile);
      Fl::error("Fl_GIF_Image: %s - unexpected EOF",infname); 
      return;
    }

    blocklen=0;

    if (i == 0x3B) break; // return 0;  eof code // MRJB: WAS DISABLED. Trailer: Sentinel ';'.

    if (i==0) {
	//LOGPRINT("unexpected 0 byte at offset %d...\n",foffset);
    }
    if (i == 0x21) {		// a "gif extension"
      //LOGPRINT("Code 0x21->Gif extension\n");
      ch = nextbyte();
      blocklen = nextbyte();

      if (ch==0xF9 && blocklen==4) { // Netscape animation extension
        //LOGPRINT("ANIMATION EXTENSION START OF (1-based) FRAME %d FOUND\n",(FrameCount+1));
        is_animation=1;
	char bits;
	bits = nextbyte();  
	//LOGPRINT("bits=%d\n",bits);

	delay=getshort();
	//LOGPRINT("Delay=%d x 10 msec\n",delay);
        FrameDelay->push_back(delay);

	transparent_pixel = nextbyte(); 
	//LOGPRINT("Transpix=%d\n",transparent_pixel);

        has_transparent=0;
	if (bits & 1) has_transparent = 1; 
	//LOGPRINT("Has_transp=%d\n",has_transparent);

	i=nextbyte(); 
	//LOGPRINT("Extension def block end (must be 0): %d\n",end);

        //LOGPRINT("Image (code 2C) is expected to follow.\n");
	continue;
      } else if (ch == 0xFF) { // Netscape repeat count
          while(blocklen>0) {
             ch=nextbyte();
             blocklen--;
          }	
	  int cmd=getshort();
          if (cmd==0x103) {
          this->repcount=getshort();
          } else { ch=getshort(); this->repcount=0; /* loop forever */ }
          ch=nextbyte();
//        for (int x=1;x<5;x++) {
//        int rc=nextbyte(); blocklen--;
//        printf("Repeat count = %d\n",rc);
//        }
	//;
	  //("Repeat count=%d\n",this->repcount);
	  continue;

      } else if (ch != 0xFE) { //Gif Comment
	Fl::warning("%s: unknown gif extension 0x%02x.", infname, ch);
      }
    } else if (i == 0x2c) {	// an image      
      //LOGPRINT("Image coming up.\n");
      int xpos=getshort(); 
      //LOGPRINT("xpos of image is %d\n",xpos);
      int ypos=getshort();
      //LOGPRINT("ypos of image is %d\n",ypos);

      int frameWidth=getshort();
      int frameHeight=getshort();
      //LOGPRINT("Width, Height=(%d,%d)\n",frameWidth,frameHeight);

      ch = nextbyte();
      //LOGPRINT("'Other characteristics': %d\n",ch);
      //LOGPRINT("I'd expect LZW data to follow now.\n");
      Interlace = ((ch & 0x40) != 0);
      //LOGPRINT("Interlace=%d\n",Interlace);
      //LOGPRINT("HaveColormap=%d\n",ch&0x80);
      localcolormap=0;
      if (ch&0x80) { 
        //LOGPRINT("Read local colormap\n");
        localcolormap=1;
	// read local color map
	int n = 2<<(ch&7);
	for (i=0; i < n; i++) {	
	  Red[i] = (uchar)nextbyte();
	  Green[i] = (uchar)nextbyte();
	  Blue[i] = (uchar)nextbyte();
	}
      }
      CodeSize = ((uchar)nextbyte())+1;
      //LOGPRINT("Code size is %d\n",CodeSize);
      // okay, this is the image we want
      FrameCount++;
	  //LOGPRINT("====Frame %d start of image data for %s====\n",FrameCount,infname);
          //LOGPRINT("Bits per pixel=%d\n",BitsPerPixel);
	  int readok=read_image_data(xpos,ypos,frameWidth,frameHeight,BitsPerPixel);
	  if (readok!=0) {
	      Fl::error("Fl_GIF_Image: %s - LZW Barf!", infname); 
	  } else {
	      /* We are done reading the image data for this frame, 
                 now convert to (full width) xpm: */
	      convert_to_xpm(xpos,ypos,frameWidth,frameHeight,BitsPerPixel);
	  }

	// Decode all frames; this is why the next line is commented out.
        //if (FrameCount==1) break; // this is the frame we wanted all along.
        //LOGPRINT("Blocklen=%d\n",blocklen);
	continue;
    } else {
      Fl::warning("%s: unknown gif code 0x%02x, offset %d", infname, i,foffset);
      blocklen = 0;
      //break; // stop reading.
    }

    // skip the data:
    while (blocklen>0) {while (blocklen--) {ch = nextbyte();} blocklen=nextbyte();}
  }
  currframe=0;

  if (FrameCount>1)
  {
      animate(this);
  } else {
	select_frame(0);
  }
  fclose(GifFile);
}


//
// End of "$Id: Fl_GIF_Image.cxx 5845 2007-05-20 00:01:06Z mike $".
//
