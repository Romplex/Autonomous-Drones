from pypozyx import PozyxSerial, get_first_pozyx_serial_port, DeviceCoordinates, Coordinates, POZYX_SUCCESS, \
    PozyxConstants, SingleRegister, POZYX_FAILURE

# positioning algorithm to use
from pypozyx.tools.version_check import perform_latest_version_check

algorithm = PozyxConstants.POSITIONING_ALGORITHM_UWB_ONLY

# positioning dimension. Others are PozyxConstants.DIMENSION_2D, PozyxConstants.DIMENSION_2_5D
dimension = PozyxConstants.DIMENSION_3D

# height of device, required in 2.5D positioning
height = 1000

# necessary data for calibration
anchors = [DeviceCoordinates(0x6951, 1, Coordinates(0, 0, 1500)),
           DeviceCoordinates(0x6e59, 2, Coordinates(5340, 0, 2000)),
           DeviceCoordinates(0x695d, 3, Coordinates(6812, -8923, 2500)),
           DeviceCoordinates(0x690b, 4, Coordinates(-541, -10979, 3000))]

# pozyx tag on drone
remote_id = 0x6760

# shortcut to not have to find out the port yourself
serial_port = get_first_pozyx_serial_port()
if serial_port is None:
    print('No Pozyx connected. Check your USB cable or your driver!')
    quit()

pozyx = PozyxSerial(serial_port)


def get_remote_position():
    """Send position data to configurator"""

    # set anchors
    status = pozyx.clearDevices()
    for anchor in anchors:
        status &= pozyx.addDevice(anchor)

    # start positioning
    while True:
        position = Coordinates()
        status = pozyx.doPositioning(position, dimension, height, algorithm)
        if status == POZYX_SUCCESS:
            print(position)
            return {
                'x': position.x,
                'y': position.y,
                'z': position.z
            }


if __name__ == '__main__':
    while True:
        get_remote_position()
