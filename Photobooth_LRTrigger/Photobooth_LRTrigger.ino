/* 
Photobooth Program

(1) Run photo capture on LR --> connect camera via USB to computer
(2) Plug Atmega32u4 based MCU into computer running it (also via USB)
(3) When trigger button on arduino is pressed, it sends the 'F12' keyboard command to take a photo
 */
#include <Keyboard.h>

#define FASTLED_INTERNAL      // Stop the annoying messages during compile
#include "FastLED.h"
#define NUM_LEDS 24 
#define DATA_PIN A0
CRGB leds[NUM_LEDS];


int switchState;
int prev_switch = LOW;

void setup() 
{
  LEDS.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
  LEDS.setBrightness(84);

  pinMode(A2, INPUT_PULLUP);

  // initialize control over the keyboard:
  Keyboard.begin();
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(200); } }

void TakePhoto()
{
  
  delay(80);

  fill_solid(leds, NUM_LEDS, CRGB::Green);

  delay(50);
  FastLED.show(); 


  // Countdown ring
  for(int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
    FastLED.show(); 
    delay(50);

    // Trigger taking photo part way through
    if (i == 18)                        // Timing worked out this way --> camera takes time to focus, etc
      Keyboard.write(KEY_F12); 
  }


  // Pretend to be a camera flash
  fill_solid(leds, NUM_LEDS, CRGB::White);      // Don't do this in any other colour than white because it often happens when the photo is taken
  FastLED.show(); 

  delay(100);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  delay(100);

  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show(); 

  delay(100);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show(); 

  // Loading sequence while photo is sent to computer via USB 
  // I stopped doing photobooths with this program beause it was taking too long to transfer photos
  for(uint8_t i = 0; i < NUM_LEDS; i++)
    leds[i].setRGB( 0, 0, 15);
  FastLED.show(); 

  // wait for photo to be sent from camera to computer
  for(uint8_t i = NUM_LEDS; i >= 0; i--)
  {
    leds[i] = CRGB::Black;
    FastLED.show(); 
    delay(300);
  }

//    delay(3000); 

  // Record switch state
  switchState = digitalRead(A2); 
  prev_switch = switchState;
}

void loop() 
{
  switchState = digitalRead(A2); 

  if (switchState != prev_switch)
  {
    delay(50);
    switchState = digitalRead(A2); 

    if (switchState != prev_switch)
      TakePhoto();
  }

  prev_switch = switchState;
}