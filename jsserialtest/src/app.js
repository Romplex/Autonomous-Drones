const $ = require('jquery');

let connectionId = -1;
const decoder = new TextDecoder('utf-8');
const serial = chrome.serial;

serial.onReceive.addListener(d => {
  const data = decoder.decode(d.data);
  $('#serialdata', document).append(data);
});

serial.onReceiveError.addListener(e => {
  console.warn(e);
  connect();
});

serial.getDevices(devices => {
  const $devices = $('#devices', document);
  const list = document.createElement('ul');
  devices.forEach(element => {
    const l = document.createElement('li');
    list.appendChild(l);
    l.innerText = element.path;
  });

  $devices.append(list);
});

connect();

function connect() {
  serial.getDevices(devices => {
    if (devices.length) {
      serial.connect(
        devices[0].path,
        {
          receiveTimeout: 10000
        },
        info => {
          connectionId = info.connectionId;
        }
      );
    }
  });
}
