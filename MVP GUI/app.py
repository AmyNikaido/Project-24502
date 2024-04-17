from flask import Flask, render_template
import json

app = Flask(__name__)

@app.route("/")
def home():
    return "<h1>HVAC Capacitor Data<h1>"

@app.route("/sample_response/")
def sample_response():
    healthy_labels = []
    healthy_data = []
    with open('healthy_data.txt', 'r') as file:
        for line in file:
            label, value = line.strip().split(';')
            healthy_labels.append(label)
            healthy_data.append(float(value))
    healthy_labels_json = json.dumps(healthy_labels)
    healthy_data_json = json.dumps(healthy_data)

    probe_labels = []
    probe_data = []
    with open('mock_degraded_data.txt', 'r') as file:
        for line in file:
            label, value = line.strip().split(';')
            probe_labels.append(label)
            probe_data.append(float(value))
    probe_labels_json = json.dumps(probe_labels)
    probe_data_json = json.dumps(probe_data)
    return render_template('sampleResponse.html', healthy_labels_json=healthy_labels_json, healthy_data_json=healthy_data_json, probe_labels_json=probe_labels_json, probe_data_json=probe_data_json)

@app.route("/degraded_response/")
def degraded_response():
    labels = []
    data = []
    with open('mock_degraded_data.txt', 'r') as file:
        for line in file:
            label, value = line.strip().split(';')
            labels.append(label)
            data.append(float(value))
    labels_json = json.dumps(labels)
    data_json = json.dumps(data)
    return render_template('degradedResponse.html', labels_json=labels_json, data_json=data_json)

@app.route("/prognostic_estimations/")
def prognostic_estimations():
    labels = []
    data = []
    with open('ARULE_data_in.txt', 'r') as file:
        for line in file:
            label, value = line.strip().split(' ')
            labels.append(label)
            data.append(float(value))
    labels_json = json.dumps(labels)
    data_json = json.dumps(data)
    return render_template('prognosticEstimations.html', labels_json=labels_json, data_json=data_json)
if __name__ == "__main__":
    app.run(debug=True)