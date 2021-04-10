/* 
Photobooth Program

 * Arduino directly triggers focus/shutter of camera via remote input (using optos as a buffer)

NOTE:
	* Disable burst modes so that only one photo is taken per button press

*/

#define FASTLED_INTERNAL			// Stop the annoying messages during compile
#include "FastLED.h"
#define NUM_LEDS 			45
#define DATA_PIN 			5
CRGB leds[NUM_LEDS];

#define CAM_SHUTT_PIN	3
#define CAM_FOCUS_PIN  	2
#define BUTTON_PIN     	4
#define AUDIO_PIN     	6


uint8_t switchState;
uint8_t prev_switch = LOW;

void setup() 
{
	LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
	LEDS.setBrightness(100);

	pinMode(BUTTON_PIN, INPUT_PULLUP);
	pinMode(CAM_SHUTT_PIN, OUTPUT);
	pinMode(CAM_FOCUS_PIN, OUTPUT);
	pinMode(AUDIO_PIN, OUTPUT);

	digitalWrite(CAM_FOCUS_PIN, HIGH);
	digitalWrite(CAM_SHUTT_PIN, HIGH);
	digitalWrite(AUDIO_PIN, HIGH);
}

void fadeall() 
{ 
	for(uint8_t i = 0; i < NUM_LEDS; i++)
		leds[i].nscale8(200);

	return;
}

void TakePhoto()
{
	// LightSequence
	digitalWrite(CAM_FOCUS_PIN, LOW); 		// Start focussing
	digitalWrite(AUDIO_PIN, LOW); 			// Trigger audio - Beep '3'

	delay(80);

	// LED countown sequence
	fill_solid(leds, NUM_LEDS, CRGB::Red);
	delay(50);
	FastLED.show(); 
	
	for(uint8_t i = 0; i < NUM_LEDS; i++)	// Set one LED to black at a time
	{
		leds[i] = CRGB::Black;
		FastLED.show(); 
		delay(50);

		// Trigger audio at specific times on LED light sequence
		if (i == 4)
			digitalWrite(AUDIO_PIN, HIGH);
		
		if (i == 14)
			digitalWrite(AUDIO_PIN, LOW); 	// Trigger audio - Beep '2'
		if (i == 18)
			digitalWrite(AUDIO_PIN, HIGH);
		
		if (i == 30)
			digitalWrite(AUDIO_PIN, LOW); 	// Trigger audio - Beep '1'
		if (i == 35)
			digitalWrite(AUDIO_PIN, HIGH);
		
		if (i == 42)
			digitalWrite(AUDIO_PIN, LOW); 	// Trigger audio - Beep '0'
	}


	digitalWrite(AUDIO_PIN, HIGH);
	digitalWrite(CAM_SHUTT_PIN, LOW);		// Trigger shutter

	// Flash LED ring twice to emulate camera flash
	
	for (uint8_t i = 0; i < 2; ++i)
	{
		fill_solid(leds, NUM_LEDS, CRGB::White);		// Only do white here because this colour often shows in photos
		FastLED.show(); 
		delay(100);

		fill_solid(leds, NUM_LEDS, CRGB::Black);
		FastLED.show();
		delay(100);
	}

	// Relase shutter & focus 
	digitalWrite(CAM_SHUTT_PIN, HIGH);		
	digitalWrite(CAM_FOCUS_PIN, HIGH);

	// Enable time to show photo & inhibit people 
		fill_solid(leds, NUM_LEDS, CRGB::Red);

	delay(50);
	FastLED.show(); 

	// Another countdown sequence while photo is being shown & to inhibit people going shutter crazy
	fill_solid(leds, NUM_LEDS, CRGB::Blue);
	for(uint8_t i = NUM_LEDS-1; i > 0; i--)
	{
		leds[i] = CRGB::Black;					// Set one LED to black at a time
		FastLED.show(); 
		delay(50);
	}

		
	switchState = digitalRead(BUTTON_PIN); 
	prev_switch = switchState;
	
}

void loop() 
{
	switchState = digitalRead(BUTTON_PIN); 

	if (!switchState)
	{
			/* code */
		

		if (switchState != prev_switch)
		{
			
			delay(100);
			switchState = digitalRead(BUTTON_PIN); 

			if (switchState != prev_switch)
			{
				TakePhoto();
			}
		}
	}

	prev_switch = switchState;

}
