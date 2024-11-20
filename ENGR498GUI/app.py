from flask import Flask, request, render_template, jsonify, send_from_directory, abort
import os

app = Flask(__name__)

# Define where to save uploaded files
UPLOAD_FOLDER = '/home/engr498/ENGR498GUI/uploads'  # Change this to the desired directory
# Path to the directory containing device folders
BASE_DEVICE_DIRECTORY = './data'

# Ensure the upload folder exists
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

@app.route('/')
def index():
    return render_template('index.html')

# Upload file route
@app.route('/upload', methods=['POST'])
def upload_file():
    try:
        # Get the uploaded file content from the request
        file_content = request.data  # This will capture the raw data sent by the Arduino

        # Save the file to the specified path
        file_path = os.path.join(UPLOAD_FOLDER, "uploaded_file.csv")
        with open(file_path, "wb") as f:  # Write the file content in binary mode
            f.write(file_content)
        
        return "File uploaded successfully", 200
    except Exception as e:
        return str(e), 500

# List devices route
@app.route('/list_devices', methods=['GET'])
def list_devices():
    # List all directories (devices) within the BASE_DEVICE_DIRECTORY
    devices = [f.name for f in os.scandir(BASE_DEVICE_DIRECTORY) if f.is_dir()]
    return jsonify(devices)

# Get data file from a specific device folder
@app.route('/data/<device>/<filename>', methods=['GET'])
def get_data_file(device, filename):
    device_folder_path = os.path.join(BASE_DEVICE_DIRECTORY, device)
    file_path = os.path.join(device_folder_path, filename)
    
    if not os.path.exists(file_path):
        return abort(404, description="File not found")
    
    return send_from_directory(device_folder_path, filename)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
