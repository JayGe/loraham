/* This is a beacon for the LoRaHam protocol by KK4VCZ.
 * https://github.com/travisgoodspeed/loraham/
 * Altered to send ping messages with B button, display pong responses from other gw, cycle power with C and display ping/beacon counts, A cycles last x messages
 */

#define CALLSIGN "UGV-FTR"

#include <SPI.h>
#include <RH_RF95.h>  //See http://www.airspayce.com/mikem/arduino/RadioHead/
#include <Adafruit_SSD1306.h>

// setup oled screen + buttons
Adafruit_SSD1306 display = Adafruit_SSD1306();

#define BUTTON_A 9 // #9 - GPIO #9, also analog input A7 used for (VBATPIN) which causes some awkwardness
#define BUTTON_B 6
#define BUTTON_C 5

int BUTTON_A_STATE = 1;
int BUTTON_B_STATE = 1;
int BUTTON_C_STATE = 1;

int LBUTTON_A_STATE = 1;
int LBUTTON_B_STATE = 1;
int LBUTTON_C_STATE = 1;

int packetnum=0; 
int pingnum=0; 
int powernum=5;

#define Last_Number 10 // number of messages to store

String Last_Heard[Last_Number]={};

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
 
/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define VBATPIN A7
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.4
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
 
// Blinky on receipt
#define LED 13

//Returns the battery voltage as a float.
float voltage(){
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
}

void radioon(){
  //Serial.println("Feather LoRa RX Test!");
  
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
  
  // Bw125Cr45Sf128 = 0,     ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
  // Bw500Cr45Sf128,            ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
  // Bw31_25Cr48Sf512,    ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
  // Bw125Cr48Sf4096, ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range

  set_radiomode(0);
  
  
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
  //Serial.println("Set power to 23.");
  Serial.print("Max packet length: "); Serial.println(RH_RF95_MAX_MESSAGE_LEN);
}

void radiooff(){
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(10);
  delay(500);
  
 // pinMode(BUTTON_A, INPUT_PULLUP); // Don't use in this way as it's used for analog voltage
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  
  pinMode(LED, OUTPUT);     
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  
  radioon();
  digitalWrite(LED, LOW);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(" Loraham  Gateway");
  display.setTextSize(3);
  display.print("GI7UGV");
  display.display();
  display.clearDisplay();

  //Beacon once at startup.
  beacon(23,0);
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

//Transmits one beacon and returns, takes a power input and whether the message is a PING or not and outputs differently
void beacon(int powernum, bool ping){
  
  //Serial.println("Transmitting..."); // Send a message to rf95_server

  char radiopacket[RH_RF95_MAX_MESSAGE_LEN];
 /* snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
           //"BEACON %s VCC=%f count=%d uptime=%ld pwr=%d",
           "%s %s VCC=%f count=%d uptime=%ld pwr=%d",
           (ping) ? "PING" : "BEACON",
           CALLSIGN,
           (float) voltage(),
           //packetnum,
           (ping) ? pingnum : packetnum,
           uptime(),
           powernum);*/
  if (ping) {
      snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
           //"BEACON %s count=%d pwr=%d",
           "%s %s seq=%d pwr=%d",
           "PING",
           CALLSIGN,
           pingnum,
           powernum);
  } else {
      snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
           //"BEACON %s VCC=%f count=%d uptime=%ld pwr=%d",
           "%s %s VCC=%f count=%d uptime=%ld pwr=%d",
           "BEACON",
           CALLSIGN,
           (float) voltage(),
           packetnum,
           uptime(),
           powernum);
  }
  Serial.print("TX "); Serial.print((ping) ? pingnum : packetnum); Serial.print(": "); Serial.println(radiopacket);
  radiopacket[sizeof(radiopacket)] = 0;

  rf95.setTxPower(powernum, false);
  
  //Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, strlen((char*) radiopacket));
 
  //Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  //pinMode(BUTTON_A, INPUT_PULLUP);

  //packetnum++;
  (ping) ? pingnum++ : packetnum++;
}


