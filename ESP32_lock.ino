//ESP32 lock mechanism with EspMQTTClient library.
// Aug. 28, 2022. (first version)

#include "EspMQTTClient.h"

EspMQTTClient *client;

//input & output pins
const int JEMAOUT=13; //photo relay to JEM-A control line
const int JEMA_ON=1; //value for pulse on
const int JEMA_OFF=0;//value for pulse off
const int JEMAIN=12; //photo relay driven by JEM-A monitor line. Pull-up. 1;Secure 0:Unsecure
const int JEMA_U=0; //value for the lock is Unsecure (Unlocked)
const int JEMA_S=1; //value for the lock is Secure (Locked)

int JEMA_last; //previous value read from JEMAIN. 1:Secure 0:Unsecure
int JEMA_current() { return(digitalRead(JEMAIN)); } //current JEMAIN value. 
boolean JEMA_pulsed; //if a pluse has been sent to JEMAOUT
//WiFi
const char SSID[] = "XXXXXXXX"; //WiFi SSID
const char PASS[] = "XXXXXXXX"; //WiFi password
//MQTT
char CLIENTID[] = "ESP32_xx:xx:xx:xx:xx:xx"; //MAC address is set in setup()
//for example, this will be set to "ESP32_84:CC:A8:7A:5F:44"
const char  MQTTADD[] = "192.168.xxx.xxx"; //Broker IP address
const short MQTTPORT = 1883; //Broker port
const char  MQTTUSER[] = "xxxxxxxx";//Can be omitted if not needed
const char  MQTTPASS[] = "XXXXXXXX";//Can be omitted if not needed
const char  SUBTOPIC[] = "mqttthing/lock/setTS"; //mqtt topic to subscribe
const char  PUBTARGET[] = "mqttthing/lock/getTS"; //mqtt topic to publish
const char  PUBCURRENT[] = "mqttthing/lock/getCS"; //mqtt topic to publish
const char  PUBDEBUG[] = "mqttthing/lock/debug"; //for debug message

void setup() {
  //Digital I/O
  pinMode(JEMAOUT, OUTPUT);
  pinMode(JEMAIN, INPUT_PULLUP);
  JEMA_last=JEMA_current();
  //Serial
  Serial.begin(115200);
  while (!Serial);
  Serial.println("ESP32 Lock Mechanism started.");
  //MQTT
  String wifiMACString = WiFi.macAddress(); //WiFi MAC address
  wifiMACString.toCharArray(&CLIENTID[6], 18, 0); //"ESP32_xx:xx:xx:xx:xx:xx"
  Serial.print("SSID: ");Serial.println(SSID);
  Serial.print("MQTT broker address: ");Serial.println(MQTTADD);
  Serial.print("MQTT clientID: ");Serial.println(CLIENTID);
  client = new EspMQTTClient(SSID,PASS,MQTTADD,MQTTUSER,MQTTPASS,CLIENTID,MQTTPORT); 
}

void onMessageReceived(const String& msg) {
  //Serial.println(msg);
  client->publish(PUBDEBUG, "Message received.");
  boolean newValue, changed=false;
  if(msg.compareTo("U")==0) { //target state is Unsecure (unlock)
    if(JEMA_current() == false) return; //already Unsecured (unlocked). do nothing.
  }
  else if(msg.compareTo("S")==0) { //target state is Secure (lock)
    if(JEMA_current() == true) return; //already Secured. do nothing.
  }
  //now send a pluse to toggle the electric lock
  digitalWrite(JEMAOUT, JEMA_ON); //activate JEM-A control line for 0.25 sec
  delay(250); 
  digitalWrite(JEMAOUT, JEMA_OFF);
  JEMA_pulsed=true;
}

void onConnectionEstablished() {
  Serial.println("WiFi/MQTT onnection established.");
  client->subscribe(SUBTOPIC, onMessageReceived); //set callback function
  client->publish(PUBDEBUG, "ESP32 Lock Mechanism is ready.");
}

void loop() {
  
  client->loop();
  int newvalue = JEMA_current();//get current JEMA value
  if(newvalue == JEMA_last) return; //no change, do nothing

  //JEMA monitor status has chenged
  if(newvalue == JEMA_S){
    if(!JEMA_pulsed) client->publish(PUBTARGET,"S");
    client->publish(PUBCURRENT,"S");
  }
  else if(newvalue == JEMA_U){
    if(!JEMA_pulsed) client->publish(PUBTARGET,"U");
    client->publish(PUBCURRENT,"U");
  }
  JEMA_last = newvalue;
  JEMA_pulsed = false;
}
