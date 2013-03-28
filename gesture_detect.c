#include <stdio.h>
#include <sys/shm.h>
#include <xorg/synaptics.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XTest.h>

SynapticsSHM* shm_init()
{
    SynapticsSHM *synshm = NULL;
    int shmid = 0;

    if ((shmid = shmget(SHM_SYNAPTICS, sizeof(SynapticsSHM), 0)) == -1) {
        if ((shmid = shmget(SHM_SYNAPTICS, 0, 0)) == -1)
            fprintf(stderr, "Can't access shared memory area. SHMConfig disabled?\n");
        else
            fprintf(stderr, "Incorrect size of shared memory area. Incompatible driver version?\n");
    } else if ((synshm = (SynapticsSHM*) shmat(shmid, NULL, SHM_RDONLY)) == NULL)
        perror("shmat");

    return synshm;
}

void press_key(Display *display, unsigned int keycode)
{
    XTestFakeKeyEvent(display, XKeysymToKeycode(display, keycode), True, 0);
}

void release_key(Display *display, unsigned int keycode)
{
    XTestFakeKeyEvent(display, XKeysymToKeycode(display, keycode), False, 0);
}

void send_key_event(Display *display, unsigned int keycode)
{
    press_key(display, keycode);
    release_key(display, keycode);
}

void do_gesture(int fingers, char direction)
{
#if defined(DEBUG)
    printf("gesture %d %c\n", fingers, direction);
#else
    Display *display = XOpenDisplay(0);
    if (display == NULL) {
        fprintf(stderr, "Failed to open display\n");
        return;
    }

    switch (fingers) {
    case 3:
        switch (direction) {
        case 'l':
            send_key_event(display, XF86XK_Launch6);
            break;
        case 'r':
            send_key_event(display, XF86XK_Launch7);
            break;
        case 'u':
            send_key_event(display, XF86XK_Launch8);
            break;
        case 'd':
            send_key_event(display, XF86XK_Launch9);
            break;
        }
        break;
    case 4:
        switch (direction) {
        case 'l':
            press_key(display, XK_Super_L);
            send_key_event(display, XF86XK_Launch6);
            release_key(display, XK_Super_L);
            break;
        case 'r':
            press_key(display, XK_Super_L);
            send_key_event(display, XF86XK_Launch7);
            release_key(display, XK_Super_L);
            break;
        case 'u':
            press_key(display, XK_Super_L);
            send_key_event(display, XF86XK_Launch8);
            release_key(display, XK_Super_L);
            break;
        case 'd':
            press_key(display, XK_Super_L);
            send_key_event(display, XF86XK_Launch9);
            release_key(display, XK_Super_L);
            break;
        }
        break;
    }

    XCloseDisplay(display);
#endif
}

int main()
{
    SynapticsSHM *synshm = NULL;
    synshm = shm_init();
    if (!synshm)
        return 1;

    int x_start;
    int y_start;
    int fingers = 0;
    int x_last;
    int y_last;

    while (1) {
        SynapticsSHM cur = *synshm;
#if defined(VERBOSE)
        printf("%d fingers down (was %d) at %d,%d\n", cur.numFingers, fingers, cur.x, cur.y);
#endif
        if (cur.numFingers > 2) {
            if (cur.numFingers > fingers) { // Start of gesture
                if (fingers >= 0) { // If accepting next gesture
                    x_start = cur.x;
                    y_start = cur.y;
                    fingers = cur.numFingers;
                    x_last = cur.x;
                    y_last = cur.y;
                }
            } else if (cur.numFingers == fingers) { // Update last to here
                x_last = cur.x;
                y_last = cur.y;
            }
        }
        if (cur.numFingers < fingers) { // Gesture released, analyze
            int dx = x_last - x_start;
            int dy = y_last - y_start;
#if defined(DEBUG)
            printf("%d finger moving from %d,%d to %d,%d | delta %d,%d\n",
                    fingers, x_start, y_start, x_last, y_last, dx, dy);
#endif
            dx >>= 8;
            if (dx < 0)
                dx++; // truncate towards 0 for negative shift divide
            dy >>= 8;
            if (dy < 0)
                dy++; // truncate towards 0 for negative shift divide

            if (!dy)
                if (dx > 0)
                    do_gesture(fingers, 'r');
                else if (dx < 0)
                    do_gesture(fingers, 'l');
            if (!dx)
                if (dy > 0)
                    do_gesture(fingers, 'd');
                else if (dy < 0)
                    do_gesture(fingers, 'u');
            fingers = -1;
        }
        if (cur.numFingers == 0) { // Everything released
            fingers = 0; // Set accept next gesture
        }
        usleep(50000);
    }

    return 0;
}