//Handles retransmission of the packet.
bool shouldirt(uint8_t *buf, uint8_t len){
 
  //Don't RT any packet containing our own callsign.
  if(strcasestr((char*) buf, CALLSIGN)){
    //Serial.println("I've already retransmitted this one.\n");
    return false;
  }
  if(strcasestr((char*) buf, "PONG")){
    Serial.println("Not resending Pong.\n");
    return false;
  }
  //Don't RT if the packet is too long.
  if(strlen((char*) buf)>128){
    //Serial.println("Length is too long.\n");
    return false;
  }
  
  //Random backoff if we might RT it.
  //delay(random(10000));
  delay(random(2000,3000));
  //Don't RT if we've gotten an incoming packet in that time.
  if(rf95.available()){
    //Serial.println("Interrupted by another packet.");
    return false;
  }

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

    blink_short();
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
      Serial.print("RX ");
      Serial.println((int) rssi);
      Serial.println((char*)buf);
      Serial.println("");

      // text display tests 
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.print((char*)buf);
      display.print(" rssi:");
      display.print((int) rssi);
      display.display();
      display.clearDisplay();

      for (int k = 9; k > 0; k--){   
        Last_Heard[k] = Last_Heard[k-1];
      }

      //Last_Heard[0]=(char*)buf;

      char last_hrdrssi[RH_RF95_MAX_MESSAGE_LEN+64];
      sprintf(last_hrdrssi, "%s rssi:%d", buf, rssi);
      Last_Heard[0]=last_hrdrssi;
     
      if(shouldirt(buf,len)){
        // Retransmit.
        Serial.println("TX ");
        uint8_t data[RH_RF95_MAX_MESSAGE_LEN];
        snprintf((char*) data,
                 RH_RF95_MAX_MESSAGE_LEN,
                 "%s\n" //First line is the original packet.
                 "RT %s rssi=%d VCC=%f uptime=%ld", //Then we append our call and strength as a repeater.
                 (char*) buf,
                 CALLSIGN,  //Repeater's callsign.
                 (int) rssi, //Signal strength, for routing.
                 voltage(), //Repeater's voltage
                 uptime()
                 );
        rf95.send(data, strlen((char*) data));
        rf95.waitPacketSent();
        Serial.println((char*) data);
        Serial.println("");
      }else{
        Serial.println("Declining to retransmit.\n");
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

  if ( analogRead(BUTTON_A) < 30 ) { // analogRead on button_a as its shared with vcc
    BUTTON_A_STATE = 0;
    if ( BUTTON_A_STATE != LBUTTON_A_STATE) {
      LBUTTON_A_STATE = 0;
      static int Times_Pressed=Last_Number-1;

      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.print(Times_Pressed);
      display.print(" ");
      display.print(Last_Heard[Times_Pressed]);
      display.display();
      display.clearDisplay();

      /*for (int i = 0; i < 5; i++){;
        Serial.print(i+1);
        Serial.print(" ");
        Serial.println(Last_Heard[i]);
      }*/
      if (Times_Pressed > 0) { //counting down so tx/rt makes more sense
        Times_Pressed--;
      } else {
        Times_Pressed = Last_Number-1;
      }

    }
  } else {
    LBUTTON_A_STATE = 1;
  }

  if ( digitalRead(BUTTON_B) != LBUTTON_B_STATE) { // On B press, send beacon out.
    LBUTTON_B_STATE = digitalRead(BUTTON_B);
    if (! LBUTTON_B_STATE){
      Serial.println("Button B");
      display.setCursor(0,0);
      display.setTextSize(2);
      display.print("TX PING ");
      display.print(pingnum);
      display.print(" PWR ");
      display.print(powernum);
      display.display();
      display.clearDisplay();
      beacon(powernum, 1);
    }
  }
  
  if ( digitalRead(BUTTON_C) != LBUTTON_C_STATE) { // On C cycle through 5 last heard, not working well
    LBUTTON_C_STATE = digitalRead(BUTTON_C);
    if (! LBUTTON_C_STATE){
      if (powernum==23) {
        powernum = 5;
      } else {
        powernum++;
      }
      display.setCursor(0,0);
      display.setTextSize(2);
      display.print("Power:");

      display.println(powernum);
      display.setTextSize(1);

      display.print("Beacon: ");
      display.print(packetnum);
      display.print(" Ping: ");
      display.print(pingnum);
      Serial.println("Button C");
      display.print("\nVCC: ");
      display.print(voltage());
      Serial.println(voltage());
      
      display.display();
      display.clearDisplay();
    }
  }
  
    digipeat(); //copy out of voltage check below 

    //Every ten minutes, we beacon just in case.
    if(millis()-lastbeacon>10*60000){
      beacon(23,0);
      lastbeacon=millis();
    }
    
/* Skipping all of this while using with a oled for testing as the pin sharing with the button sometimes triggers the below still, might want to bridge button to different pin
  //Only digipeat if the battery is in good shape.
  if(voltage()>3.5 || voltage()<1){
    //Only digipeat when battery is high. <1 check for when button A is pressed on the oled board pulling it to 0
    digipeat();

    //Every ten minutes, we beacon just in case.
    if(millis()-lastbeacon>10*60000){
      beacon();
      lastbeacon=millis();
    }
  } else {
    //Transmit a beacon every ten minutes when battery is low.
    radiooff();
    Serial.println("Low Battery Pause");
    display.setCursor(0,0);
    display.setTextSize(1);
    display.print("ZZZZZZZZ");
    display.print(packetnum);
    display.display();
    display.clearDisplay();
    delay(10*60000);
    radioon();
    beacon();
  };
  */
}

void blink_long() { 
  digitalWrite(LED,HIGH);
  delay(300);
  digitalWrite(LED,LOW);
  delay(25);
}

void blink_short() { 
  digitalWrite(LED,HIGH);
  delay(100);
  digitalWrite(LED,LOW);
  delay(25);
}


void set_radiomode(int mode) {
  if (mode == 0) { 
    rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
  } else if (mode == 1) {
    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
  } else if (mode == 2) {
    rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);
  } else if (mode == 3) {
    rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);
  }
}

