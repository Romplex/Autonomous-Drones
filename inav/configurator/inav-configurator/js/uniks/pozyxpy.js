const pythonBridge = require('python-bridge');

function printPozyxError(err) {
  console.error('POZYX-Error: ', err);
}

const PozyxPy = function() {
  const coords = POZYX.anchors.map(a => a.Coordinates);

  this.py = pythonBridge();

  // python imports
  this.py.ex`
    from pypozyx import PozyxSerial, get_first_pozyx_serial_port, DeviceCoordinates, Coordinates, POZYX_SUCCESS, PozyxConstants
    print('POZYXPY: got imports')
    `.catch(this.py.Exception, printPozyxError);

  // python global values
  this.py.ex`
    algorithm = PozyxConstants.POSITIONING_ALGORITHM_UWB_ONLY
    dimension = PozyxConstants.DIMENSION_3D
    height = 1000
    anchors = [DeviceCoordinates(0x6951, 1, Coordinates( \
      ${coords[0][0]}, \
      ${coords[0][1]}, \
      ${coords[0][2]})),
    DeviceCoordinates(0x6e59, 2, Coordinates( \
      ${coords[1][0]}, \
      ${coords[1][1]}, \
      ${coords[1][2]})),
    DeviceCoordinates(0x695d, 3, Coordinates( \
      ${coords[2][0]}, \
      ${coords[2][1]}, \
      ${coords[2][2]})),
    DeviceCoordinates(0x690b, 4, Coordinates( \
      ${coords[3][0]}, \
      ${coords[3][1]}, \
      ${coords[3][2]}))]

    serial_port = get_first_pozyx_serial_port()

    if serial_port is None:
        print('No Pozyx connected. Check your USB cable or your driver!')
        quit()

    pozyx = PozyxSerial(serial_port)
    print('POZYXPY: set globals')`.catch(this.py.Exception, printPozyxError);

  // python functions
  this.py.ex`
    def get_position(tag_id=None):
        status = pozyx.clearDevices(remote_id=tag_id)
        for anchor in anchors:
            status &= pozyx.addDevice(anchor, remote_id=tag_id)

            while True:
                position = Coordinates()
                status = pozyx.doPositioning(position, dimension, height, algorithm, remote_id=tag_id)
                if status == POZYX_SUCCESS:
                  return {'x': position.x, 'y': position.y, 'z': position.z}

    print('POZYXPY: defined functions')
    `.catch(this.py.Exception, printPozyxError);
};

PozyxPy.prototype.getPosition = function(tagId) {
  if (tagId) {
    return this.py`get_position(${tagId})`;
  }

  return this.py`get_position()`;
};
