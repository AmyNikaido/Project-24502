from flask import Flask, render_template
import json

# creates a Flask application instance named app
app = Flask(__name__)

# home page
@app.route("/")
def home():
    # only returns the heading below
    return "<h1>HVAC Capacitor Data<h1>"

# sample response page
@app.route("/sample_response/")
def sample_response():
    # gets data from data file
    healthy_labels = []
    healthy_data = []
    with open('healthy_data.txt', 'r') as file:
        for line in file:
            # uses ; as delimiter
            label, value = line.strip().split(';')
            healthy_labels.append(label)
            healthy_data.append(float(value))
    
    # converts data into json formatted strings
    # when I was making this, many sources recommended using tojson to import the data (this would be in the HTML files not here)
    # I was not able to figure out how to make it work for MVP 1, but maybe you can for MVP 2
    # I will leave the ChatGPT conversation about it in the GUI documentation file
    healthy_labels_json = json.dumps(healthy_labels)
    healthy_data_json = json.dumps(healthy_data)

    #gets data from data file
    probe_labels = []
    probe_data = []
    with open('mock_degraded_data.txt', 'r') as file:
        for line in file:
            # uses ; as delimiter
            label, value = line.strip().split(';')
            probe_labels.append(label)
            probe_data.append(float(value))
    
    # converts data into json formatted strings
    probe_labels_json = json.dumps(probe_labels)
    probe_data_json = json.dumps(probe_data)
    
    # creates webpage from HTML template and passes the json data to the HTML page
    return render_template('sampleResponse.html', healthy_labels_json=healthy_labels_json, healthy_data_json=healthy_data_json, probe_labels_json=probe_labels_json, probe_data_json=probe_data_json)

# degraded response page
@app.route("/degraded_response/")
def degraded_response():
    # gets data from file
    labels = []
    data = []
    with open('mock_degraded_data.txt', 'r') as file:
        for line in file:
            # uses ; as delimiter
            label, value = line.strip().split(';')
            labels.append(label)
            data.append(float(value))
    
    # converts data into json formatted strings
    labels_json = json.dumps(labels)
    data_json = json.dumps(data)
    
    # creates webpage from HTML template and passes the json data to the HTML page
    return render_template('degradedResponse.html', labels_json=labels_json, data_json=data_json)

# prognostic estimations page
# this is where you will have to implement ARULE
@app.route("/prognostic_estimations/")
def prognostic_estimations():
    # get data from file
    labels = []
    data = []
    with open('ARULE_data_in.txt', 'r') as file: # not real data
        for line in file:
            # uses a space as a delimiter
            label, value = line.strip().split(' ')
            labels.append(label)
            data.append(float(value))
    
    # converts data to json formatted strings
    labels_json = json.dumps(labels)
    data_json = json.dumps(data)
    
    # creates webpage from HTML template and passes the json data to the HTML page
    return render_template('prognosticEstimations.html', labels_json=labels_json, data_json=data_json)

# runs the app with debugging
if __name__ == "__main__":
    app.run(debug=True)