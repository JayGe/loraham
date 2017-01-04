/* This is a beacon for the LoRaHam protocol by KK4VCZ.
 * https://github.com/travisgoodspeed/loraham/
 *  
 */

#define CALLSIGN "GI7UGV-1"

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

int packetnum=0; // globally available for button presses

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
  Serial.println("Feather LoRa RX Test!");
  
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
  display.setTextColor(WHITE);
  display.print("GI7UGV");
  display.display();
  display.clearDisplay();

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
  
  //Serial.println("Transmitting..."); // Send a message to rf95_server
  
  char radiopacket[RH_RF95_MAX_MESSAGE_LEN];
  snprintf(radiopacket,
           RH_RF95_MAX_MESSAGE_LEN,
           "BEACON %s VCC=%f count=%d uptime=%ld",
           CALLSIGN,
           (float) voltage(),
           packetnum,
           uptime());

  Serial.print("TX "); Serial.print(packetnum); Serial.print(": "); Serial.println(radiopacket);
  radiopacket[sizeof(radiopacket)] = 0;
  
  //Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, strlen((char*) radiopacket));
 
  //Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  //pinMode(BUTTON_A, INPUT_PULLUP);

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
    Serial.println("Length is too long.\n");
    return false;
  }
  
  //Random backoff if we might RT it.
  delay(random(10000));
  //Don't RT if we've gotten an incoming packet in that time.
  if(rf95.available()){
    Serial.println("Interrupted by another packet.");
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

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print((char*)buf);
  display.print(" rssi:");
  display.print((int) rssi);
  display.print(" ");
  display.print((float) voltage());
  display.print("v");
  display.display();
  display.clearDisplay();

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
        //digitalWrite(LED, LOW);
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
      Serial.println("Button A");
      display.setCursor(0,0);
      display.setTextSize(2);
      display.print("Button A");
      display.display();
      display.clearDisplay();
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
      display.print("Beacon TX ");
      display.print(packetnum);
      display.display();
      display.clearDisplay();
      beacon();
    }
  }
  
  if ( digitalRead(BUTTON_C) != LBUTTON_C_STATE) {
    LBUTTON_C_STATE = digitalRead(BUTTON_C);
    if (! LBUTTON_C_STATE){
      Serial.println("Button C");
      display.setCursor(0,0);
      display.setTextSize(2);
      display.print("Button C");
      display.display();
      display.clearDisplay();
    }
  }

  //Only digipeat if the battery is in good shape.
  if(voltage()>3.5 || voltage()<0.5){
    //Only digipeat when battery is high. <0.5 check for when button A is pressed on the oled board pulling it to 0
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
    delay(10*60000);
    radioon();
    beacon();
  };
}

