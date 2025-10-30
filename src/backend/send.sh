#!/bin/bash

IMAGE_FILE=$1
SERVER="http://127.0.0.1:5000"

if [ -z "$IMAGE_FILE" ]; then
    echo "Usage: $0 <image_file>"
    exit 1
fi

# Отправляем запрос
curl -X POST $SERVER/recognize \
  -H "Content-Type: application/json" \
  --data-binary @"$IMAGE_FILE"
