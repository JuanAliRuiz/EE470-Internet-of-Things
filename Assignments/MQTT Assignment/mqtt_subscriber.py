import paho.mqtt.client as mqtt
import mysql.connector

# ---------------------------
# MQTT SETTINGS
# ---------------------------
BROKER_URL  = "broker.hivemq.com"
BROKER_PORT = 8000                        # Websocket port
TOPIC       = "testtopic/temp/outTopic/PotentiometerAli" 

# ---------------------------
# MYSQL SETTINGS (Hostinger)
# ---------------------------
HOST     = "195.35.61.68"
USER     = "u148378080_db_juanaliruiz"
PASSWORD = "Jazmine03232018!"
DATABASE = "u148378080_juanaliruiz"

# ------------------------------------------
# DATABASE INSERT FUNCTION
# ------------------------------------------
def push_value_to_db(sensor_value):
    try:
        connection = mysql.connector.connect(
            host=HOST,
            user=USER,
            password=PASSWORD,
            database=DATABASE
        )
        cursor = connection.cursor()
        query = "INSERT INTO sensor_value (value) VALUES (%s)"
        cursor.execute(query, (sensor_value,))
        connection.commit()
        print(f"  -> Inserted {sensor_value} into sensor_value table")

    except mysql.connector.Error as err:
        print(f"[DB ERROR] {err}")

    finally:
        try:
            cursor.close()
            connection.close()
        except:
            pass

# ------------------------------------------
# MQTT CALLBACK: ON CONNECT
# ------------------------------------------
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to HiveMQ broker!")
        client.subscribe(TOPIC)
        print(f"Subscribed to topic: {TOPIC}")
    else:
        print(f"[ERROR] Connection failed, code {rc}")

# ------------------------------------------
# MQTT CALLBACK: ON MESSAGE
# ------------------------------------------
def on_message(client, userdata, msg):
    raw = msg.payload.decode().strip()
    print(f"Received message: {raw} from topic: {msg.topic}")

    # Try converting payload to a number
    try:
        sensor_value = float(raw)
    except ValueError:
        print("  -> Payload not numeric, skipping DB insert")
        return

    # Insert into DB
    push_value_to_db(sensor_value)

# ------------------------------------------
# MQTT CLIENT SETUP
# ------------------------------------------
client = mqtt.Client(transport="websockets") 
client.on_connect = on_connect
client.on_message = on_message

print("Connecting to broker...")
client.connect(BROKER_URL, BROKER_PORT, 60)

# Blocking loop — runs until Ctrl+C
client.loop_forever()
