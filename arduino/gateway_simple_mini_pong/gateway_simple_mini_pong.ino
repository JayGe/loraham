/* This is a beacon for the LoRaHam protocol by KK4VCZ.
 * https://github.com/travisgoodspeed/loraham/
 * altered for uno, dragino board, and ping response
 */

//Please change these two to describe your hardware.
#define CALLSIGN "UGV-MINI"
#define COMMENTS "MINI gw"

// SL18B20 Stuff
#include <OneWire.h> 
#include <DallasTemperature.h>

//#include <SPI.h>
#include <RH_RF95.h>  //See http://www.airspayce.com/mikem/arduino/RadioHead/

// DHT stuff
#include <DHT.h>
#define DHTTYPE DHT22
#define DHTPIN  7

DHT dht(DHTPIN, DHTTYPE);

// SL18B20
#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

/* for feather32u4 
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define VBATPIN A9  */
 
/* for feather m0  
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7 */

//Uncomment this line to use the UART instead of USB in M0.
//#define Serial Serial1

/* Dragino shield for UNO */
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

/* for shield 
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 7
*/
 
 
/* for ESP w/featherwing 
#define RFM95_CS  2    // "E"
#define RFM95_RST 16   // "D"
#define RFM95_INT 15   // "B"
*/
 
/* Feather 32u4 w/wing
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     2    // "SDA" (only SDA/SCL/RX/TX have IRQ!)
*/
 
/* Feather m0 w/wing 
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     6    // "D"
*/
 
/* Teensy 3.x w/wing 
#define RFM95_RST     9   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     4    // "C"
*/

 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.4
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
 
// Blinky on receipt
#define LED 13

//Returns the battery voltage as a float.
float voltage(){
  /*
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
  */
  return 5;
}

void radioon(){
  Serial.println("Mini LoRa Responder!");
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    //while (1);
  }
  Serial.println("LoRa radio init OK!");
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    //while (1);
  }else{
    Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  }
 
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
 
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
  Serial.println("Set power to 23.");
  Serial.print("Max packet length: "); Serial.println(RH_RF95_MAX_MESSAGE_LEN);
}

void radiooff(){
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
}

void setup() {
  pinMode(LED, OUTPUT);     
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  // Start the sensors
  dht.begin();
  sensors.begin(); 

  delay(1000);
  //while (!Serial);
  Serial.begin(9600);
  Serial.setTimeout(10);
  delay(100);
 
  radioon();
  digitalWrite(LED, LOW);

  //Beacon once at startup.
  beacon();
}

//! Uptime in seconds, correcting for rollover.
long int uptime(){
  static unsigned long rollover=0;
  static unsigned long lastmillis=millis();

  //Account for rollovers, every ~50 days or so.
  if(lastmillis>millis()){
    rollover+=(lastmillis>>10);
    lastmillis=millis();
  }

  return(rollover+(millis()>>10));
}

//Transmits one beacon and returns.
void beacon(){
  static int packetnum=0;
  float vcc=voltage();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  Serial.print("DHT22: "); 
  Serial.println(t);
  sensors.requestTemperatures(); // Send the command to get temperature readings 

  float T = sensors.getTempCByIndex(0);
  Serial.print("DS18B20: ");
  Serial.println(T);
  
  //Serial.println("Transmitting..."); // Send a message to rf95_server
  
  char radiopacket[RH_RF95_MAX_MESSAGE_LEN];
  snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
//           "BEACON %s %s VCC=%d.%03d dht:%d.%02dC ds18:%d.%02dC count=%d uptime=%ld",
           "BEACON %s %s VCC=%d.%03d dht=%d.%02d dhh=%d.%02d ds18=%d.%02d count=%d uptime=%ld",
           CALLSIGN,
           COMMENTS,
           (int) vcc,
           (int) (vcc*1000)%1000,
           (int) t,
           (int) (t*100)%100,
           (int) h,
           (int) (h*100)%100,
           (int) T,
           (int) (T*100)%100,
           packetnum,
           uptime());

  radiopacket[sizeof(radiopacket)] = 0;
  
  //Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, strlen((char*) radiopacket));
 
  rf95.waitPacketSent();
  packetnum++;
}

