import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import json


def sendToThingsboard(EUI, payload):
    cred = {"username": EUI, "password": ""}
    publish.single("v1/devices/"+EUI+"/telemetry", payload=payload, qos=0,
                   retain=False, hostname="demo.thingsboard.io", port=1883,
                   client_id="SergeTest", keepalive=60, will=None,
                   auth=cred, tls=None, protocol=mqtt.MQTTv311,
                   transport="tcp")


if __name__ == '__main__':
    toSend = json.dumps({"temperature": 32, "humidity": 60, "light": 78})
    print(toSend)
    sendToThingsboard("enter EUI here", toSend)
