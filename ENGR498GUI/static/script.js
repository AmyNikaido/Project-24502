let voltageChart = null;  // Declare chart globally so it can be destroyed later
let capacitorHealthChart = null;
let allDevices = [];  // Store the list of all devices for filtering

// Function to load all devices from the server
function loadDevices() {
    fetch('/list_devices')
        .then(response => response.json())
        .then(devices => {
            allDevices = devices; // Store the list of all devices for filtering
            displayDevices(devices); // Display the full list of devices initially
        })
        .catch(error => console.error('Error loading devices:', error));
}

// Function to fetch and display graphs for the selected device
function showGraphsForDevice(device) {
    // Update the titles to show the current device
    //document.getElementById('voltage-title').innerText = `Voltage data for ${device}`;
    //document.getElementById('capacitor-title').innerText = `Capacitor Health data for ${device}`;

    // Fetch voltage data for the selected device
    fetch(`/data/${device}/voltageData.txt`)
        .then(response => {
            if (!response.ok) {
                throw new Error('Voltage data not found');
            }
            return response.text();
        })
        .then(data => {
            updateVoltageChart(data);  // Process voltage data and update chart
        })
        .catch(error => console.error('Error loading voltage data:', error));

    // Fetch capacitor health data for the selected device
    fetch(`/data/${device}/capacitorHealthData.txt`)
        .then(response => {
            if (!response.ok) {
                throw new Error('Capacitor Health data not found');
            }
            return response.text();
        })
        .then(data => {
            updateCapacitorHealthChart(data);  // Process capacitor health data and update chart
        })
        .catch(error => console.error('Error loading capacitor health data:', error));
}

// Update the voltage chart with the fetched data
function updateVoltageChart(data) {
    const ctx = document.getElementById('voltageChart').getContext('2d');
    const voltageData = parseData(data);  // Parse the data into labels and values

    // Destroy the old chart instance before creating a new one
    if (voltageChart) {
        voltageChart.destroy();
    }

    voltageChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: voltageData.labels,
            datasets: [{
                label: 'Voltage vs Time',
                data: voltageData.values,
                borderColor: 'rgb(75, 192, 192)',
                tension: 0.1
            }]
        }
    });
}

// Update the capacitor health chart with the fetched data
function updateCapacitorHealthChart(data) {
    const ctx = document.getElementById('capacitorHealthChart').getContext('2d');
    const capacitorData = parseData(data);  // Parse the data into labels and values

    // Destroy the old chart instance before creating a new one
    if (capacitorHealthChart) {
        capacitorHealthChart.destroy();
    }

    capacitorHealthChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: capacitorData.labels,
            datasets: [{
                label: 'Capacitor Health vs Time',
                data: capacitorData.values,
                borderColor: 'rgb(255, 99, 132)',
                tension: 0.1
            }]
        }
    });
}

// Utility function to parse text data into a format usable by the chart
function parseData(data) {
    const lines = data.split('\n');
    const labels = [];
    const values = [];

    lines.forEach(line => {
        const [label, value] = line.split(',');
        if (label && value) {  // Ensure both label and value exist
            labels.push(label.trim());
            values.push(parseFloat(value.trim()));
        }
    });

    return { labels, values };
}

// Function to display the filtered devices in the dropdown
function displayDevices(devices) {
    const deviceList = document.getElementById('searchResults');
    deviceList.innerHTML = '';  // Clear the existing list

    devices.forEach(device => {
        const div = document.createElement('div');
        div.textContent = device;
        div.classList.add('device-item');
        div.onclick = function() {
            showGraphsForDevice(device); // Load graphs when clicking on a device
        };
        deviceList.appendChild(div);
    });

    document.querySelector('.search-results').style.display = devices.length ? 'block' : 'none'; // Show or hide dropdown
}

// Function to filter devices based on search input
function filterDevices() {
    const query = document.getElementById('deviceSearch').value.toLowerCase();
    const filteredDevices = allDevices.filter(device => device.toLowerCase().includes(query));
    displayDevices(filteredDevices); // Display the filtered devices
}

// Load devices on page load
window.onload = loadDevices;
