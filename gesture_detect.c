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

    int i = 0;
    int x[4];
    int y[4];
    int z[4];
    int f[4];
    int w[4];
    int accept_next = 1;
    int last_time = 0;

    while (1) {
        SynapticsSHM cur = *synshm;
        i++;
        i &= 3;
        x[i] = cur.x;
        y[i] = cur.y;
        z[i] = cur.z;
        f[i] = cur.numFingers;
        w[i] = cur.fingerWidth;

        if (f[i] == 0)
            accept_next = 1;

        if (f[0] > 2 && f[0] == f[1] && f[1] == f[2] && accept_next) {
            int a = i;
            int b = (i-1)&3;
            int c = (i-2)&3;

            int x1 = x[a] - x[b];
            int x2 = x[b] - x[c];

            int y1 = y[a] - y[b];
            int y2 = y[b] - y[c];

#if defined(DEBUG)
            printf("X %d %d | Y %d %d\n", x1, x2, y1, y1);
#endif

            int x1h = x1 >> 6;
            if (x1h < 0) x1h++; // truncate towards 0
            int x2h = x2 >> 6;
            if (x2h < 0) x2h++; // truncate towards 0

            int x1v = x1 >> 6;
            if (x1v < 0) x1v++; // truncate towards 0
            int x2v = x2 >> 6;
            if (x2v < 0) x2v++; // truncate towards 0

            int y1h = y1 >> 6;
            if (y1h < 0) y1h++; // truncate towards 0
            int y2h = y2 >> 6;
            if (y2h < 0) y2h++; // truncate towards 0

            int y1v = y1 >> 6;
            if (y1v < 0) y1v++; // truncate towards 0
            int y2v = y2 >> 6;
            if (y2v < 0) y2v++; // truncate towards 0

            if (!y1h && !y2h) {
                if (x1h > 0 && x2h > 0)
                    do_gesture(f[0], 'r');
                    accept_next = 0;
                if (x1h < 0 && x2h < 0)
                    do_gesture(f[0], 'l');
                    accept_next = 0;
            }
            else if (!x1h && !x2h) {
                if (y1h > 0 && y2h > 0)
                    do_gesture(f[0], 'd');
                    accept_next = 0;
                if (y1h < 0 && y2h < 0)
                    do_gesture(f[0], 'u');
                    accept_next = 0;
            }
        }
        usleep(50000);
    }

    return 0;
}

