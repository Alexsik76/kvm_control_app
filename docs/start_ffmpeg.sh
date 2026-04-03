/home/alex/kvm_engine_py/kvm_engine | ffmpeg \
    -loglevel warning \
    -f h264 \
    -r 25 \
    -i pipe:0 \
    -c:v copy \
    -bsf:v "setts=pts=N/25/TB:dts=N/25/TB" \
    -rtsp_transport tcp \
    -f rtsp \
    rtsp://admin:password@localhost:8554/kvm