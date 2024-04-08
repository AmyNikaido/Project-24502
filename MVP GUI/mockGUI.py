from flask import Flask, render_template

app = Flask(__name__, template_folder='templates')

@app.route("/")
def home():
    return "<h1>HVAC Capacitor Data<h1>"

@app.route("/sample_response/")
def sample_response():
    return render_template("sampleResponse.html", content="Sample Response")

@app.route("/degraded_response/")
def degraded_response():
    return render_template("degradedResponse.html", content="Degraded Response")

@app.route("/prognostic_estimations/")
def prognostic_estimations():
    return render_template("prognosticEstimations.html", content="Prognostic Estimations")

if __name__ == "__main__":
    app.run(debug=True)