from pypozyx import PozyxSerial, get_first_pozyx_serial_port, Data
from pypozyx.definitions.bitmasks import POZYX_INT_STATUS_RX_DATA
from pypozyx.definitions.constants import POZYX_SUCCESS


def loop():
    """ communication with pozyx tag on drone """

    print("start loop")

    while True:
        if pozyx.waitForFlag(POZYX_INT_STATUS_RX_DATA, 50):
            # works for messages with length >= 5
            data_format = 'bbbbbb'
            data = Data([0] * len(data_format), data_format)
            flag = pozyx.readRXBufferData(data)
            received_msg = data.data
            if flag == POZYX_SUCCESS and received_msg != []:
                for d in received_msg:
                    print(str(chr(d)))
                message = 'Nachricht erhalten'.encode('utf-8')
                pozyx.sendData(0x6760, message)


if __name__ == "__main__":
    """ setup """

    # shortcut to not have to find out the port yourself
    serial_port = get_first_pozyx_serial_port()
    if serial_port is None:
        print('No Pozyx connected. Check your USB cable or your driver!')
        quit()

    pozyx = PozyxSerial(serial_port)
    pozyx.setUWBChannel(1)

    loop()
