@echo off
echo [INFO] Starting FFmpeg UDP test stream...
echo [INFO] Resolution: 1280x720 @ 60 FPS
echo [INFO] Codec: H.264 (libx264), Ultrafast, Zero-Latency
echo [INFO] Keyframe Interval: 60 (1 IDR per second)
echo [INFO] Destination: udp://127.0.0.1:5000
echo.

ffmpeg -re -f lavfi -i testsrc=size=1280x720:rate=60 -pix_fmt yuv420p -c:v libx264 -preset ultrafast -tune zerolatency -profile:v baseline -x264-params "keyint=30:slice-max-size=1200" -bsf:v dump_extra -f h264 "udp://127.0.0.1:5000"

pause