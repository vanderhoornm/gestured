all:
	gcc -o gestured gesture_detect.c -lX11 -lXtst

debug:
	gcc -o gestured gesture_detect.c -lX11 -lXtst -DDEBUG

verbose:
	gcc -o gestured gesture_detect.c -lX11 -lXtst -DDEBUG -DVERBOSE

clean:
	rm gestured

install:
	cp gestured ~/.kde4/Autostart/

uninstall:
	rm ~/.kde4/Autostart/gestured
