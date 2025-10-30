from flask import Flask, request, jsonify
import cv2
import numpy as np
import base64
import logging
import time
import threading
import sys
import os
import torch

current_dir = os.getcwd()
anprsystem_path = os.path.join(current_dir, "ANPR-System")
sys.path.append(anprsystem_path)
from inference import Config, YOLODetector, CRNNRecognizer, ANPR_Pipeline, Visualizer


# Настройка логирования
logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s.%(msecs)03d] %(levelname)s: %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

# Глобальная инициализация EasyOCR (один раз при запуске)
logger.info("Initializing...")
Config.YOLO_MODEL_PATH = os.path.join(anprsystem_path, Config.YOLO_MODEL_PATH)
Config.OCR_MODEL_PATH = os.path.join(anprsystem_path, Config.OCR_MODEL_PATH)
Config.DEVICE = torch.device("cpu")  # Явно указываем device

detector = YOLODetector(Config.YOLO_MODEL_PATH, Config.DEVICE)
recognizer = CRNNRecognizer(Config.OCR_MODEL_PATH, Config.DEVICE)
pipeline = ANPR_Pipeline(recognizer)
logger.info("initialized successfully!")

# Счетчик запросов для мониторинга
request_counter = 0
counter_lock = threading.Lock()

def process_image(image_data, current_request):
    """Обрабатывает изображение и возвращает распознанный текст"""
    try:
        nparr = np.frombuffer(image_data, np.uint8)
        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if img is None:
            return {"error": "Failed to decode image"}
        
        # Распознаем текст
        detections = detector.track(img) # для нескольких кадров
        results = pipeline.process_frame(img, detections)
        print("result:", results);
        
        # Форматируем результаты
        plates = []
        for res in results:
            text = res.get('text', '').strip()
            confidence = res.get('confidence', 0.0)
            plates.append({
                'text': text.strip(),
                'confidence': confidence,
            })
            
            # Логируем каждый распознанный текст
            logger.info(f"[Request #{current_request}]: '{text}' (confidence: {confidence:.3f})")
        
        return {'plates': plates}
        
    except Exception as e:
        logger.error(f"Error processing image: {str(e)}")
        return {"error": str(e)}

@app.route('/recognize', methods=['POST'])
def recognize_plate():
    start_time = time.time()
    global request_counter
    
    with counter_lock:
        request_counter += 1
        current_request = request_counter
    
    start_time = time.time()
    logger.info(f"[Request #{current_request}]: Processing started")
    
    try:
        t1 = time.time()
        image_data = request.get_data()
        if len(image_data) == 0:
            logger.warning(f"[Request #{current_request}]: Empty request")
            return jsonify({'error': 'No image data provided'}), 400

        t2 = time.time()
        logger.info(f"[Request #{current_request}]: Receive: {(t2-t1):.3f}s")

        # Обрабатываем изображение
        result = process_image(image_data, request_counter)
        t3 = time.time()
        logger.info(f"[Request #{current_request}]: Process: {(t3-t2):.3f}s")
                
        if 'error' in result:
            return jsonify(result), 500
        else:
            return jsonify(result)
            
    except Exception as e:
        processing_time = (datetime.now() - start_time).total_seconds()
        logger.error(f"[Request #{current_request}]: Error after {processing_time:.3f}s: {str(e)}")
        return jsonify({'error': 'Internal server error'}), 500

@app.route('/health', methods=['GET'])
def health_check():
    """Проверка статуса сервера"""
    return jsonify({
        'status': 'healthy',
        'easyocr_initialized': True,
        'total_requests_processed': request_counter
    })

@app.route('/stats', methods=['GET'])
def get_stats():
    """Статистика сервера"""
    return jsonify({
        'total_requests': request_counter,
        'easyocr_languages': ['ru', 'en']
    })

if __name__ == '__main__':
    logger.info("Starting OCR Server on http://127.0.0.1:5000")
    logger.info("Available endpoints:")
    logger.info("  POST /recognize - recognize text in image")
    logger.info("  GET  /health    - health check")
    logger.info("  GET  /stats     - server statistics")

    from waitress import serve
    logger.info("Starting production server on http://127.0.0.1:5000")
    serve(app, host='127.0.0.1', port=5000, threads=8)
