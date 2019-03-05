// -- app.js --
const express = require('express') ;
const $ = require('jquery');
const fetch = require("node-fetch")

var app = express();

app.listen(3000, () => {
  console.log("Server running on port 3000...")
});

app.get("/js/getpozyx", (req, res, next) => {
  res.json(["x",1.0, "y",2.2, "z", -3.3]);
});

const getData = async options => {
  try {
    const response = await fetch(options.uri, options);
    
    // data you received from python server
    const json = await response.json();
    $('#serialdata', document).append(JSON.stringify(json) + "\n");
    console.log(json);
  } catch (error) {
    console.log(error);
  }
};

$('#btn', document).click(function() {
  console.log("btn");
  // data you want to send to python server
  let d = {
    data0: "pozyx",
    data1: "position"
  };
  let i = JSON.stringify(d);
  let options = {
    method: 'POST',
    headers: {
      'Accept': 'application/json',
      'Content-Type': 'application/json'
    },
    uri: 'http://localhost:5000/py/getpozyx',
    body: i,
    json: true
  }

  getData(options);
});
