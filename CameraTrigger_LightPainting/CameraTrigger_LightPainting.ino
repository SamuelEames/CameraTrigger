/* 
	Menu driven control of a sound board over UART.
	Commands for playing by # or by name (full 11-char name)
	Hard reset and List files (when not playing audio)
	Vol + and - (only when not playing audio)
	Pause, unpause, quit playing (when playing audio)
	Current play time, and bytes remaining & total bytes (when playing audio)

	
*/

#define FASTLED_INTERNAL			// Stop the annoying messages during compile
#include "FastLED.h"
#include "Adafruit_Soundboard.h"


//////////////////////////////////////////////////// ARDUINO PIN SETUP
// NOTE: SFX module connected to Serial1 port (pins 0-1)

#define PIN_SFX_RST 		4			// Reset line for SFX

#define PIN_LED_RING		6			// Data pin for ring of pixel LEDs
#define PIN_LED_STATUS	21		// Data pin for status LEDs on box
#define PIN_LED_ROOM		10		// PWM Output to run room light

#define PIN_BTN_TIME		20		// Button used to set time period for light painting
#define PIN_BTN_START		19		// Button use to trigger photo sequence
#define PIN_BTN_FOOTSW	5			// Footswitch for normal photobooth setup

#define PIN_CAM_FOCUS		8			// Focus trigger
#define PIN_CAM_SHUTTER	9			// Shutter trigger



/* SEQUENCE

 (1) Select time period for light painting
 			* LED ring lights up quadrants according to number of seconds selected
 			* Time also show on status LEDs on box
 (2) Press start button
 (3) Start sequence
 			* LED ring does 5s countdown with beep audio on every second
 			* Fade down status lights on box
 			* Room light fades off
 (4) Take photo!
 			* Shutter is held on
 			* Dim LED ring shows remaining time
 			* Start button can be pressed to stop photo early
 (5) On compeltion (or early stop)
 			* Fade up room light & box status lights across 2s
 			* Inhibit retriggering until lights are back to full (to stop trigger happy people)
 			* Loading sequence on LED ring during this too

*/ 



//////////////////////////////////////////////////// LED SETUP
#define NUM_LEDS_RING 			45
#define NUM_LEDS_STAT 			5

CRGB leds_ring[NUM_LEDS_RING];
CRGB leds_stat[NUM_LEDS_STAT];

// Order of status LED pixel array
enum statusLEDs:uint8_t { START, TIMER_15S, TIMER_30S, TIMER_45S, TIMER_60S };


//////////////////////////////////////////////////// SOUND MODULE SETUP

Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, PIN_SFX_RST);
// NOTE: Connect UG to ground to have the sound board boot into UART mode

// Audio filenames (format should be 8chars, no '.' and then extension)
char Audio_Beep[] = "T01NEXT0WAV";




void setup() 
{

	while (!Serial) {;} // Wait for serial to connect
	Serial.begin(115200);
	
	// Setup sound board module
	Serial1.begin(9600);									// Open serial connection

	if (!sfx.reset()) 										// Rest & check it's there
		Serial.println("SFX module not found");
	else
		Serial.println("SFX module found!");


	// PIXEL LED SETUP
	FastLED.addLeds<WS2812B, PIN_LED_RING, GRB>(leds_ring, NUM_LEDS_RING); 
	FastLED.addLeds<WS2812B, PIN_LED_STATUS, GRB>(leds_stat, NUM_LEDS_STAT); 
	
	FastLED.setMaxPowerInVoltsAndMilliamps(5,200); 						// Limit total power draw of LEDs to 200mA at 5V
	
	fill_solid(leds_ring, NUM_LEDS_RING, CRGB::Black);				// Black out LEDs
	fill_solid(leds_stat, NUM_LEDS_STAT, CRGB::Black);
	FastLED.show();



}





void loop() 
{

	// Play audio
	if (! sfx.playTrack(Audio_Beep) )
		Serial.println("Failed to play track?");

	delay(1000);



}