//Handles retransmission of the packet.
bool shouldirt(uint8_t *buf, uint8_t len){
  //Don't RT any packet containing our own callsign.
  if(strcasestr((char*) buf, CALLSIGN)){
    //Serial.println("I've already retransmitted this one.\n");
    return false;
  }
  //Don't RT if the packet is too long.
  if(strlen((char*) buf)>128){
    //Serial.println("Length is too long.\n");
    return false;
  }

  if (strcasestr((char*) buf, "PONG")) {
    Serial.println("Not resending PONG");
    return false; 
  }
  
  //Random backoff if we might RT it.
  delay(random(5000));

  //Don't RT if we've gotten an incoming packet in that time.
  //if(rf95.available()){
  //  Serial.println("Interrupted by another packet.");
  //  return false;
  //}

  //No objections.  RT it!
  return true;
}

//If a packet is available, digipeat it.  Otherwise, wait.
void digipeat(){
  //digitalWrite(LED, LOW);
  //Try to receive a reply.
  if (rf95.available()){
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    int rssi=0;
    float vcc=voltage();
    /*
     * When we receive a packet, we repeat it after a random
     * delay if:
     * 1. It asks to be repeated.
     * 2. We've not yet received a different packet.
     * 3. We've waited a random amount of time.
     * 4. The first word is not RT.
     */
    if (rf95.recv(buf, &len)){
      rssi=rf95.lastRssi();
      //digitalWrite(LED, HIGH);
      //RH_RF95::printBuffer("Received: ", buf, len);
      //Serial.print("Got: ");
      buf[len]=0;
      Serial.println("RX ");

      Serial.println((char*)buf);
      Serial.println("");

      if(shouldirt(buf,len)){
        bool ping_rx=0;
        int rx_count=0;
        // Retransmit.
        Serial.println("TX ");

        if(strcasestr((char*) buf, "PING")){
          Serial.println("Received PING.\n");
          ping_rx=1;
    
          // Read each command pair, spaces
          char* parameters = strtok((char*) buf, " ");
          while (parameters != 0){
            // Split the command in two values when has =
            char* values = strchr(parameters, '=');
            // If we have a value:
            if (values != 0){
              // Actufpongally split the string in 2: replace '=' with 0
              *values = 0;
              //String parameter = command;
              ++values;
              //int position = atoi(values);
              if ((String) parameters == "seq"){
                rx_count = atoi(values);
              }
            }
           // Find the next command in input string
           parameters = strtok(0, " ");
          }
        }
        
        uint8_t data[RH_RF95_MAX_MESSAGE_LEN];
        if (strcasestr((char*) buf, "PING")) {
          snprintf((char*) data,
                   RH_RF95_MAX_MESSAGE_LEN,
                   "PONG %s seq=%d rpt=%d", //Then we append our call and strength
                   CALLSIGN,  //Repeater's callsign.
                   (int) rx_count,
                   (int) rssi //Signal strength, for routing.
                   );  
        } else {
          snprintf((char*) data,
                 RH_RF95_MAX_MESSAGE_LEN,
                 "%s\n" //First line is the original packet.
                 "RT %s rssi=%d VCC=%d.%03d uptime=%ld", //Then we append our call and strength as a repeater.
                 (char*) buf,
                 CALLSIGN,  //Repeater's callsign.
                 (int) rssi, //Signal strength, for routing.
                 (int) vcc,
                 (int) (vcc*1000)%1000,
                 uptime()
                 );
        }
        rf95.send(data, strlen((char*) data));
        rf95.waitPacketSent();
        Serial.println((char*) data);
        //digitalWrite(LED, LOW);
        Serial.println("");
      }else{
        //Serial.println("Declining to retransmit.\n");
      }
    }else{
      Serial.println("Receive failed");
    }
  }else{
    delay(10);
  }
}

void loop(){
  static unsigned long lastbeacon=millis();
  
  //Only digipeat if the battery is in good shape.
  if(voltage()>3.5){
    //Only digipeat when battery is high.
    digipeat();

    //Every ten minutes, we beacon just in case.
    if(millis()-lastbeacon>10*60000){
      beacon();
      lastbeacon=millis();
    }
  }else{
    //Transmit a beacon every ten minutes when battery is low.
    radiooff();
    delay(10*60000);
    radioon();
    beacon();
  };
}

