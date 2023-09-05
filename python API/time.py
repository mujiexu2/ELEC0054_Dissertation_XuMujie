import serial.tools.list_ports
import time
import csv

def extract_values(data_str):
    try:
        return float(data_str.split('=')[1].strip())
    except (IndexError, ValueError) as e:
        print(f"Error parsing data string: {data_str}. Error: {e}")
        return 0.0  # 或者返回其他默认值

plist = list(serial.tools.list_ports.comports())

if len(plist) <= 0:
    print("The Serial port can't find!")
else:
    print("find!")
    for i, divice in enumerate(plist):
        print(i+1, ':', divice)
    Num = int(input("please select which port you connect:"))
    serialName = plist[Num-1][0]         #选择设备
    print("connect to " + serialName)
    ser = serial.Serial(serialName, 9600, timeout=1)    #初始化串口

    # 使用当前时间来命名文件
    current_time = time.strftime("%Y%m%d_%H%M%S")
    file_name = f'data_{current_time}.csv'
    f = open(file_name, 'w', encoding='utf-8', newline='')  #最后一个参数用来消除空行
    csv_writer = csv.writer(f)
    
    # 写入列名
    headers = ['Epoch', 'UTC Time', 'UK Time', 'Elapsed Time', 'x_accel', 'y_accel', 'z_accel', 'x_gyro', 'y_gyro', 'z_gyro', 'x_mag', 'y_mag', 'z_mag']
    csv_writer.writerow(headers)
    
    i = 0
    while True:
        ser.read_until(b'#')
        timestamp_str = ser.read_until(b'&').strip(b'&').decode()
        timestamp = int(timestamp_str.replace("#", ""))
        utc_1 = ser.read_until(b'&').strip(b'&').decode()
        utc_2 = ser.read_until(b'&').strip(b'&').decode()
        time_values = ser.read_until(b'&').strip(b'&').decode()
        accel_data = ser.read_until(b'&').strip(b'&').decode().split('/')
        gyro_data = ser.read_until(b'&').strip(b'&').decode().split('/')
        mag_data = ser.read_until(b'&').strip(b'&').decode().split('/')

        # 解析加速度、陀螺仪和磁场数据
        x_accel, y_accel, z_accel = [extract_values(val) for val in accel_data]
        x_gyro, y_gyro, z_gyro = [extract_values(val) for val in gyro_data]
        x_mag, y_mag, z_mag = [extract_values(val) for val in mag_data]

        csv_writer.writerow([timestamp, utc_1, utc_2, time_values, x_accel, y_accel, z_accel, x_gyro, y_gyro, z_gyro, x_mag, y_mag, z_mag])
        f.flush()  # 确保数据写入文件       
        print([timestamp, utc_1, utc_2, time_values, x_accel, y_accel, z_accel, x_gyro, y_gyro, z_gyro, x_mag, y_mag, z_mag])

        # 清空缓冲区，为下一次数据接收做准备
        ser.flush()
