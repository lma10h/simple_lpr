# Ubuntu/Debian:
## имитируем камеру
python3 mjpeg_streamer.py
## запускаем детекцию и распознавание
python3 ../ocr_server.py
## запускаем фронт LPR
./plate_recognition http://127.0.0.1:8080/video
