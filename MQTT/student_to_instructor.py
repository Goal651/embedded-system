import paho.mqtt.client as mqtt


# where you subscribe
def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(
        "/instructor_gabriel"
    )  

# where you listens to messages from the publisher
def on_message(client, userdata, msg):
    print("Instructor Gabriel: " + str(msg.payload.decode()))


# Display messages from the Instructor
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Connect to the MQTT broker
client.connect("82.165.97.169", 1883, 60)

# Start the network loop
client.loop_start()
while True:
    message = input("You: ")
    client.publish("/bugiri_wilson_goal", message)
