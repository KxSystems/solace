docker build -f Dockerfile.run -t solaceclient:latest .
docker run -it --rm -h lptp663 --name solaceq solaceclient:latest
