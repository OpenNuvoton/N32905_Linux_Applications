./aplay -t raw -c 2 -f S16_LE -r 8000 8k.pcm

./arecord -D plughw:1,0 -d 100 -c 1 -f S16_LE -r 8000 -t raw foobar.raw
./amixer -c 1 set PCM 60%
