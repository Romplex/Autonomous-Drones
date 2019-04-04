from pypozyx import PozyxSerial, get_first_pozyx_serial_port
from pypozyx.definitions.constants import POZYX_SUCCESS

# pozyx tag on drone
remote_id = 0x6748

# shortcut to not have to find out the port yourself
serial_port = get_first_pozyx_serial_port()
if serial_port is None:
    print('No Pozyx connected. Check your USB cable or your driver!')
    quit()

pozyx = PozyxSerial(serial_port)


def send_test_message(msg):
    while True:
        status = pozyx.sendData(remote_id, msg.encode('utf-8'))
        if status == POZYX_SUCCESS:
            break


if __name__ == '__main__':
    send_test_message("XD")
