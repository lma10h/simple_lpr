import cv2
import os

# Открываем видео
video_path = "your_video.mp4"  # укажи путь к своему видео
cap = cv2.VideoCapture(video_path)

# Получаем FPS видео
fps = cap.get(cv2.CAP_PROP_FPS)
print(f"FPS видео: {fps}")

# Создаем папку для фреймов
output_dir = "frames"
os.makedirs(output_dir, exist_ok=True)

frame_count = 0
saved_count = 0

while True:
    ret, frame = cap.read()
    if not ret:
        break
    
    # Сохраняем каждый N-ый фрейм, где N = FPS (1 кадр в секунду)
    if frame_count % int(fps) == 0:
        filename = f"{output_dir}/frame_{saved_count:06d}.jpg"
        cv2.imwrite(filename, frame)
        saved_count += 1
        print(f"Сохранен кадр: {filename}")
    
    frame_count += 1

cap.release()
print(f"Готово! Сохранено {saved_count} кадров из {frame_count}")
