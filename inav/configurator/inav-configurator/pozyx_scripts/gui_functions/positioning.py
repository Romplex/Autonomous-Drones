from pypozyx import PozyxSerial, DeviceCoordinates, Coordinates, PozyxConstants, \
    POZYX_FAILURE
from pypozyx.definitions.constants import POZYX_SUCCESS
from functools import wraps

from gui_functions.serial_ports import get_pozyx_serial_port

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
remote_id = 0x6748

POZYX_CONNECTED_TO_BASE = True

# shortcut to not have to find out the port yourself
serial_port = get_pozyx_serial_port()
if serial_port is None:
    POZYX_CONNECTED_TO_BASE = False
else:
    pozyx = PozyxSerial(serial_port)

MAX_TRIES = 20


def check_connection(func):
    @wraps(func)
    def check():
        if not POZYX_CONNECTED_TO_BASE:
            return {'error': 'No pozyx device connected! check usb connection.'}
        return func()
    return check


@check_connection
def get_remote_position():
    """Send position data to configurator"""

    # set anchors
    status = pozyx.clearDevices()
    for anchor in anchors:
        status &= pozyx.addDevice(anchor)

    # start positioning
    tries = MAX_TRIES
    while tries > 0:
        position = Coordinates()
        status = pozyx.doPositioning(position, dimension, height, algorithm)
        if status == POZYX_SUCCESS:
            return {
                'x': position.x,
                'y': position.y,
                'z': position.z
            }
        tries -= 1
    return {'error': 'At least one anchor inactive! Check connection to power supply.'}


if __name__ == '__main__':
    for i in range(10):
        x = get_remote_position()
        print(x)
