/***************************************************************
*                                                              *
*            Taiko controller for Arduino Leonardo             *
*          and models with ATmega32u4 microprecessors          *
*                                                              *
*                      Erick Andersson                         *
*                erick.andersson98@gmail.com                   *
*                                                              *
****************************************************************/
// Compatibility with Taiko Jiro
#define MODE_JIRO 1

#define MODE_DEBUG 0

#define CHANNELS 4

// Caches for the soundwave and power
#define SAMPLE_CACHE_LENGTH 12
#define POWER_CACHE_LENGTH 3

// Light and heavy hit thresholds

#define LIGHT_THRES_DON 150000
#define HEAVY_THRES_DON 500000

#define LIGHT_THRES_KAT 150000
#define HEAVY_THRES_KAT 500000

// Forced sampling frequency
#define FORCED_FREQ 1000

//#include <ctime>
#include <limits.h>
#include <Keyboard.h>
#include "cache.h"

#if MODE_JIRO
#define HEAVY_THRES LONG_MAX
#endif

const long int TIME_THRESH = 10; // In milliseconds

unsigned long int lastTime;

char lastHit = 'z';
unsigned long int lastHitTimestamp;

int channelSample [CHANNELS];
int lastChannelSample [CHANNELS];
Cache <int, SAMPLE_CACHE_LENGTH> sampleCache [CHANNELS];

long int power [CHANNELS];
Cache <long int, POWER_CACHE_LENGTH> powerCache [CHANNELS];

bool triggered [CHANNELS];

int pins[] = {A0, A1, A2, A3};  // R kat, R don, L don, L kat
char lightKeys[] = {'j', 'h', 'g', 'f'};
char heavyKeys[] = {'t', 'y', 'r', 'u'};

void setup() {
  Serial.begin (9600);
  Keyboard.begin ();
  analogReference (DEFAULT);
  for (short int i = 0; i < CHANNELS; i++) {
    power [i] = 0;
    lastChannelSample [i] = 0;
    triggered [i] = false;
  }
  lastTime = millis();
  lastHitTimestamp = millis();
}

void loop() {
  
  for (short int i = 0; i < CHANNELS; i++) {
    
    channelSample[i] = analogRead (pins [i]);
    sampleCache [i].put (channelSample [i] - lastChannelSample [i]);
    
    long int tempInt;
    tempInt = sampleCache [i].get (1);
    power [i] -= tempInt * tempInt;
    tempInt = sampleCache [i].get ();
    power [i] += tempInt * tempInt;
    
    if ((i == 0)||(i == 3)){ //Kat
      if (power [i] < LIGHT_THRES_KAT) {
        power [i] = 0;
      }
    }
    else { //Don
      if (power [i] < LIGHT_THRES_DON) {
        power [i] = 0;
      }
    }
    
    powerCache [i].put (power [i]);
    lastChannelSample [i] = channelSample [i];
    if (powerCache [i].get (1) == 0) {
      triggered [i] = false;
    }

    if (!triggered [i]) {
      long int actualTime;
      for (short int j = 0; j < POWER_CACHE_LENGTH - 1; j++) {
        if ((i == 0) || (i == 3)){ //Kat
          if (powerCache [i].get (j - 1) >= powerCache [i].get (j)) {
            break;
          } else if (powerCache [i].get (1) >= HEAVY_THRES_KAT) {
            triggered [i] = true;
            Keyboard.print (heavyKeys [i]);
          } else if (powerCache [i].get (1) >= LIGHT_THRES_KAT) {
            actualTime = millis(); // get the actual time

            if (lastHit != lightKeys[i]){ // look for the time of the last hit
              // if the time between the last and current hit is bigger than a given time, accept it.
              if (actualTime-lastHitTimestamp > TIME_THRESH){
                triggered [i] = true;
                Keyboard.print (lightKeys [i]);
                lastHit = lightKeys[i];    
              }
              // otherwise, drop the input.
            }
          }
        }
        else { //Don
          if (powerCache [i].get (j - 1) >= powerCache [i].get (j)) {
            break;
          } else if (powerCache [i].get (1) >= HEAVY_THRES_DON) {
            triggered [i] = true;
            Keyboard.print (heavyKeys [i]);
          } else if (powerCache [i].get (1) >= LIGHT_THRES_DON) {
            actualTime = millis(); // get the actual time

            if (lastHit != lightKeys[i]){ // look for the time of the last hit
              // if the time between the last and current hit is bigger than a given time, accept it.
              if (actualTime-lastHitTimestamp > TIME_THRESH){
                triggered [i] = true;
                Keyboard.print (lightKeys [i]);
                lastHit = lightKeys[i];    
              }
              // otherwise, drop the input.
            }
          }
        }

        lastHitTimestamp = actualTime; 
      }
    }
    
    #if MODE_DEBUG
        Serial.print (power [i]);
        Serial.print ("\t");
    #endif

  // End of each channel
  }

  #if MODE_DEBUG
    Serial.print (50000);
    Serial.print ("\t");
    Serial.print (0);
    Serial.print ("\t");

    Serial.println ("");
  #endif

  // Force the sample frequency to be less than 1000Hz
  unsigned int frameTime = micros () - lastTime;
  lastTime = micros ();
  if (frameTime < FORCED_FREQ) {
    delayMicroseconds (FORCED_FREQ - frameTime);
  } else {
    // Performance bottleneck;
    Serial.print ("Exception: forced frequency is too high for the microprocessor to catch up.");
  }
}
