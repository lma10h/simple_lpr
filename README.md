Ubuntu/Debian:
sudo apt-get install libopencv-dev 
python3-dev python3-pip
easyocr

Windows (vcpkg):
vcpkg install opencv 
#tesseract
easyocr

python3 mjpeg_streamer.py
rm -f debug_* && ./plate_recognition http://127.0.0.1:8080/video
