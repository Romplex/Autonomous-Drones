// -- app.js --
const $ = require('jquery');
const pythonBridge = require('python-bridge');

$('#pozyxBtn', document).click(() => {
  const python = pythonBridge();
  python.ex`
  from pypozyx import get_first_pozyx_serial_port, PozyxSerial, SingleRegister
  from pypozyx.definitions.registers import POZYX_WHO_AM_I
  
  def doSomething():
      port = get_first_pozyx_serial_port()
      print('Port:', port)
      p = PozyxSerial(port)
  
      whoami = SingleRegister()
      p.regRead(POZYX_WHO_AM_I, whoami)
  
      print('WhoAmI:', whoami)
      return port, whoami.data`.catch(python.Exception, console.error);

  python`doSomething()`
    .then(data => {
      console.log(`Port: ${data[0]}, WhoAmI: ${data[1][0]}`);
    })
    .catch(python.Exception, console.error);

  python.end();
});
