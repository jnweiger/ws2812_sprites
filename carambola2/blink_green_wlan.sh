while true; do
  echo 0 > /sys/class/leds/carambola2\:green\:wlan/brightness
  sleep 2
  echo 1 > /sys/class/leds/carambola2\:green\:wlan/brightness
  sleep 1
done
