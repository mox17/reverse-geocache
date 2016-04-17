/*
reverse_geocache_sample_for_rev_2_shield.ino
Sample Arduino Puzzle Box sketch
Written by Mikal Hart
COPYRIGHT (c) 2008-2013 The Sundial Group
All Rights Reserved.

  http://www.sundial.com
 
This software is licensed under the terms of the Creative
Commons "Attribution Non-Commercial Share Alike" license, version
3.0, which grants the limited right to use or modify it NON-
COMMERCIALLY, so long as appropriate credit is given and
derivative works are licensed under the IDENTICAL TERMS.  For
license details see

  http://creativecommons.org/licenses/by-nc-sa/3.0/
 
This source code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
This sketch illustrates how one might implement a basic puzzle box 
that incorporates rudimentary aspects of the technology in the
Reverse Geocache(tm) puzzle and the Sundial Quest Box(tm).
 
"Reverse Geocache" and "Quest Box" are trademarks of The Sundial Group.

Certain technologies used here are Patent Pending.

For supporting libraries (PWMServo, TinyGPS) see

  http://arduiniana.org
  
This code is available at

  http://www.sundial.com

*/

#include <PWMServo.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

/*
 Note: This code is designed to work only with Reverse Geocache(tm)
 shields rev 2.03 or greater made by The Sundial Group.

 The following 5 values should be adjusted according to your situation.
 The easiest way to get a latitude and longitude is to right-click in 
 Google Maps and select "What's here?".
 
 You might also wish to alter the messages that are printed in the code body.
*/

// -------------------------------------------------------------------
static const int   SERVO_CLOSED_ANGLE = 90;    // degrees (0-180)
static const int   SERVO_OPEN_ANGLE = 165;     // degrees (0-180)
static const float DEST_LATITUDE =  65.090268;  // degrees (-90 to 90)
static const float DEST_LONGITUDE = 25.708708; // degrees (-180 to 180)
static const int   RADIUS = 50;              // meters
static const int   LCD_CONTRAST = 20;          // (0-255)
// -------------------------------------------------------------------

/* Fixed values should not need changing */
static const int DEF_ATTEMPT_MAX = 50;
static const int EEPROM_OFFSET = 100;

/* Pin assignments */
static const int GPS_RX_PIN = 4, GPS_TX_PIN = 3; // GPS
static const int LCD_ENABLE_PIN = 7, LCD_RS_PIN = 5, LCD_RW_PIN = 8; // LCD
static const int LCD_DB4_PIN = 14, LCD_DB5_PIN = 15, LCD_DB6_PIN = 16, LCD_DB7_PIN = 17;
static const int LCD_CONTRAST_PIN = 6; 
static const int POLOLU_SWITCH_PIN = 12; // Pololu switch control
static const int SERVO_CONTROL_PIN = 9; // Servo control
static const int LED_PIN = 2; // The button LED
static const int PIEZO_PIN = 11; // Piezo buzzer outlet

/* The basic objects needed */
static SoftwareSerial ss(GPS_RX_PIN, GPS_TX_PIN);
static LiquidCrystal lcd(LCD_RS_PIN, LCD_RW_PIN, LCD_ENABLE_PIN, LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN);
static TinyGPS tinyGps; 
static int attemptCounter;
static PWMServo servo;

