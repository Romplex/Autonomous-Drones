from pypozyx import PozyxSerial, DeviceCoordinates, Coordinates, PozyxConstants, \
    SingleRegister, Data
from pypozyx.definitions.bitmasks import POZYX_INT_STATUS_RX_DATA
from threading import Thread

from gui_functions.serial_ports import get_pozyx_serial_port


# necessary data for calibration
anchors = [DeviceCoordinates(0x6951, 1, Coordinates(0, 0, 1500)),
           DeviceCoordinates(0x6e59, 2, Coordinates(5340, 0, 2000)),
           DeviceCoordinates(0x695d, 3, Coordinates(6812, -8923, 2500)),
           DeviceCoordinates(0x690b, 4, Coordinates(-541, -10979, 3000)),
           DeviceCoordinates(0x6748, 5, Coordinates(6812, -4581, 200))]

# positioning algorithm to use
algorithm = PozyxConstants.POSITIONING_ALGORITHM_UWB_ONLY

# positioning dimension. Others are PozyxConstants.DIMENSION_2D, PozyxConstants.DIMENSION_2_5D
dimension = PozyxConstants.DIMENSION_3D

# height of device, required in 2.5D positioning
height = 1000

# pozyx tag on drone
remote_id = 0

POZYX_CONNECTED_TO_BASE = True

# shortcut to not have to find out the port yourself
serial_port = get_pozyx_serial_port()
if serial_port is None:
    POZYX_CONNECTED_TO_BASE = False
else:
    pozyx = PozyxSerial(serial_port)
MAX_TRIES = 20


def communication():
    if POZYX_CONNECTED_TO_BASE:
        Thread(target=get_rx_data).start()
        while True:
            msg = input()
            send_tx_data(msg)
    else:
        print('Not Connected!')


def get_rx_data():
    while True:
        if pozyx.waitForFlag(POZYX_INT_STATUS_RX_DATA, 1):
            data_length = SingleRegister()
            pozyx.getLastDataLength(data_length)
            received_data = Data(([0] * data_length.value), 'B' * data_length.value)
            pozyx.readRXBufferData(received_data)
            msg = ''
            for i in range(data_length.value):
                msg += chr(received_data.data[i])
            print('received: ' + msg)


def send_tx_data(msg):
    pozyx.sendData(remote_id, msg.encode())
    print('sent: ' + msg)


if __name__ == '__main__':
    communication()
