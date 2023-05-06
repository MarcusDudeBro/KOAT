from flask import Flask
from flask import request
from flask import Response
from flask_cors import CORS, cross_origin

app = Flask(__name__)
CORS(app)

data = {
  'connection': 'false',
  'posX': 0,
  'posY': 0,
  'posZ': 0
}

@app.route('/')
@cross_origin()
def hello_world():
    return 'Hello, World!'

@app.route('/connection', methods=['PUT', 'GET'])
@cross_origin()
def connection():
    if request.method == 'GET':
        return {'connection': data['connection']}
    elif request.method == 'PUT':
        data['connection'] = request.args.get("connected")
        return Response(status=200)

@app.route('/position', methods=['PUT', 'GET'])
@cross_origin()
def position():
    if request.method == 'GET':
        return {
            'posX': data['posX'],
            'posY': data['posY'],
            'posZ': data['posZ'],
        }
    elif request.method == 'PUT':
        update_x = request.args.get("x")
        update_y = request.args.get("y")
        update_z = request.args.get("z")
        data['posX'] = float(update_x)
        data['posY'] = float(update_y)
        data['posZ'] = float(update_z)
        return Response(status=200)

