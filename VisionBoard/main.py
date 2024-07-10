# Edge Impulse - OpenMV Image Classification Example
from pyb import LED
import sensor, image, time, os, tf, uos, gc,math, struct, lcd
import Message
sensor.reset()                         # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)    # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)      # Set frame size to QVGA (320x240)
sensor.set_windowing((240, 240))       # Set 240x240 window.
sensor.skip_frames(time=2000)          # Let the camera adjust.
sensor.set_auto_gain(False)     # 追踪颜色关闭自动增益
sensor.set_auto_whitebal(False) # 追踪颜色关闭白平衡
sensor.set_hmirror(True)
sensor.set_vflip(True)
net = None
labels = None

class object(object):
    flag = 0
    x = 0
    y = 0
    width = 0
    hight = 0
    fps = 0
object = object()
try:
    # load the model, alloc the model file on the heap if we have at least 64K free after loading
    net = tf.load("trained.tflite", load_to_fb=uos.stat('trained.tflite')[6] > (gc.mem_free() - (64*1024)))
except Exception as e:
    print(e)
    raise Exception('Failed to load "trained.tflite", did you copy the .tflite and labels.txt file onto the mass-storage device? (' + str(e) + ')')

try:
    labels = [line.rstrip('\n') for line in open("labels.txt")]
except Exception as e:
    raise Exception('Failed to load "labels.txt", did you copy the .tflite and labels.txt file onto the mass-storage device? (' + str(e) + ')')

clock = time.clock()
while(True):
    clock.tick()

    img = sensor.snapshot()

    # default settings just do one detection... change them to search the image...
    for obj in net.classify(img, min_scale=1.0, scale_mul=0.8, x_overlap=0.5, y_overlap=0.5):
        predictions_list = list(zip(labels, obj.output()))
        object.x = int(obj.rect()[0])
        object.y = int(obj.rect()[1])
        object.width = int(obj.rect()[2])
        if predictions_list[0][1] > 0.7:
            object.flag = int(predictions_list[0][0])
        elif predictions_list[1][1] > 0.7:
            object.flag = int(predictions_list[1][0])
        elif predictions_list[2][1] > 0.7:
            object.flag = int(predictions_list[2][0])
        else:
            object.flag = int(9)
        print(object.flag)
        print(predictions_list[0][1])
        print(predictions_list[1][1])
        print(predictions_list[2][1])
        object.fps = int(clock.fps())
    Message.UartSendData(
        Message.ObjectDataPackColor(
            object.flag, object.width, object.x, object.y, object.fps
        )
    )
    Message.UartReadBuffer()
