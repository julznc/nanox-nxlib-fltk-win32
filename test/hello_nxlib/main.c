
#include <X11/Xlib.h> // Every Xlib program must include this
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>   // I include this to test return values the lazy way
#include <unistd.h>   // So we got the profile for 10 seconds
#include <nano-X.h>

#define NIL (0)       // A name for the void pointer

void terminate(Display *dpy)
{
         XCloseDisplay(dpy);
         GrClose(); //Nano-X function - return to text screen mode
         exit(0);
         
}

int main(int argc, char ** argv){
	
	XEvent e;
	
	KeySym          keysym;
    XComposeStatus  compose;
	char            buf[40];      
    int             bufsize = 40;
	int				length;
	int i;
	
	XFontStruct *font;
	GC gc;
	
	int blackColor, whiteColor;
	const char *text   = "hello world!";
	const char *keymsg = "F:\\>        ";
	//char *letter = " ";
	char letter[2] = {0};
	
	Atom wdw;
	Window w;
	Display *dpy;
	
	printf("nxlib test started...\n");
	
	dpy = XOpenDisplay(NIL);
	assert(dpy);
	
	blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	whiteColor = WhitePixel(dpy, DefaultScreen(dpy));
	
	wdw = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	
	// Create the window
	
	w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 50, 50, 
	             200, 100, 2, blackColor, blackColor);
	
	// We want to get MapNotify events
	
	XSelectInput(dpy, w, ButtonPressMask|StructureNotifyMask|KeyPressMask);
	
	// "Map" the window (that is, make it appear on the screen)
	
	XMapWindow(dpy, w);
	
	// Get the fixed font.
	font = XLoadQueryFont( dpy, "system" );
	if( !font ){
		printf("Error, couldn't load font\n" );
		return 1 ;
	}
	
	// Create a "Graphics Context"
	
	gc = XCreateGC(dpy, w, 0, NIL);
	
	// Tell the GC we draw using the white color
	
	XSetForeground(dpy, gc, whiteColor);
	
	// Wait for the MapNotify event
	
	for(;;) {
	    XNextEvent(dpy, &e);
	    if (e.type == MapNotify)
		  break;
	    }
	
	while(1){
	    XNextEvent(dpy, &e);
		
		switch(e.type){
	
		case ClientMessage:
	        // Check if click on "X" - close window and exit 
			if (e.xclient.data.l[0] == wdw) terminate(dpy);
			break;
	
	    case ConfigureNotify:
	         // Draw string and line
	         XDrawString( dpy, w, gc, 10, 16, text, strlen( text ) );
	         XDrawString( dpy, w, gc, 10, 36, keymsg, strlen( text ) );
	         //XDrawLine(dpy, w, gc, 10, 60, 180, 20);
	         break;
	
	    case ButtonPress:
	    	printf("ButtonPress event\n" );
			// close window and exit if button pressed within window
			// terminate(dpy);
			break;
	
		 case KeyPress:
	    	 printf("KeyPress event\n" );
		    length = XLookupString((XKeyEvent*)&e, buf, bufsize, &keysym, &compose);
		    if (length < 0)
		    	break;
            /* clear the old letter */
            XSetForeground(dpy, gc, blackColor);
            for (i=0;i<20;i++) { XDrawLine(dpy, w, gc, 35, 26+i, 55, 26+i); }
            XSetForeground(dpy, gc, whiteColor);
            letter[0]=(char)keysym;
			XDrawString( dpy, w, gc, 40, 36, letter, 1);
			break;
		 case KeyRelease:
			 printf("KeyRelease event\n");
			 break;
	  
	    default: /* ignore any other event types. */
	            break;
	    }
	} //while


}
