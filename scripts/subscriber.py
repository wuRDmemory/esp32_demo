import os
import requests
import cv2

if __name__ == '__main__':
    # # Send a GET request to the URL
    # response = requests.get('https://192.168.1.6:80/stream', headers={'Host': 'test.com'}, verify=False, stream=True)
    # # Print the response
    # print(response.text)
    cap = cv2.VideoCapture('http://192.168.1.6:80/stream')
    print(cap.isOpened())
    while cap.isOpened():
        ret, frame = cap.read()
        if ret:
            cv2.imshow('frame', frame)
            if cv2.waitKey(1) == ord('q'):
                break
        else:
            break