/* The Arduino setup() function */
void setup()
{
  /* Uncomment this code if you want to reset the attempt counter. */
  
    //EEPROM.write(EEPROM_OFFSET, 0);
    //exit(0);
  

  /* First, make sure Pololu switch pin is OUTPUT and LOW */
  pinMode(POLOLU_SWITCH_PIN, OUTPUT);
  digitalWrite(POLOLU_SWITCH_PIN, LOW);

  /* attach servo motor */
  servo.attach(SERVO_CONTROL_PIN);

  /* establish a debug session with a host computer */
  Serial.begin(115200);

  /* establish communications with the GPS module */
  ss.begin(38400);

  /* set the LCD contrast value */
  pinMode(LCD_CONTRAST_PIN, OUTPUT);
  analogWrite(LCD_CONTRAST_PIN, LCD_CONTRAST);

  /* establish communication with 8x2 LCD */
  lcd.begin(16, 2); // this for an 8x2 LCD -- adjust as needed 
  
  /* turn on the LED in the button */
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  /* make sure motorized latch is closed */
  servo.write(SERVO_CLOSED_ANGLE); 
  
  /* read the attempt counter from the EEPROM */
  attemptCounter = EEPROM.read(EEPROM_OFFSET);
  if (attemptCounter == 0xFF) // brand new EEPROM?
    attemptCounter = 0;

  /* increment it with each run */
  ++attemptCounter;

  /* Copyright notice */
  //Msg(lcd, "(C) 2013", "Sundial", 1500);

  /* Greeting */
  Msg(lcd, "Welcome to your", "puzzle box!",  3000);

  /* Game over? */
  if (attemptCounter >= DEF_ATTEMPT_MAX)
  {
    Msg(lcd, "Sorry! No more", "attempts allowed!", 4000);
    PowerOff();
  }

  /* Print out the attempt counter */
  Msg(lcd, "This is", "attempt", 2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(attemptCounter);
  lcd.print(" of "); 
  lcd.print(DEF_ATTEMPT_MAX);
  delay(2000);

  /* Save the new attempt counter */
  EEPROM.write(EEPROM_OFFSET, attemptCounter);

  Msg(lcd, "Seeking GPS", "Signal...", 0);
}

/* The Arduino loop() function */
void loop()
{
  /* Has a valid NMEA sentence been parsed? */
  if (ss.available() && tinyGps.encode(ss.read()))
  {
    float lat, lon;
    unsigned long fixAge;

    /* Have we established our location? */
    tinyGps.f_get_position(&lat, &lon, &fixAge);
    if (fixAge != TinyGPS::GPS_INVALID_AGE)
    {
      /* We got a fix! */
      Chirp(true);
      
      /* Calculate the distance to the destination */
      float distance_meters = TinyGPS::distance_between(lat, lon, DEST_LATITUDE, DEST_LONGITUDE);

      /* Are we close?? */
      if (distance_meters <= RADIUS)
      {
        Msg(lcd, "Access", "granted!", 2000);
        servo.write(SERVO_OPEN_ANGLE);
      }

      /* Nope.  Print the distance. */
      else
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Distance");
        lcd.setCursor(0, 1);
        if (distance_meters < 1000)
        {
          lcd.print((int)distance_meters);
          lcd.print(" m.");
        }

        else
        {
          lcd.print((int)(distance_meters / 1000));
          lcd.print(" km.");
        }
        delay(4000);
        Msg(lcd, "Access", "Denied!", 2000);
      }

      PowerOff();
    }
  }

  /* Turn off after 5 minutes */
  if (millis() >= 300000)
    PowerOff();
}

/* Called to shut off the system using the Pololu switch */
void PowerOff()
{
  Chirp(false);
  Msg(lcd, "Powering", "Off!", 2000);
  lcd.clear(); 
  
  /* Bring Pololu switch control pin HIGH to turn off */
  digitalWrite(POLOLU_SWITCH_PIN, HIGH);

  /* This is the back door.  If we get here, then the battery power */
  /* is being bypassed by the USB port.  We'll wait a couple of */
  /* minutes and then grant access. */
  delay(120000);
  servo.write(SERVO_OPEN_ANGLE); // and open the box 

  /* Reset the attempt counter */
  EEPROM.write(EEPROM_OFFSET, 0); 
  
  /* Leave the latch open for 10 seconds */
  delay(10000); 

  /* And then seal it back up */
  servo.write(SERVO_CLOSED_ANGLE); 

  /* Exit the program for real */
  exit(1);
} 

/* A helper function to display messages of a specified duration */
void Msg(LiquidCrystal &lcd, const char *top, const char *bottom, unsigned long del)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(top);
  lcd.setCursor(0, 1);
  lcd.print(bottom);
  delay(del);
}

static void Chirp(bool up)
{
  static const int A = 1760;
  static const int E = 2637;
  static const int CS = 2218;
  static const int duration = 100;

  int tone1 = up ? A : E;
  int tone2 = up ? E : A;

  ss.end();
  tone(PIEZO_PIN, tone1, duration);
  delay(duration);
  noTone(PIEZO_PIN);
  tone(PIEZO_PIN, tone2, duration);
  delay(duration);
  noTone(PIEZO_PIN);
  ss.begin(4800);
}


