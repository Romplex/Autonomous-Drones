from functools import wraps

PYPOZYX_INSTALLED = True

try:
    from pypozyx import PozyxSerial, get_serial_ports, DeviceCoordinates, Coordinates, POZYX_SUCCESS, PozyxConstants, \
    SingleRegister, Data
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


def check_connection(func):

    @wraps(func)
    def check():
        if not PYPOZYX_INSTALLED:
            return send_error_msg('PyPozyx not installed!. Run - pip install pypozyx')
        if not POZYX_CONNECTED_TO_BASE:
            return send_error_msg('No pozyx device connected! Check USB connection')
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
                'Can\'t connect to at least {} anchors. Check the anchor\'s power connection and the pozyx USB '
                'connection'.format(inactive_anchors))
        return func()

    return check


def send_message(msg):
    for entry in msg:
        pozyx.sendData(destination=0, data='XD')
    # with open('log.txt', 'w') as log_file:
    #     log_file.write('XD' if msg is None else str(msg))
    # data = Data(msg)


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
