# build ws2812_sprites executable when user executes "make"
ws2812_sprites: ws2812_sprites.o libmmio.o
	$(CC) $(LDFLAGS) ws2812_sprites.o libmmio.o -o ws2812_sprites
ws2812_sprites.o: ws2812_sprites.c mmio.h
	$(CC) $(CFLAGS) -c ws2812_sprites.c
libmmio.o: libmmio.c mmio.h utils.h compiler.h
	$(CC) $(CFLAGS) -c libmmio.c

# remove object files and executable when user executes "make clean"
clean:
	rm *.o ws2812_sprites
