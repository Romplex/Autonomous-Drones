from pypozyx import get_first_pozyx_serial_port, PozyxSerial, SingleRegister
from pypozyx.definitions.registers import POZYX_WHO_AM_I

def doSomething():
    port = get_first_pozyx_serial_port()
    print('Port:', port)
    p = PozyxSerial(port)

    whoami = SingleRegister()
    p.regRead(POZYX_WHO_AM_I, whoami)

    print('WhoAmI:', whoami)
    return port, whoami.data