from pypozyx import PozyxSerial, get_first_pozyx_serial_port, DeviceCoordinates, Coordinates, PozyxConstants, \
    SingleRegister, Data, DeviceList
from pypozyx.definitions.constants import POZYX_SUCCESS
from pypozyx.definitions.bitmasks import POZYX_INT_STATUS_RX_DATA


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


def send_test_message(receiver,msg): #reveiver needs to be a int Network id 0x0000
    while True:
        if receiver == None:
            status = pozyx.sendData(0, msg.encode('utf-8'))
        else:
            status = pozyx.sendData(receiver, msg.encode('utf-8'))

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

def get_received_message():

    if (pozyx.waitForFlag(POZYX_INT_STATUS_RX_DATA, 1)):
        l = SingleRegister()
        pozyx.getRead(address=0x84, data=l)  # laenge
        d = Data(([0] * l.value), 'B' * l.value)  # container mit laenge X listen jede im Byte format
        pozyx.readRXBufferData(data=d)
        for i in range(0, l.value - 1):
            print(chr(d.__getitem__(i)))  # do something with every char
        return d

def get_device_list():
    """returns array with all devices stored"""
    pozyx.doDiscovery(discovery_type=PozyxConstants.DISCOVERY_ALL_DEVICES)
    list_size = SingleRegister()
    pozyx.getDeviceListSize(list_size)
    device_list = DeviceList(list_size=list_size[0])
    pozyx.getDeviceIds(device_list)
    return device_list

def get_tags_list():
    pozyx.doDiscovery(discovery_type=PozyxConstants.DISCOVERY_ALL_DEVICES)
    list_size = SingleRegister()
    pozyx.getDeviceListSize(list_size)
    device_list = get_d
    pozyx.getDeviceIds(device_list)

    print(list_size[0])
    tag_list = DeviceList(list_size=list_size[0] - 4)

    for i in range(0, list_size[0] - 4):
        tag_list[i] = device_list[i + 4]

if __name__ == '__main__':
    get_remote_position()
    send_test_message("XD")


