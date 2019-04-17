from functools import wraps

PYPOZYX_INSTALLED = True

try:
    from pypozyx import PozyxSerial, get_serial_ports, DeviceCoordinates, Coordinates, POZYX_SUCCESS, PozyxConstants, \
        SingleRegister
except ModuleNotFoundError:
    PYPOZYX_INSTALLED = False


def inspect_port(port):
    try:
        if "Pozyx Labs" in port.manufacturer:
            return True
    except TypeError:
        pass
    try:
        if "Pozyx" in port.product:
            return True
    except TypeError:
        pass
    try:
        # assure it is NOT the flight controller
        if "0483:" in port.hwid and not port.serial_number.lower().startswith('0x'):
            return True
    except TypeError:
        pass
    return False


def get_pozyx_serial_port():
    for port in get_serial_ports():
        if inspect_port(port):
            return port.device


def send_error_msg(msg):
    return {'error': msg + ' then refresh the Pozyx tab.'}


if PYPOZYX_INSTALLED:
    remote_id = 0x6760
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

    MAX_TRIES = 20


def check_connection(func):
    @wraps(func)
    def check():
        if not PYPOZYX_INSTALLED:
            return send_error_msg('PyPozyx not installed!. Run - pip install pypozyx')
        if not POZYX_CONNECTED_TO_BASE:
            return send_error_msg('No pozyx device connected! Check USB connection')
        return func()

    return check


def send_message(msg):
    try:
        pozyx.sendData(remote_id, msg.encode())
        return {'success': msg}
    except Exception as e:
        return {'error': str(e)}


@check_connection
def get_position():
    # set anchors
    status = pozyx.clearDevices()
    for anchor in anchors:
        status &= pozyx.addDevice(anchor, remote_id=remote_id)

    # start positioning
    for _ in range(MAX_TRIES):
        position = Coordinates()
        status = pozyx.doPositioning(position, dimension, height, algorithm, remote_id=remote_id)
        if status == POZYX_SUCCESS:
            return {
                'x': position.x,
                'y': position.y,
                'z': position.z
            }
    if serial_port not in get_serial_ports():
        return send_error_msg('Connection to pozyx device lost! Check USB connection')
    return send_error_msg('At least one anchor inactive! Assure connection to power supply')


@check_connection
def who_am_i():
    whoami = SingleRegister()
    pozyx.getWhoAmI(whoami)
    return {'i_am': 'XD'}
