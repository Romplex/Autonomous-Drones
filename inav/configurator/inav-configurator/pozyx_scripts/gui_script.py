import json
from functools import wraps

PYPOZYX_INSTALLED = True
BUSY_SERIAL = False

try:
    from pypozyx import PozyxSerial, get_serial_ports, DeviceCoordinates, SingleRegister, DeviceList, \
        PozyxConnectionError
    from pypozyx import Coordinates, POZYX_SUCCESS, PozyxConstants, Data
except ModuleNotFoundError:
    PYPOZYX_INSTALLED = False


def inspect_port(port):
    try:
        if 'Pozyx Labs' in port.manufacturer:
            return True
    except TypeError:
        pass
    try:
        if 'Pozyx' in port.product:
            return True
    except TypeError:
        pass
    try:
        # assure it is NOT the flight controller
        if '0483:' in port.hwid and not port.serial_number.lower().startswith('0x'):
            return True
    except TypeError:
        pass
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
    remote_id = 0x6758
    algorithm = PozyxConstants.POSITIONING_ALGORITHM_UWB_ONLY
    dimension = PozyxConstants.DIMENSION_3D
    height = 1000

    anchors = [DeviceCoordinates(0x6951, 1, Coordinates(0, 0, 1500)),
               DeviceCoordinates(0x6e59, 2, Coordinates(5340, 0, 2000)),
               DeviceCoordinates(0x695d, 3, Coordinates(6812, -8923, 2500)),
               DeviceCoordinates(0x690b, 4, Coordinates(-541, -10979, 3000)),
               DeviceCoordinates(0x6748, 5, Coordinates(6812, -4581, 200))]

    POZYX_CONNECTED_TO_BASE = True

    serial_port = get_pozyx_serial_port()

    if serial_port is None:
        POZYX_CONNECTED_TO_BASE = False
    else:
        try:
            pozyx = PozyxSerial(serial_port)
            status = pozyx.clearDevices()

            for anchor in anchors:
                status &= pozyx.addDevice(anchor)

            remote_check = SingleRegister()
            pozyx.getWhoAmI(remote_check, remote_id=remote_id)
            if remote_check.data == [0]:
                remote_id = None

            MAX_TRIES = 1000
        except PozyxConnectionError:
            BUSY_SERIAL = True


def check_connection(func):
    """Check for errors before executing a pozyx function"""

    @wraps(func)
    def check(*args):
        if BUSY_SERIAL:
            return {'error': 'Busy serial port! You may need to reboot your PC.'}
        if not PYPOZYX_INSTALLED:
            return send_error_msg('PyPozyx not installed!. Run - pip install pypozyx')
        if not POZYX_CONNECTED_TO_BASE:
            return send_error_msg('No pozyx device connected! Check USB connection')
        if serial_port not in get_serial_port_names():
            return send_error_msg('Connection to pozyx device lost! Check USB connection')
        return func(*args)

    return check


@check_connection
def send_msp_message(msg):
    message = list(msg.values())
    size = len(message)
    if size > 27:
        return {'error': 'message too long!'}
    d = Data(data=message, data_format=size * 'B')
    pozyx.sendData(destination=remote_id, data=d)
    return {'success': 'WP sent'}


def set_remote_id(r_id):
    global remote_id
    try:
        r_id = int(r_id)
        r_id = None if r_id == 1 else r_id
        if r_id == remote_id:
            return {'success': 'remote id remains unchanged.'}
        remote_id = r_id
        return {'success': 'remote id set to {}'.format(remote_id)}
    except ValueError:
        remote_id = None
        return {'error': 'Can\'t set remote id!'}


@check_connection
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

    # error handling
    inactive_anchors = 0
    if remote_id:
        network_id = SingleRegister()
        pozyx.getWhoAmI(network_id, remote_id=remote_id)
        if network_id.data == [0]:
            return send_error_msg(
                'Could not establish connection to device with ID {}'.format(remote_id.decode('utf-8')))
    for a in anchors:
        network_id = SingleRegister()
        pozyx.getWhoAmI(network_id, remote_id=a.network_id)
        if network_id.data == [0]:
            inactive_anchors += 1
    if inactive_anchors > 1:
        return send_error_msg(
            'Can\'t connect to at least {} anchors. Check the anchor\'s power connection '
            'and the pozyx\'s USB connection'.format(inactive_anchors))
