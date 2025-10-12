#!/usr/bin/env python3
import cv2
from http.server import HTTPServer, BaseHTTPRequestHandler
import time

class VideoHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/video':
            print(f"🎥 Client connected: {self.client_address[0]}")
            self.send_response(200)
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=--myboundary')
            self.send_header('Connection', 'keep-alive')
            self.send_header('Cache-Control', 'no-cache')
            self.end_headers()
            
            cap = cv2.VideoCapture('ru1_mjpeg_cut.mp4')
            fps = cap.get(cv2.CAP_PROP_FPS) or 10
            frame_delay = 1.0 / fps
            
            try:
                while True:
                    ret, frame = cap.read()
                    if not ret:
                        # Перезапускаем видео
                        cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                        continue
                    
                    # Кодируем в JPEG
                    ret, jpeg = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
                    
                    if ret:
                        try:
                            self.wfile.write(b"\r\n--myboundary\r\n")
                            self.wfile.write(b"Content-Type: image/jpeg\r\n\r\n")
                            self.wfile.write(jpeg.tobytes())
                            self.wfile.write(b"\r\n")
                        except BrokenPipeError:
                            print("🔌 Client disconnected")
                            break
                    
            except Exception as e:
                print(f"❌ Error: {e}")
            finally:
                cap.release()
                
        else:
            self.send_error(404)
    
    def log_message(self, format, *args):
        pass

print("🚀 Starting OpenCV MJPEG Stream Server...")
print("📹 Stream URL: http://127.0.0.1:8080/video")
print("⏹️  Press Ctrl+C to stop")

try:
    server = HTTPServer(('0.0.0.0', 8080), VideoHandler)
    server.serve_forever()
except KeyboardInterrupt:
    print("\n🛑 Server stopped")
