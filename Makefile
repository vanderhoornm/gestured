all:
	gcc -o gestured gesture_detect.c -lX11 -lXtst

debug:
	gcc -o gestured gesture_detect.c -lX11 -lXtst -DDEBUG

clear:
	rm gestured
