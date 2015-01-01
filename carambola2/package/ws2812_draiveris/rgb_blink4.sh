insmod ws2812-draiveris gpios=7 inverted=1

while true; do
  printf ' \0\0\0 \0\0\0 \1\1\1' > /dev/ws2812 
  sleep 1 
  printf '\1\1\1 \0\0\0 \0\0\0 ' > /dev/ws2812 
  sleep 1
done

