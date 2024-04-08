from flask import Flask

app = Flask(__name__)

@app.route("/")
def home():
    return "<h1>HVAC Capacitor Data<h1>"

@app.route("/sample_response/")
def sample_response():
    return "<h1>Sample Response<h1>"

@app.route("/degraded_response/")
def degraded_response():
    return "<h1>Degraded Response<h1>"

@app.route("/prognostic_estimations/")
def prognostic_estimations():
    return "<h1>Prognostic Estimations<h1>"

if __name__ == "__main__":
    app.run()