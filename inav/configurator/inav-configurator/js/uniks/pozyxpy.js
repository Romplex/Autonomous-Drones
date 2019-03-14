const pythonBridge = require('python-bridge');

const PozyxPy = function() {
  this.py = pythonBridge();

  // python imports
  this.py.ex`
    from pypozyx import PozyxSerial, get_first_pozyx_serial_port, DeviceCoordinates, Coordinates, POZYX_SUCCESS, PozyxConstants
    print('POZYXPY: got imports')
    `;

  // python global values
  this.py.ex`
    algorithm = PozyxConstants.POSITIONING_ALGORITHM_UWB_ONLY
    dimension = PozyxConstants.DIMENSION_3D
    height = 1000
    anchors = [DeviceCoordinates(0x6951, 1, Coordinates(0, 0, 1500)),
        DeviceCoordinates(0x6e59, 2, Coordinates(5340, 0, 2000)),
        DeviceCoordinates(0x695d, 3, Coordinates(6812, -8923, 2500)),
        DeviceCoordinates(0x690b, 4, Coordinates(-541, -10979, 3000))]

    remote_id = 0x6760
    serial_port = get_first_pozyx_serial_port()

    if serial_port is None:
        print('No Pozyx connected. Check your USB cable or your driver!')
        quit()

    pozyx = PozyxSerial(serial_port)
    print('POZYXPY: set globals')`;

  // python functions
  this.py.ex`
    def get_remote_position():
        status = pozyx.clearDevices(remote_id=remote_id)
        for anchor in anchors:
            status &= pozyx.addDevice(anchor, remote_id=remote_id)

            while True:
                position = Coordinates()
                status = pozyx.doPositioning(position, dimension, height, algorithm, remote_id=remote_id)
                if status == POZYX_SUCCESS:
                    return {'x': position.x, 'y': position.y, 'z': position.z}

    print('POZYXPY: defined functions')
    `.catch(this.py.Exception, console.error);
};

PozyxPy.prototype.getPosition = function() {
  return this.py`get_remote_position()`;
};
