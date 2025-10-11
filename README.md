Ubuntu/Debian:
sudo apt-get install libopencv-dev 
#tesseract-ocr tesseract-ocr-eng tesseract-ocr-rus libtesseract-dev libleptonica-dev
easyocr

Windows (vcpkg):
vcpkg install opencv 
#tesseract
easyocr

python3 mjpeg_streamer.py
rm -f debug_* && ./plate_recognition http://127.0.0.1:8080/video
