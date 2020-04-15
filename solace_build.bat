docker build -f Dockerfile.build -t solacedev .
docker run --rm -it -v C:\Users\sshanks\Development\solace\:/source/code solacedev /bin/bash -c "cd build && cmake ../ && make install"
