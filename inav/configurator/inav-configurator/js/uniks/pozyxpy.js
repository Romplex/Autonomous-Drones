const pythonBridge = require('python-bridge');

function printPozyxError(err) {
  GUI.log('POZYX-Error: ' + err);
}

const PozyxPy = function() {
  const coords = POZYX.anchors.map(a => a.Coordinates);
  this.py = pythonBridge();

  // python imports
  this.py.ex`
      from pypozyx import PozyxSerial, get_serial_ports, DeviceCoordinates, Coordinates, POZYX_SUCCESS, PozyxConstants, SingleRegister
      from functools import wraps
      print('POZYXPY: got imports')
      `.catch(this.py.Exception, (err) => printPozyxError(err.message));

  // python global values
  this.py.ex`
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
      print('POZYXPY: set globals')`.catch(this.py.Exception, (err) => printPozyxError(err.message));

  // python functions
  this.py.ex`
       def check_connection(func):
          @wraps(func)
          def check():
              if not POZYX_CONNECTED_TO_BASE:
                  return {'error': 'No pozyx device connected! check usb connection.'}
              return func()
          return check
      
      @check_connection
      def get_position(tag_id=None):
          # set anchors
          status = pozyx.clearDevices()
          for anchor in anchors:
              status &= pozyx.addDevice(anchor)
      
          # start positioning
          tries = MAX_TRIES
          while tries > 0:
              position = Coordinates()
              status = pozyx.doPositioning(position, dimension, height, algorithm)
              if status == POZYX_SUCCESS:
                  return {
                      'x': position.x,
                      'y': position.y,
                      'z': position.z
                  }
              tries -= 1
          return {'error': 'At least one anchor inactive! Check connection to power supply.'}
                    
      @check_connection
      def who_am_i():
          whoami = SingleRegister()
          pozyx.getWhoAmI(whoami)
          return {'i_am': 'XD'}
    `.catch(this.py.Exception, (err) => printPozyxError(err.message));
};

PozyxPy.prototype.getPosition = function(tagId) {
  if (tagId) {
    return this.py`get_position(${tagId})`;
  }

  return this.py`get_position()`;
};

  PozyxPy.prototype.getWhoAmI = function() {
  return this.py`who_am_i()`;
};
