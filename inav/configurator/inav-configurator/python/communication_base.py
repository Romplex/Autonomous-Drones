from pypozyx import PozyxSerial, get_first_pozyx_serial_port, DeviceCoordinates, Coordinates, PozyxConstants, \
    POZYX_FAILURE
from pypozyx.definitions.constants import POZYX_SUCCESS


# necessary data for calibration
anchors = [DeviceCoordinates(0x6951, 1, Coordinates(0, 0, 1500)),
           DeviceCoordinates(0x6e59, 2, Coordinates(5340, 0, 2000)),
           DeviceCoordinates(0x695d, 3, Coordinates(6812, -8923, 2500)),
           DeviceCoordinates(0x690b, 4, Coordinates(-541, -10979, 3000))]

# positioning algorithm to use
algorithm = PozyxConstants.POSITIONING_ALGORITHM_UWB_ONLY

# positioning dimension. Others are PozyxConstants.DIMENSION_2D, PozyxConstants.DIMENSION_2_5D
dimension = PozyxConstants.DIMENSION_3D

# height of device, required in 2.5D positioning
height = 1000

# pozyx tag on drone
remote_id = 0x6760

POZYX_CONNECTED_TO_BASE = True

# shortcut to not have to find out the port yourself
serial_port = get_first_pozyx_serial_port()
if serial_port is None:
    POZYX_CONNECTED_TO_BASE = False
else:
    pozyx = PozyxSerial(serial_port)

MAX_TRIES = 20


def send_test_message(msg):
    if not POZYX_CONNECTED_TO_BASE:
        return {
            'error_msg': 'No Pozyx connected. Check your USB cable or your driver!'
        }
    while True:
        tries = 0
        while True:
            status = pozyx.sendData(0, msg.encode('utf-8'))
            tries += 1
            if status == POZYX_SUCCESS or tries == MAX_TRIES:
                break


def get_remote_position():
    """Send position data to configurator"""

    if not POZYX_CONNECTED_TO_BASE:
        return {
            'error_msg': 'No Pozyx connected. Check your USB cable or your driver!'
        }

    # set anchors
    status = pozyx.clearDevices(remote_id=remote_id)
    for anchor in anchors:
        status &= pozyx.addDevice(anchor, remote_id=remote_id)

    if status == POZYX_FAILURE:
        return {
            'error_msg': 'At least one anchor inactive. Check power supply!'
        }

    # start positioning
    while True:
        position = Coordinates()
        status = pozyx.doPositioning(position, dimension, height, algorithm, remote_id=remote_id)
        if status == POZYX_SUCCESS:
            print(position)
            return {
                'x': position.x,
                'y': position.y,
                'z': position.z
            }


if __name__ == '__main__':
    get_remote_position()
    # send_test_message("msg ")


