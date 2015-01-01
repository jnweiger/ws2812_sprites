# build ws2812-tiny-vncview executable when user executes "make"
ws2812-tiny-vncview: ws2812-tiny-vncview.o tiny_vncview.o
	$(CC) $(LDFLAGS) ws2812-tiny-vncview.o tiny_vncview.o -o ws2812-tiny-vncview
ws2812-tiny-vncview.o: ws2812-tiny-vncview.c tiny_vncview.h
	$(CC) $(CFLAGS) -c ws2812-tiny-vncview.c
tiny_vncview.o: tiny_vncview.c tiny_vncview.h
	$(CC) $(CFLAGS) -c tiny_vncview.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o ws2812-tiny-vncview
