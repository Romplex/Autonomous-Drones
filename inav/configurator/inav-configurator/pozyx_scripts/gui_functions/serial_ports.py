from pypozyx import get_serial_ports


def get_pozyx_serial_port():
    for port in get_serial_ports():
        if inspect_port(port):
            return port.device


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


if __name__ == '__main__':
    print(get_pozyx_serial_port())
