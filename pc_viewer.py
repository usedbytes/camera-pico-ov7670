import numpy as np
from PIL import Image
from matplotlib import pyplot as plt
import serial
import cv2

ser = serial.Serial('/dev/ttyACM0')

fig = plt.figure(figsize=(1, 2))
viewer = fig.add_subplot(111)
plt.ion() # Turns interactive mode on (probably unnecessary)
fig.show() # Initially shows the figure
net = np.arange(0, 64, 8, dtype=int)
while True:
    
    data = ser.readline().decode("utf-8")
    if data[0] != '[':
        continue

    output = ser.readline().decode("utf-8")
    if output[0] != '{':
        continue

    header = data[:15]
    img = data[16:]
    res = list(map(int, output[16:].split(" ")))
    res = np.reshape(res, [8, 8])
    bitmap = list(map(int, img.split(" ")))

    # print(header)
    xdim = int(header[1:4])
    ydim = int(header[5:8])
    # print("{}x{}".format(xdim, ydim))
    # print(res)
    im = Image.new("L",(xdim,ydim))

    frame = np.reshape(np.asarray(bitmap, np.uint8), (xdim, ydim))
    frame = np.pad(frame, [(0, 4), (0, 4)], mode='constant', constant_values=0)
    frame_img = np.reshape(frame, [64, 64, 1])

    for s in range(8):
        for e in range(8):
            if res[e, s] > 0:
                overlay = frame_img.copy()
                cv2.rectangle(overlay,
                                (net[s], net[e]),
                                (net[s + 1], net[e + 1]),
                                (255, 255, 0),
                                -1)  # A filled rectangle
                alpha = 0.5  # Transparency factor.
                # Following line overlays transparent rectangle over the image
                frame_img = cv2.addWeighted(overlay, alpha, frame_img, 1 - alpha, 0)
                
    cv2.imshow('frame', frame_img)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

    # viewer.clear() # Clears the previous image
    # viewer.imshow(im, cmap='gray', vmin=0, vmax=255) # Loads the new image
    # plt.pause(.1) # Delay in seconds
    # fig.canvas.draw() # Draws the image to the screen

