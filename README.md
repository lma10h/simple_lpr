Ubuntu/Debian:
sudo apt-get install libopencv-dev 
python3-dev python3-pip
easyocr

# раздаем видео поток
python3 mjpeg_streamer.py
# запускаем сервис распознавания
python3 ../ocr_server.py
# запускаем фронт LPR
./plate_recognition http://127.0.0.1:8080/video
