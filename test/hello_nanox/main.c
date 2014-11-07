
#include <stdio.h>
#include <string.h>

#include <nano-X.h>

#define WIDTH	240
#define HEIGHT	320
#define	MARGIN	50

GR_WINDOW_ID 	w;
GR_GC_ID	gc;
GR_EVENT	event;

int main(int argc, char *argv[])
{
	int fid;
	
	if (GrOpen() < 0) {
		printf ("Cannot open graphics!\n");
		return -1;
	}
	printf("GrOpen() success!\n");
	
	w = GrNewWindowEx(0, "Nano-X test", GR_ROOT_WINDOW_ID, MARGIN, MARGIN,
			WIDTH-MARGIN*2, HEIGHT-MARGIN*2, MWRGB( 0, 0, 0 ));
	if(!w) {
		printf("Couldn't get a window\n");
		GrClose();
		return 1;
	}
	printf("GrNewWindow() OK.\n");
	//GrSelectEvents(w, GR_EVENT_MASK_EXPOSURE);
	GrSelectEvents(w, GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_EXPOSURE);
	GrMapWindow(w);
	gc = GrNewGC();
	if(gc>0)
		printf("GrNewGC() OK.\n");
	
	fid = GrCreateFontEx((GR_CHAR *)"arial", 20, 10, NULL);
	if(fid>0)
		printf("Font created.\n");
	GrSetFontAttr(fid, GR_TFANTIALIAS, 0);
	GrSetGCFont(gc, fid);
	
	while (1) {
		GrGetNextEvent(&event);
		//printf("got event type = 0x%08x\n", event.type);
		switch (event.type) {
			case GR_EVENT_TYPE_BUTTON_DOWN:
				GrText(w, gc, 20, 70, "_________", -1, MWTF_BOTTOM);
				GrText(w, gc, 20, 120, "BUTTON_DOWN", -1, MWTF_BOTTOM);
				break;
				
				
			case GR_EVENT_TYPE_BUTTON_UP:
				GrText(w, gc, 20, 120, "___________", -1, MWTF_BOTTOM);
				GrText(w, gc, 20, 70, "BUTTON_UP", -1, MWTF_BOTTOM);
				break;

			case GR_EVENT_TYPE_EXPOSURE:
				if (event.exposure.wid == w){
					printf("GR_EVENT_TYPE_EXPOSURE\n");
					GrText(w, gc, 20, 50, "NanoX Test", -1, MWTF_BOTTOM);
				}
				break;
			case GR_EVENT_TYPE_CLOSE_REQ:
				GrClose();
				return 0;
		}
	}
}

