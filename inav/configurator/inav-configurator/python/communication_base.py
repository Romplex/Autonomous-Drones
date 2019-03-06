from pypozyx import PozyxSerial, get_first_pozyx_serial_port, DeviceCoordinates, Coordinates, PozyxConstants
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

# shortcut to not have to find out the port yourself
serial_port = get_first_pozyx_serial_port()
if serial_port is None:
    print('No Pozyx connected. Check your USB cable or your driver!')
    quit()

pozyx = PozyxSerial(serial_port)


def send_test_message(msg):
    while True:
        status = pozyx.sendData(0x6760, msg.encode('utf-8'))
        if status == POZYX_SUCCESS:
            break


def get_remote_position():
    """Send position data to configurator"""

    # set anchors
    status = pozyx.clearDevices(remote_id=remote_id)
    for anchor in anchors:
        status &= pozyx.addDevice(anchor, remote_id=remote_id)

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
    send_test_message("XD")


