# -- python_server.py --
from flask import Flask, request
import json
import threading
# import sys
import requests
import time
     
app = Flask(__name__)
     
@app.route('/')
def index():
    return "Flask server"
     
@app.route('/py/getpozyx', methods = ['POST'])
def postdata():
    data = request.get_json()
    
    if not data['data0'] == 'pozyx':
        return

    if data['data1'] == 'position':
        print(data)
        # TODO uniks get pozyx x/y/z position via python pozyx library
        x = 1.0
        y = 2.0
        z = -10.0
        return json.dumps({"x":x, "y":y, "z":z})
    else:
        return 
    
     
def background_send():
    while 1:
        # get pozyx data from js server every 3 seconds
        try:
            msg = requests.get("http://localhost:3000/js/getpozyx")
            print("answer: ", msg.text)
        except requests.exceptions.RequestException as e:
            print(e)
        
        time.sleep(2)

if __name__ == "__main__":

    # start thread
    th = threading.Thread(target=background_send)
    th.daemon = True
    th.start()

    app.run(port=5000)