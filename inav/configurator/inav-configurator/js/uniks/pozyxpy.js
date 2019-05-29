// const pythonBridge = require('python-bridge');

function printPozyxError(err) {
  console.error(err);
  GUI.log('POZYX-Error: ' + err);
}

const PozyxPy = function() {
  const coords = POZYX.anchors.map(a => a.Coordinates);
  this.py = pythonBridge();

  this.py
    .exFile('./pozyx_scripts/gui_functions/gui_script.py')
    .catch(printPozyxError);
};

PozyxPy.prototype.exit = function() {
  this.py.kill();
};

PozyxPy.prototype.getPosition = function() {
  return this.py`get_position()`;
};

PozyxPy.prototype.sendMSPMessage = function(mission) {
  console.log('message to send:' + mission);
  return this.py`send_msp_message(${mission})`;
};

PozyxPy.prototype.getWhoAmI = function() {
  return this.py`who_am_i()`;
};

PozyxPy.prototype.getTagIds = function() {
  return this.py`get_tag_ids()`;
};

PozyxPy.prototype.setRemoteId = function(id) {
  return this.py`set_remote_id(${id})`;
};
