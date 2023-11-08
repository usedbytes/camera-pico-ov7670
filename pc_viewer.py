import numpy as np
from PIL import Image
from matplotlib import pyplot as plt
import serial

ser = serial.Serial('/dev/ttyACM0')

fig = plt.figure(figsize=(1, 2))
viewer = fig.add_subplot(111)
plt.ion() # Turns interactive mode on (probably unnecessary)
fig.show() # Initially shows the figure

while True:
    
    data = ser.readline().decode("utf-8")
    if data[0] != '[':
        continue

    header = data[:15]
    img = data[16:]
    bitmap = list(map(int, img.split(" ")))
    print(header)
    xdim = int(header[1:4])
    ydim = int(header[5:8])
    print("{}x{}".format(xdim, ydim))
    im = Image.new("L",(xdim,ydim))
    im.putdata(bitmap)
    viewer.clear() # Clears the previous image
    viewer.imshow(im, cmap='gray', vmin=0, vmax=255) # Loads the new image
    plt.pause(.1) # Delay in seconds
    fig.canvas.draw() # Draws the image to the screen

