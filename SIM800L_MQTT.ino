#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_FONA.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_FONA.h>

#define FONA_RX 11
#define FONA_TX 10
#define FONA_RST 7

SoftwareSerial SIM800ss = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA SIM800 = Adafruit_FONA(FONA_RST);

int net_status;
boolean gprs_on = false;
boolean tcp_on = false;

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Momshad"
#define AIO_KEY         "a4cd964d9a2d411d844a18028399de5e"

// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&SIM800, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

// FONAconnect is a helper function that sets up the FONA and connects to
// the GPRS network. See the fonahelper.cpp tab above for the source!
//boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);


// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/photocell");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

uint8_t txfailures = 0;
#define MAXTXFAILURES 3

void sim800l_setup() {
  if (! SIM800.begin(SIM800ss)) {
    Serial.println("Couldn't find SIM800L");
    while (1);
  }

  Serial.println("SIM800L is OK");
  delay(1000);

  //Network Registration:
  Serial.println("Waiting to be registered to network...");
  net_status = SIM800.getNetworkStatus();
  while (net_status != 1) {
    net_status = SIM800.getNetworkStatus();
    delay(2000);
  }
  Serial.println("Registered to home network!");

  //Enabling GPRS:
  Serial.print("Turning on GPRS... ");
  delay(2000);
  while (!gprs_on) {
    if (!SIM800.enableGPRS(true)) {
      Serial.println("Failed to turn on GPRS");
      Serial.println("Trying again...");
      delay(2000);
      gprs_on = false;
    } else {
      Serial.println("GPRS now turned on");
      delay(2000);
      gprs_on = true;
    }
  }
}

void updateSerial()
{
  delay(500);
  while (Serial.available())
    SIM800ss.write(Serial.read());//Forward what Serial received to Software Serial Port
  while (SIM800ss.available())
    Serial.write(SIM800ss.read());//Forward what Software Serial received to Serial Port
}

void setup() {
  while (!Serial);

  Serial.begin(9600);

  Serial.println("Initializing SIM800L....");

  SIM800ss.begin(4800); // if you're using software serial

  sim800l_setup();
  if (gprs_on) {
    mqtt.subscribe(&onoffbutton);
  }
}

uint32_t x = 0;

void loop() {
  updateSerial();

  MQTT_connect();
  // Now we can publish stuff!
  Serial.print(F("\nSending photocell val "));
  Serial.print(x);
  Serial.print("...");
  if (! photocell.publish(x++)) {
    Serial.println(F("Failed"));
    txfailures++;
  } else {
    Serial.println(F("OK!"));
    txfailures = 0;
  }

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
    }
  }

}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(2000);  //  prev = wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}
