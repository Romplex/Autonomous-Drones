from time import sleep
from contextlib import suppress

PYPOZYX_INSTALLED = True

try:
    from pypozyx import PozyxSerial, get_serial_ports, DeviceCoordinates, SingleRegister
    from pypozyx import Coordinates, POZYX_SUCCESS, PozyxConstants, Data
except ModuleNotFoundError:
    PYPOZYX_INSTALLED = False


def inspect_port(port):
    with suppress(TypeError):
        if 'Pozyx Labs' in port.manufacturer:
            return True
    with suppress(TypeError):
        if 'Pozyx' in port.product:
            return True
    with suppress(TypeError):
        # assure it is NOT the flight controller
        if '0483:' in port.hwid and not port.serial_number.lower().startswith('0x'):
            return True
    return False


def get_pozyx_serial_port():
    for port in get_serial_ports():
        if inspect_port(port):
            return port.device


def get_serial_port_names():
    return [port.device for port in get_serial_ports()]


def send_error_msg(msg):
    return {'error': msg + ' then refresh the Pozyx tab.'}


if PYPOZYX_INSTALLED:
    remote_id = None
    algorithm = PozyxConstants.POSITIONING_ALGORITHM_UWB_ONLY
    dimension = PozyxConstants.DIMENSION_3D
    height = 1000
    anchors = [DeviceCoordinates(0x6951, 1, Coordinates(0, 0, 1500)),
               DeviceCoordinates(0x6e59, 2, Coordinates(5340, 0, 2000)),
               DeviceCoordinates(0x695d, 3, Coordinates(6812, -8923, 2500)),
               DeviceCoordinates(0x690b, 4, Coordinates(-541, -10979, 3000))]

    POZYX_CONNECTED_TO_BASE = True

    serial_port = get_pozyx_serial_port()

    if serial_port is None:
        POZYX_CONNECTED_TO_BASE = False
    else:
        pozyx = PozyxSerial(serial_port)
        # set anchors
        status = pozyx.clearDevices()
        for anchor in anchors:
            status &= pozyx.addDevice(anchor, remote_id=remote_id)

    MAX_TRIES = 20
    MISSION_DONE = [36, 77, 60, 0, 20, 20]
    WP_INDEX = 6


def check_connection(positioning=True):
    """Check for errors before executing a pozyx function"""
    def inner(func):
        def check(*args):
            if not PYPOZYX_INSTALLED:
                return send_error_msg('PyPozyx not installed!. Run - pip install pypozyx')
            if not POZYX_CONNECTED_TO_BASE:
                return send_error_msg('No pozyx device connected! Check USB connection')
            if positioning:
                if serial_port not in get_serial_port_names():
                    return send_error_msg('Connection to pozyx device lost! Check USB connection')
                inactive_anchors = 0
                for a in anchors:
                    network_id = SingleRegister()
                    pozyx.getWhoAmI(network_id, remote_id=a.network_id)
                    if network_id.data == [0]:
                        inactive_anchors += 1
                if inactive_anchors > 1:
                    return send_error_msg(
                        'Can\'t connect to at least {} anchors. Check the anchor\'s power connection '
                        'and the pozyx\'s USB connection'.format(inactive_anchors))
            return func(*args)
        return check
    return inner


@check_connection(positioning=False)
def send_mission(way_point_data):
    way_point = list(way_point_data.values())
    stop = False
    if way_point == MISSION_DONE:
        # all way points have been sent
        status_data = 't'
        stop = True
    elif way_point[WP_INDEX] == 1:
        # first way point
        status_data = 's'
    else:
        # 2nd+ way point
        status_data = 'c'
    pozyx.sendData(destination=0, data=status_data.encode())
    if stop:
        return {'success': 'All points sent'}
    sleep(0.1)
    for number in way_point:
        pozyx.sendData(destination=0, data=str(number).encode())
        sleep(0.1)
    return {'success': 'WP sent'}


@check_connection()
def get_position():
    # start positioning
    for _ in range(MAX_TRIES):
        position = Coordinates()
        if POZYX_SUCCESS == pozyx.doPositioning(position, dimension, height, algorithm, remote_id=remote_id):
            return {
                'x': position.x,
                'y': position.y,
                'z': position.z
            }
