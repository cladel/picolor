#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

//#include "conversion.h"


#define WIDTH 60
#define HEIGHT 60
#define MARGIN 2
#define RECTSIZE WIDTH-2*MARGIN
#define UINT8MASK 0b11111111


struct x11_data{
    Display * display;
    XColor c;
    XImage *image;
    Window root;
    Window vue;
    Colormap colormap;
    GC gc;
    XWindowChanges npos;
    int rooth, rootw;
};


void saveColor(int r, int g, int b);
void getMousePosition(struct x11_data *, int *, int *, Window *);
void initXWindow(struct x11_data * );
void loopWindow(struct x11_data *);


static int _XlibErrorHandler(Display *display, XErrorEvent *event) {
    fprintf(stderr, "Xlib error: code %d.\n", event->error_code);
    return True;
}

static inline void setRootSize(struct x11_data * data){

    // Size of the screen
    XWindowAttributes att;
    XGetWindowAttributes(data->display, data->root, &att);
    data->rootw = att.width;
    data->rooth = att.height;
}

int main(int argc, char ** argv) {

    printf("----------- Picolor-v0.1.0 -----------\n\n");
    // Init Display
    struct x11_data display_data;
    display_data.display = XOpenDisplay(NULL);
    if(display_data.display == NULL){
        return EXIT_FAILURE;
    }

    printf("Picker ON - Keyboard is now captured.\nPress ESC to close the picker.\nPress TAB to capture a color.\n\n");
    XSetErrorHandler(_XlibErrorHandler);
    display_data.root = XRootWindow (display_data.display, XDefaultScreen (display_data.display));
    display_data.colormap = XDefaultColormap(display_data.display, XDefaultScreen (display_data.display));
    setRootSize(&display_data);

    //Prepare window opening
    initXWindow(&display_data);

    // Main loop
    loopWindow(&display_data);

    // Free resources
    XFreeGC(display_data.display, display_data.gc);
    XDestroyWindow(display_data.display,display_data.vue);
    XCloseDisplay(display_data.display);
    printf("\nPicker OFF\n");
    return EXIT_SUCCESS;

}

void getMousePosition(struct x11_data * data, int * mx, int * my, Window * window_returned){
    static int error = 0;
    int win_x, win_y;
    unsigned int mask_return;

    // Find pointer position
    if (XQueryPointer(data->display, data->root, window_returned,
                       window_returned, mx, my, &win_x, &win_y,
                       &mask_return) != True) {
        error++;
        if(error > 50)exit(EXIT_FAILURE);
    }

    // Select corresponding color on screen
    data->image = XGetImage (data->display,data->root, *mx, *my, 1, 1, AllPlanes , XYPixmap);
    data->c.pixel = XGetPixel(data->image, 0, 0);
    XQueryColor (data->display, data->colormap, &data->c);

    XFree (data->image);

}

void saveColor(int r, int g, int b){
    // Print color
    static unsigned int nb_saved = 0;
    if(nb_saved==0)printf("\u25A3     HEX   \t     RGB\n\n");
    int hex = (b & UINT8MASK) + ((g & UINT8MASK) << 8) + ((r & UINT8MASK) << 16);
    printf("\x1b[38;2;%d;%d;%dm\u25A3\x1b[0m   #%06X \t %3d %3d %3d\n", r, g, b, hex, r, g, b);
    nb_saved++;
}

static inline void calculatePosition(int * absp, int * dest, int axisl, int l){
    // Calculate position of the window, so it doesn't go out of the screen
    *absp = *dest + l / 3;
    if(*absp + l > axisl){
        *absp = *dest - 4*l/3;
    }
    *dest = *absp;
}

void loopWindow(struct x11_data * data) {
    // Map window to the screen
    XMapWindow(data->display, data->vue);

    // Wait for the MapNotify event
    XEvent ev;
    for (;;) {
        XNextEvent(data->display, &ev);
        if (ev.type == MapNotify)
            break;
    }

    Window window_returned;
    int revert;
    unsigned int i=0;
    int x,y;

    while (i++<2000) { // Last max ~ 1 min (Avoids bugs issues)

        // Get cursor position and pixel color
        getMousePosition(data, &data->npos.x, &data->npos.y, &window_returned);

        // Make sure window has input focus
        XGetInputFocus(data->display,&window_returned,&revert);
        if(&window_returned != &data->vue){
            XSetInputFocus(data->display,data->vue,RevertToParent,CurrentTime);
        }

        // Check if key was pressed
        if (XCheckMaskEvent(data->display, KeyPressMask, &ev)) {

            // Exit on ESC key press
            if (ev.xkey.keycode == 0x09) break;
            else if(ev.xkey.keycode == 23){   // TAB key press
                // Handle selected color
                saveColor( data->c.red / 256, data->c.green / 256, data->c.blue / 256);
            }
          //  else { XSendEvent(data->display, PointerWindow, False, KeyPressMask, &ev); }

        }

        // Set ideal position for the window
        calculatePosition(&x, &data->npos.x, data->rootw, WIDTH);
        calculatePosition(&y, &data->npos.y, data->rooth, HEIGHT);

        // Change position
        XConfigureWindow(data->display, data->vue, CWX | CWY, &data->npos);

        // Draw
        XSetForeground(data->display, data->gc, data->c.pixel);
        XFillRectangle(data->display, data->vue, data->gc, MARGIN, MARGIN, RECTSIZE, RECTSIZE);

        // Send the "Draw" request to the server
        XFlush(data->display);


        usleep(30000); // 30ms

    }

    // Hide window
    XUnmapWindow(data->display, data->vue);
    for (;;) {
        XNextEvent(data->display, &ev);
        if (ev.type == UnmapNotify)
            break;
    }
}

void initXWindow(struct x11_data * data) {

    // Background color
    XColor back;
    back.red = 60 * 256;  // 0-65535
    back.green = 63 * 256;
    back.blue = 65 * 256;
    XAllocColor(data->display, data->colormap, &back);

    // Make borderless
    XSetWindowAttributes att;
    att.override_redirect = True; // No window manager
    att.background_pixel = back.pixel;

    // Create window
    data->vue = XCreateWindow(data->display, data->root, 0, 0, WIDTH, HEIGHT,
                              0, CopyFromParent, CopyFromParent, CopyFromParent, CWOverrideRedirect | CWBackPixel,
                              &att);


    // Set mask to get MapNotify and KeyPress events
    XSelectInput(data->display, data->vue, StructureNotifyMask | KeyPressMask);

    // Create a "Graphics Context"
    data->gc = XCreateGC(data->display, data->vue, 0, NULL);

    // Set background (will show after Flush and Map)
    XSetBackground(data->display, data->gc, back.pixel);

}