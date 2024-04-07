from flask import Flask, jsonify

app = Flask(__name__)


@app.route("/")
def say_hello():
    return jsonify({"msg": "Hello from Flask"})


if __name__ == "__main__":
    # Please do not set debug=True in production
    app.run(debug=True)