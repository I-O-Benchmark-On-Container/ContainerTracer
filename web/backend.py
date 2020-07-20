#!/usr/bin/python3
# source from: https://blog.nerdfactory.ai/2019/04/04/flask-unittest.html
import flask_restful
import flask
from flask_restful import reqparse

app = flask.Flask(__name__)
api = flask_restful.Api(app)


def multiply(x, y):
    return x * y


class HelloWorld(flask_restful.Resource):
    def get(self):
        parser = reqparse.RequestParser()

        # parameter1 과 parameter2를 parsing
        parser.add_argument("parameter1")
        parser.add_argument("parameter2")
        args = parser.parse_args()

        # 해당 변수에 parameter1과 parameter2를 할당
        parameter1 = args["parameter1"]
        parameter2 = args["parameter2"]

        # 해당 변수 중 하나라도 None일 경우 아래를 return
        if (not parameter1) or (not parameter2):
            return {"state": 0, "response": None}

        parameter1 = int(parameter1)
        parameter2 = int(parameter2)

        # 두 변수 모두를 곱하여 아래를 return
        result = multiply(parameter1, parameter2)
        return {"state": 1, "response": result}


api.add_resource(HelloWorld, "/api/multiply")

if __name__ == "__main__":
    app.run()
