# ************************************ Message ***********************************#
from pyb import UART

uart = UART(2, 115200)  # 初始化串口 波特率 115200

class Receive(object):
    uart_buf = []
    _data_len = 0
    _data_cnt = 0
    state = 0

R = Receive()


def UartSendData(Data):
    uart.write(Data)

# 串口数据解析
def ReceiveAnl(data_buf, num):
    # 和校验
    sum = 0
    i = 0
    while i < (num):
        sum = sum + data_buf[i]
        i = i + 1
    sum = sum % 256  # 求余
    if sum != data_buf[num - 1]:
        return

# 串口通信协议接收
def ReceivePrepare(data):
    if R.state == 0:
        if data == 0xAA:  # 帧头
            R.uart_buf.append(data)
            R.state = 1
        else:
            R.state = 0
    elif R.state == 1:
        if data == 0xAF:
            R.uart_buf.append(data)
            R.state = 2
        else:
            R.state = 0
    elif R.state == 2:
        if data == 0x05:
            R.uart_buf.append(data)
            R.state = 3
        else:
            R.state = 0
    elif R.state == 3:
        if data == 0x02:  # 有效数据个数
            R.state = 4
            R.uart_buf.append(data)
            R._data_len = data
        else:
            R.state = 0
    elif R.state == 4:  # 模式选择 1/2/3 (WorkMode)
        if data == 1 or data == 2 or data == 3:
            R.uart_buf.append(data)
            R.state = 5
        else:
            R.state = 0
    elif R.state == 5:  # 自定义数据 (Workdata)
        if data == 1 or data == 2 or data == 3:
            R.uart_buf.append(data)
            R.state = 6
        else:
            R.state = 0
    elif R.state == 6:
        R.state = 0
        R.uart_buf.append(data)
        ReceiveAnl(R.uart_buf, 7)
        R.uart_buf = []  # 清空缓冲区，准备下次接收数据
    else:
        R.state = 0

# 读取串口缓存
def UartReadBuffer():
    i = 0
    Buffer_size = uart.any()
    while i < Buffer_size:
        ReceivePrepare(uart.readchar())
        i = i + 1

# 检测数据打包（检测分类）
def ObjectDataPackColor(flag, width, x, y, fps):
    pack_data = bytearray(
        [
            0xAA,
            0x29,
            0x42,
            0x00,
            flag,
            width >> 8,
            width,
            x >> 8,
            x,
            (-y) >> 8,
            (-y),
            fps,
            0x00,
        ]
    )
    lens = len(pack_data)  # 数据包大小
    pack_data[3] = 8
    # 有效数据个数
    i = 0
    sum = 0
    # 和校验
    while i < (lens - 1):
        sum = sum + pack_data[i]
        i = i + 1
    pack_data[lens - 1] = sum
    return pack_data


# 线检测数据打包
def LineDataPack(flag, angle, distance, crossflag, crossx, crossy, T_ms):
    if flag == 0:
        print(
            "found: angle", angle, "  distance=", distance, "   线状态   未检测到直线"
        )
    elif flag == 1:
        print("found: angle", angle, "  distance=", distance, "   线状态   直线")
    elif flag == 2:
        print("found: angle", angle, "  distance=", distance, "   线状态   左转")
    elif flag == 3:
        print("found: angle", angle, "  distance=", distance, "   线状态   右转")

    line_data = bytearray(
        [
            0xAA,
            0x29,
            0x43,
            0x00,
            flag,
            angle >> 8,
            angle,
            distance >> 8,
            distance,
            crossflag,
            crossx >> 8,
            crossx,
            (-crossy) >> 8,
            (-crossy),
            T_ms,
            0x00,
        ]
    )
    lens = len(line_data)  # 数据包大小
    line_data[3] = 11
    # 有效数据个数
    i = 0
    sum = 0
    # 和校验
    while i < (lens - 1):
        sum = sum + line_data[i]
        i = i + 1
    line_data[lens - 1] = sum
    return line_data

# 用户数据打包
def UserDataPack(data0, data1, data2, data3, data4, data5, data6, data7, data8, data9):
    UserData = bytearray(
        [
            0xAA,
            0x05,
            0xAF,
            0xF1,
            0x00,
            data0,
            data1,
            data2 >> 8,
            data2,
            data3 >> 8,
            data3,
            data4 >> 24,
            data4 >> 16,
            data4 >> 8,
            data4,
            data5 >> 24,
            data5 >> 16,
            data5 >> 8,
            data5,
            data6 >> 24,
            data6 >> 16,
            data6 >> 8,
            data6,
            data7 >> 24,
            data7 >> 16,
            data7 >> 8,
            data7,
            data8 >> 24,
            data8 >> 16,
            data8 >> 8,
            data8,
            data9 >> 24,
            data9 >> 16,
            data9 >> 8,
            data9,
            0x00,
        ]
    )
    lens = len(UserData)  # 数据包大小
    UserData[4] = lens - 6
    # 有效数据个数
    i = 0
    sum = 0
    # 和校验
    while i < (lens - 1):
        sum = sum + UserData[i]
        i = i + 1
    UserData[lens - 1] = sum
    return UserData

# ************************************ Message ***********************************#
