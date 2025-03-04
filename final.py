import json
import os
import paho.mqtt.client as mqtt
import pandas as pd
import numpy as np
import threading
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
from sklearn.ensemble import RandomForestRegressor
from sklearn.preprocessing import StandardScaler

# Load dataset
df = pd.read_csv("sensor_data.csv")

# Define sliding window size
WINDOW_SIZE = 5  # Number of past readings to use for prediction

# Prepare dataset with past N readings as features
def create_windowed_data(df, window_size):
    features, labels = [], []
    for i in range(len(df) - window_size):
        features.append(df.iloc[i:i + window_size].values.flatten())
        labels.append(df.iloc[i + window_size]["Power (mW)"])
    return np.array(features), np.array(labels)

X, y = create_windowed_data(df, WINDOW_SIZE)

# Scale features
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

# Train model
model = RandomForestRegressor(n_estimators=200, random_state=42)
model.fit(X_scaled, y)

# MQTT settings
MQTT_BROKER = "a2yy6y3qk9doo3-ats.iot.ap-northeast-1.amazonaws.com"
MQTT_PORT = 8883
MQTT_TOPIC = "esp8266/pub"

# Certificate paths
CERTIFICATE_PATH = "F:/backups/project stuff/test_stuff/DeviceCertificate.pem.crt"
PRIVATE_KEY_PATH = "F:/backups/project stuff/test_stuff/privateKey.pem.key"
CA_CERT_PATH = "F:/backups/project stuff/test_stuff/AmazonRootCA1.pem"

# Check if certificate files exist
for path in [CA_CERT_PATH, CERTIFICATE_PATH, PRIVATE_KEY_PATH]:
    if not os.path.exists(path):
        print(f"Error: Certificate file not found: {path}")
        exit(1)

# Store past readings in a queue (sliding window)
past_readings = deque(maxlen=WINDOW_SIZE)
actual_power = deque(maxlen=50)  # Store last 50 actual power values for graph
predicted_power = deque(maxlen=50)  # Store last 50 predicted power values

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to MQTT broker successfully")
        client.subscribe(MQTT_TOPIC)
        print(f"Subscribed to topic: {MQTT_TOPIC}")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        print(f"Received message on {msg.topic}: {payload}")

        data = json.loads(payload)
        new_reading = [data["voltage"], data["current"], data["temperature"], data["light"], data["power"]]
        
        actual_power.append(data["power"])  # Store actual power value
        past_readings.append(new_reading)  # Add new reading to queue

        if len(past_readings) == WINDOW_SIZE:
            input_data = np.array(past_readings).flatten().reshape(1, -1)
            input_data_scaled = scaler.transform(input_data)
            future_power_pred = model.predict(input_data_scaled)[0]
            predicted_power.append(future_power_pred)
            print(f"Predicted Future Power: {future_power_pred:.2f} mW")

    except Exception as e:
        print("Error processing message:", e)

# MQTT client setup
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.tls_set(CA_CERT_PATH, certfile=CERTIFICATE_PATH, keyfile=PRIVATE_KEY_PATH)
client.on_connect = on_connect
client.on_message = on_message

# Start MQTT in a separate thread
def mqtt_thread():
    print("Connecting to MQTT broker...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()

threading.Thread(target=mqtt_thread, daemon=True).start()

# Graph Setup
fig, ax = plt.subplots()
ax.set_title("Live Power Consumption Prediction")
ax.set_xlabel("Time (Last 50 Readings)")
ax.set_ylabel("Power (mW)")
ax.grid(True)

def update_plot(frame):
    ax.clear()
    ax.set_title("Live Power Consumption Prediction")
    ax.set_xlabel("Time (Last 50 Readings)")
    ax.set_ylabel("Power (mW)")
    ax.grid(True)

    if len(actual_power) > 0:
        x_vals = list(range(len(actual_power)))

        # Plot actual power (solid line)
        ax.plot(x_vals, list(actual_power), 'bo-', label="Actual Power", markersize=5)

        # Plot predicted power (dotted line shifted forward)
        if len(predicted_power) > 0:
            shift = 2  # Shift prediction slightly forward
            x_pred = [x + shift for x in x_vals[-len(predicted_power):]]
            ax.plot(x_pred, list(predicted_power), 'r.--', label="Predicted Power", markersize=5)

        # Add legends
        ax.legend()

# Live updating graph
ani = animation.FuncAnimation(fig, update_plot, interval=1000)

plt.show()

