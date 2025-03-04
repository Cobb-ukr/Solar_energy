import json
import os
import paho.mqtt.client as mqtt

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

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to MQTT broker successfully")
        client.subscribe(MQTT_TOPIC)
        print(f"Subscribed to topic: {MQTT_TOPIC}")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    try:
        # Decode the message payload
        payload = msg.payload.decode()
        print(f"Received message on {msg.topic}: {payload}")

        # Parse the JSON payload (if applicable)
        try:
            data = json.loads(payload)
            print("Parsed JSON values:")
            for key, value in data.items():
                print(f"{key}: {value}")
        except json.JSONDecodeError:
            print("Message is not in JSON format. Raw message printed above.")
    except Exception as e:
        print("Error processing message:", e)

# MQTT client setup
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.tls_set(CA_CERT_PATH, certfile=CERTIFICATE_PATH, keyfile=PRIVATE_KEY_PATH)
client.on_connect = on_connect
client.on_message = on_message

# Connect and start loop
print("Connecting to MQTT broker...")
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_forever()