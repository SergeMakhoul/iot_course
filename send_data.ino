#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <DHT.h>
#include <DHT_U.h>

#define LMIC_DEBUG_LEVEL 0

/////////// defining pin for temp and humidity using its library
#define DHTPIN A0
#define DHTTYPE DHT11

DHT_Unified dht(DHTPIN, DHTTYPE);

#define LightPin A1


/////////////////////////////////////////////////////////////////////


// This should be in little endian format.
static const u1_t PROGMEM DEVEUI[8] = {0xfb, 0xdd, 0x83, 0x3a, 0x39, 0x6a, 0x8d, 0x6c};
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

static const u1_t PROGMEM APPKEY[16] = {0x37, 0xda, 0x53, 0xb2, 0x16, 0xbf, 0x56, 0xc2, 0xc6, 0x9b, 0xd6, 0x30, 0xa8, 0x09, 0x7b, 0x1e};
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

////////////////////////////////////////////////////////////////////

static osjob_t sendjob;

// Schedule TX every this many seconds
const unsigned TX_INTERVAL = 40;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 9,
  .dio = {2, 6, 7},
};


///////////////////////////
// constant sensor pins
//const int tempPin = 7;
//const int humidPin = 8;
//const int phPin = 9;
//const int lightPin = 10;
//const int moisturePin = 11;
///////////////////////////


////////////////////////////////////////////////////////////////////

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      // After a call to LMIC_enableTracking() no beacon was received within the beacon interval. Tracking needs to be restarted.
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    
    case EV_BEACON_FOUND:
      // After a call to LMIC_enableTracking() the first beacon has been received within the beacon interval.
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    
    case EV_BEACON_MISSED:
      // No beacon was received at the expected time.
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    
    case EV_BEACON_TRACKED:
      // The next beacon has been received at the expected time.
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    
    case EV_JOINING:
      // The node has started joining the network.
      Serial.println(F("EV_JOINING"));
      break;
    
    case EV_JOINED:
      // The node has successfully joined the network and is now ready for data exchanges.
      Serial.println(F("EV_JOINED"));
      break;
    
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    
    case EV_JOIN_FAILED:
      // The node could not join the network (after retrying).
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    
    case EV_REJOIN_FAILED:
      // The node did not join a new network but is still connected to the old network.
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    
    case EV_TXCOMPLETE:
      /* The data prepared via LMIC_setTxData() has been sent, and eventually downstream data has been received in return.
      If confirmation was requested, the acknowledgement has been received. */
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      
      if (LMIC.dataLen) {
        // data received in rx slot after tx
        Serial.print(F("Data Received: "));
        Serial.write(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
        Serial.println();
      }
      
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    
    case EV_LOST_TSYNC:
      // Beacon was missed repeatedly and time synchronization has been lost. Tracking or pinging needs to be restarted.
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    
    case EV_RESET:
      // Session reset due to rollover of sequence counters. Network will be rejoined automatically to acquire new session.
      Serial.println(F("EV_RESET"));
      break;
    
    case EV_RXCOMPLETE:
      // Downstream data has been received.
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    
    case EV_LINK_DEAD:
      // No confirmation has been received from the network server for an extended period of time. Transmissions are still possible but their reception is uncertain.
      Serial.println(F("EV_LINK_DEAD"));
      break;
    
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

////////////////////////////////////////////////////////////////////

float getTemperature(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  return event.temperature;
  }

float getHumidity(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);

  return event.relative_humidity;
  }

float getLight(){
  return 100 * (1 - analogRead(LightPin)/1024);
}

////////////////////////////////////////////////////////////////////

byte buffer[52];
String mychar = String('"')

void do_send(osjob_t* j) {  
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {    
    // Prepare upstream data transmission at the next possible time.
    
    String message;
    message += mychar + "Light : " + mychar + String(getLight());
    message += mychar + ", Temperature : " + mychar + String(getTemperature());
    message += mychar + ", Humidity : " + mychar + String(getHumidity());
    
    message.getBytes(buffer, message.length() + 1);
    Serial.println("Sending: " + message);
    
    LMIC_setTxData2(1, (uint8_t*) buffer, message.length() , 0);
    Serial.println(F("Packet queued"));
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

// Leave this empty
static const u1_t PROGMEM APPEUI[8] = {};
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting"));

  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);

#ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
#endif

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7B),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

  // Disable link check validation
  LMIC_setLinkCheckMode(0);
//  LMIC_setDrTxpow(DR_SF7, 23);
//  LMIC_setDrTxpow(DR_SF8, 23);
//  LMIC_setDrTxpow(DR_SF9, 23);
//  LMIC_setDrTxpow(DR_SF10, 23);
//  LMIC_setDrTxpow(DR_SF11, 23);
//  LMIC_setDrTxpow(DR_SF12, 23);

  // Start job
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
