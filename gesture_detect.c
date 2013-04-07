/*
Gesture Daemon - Detects touchpad gestures and generates key events. uses synaptics shared memory
Copyright (C) 2013  Maurits van der Hoorn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

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

void do_gesture(int fingers, int direction)
{
#if defined(DEBUG)
    printf("gesture %d %d\n", fingers, direction);
#else
    Display *display = XOpenDisplay(0);
    if (display == NULL) {
        fprintf(stderr, "Failed to open display\n");
        return;
    }

    if (fingers == 4)
        press_key(display, XK_Super_L);

    switch (direction) {
    case -4: // NW
        press_key(display, XK_Control_L);
        send_key_event(display, XF86XK_Launch6);
        release_key(display, XK_Control_L);
        break;
    case -3: // N
        send_key_event(display, XF86XK_Launch8);
        break;
    case -2: // NE
        press_key(display, XK_Control_L);
        send_key_event(display, XF86XK_Launch8);
        release_key(display, XK_Control_L);
        break;
    case -1: // W
        send_key_event(display, XF86XK_Launch6);
        break;
    case 0: // 0
        break;
    case 1: // E
        send_key_event(display, XF86XK_Launch7);
        break;
    case 2: // SW
        press_key(display, XK_Control_L);
        send_key_event(display, XF86XK_Launch9);
        release_key(display, XK_Control_L);
        break;
    case 3: // S
        send_key_event(display, XF86XK_Launch9);
        break;
    case 4: // SE
        press_key(display, XK_Control_L);
        send_key_event(display, XF86XK_Launch7);
        release_key(display, XK_Control_L);
        break;
    }

    if (fingers == 4)
        release_key(display, XK_Super_L);

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
            int direction;
#if defined(DEBUG)
            printf("%d finger moving from %d,%d to %d,%d | delta %d,%d\n",
                    fingers, x_start, y_start, x_last, y_last, dx, dy);
#endif
            if (dy < -256)
                direction = -3;
            else if (dy > 256)
                direction = 3;
            else
                direction = 0;

            if (dx < -256)
                direction--;
            else if (dx > 256)
                direction++;

            do_gesture(fingers, direction);

            fingers = -1;
        }
        if (cur.numFingers == 0) { // Everything released
            fingers = 0; // Set accept next gesture
        }
        usleep(50000);
    }

    return 0;
}

