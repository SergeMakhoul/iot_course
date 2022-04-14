import base64
import datetime
import json
import re
import sys

import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import pymongo

# helper functions


def printline():
    print("_______________\n")


def switch(rc):
    switcher = {
        0: "Connection successful",
        1: "Connection refused – incorrect protocol version",
        2: "Connection refused – invalid client identifier",
        3: "Connection refused – server unavailable",
        4: "Connection refused – bad username or password",
        5: "Connection refused – not authorised"
    }

    return switcher.get(rc, "Invalid return code")


def sendToThingsboard(EUI, payload):
    cred = {"username": EUI, "password": ""}
    publish.single("v1/devices/"+EUI+"/telemetry", payload=payload, qos=0,
                   retain=False, hostname="demo.thingsboard.io", port=1883,
                   client_id="SergeTest", keepalive=60, will=None,
                   auth=cred, tls=None, protocol=mqtt.MQTTv311,
                   transport="tcp")


# connecting to the database
myclient = pymongo.MongoClient("mongodb://localhost:27017/")
mydb = myclient["testDatabase"]
mycol = mydb["data"]

# getting the highest id in order to stack on top of it
idToUse = 1
try:
    idToUse = mycol.find_one(sort=[("_id", -1)])["_id"] + 1
except TypeError:
    print("Collection is empty")
    printline()
except Exception as e:
    print("There was an error:", e)
finally:
    print("_id to begin with:", idToUse)
    printline()

# static data for the server connection

hostname = "212.98.137.194"
port = 1883

# use a specific topic instead of "#" to avoid getting all the data
topic = "#"

# functions for connecting and retrieving the data


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to " + hostname + " with result code: " + str(rc))
        printline()
        client.subscribe(topic)

    else:
        print("There was an error while connecting")
        print("Returned code is:", rc, "-", switch(rc))
        print("System will now stop")
        printline()
        sys.exit()


def on_message(client, userdata, msg):
    global idToUse

    try:
        msg = json.loads(msg.payload)

        if "applicationName" not in msg:
            return

        # creating my own dictionary by formatting the one received
        entry = {}
        entry["_id"] = idToUse
        entry["time"] = datetime.datetime.now()

        for key in msg:
            if key == "rxInfo":
                entry[key] = msg[key][0]

            elif key == "object":
                dic = {}
                data = re.split(':|\n|,|;', msg[key]["payload"])

                for i in range(len(data)):
                    if i % 2 == 0:
                        dic[data[i].strip()] = int(data[i+1].strip())

                sendToThingsboard(msg["devEUI"], json.dumps(dic))

                entry[key] = dic

            else:
                entry[key] = msg[key]

        try:
            # inserting the dictionary into the database
            mycol.insert_one(entry)
            print("Inserted in table with id:", idToUse)
            printline()
            idToUse += 1

        except Exception as e:
            print("There was an error:", e)
            return

    except Exception as e:
        print("There was an error:", e)
        return


if __name__ == "__main__":
    myClient = mqtt.Client("IoT")
    myClient.username_pw_set("username", "password")

    print("Trying to connect to", hostname)
    myClient.connect(hostname, port)

    myClient.on_connect = on_connect
    myClient.on_message = on_message

    myClient.loop_forever()
