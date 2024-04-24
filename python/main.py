import serial
import uinput
from time import sleep

ser = serial.Serial('/dev/ttyACM0', 115200)
# ser = serial.Serial('/dev/rfcomm0', 115200)

# Create new keyboard device
device = uinput.Device([
    uinput.KEY_LEFT,
    uinput.KEY_RIGHT,
    uinput.KEY_UP,
    uinput.KEY_A,
    uinput.KEY_D,
    uinput.KEY_W,
])


def parse_data(data):
    axis = data[0]  
    value = int.from_bytes(data[1:3], byteorder='little', signed=True)
    print(f"Received data: {data}")
    print(f"axis: {axis}, value: {value}")
    return axis, value

def press_key(axis, value):
    if value == 1:
        if value +10*axis == 1:
            device.emit(uinput.KEY_A, 1)
            sleep(0.1)
            device.emit(uinput.KEY_A, 0)
        
        if value +10*axis == 11:
            device.emit(uinput.KEY_LEFT, 1)
            
            device.emit(uinput.KEY_LEFT, 0)
    
    if value == 2:
        if value +10*axis == 2:
            device.emit(uinput.KEY_D, 1)
            sleep(0.1)
            device.emit(uinput.KEY_D, 0)
        if value +10*axis == 12:
            device.emit(uinput.KEY_RIGHT, 1)
            sleep(0.1)
            device.emit(uinput.KEY_RIGHT, 0)
    
    #botao vermelho pula
    if value == 3:
        if value +10*axis == 3:
            device.emit(uinput.KEY_W, 1)
            sleep(0.1)
            device.emit(uinput.KEY_W, 0)
        
        if value +10*axis == 13:
            device.emit(uinput.KEY_UP, 1)
            sleep(0.1)
            device.emit(uinput.KEY_UP, 0)


# def move_mouse(axis, value):
#     if axis == 0:    # X-axis
#         device.emit(uinput.REL_X, value)
#     elif axis == 1:  # Y-axis
#         device.emit(uinput.REL_Y, value)

def click_button(button):
    if button == 0:
        device.emit(uinput.BTN_LEFT, 1)
        device.emit(uinput.BTN_LEFT, 0)
    elif button == 1:
        device.emit(uinput.BTN_RIGHT, 1)
        device.emit(uinput.BTN_RIGHT, 0)

try:
    # sync package
    while True:
        print('Waiting for sync package...')
        while True:
            data = ser.read(1)
            if data == b'\xff':
                break

        # Read 4 bytes from UART
        data = ser.read(3)
        axis, value = parse_data(data)
        press_key(axis, value)

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
