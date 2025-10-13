# ocr_server.py
from flask import Flask, request, jsonify
import easyocr
import cv2
import numpy as np
import base64
import logging
from datetime import datetime
import threading

# Настройка логирования
logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(levelname)s: %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

# Глобальная инициализация EasyOCR (один раз при запуске)
logger.info("Initializing EasyOCR...")
reader = easyocr.Reader(['ru'])  # русский
logger.info("EasyOCR initialized successfully!")

# Счетчик запросов для мониторинга
request_counter = 0
counter_lock = threading.Lock()

def process_image(image_data):
    """Обрабатывает изображение и возвращает распознанный текст"""
    try:
        # Декодируем base64
        nparr = np.frombuffer(image_data, np.uint8)
        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if img is None:
            return {"error": "Failed to decode image"}
        
        # Grayscale + Masking
        img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

        allowlist_chars = '0123456789АВЕКМНОРСТУХ'
        # Распознаем текст
        results = reader.readtext(img, allowlist=allowlist_chars)
        
        # Форматируем результаты
        plates = []
        for bbox, text, confidence in results:
            plates.append({
                'text': text.strip(),
                'confidence': float(confidence),
                'bbox': [[float(x), float(y)] for x, y in bbox]
            })
            
            # Логируем каждый распознанный текст
            logger.info(f"Recognized: '{text}' (confidence: {confidence:.3f})")
        
        return {'plates': plates}
        
    except Exception as e:
        logger.error(f"Error processing image: {str(e)}")
        return {"error": str(e)}

@app.route('/recognize', methods=['POST'])
def recognize_plate():
    global request_counter
    
    with counter_lock:
        request_counter += 1
        current_request = request_counter
    
    start_time = datetime.now()
    logger.info(f"[Request #{current_request}] Processing started at {start_time.strftime('%H:%M:%S.%f')[:-3]}")
    
    try:
        data = request.get_json()
        if not data or 'image' not in data:
            logger.warning(f"[Request #{current_request}] No image data in request")
            return jsonify({'error': 'No image data provided'}), 400
        
        # Декодируем base64
        image_data = base64.b64decode(data['image'])
        logger.info(f"[Request #{current_request}] Image received: {len(image_data)} bytes")
        
        # Обрабатываем изображение
        result = process_image(image_data)
        
        processing_time = (datetime.now() - start_time).total_seconds()
        logger.info(f"[Request #{current_request}] Processing completed in {processing_time:.3f}s")
        
        if 'error' in result:
            return jsonify(result), 500
        else:
            return jsonify(result)
            
    except Exception as e:
        processing_time = (datetime.now() - start_time).total_seconds()
        logger.error(f"[Request #{current_request}] Error after {processing_time:.3f}s: {str(e)}")
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
    
    # Запускаем сервер
    app.run(
        host='127.0.0.1', 
        port=5000, 
        debug=False,  # False для production
        threaded=True  # Обрабатываем запросы параллельно
    )
