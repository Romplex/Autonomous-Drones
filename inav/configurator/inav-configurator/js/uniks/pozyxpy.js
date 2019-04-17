// const pythonBridge = require('python-bridge');

function printPozyxError(err) {
  console.error(err);
  GUI.log('POZYX-Error: ' + err);
}

const PozyxPy = function() {
  const coords = POZYX.anchors.map(a => a.Coordinates);
  this.py = pythonBridge();

  this.py.exFile('./pozyx_scripts/gui_functions/gui_script.py').catch(printPozyxError);
};


PozyxPy.prototype.getPosition = function() {
  return this.py`get_position()`;
};

PozyxPy.prototype.sendMission = function(msg) {
  return this.py`send_message(${msg})`
};

PozyxPy.prototype.getWhoAmI = function() {
  return this.py`who_am_i()`;
};
