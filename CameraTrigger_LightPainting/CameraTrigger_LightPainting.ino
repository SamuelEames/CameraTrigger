/* Light Painting Camera controller

Devices
 * Arduino Pro Micro
 * Adafruit Audio FX Sound Board (running in Serial control mode because I didn't want to use a ridiculous amount of pins)
 * 45-LED pixel ring (placed around camera lens to prompt people to look at the camera and not the screen when using it as a photobooth)
 * Some more buttons & leds to change settings / show status

Note: shutter & focus trigger outputs form arduino are sent through optos to camera remote input; power rails/gnd are not connected.

SEQUENCE/PROGRAM NOTES

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
 (6) Repeat!

*/ 

#define FASTLED_INTERNAL			// Stop the annoying messages during compile
#include "FastLED.h"
#include "Adafruit_Soundboard.h"


//////////////////////////////////////////////////// ARDUINO PIN SETUP
// NOTE: SFX module connected to Serial1 port (pins 0-1)

#define PIN_SFX_RST 		4			// Reset line for SFX

#define PIN_LED_RING		3			// Data pin for ring of pixel LEDs
#define PIN_LED_STATUS	19		// Data pin for status LEDs on box
#define PIN_LED_ROOM		10		// Relay Output to run room light

#define PIN_BTN_TIME		20		// Button used to set time period for light painting
#define PIN_BTN_START		21		// Button use to trigger photo sequence
#define PIN_BTN_FOOTSW	5			// Footswitch for normal photobooth setup

#define PIN_CAM_FOCUS		9			// Focus trigger
#define PIN_CAM_SHUTTER	8			// Shutter trigger

#define BEEEP_PIN     	16		// Active Piezo Buzzer


//////////////////////////////////////////////////// BUTTON DEBOUNCING
#define DEBOUNCE 20  					// ms to debounce

uint8_t buttons[] = {PIN_BTN_TIME, PIN_BTN_START, PIN_BTN_FOOTSW};
enum buttonOrder:uint8_t {BTN_TIME, BTN_START, BTN_FOOTSW};

#define NUM_BTNS sizeof(buttons)
uint8_t pressed[NUM_BTNS], justpressed[NUM_BTNS], justreleased[NUM_BTNS];			// Button flags


//////////////////////////////////////////////////// LED SETUP
#define NUM_LEDS_RING 	45
#define NUM_LEDS_STAT 	5

CRGB leds_ring[NUM_LEDS_RING];
CRGB leds_stat[NUM_LEDS_STAT];

// Order of status LED pixel array
enum statusLEDs:uint8_t { START, TIMER_15S, TIMER_30S, TIMER_45S, TIMER_60S };


//////////////////////////////////////////////////// SOUND MODULE SETUP

Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, PIN_SFX_RST);
// NOTE: Connect UG to ground to have the sound board boot into UART mode

// Audio filenames (format should be 8chars, no '.' and then extension)
char Audio_Beep[] = "T01NEXT0WAV";


//////////////////////////////////////////////////// MISC VARIABLES

enum state:uint8_t { IDLE, CDOWN2_START, CDOWN2_END, CDOWN2_IDLE};
uint8_t myState = IDLE;


const uint8_t exposureDuration[] = {15, 30, 45, 60};						// Exposure duration in seconds
uint8_t exposureSelection = 0;																	// i of exposureDuration used to select exposure time

uint32_t countdownTimer_start;																	// Holds starting millis time of each countdown period

#define PERIOD_CDOWN2_START	5000
#define PERIOD_CDOWN2_IDLE	3000

////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() 
{
	// Piezo - turn off straight away because it takes all the power
	pinMode(BEEEP_PIN, OUTPUT);
	digitalWrite(BEEEP_PIN, HIGH);

	// while (!Serial) {;} // Wait for serial to connect
	// Serial.begin(115200);
	
	// Setup sound board module
	Serial1.begin(9600);									// Open serial connection

	if (!sfx.reset()) 										// Rest & check it's there
		Serial.println("SFX module not found");
	else
		Serial.println("SFX module found!");


	// PIXEL LED SETUP
	FastLED.addLeds<WS2812B, PIN_LED_RING, GRB>(leds_ring, NUM_LEDS_RING); 
	FastLED.addLeds<WS2812B, PIN_LED_STATUS, GRB>(leds_stat, NUM_LEDS_STAT); 
	
	FastLED.setMaxPowerInVoltsAndMilliamps(5,300); 						// Limit total power draw of LEDs to 200mA at 5V
	
	fill_solid(leds_ring, NUM_LEDS_RING, CRGB::Black);				// Black out LEDs
	fill_solid(leds_stat, NUM_LEDS_STAT, CRGB::Black);
	FastLED.show();

	// IO Setup
	pinMode(PIN_LED_ROOM, 		OUTPUT);
	digitalWrite(PIN_LED_ROOM, LOW);

	pinMode(PIN_CAM_FOCUS, 		OUTPUT);
	pinMode(PIN_CAM_SHUTTER, 	OUTPUT);

	for (uint8_t i = 0; i < NUM_BTNS; ++i)
		pinMode(buttons[i], INPUT_PULLUP);	



}


////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() 
{
	static uint8_t lastState = IDLE;

	// // Play audio
	// if (! sfx.playTrack(Audio_Beep) )
	// 	Serial.println("Failed to play track?");

	// delay(1000);

	check_BTNS();
	

	// Serial.print("Current State = ");
	// Serial.println(myState, DEC);




	switch (myState)
	{
		case IDLE:
			// End focus / Shutter
			digitalWrite(PIN_CAM_FOCUS, LOW);
			digitalWrite(PIN_CAM_SHUTTER, LOW);


			if ((lastState != IDLE) && (lastState != CDOWN2_IDLE))
			{
				// Single beep
				digitalWrite(BEEEP_PIN, LOW);
				delay(300);		// Ensure shutter is closed before turning on light
				digitalWrite(BEEEP_PIN, HIGH);
				
			}
			
			

			if (justpressed[BTN_START] || justpressed[BTN_FOOTSW] )				// Start sequence on start btn / footswitch
			{
				myState = CDOWN2_START;
				countdownTimer_start = millis();
				justpressed[BTN_START] = 0;
			}	
			
			set_exposureTime();


			leds_stat[START] = CRGB::Green;										// Set start LED to solid green

			ledRingCountdown(0);




			// Turn on room light
			digitalWrite(PIN_LED_ROOM, HIGH);

			lastState = IDLE;
			break;


		case CDOWN2_START:
			// Check if timer has elapsed
			if ( (countdownTimer_start +  PERIOD_CDOWN2_START) <= millis() )
			{
				myState = CDOWN2_END;
				countdownTimer_start = millis();
			}

			// jump back out if start button was pressed again
			if (justpressed[BTN_START])				
			{
				myState = IDLE;
				justpressed[BTN_START] = 0;
			}	

			ledRingCountdown(PERIOD_CDOWN2_START);

			// Start focus
			digitalWrite(PIN_CAM_FOCUS, HIGH);

			// Turn off room light
			digitalWrite(PIN_LED_ROOM, LOW);

			lastState = CDOWN2_START;
			break;


		case CDOWN2_END:

			if (justpressed[BTN_START])				// Stop sequence early
			{
				myState = CDOWN2_IDLE;
				countdownTimer_start = millis();
				justpressed[BTN_START] = 0;
			}

			// Check if timer has elapsed
			if ( (countdownTimer_start + (1000 * (uint32_t) exposureDuration[exposureSelection])) <= millis() )
			{
				countdownTimer_start = millis();
				myState = CDOWN2_IDLE;
			}

			ledRingCountdown(1000 * (uint32_t) exposureDuration[exposureSelection]);

			// Start Shutter
			digitalWrite(PIN_CAM_SHUTTER, HIGH);

			lastState = CDOWN2_END;
			break;


		case CDOWN2_IDLE:
			// End focus / Shutter
			digitalWrite(PIN_CAM_FOCUS, LOW);
			digitalWrite(PIN_CAM_SHUTTER, LOW);

			if (lastState != CDOWN2_IDLE)
			{
				// Single beep
				digitalWrite(BEEEP_PIN, LOW);
				delay(300);		// Ensure shutter is closed before turning on light
				digitalWrite(BEEEP_PIN, HIGH);
				
			}


			set_exposureTime();

			// Check if timer has elapsed
			if ( (countdownTimer_start +  PERIOD_CDOWN2_IDLE) <= millis() )
				myState = IDLE;

			ledRingCountdown(PERIOD_CDOWN2_IDLE);



			// Turn on room light
			digitalWrite(PIN_LED_ROOM, HIGH);

			lastState = CDOWN2_IDLE;
			break;

		default:
			break;

	}

	FastLED.show();


}

////////////////////////////////////////////////////////////////////////////////////////////////////////


void setTimeLEDs()
{
	// Sets timer LEDs



	return;
}


void ledRingCountdown(uint32_t period)
{
	uint32_t elapsed_time = millis() - countdownTimer_start;
	float 	elapsed_percent = elapsed_time / (float) period;



	



	switch (myState)
	{
		case IDLE:
			// Ligth quadrants according to time period selected

			fill_solid(leds_ring, NUM_LEDS_RING, CRGB::Black);

			for (uint8_t i = 0; i < exposureSelection + 1; ++i)
			{
				for (uint8_t j = 0; j < NUM_LEDS_RING/4; ++j)
				{
					leds_ring[(i*NUM_LEDS_RING/4) + j] = CRGB::White;	
				}				
			}




			// Turn brightness up
			FastLED.setBrightness(255);
			break;


		case CDOWN2_START:
			for (uint8_t i = 0; i < NUM_LEDS_RING; ++i)
			{
				if ( (elapsed_percent * NUM_LEDS_RING) >= i )
					leds_ring[i] = CRGB::Red;	
				else
					leds_ring[i] = CRGB::Black;	
			}

			// Beep with countdown
			if (elapsed_percent >= 0 && elapsed_percent <= 0.1)
				digitalWrite(BEEEP_PIN, LOW);
			else if (elapsed_percent >= 0.2 && elapsed_percent <= 0.3)
				digitalWrite(BEEEP_PIN, LOW);
			else if (elapsed_percent >= 0.4 && elapsed_percent <= 0.5)
				digitalWrite(BEEEP_PIN, LOW);
			else if (elapsed_percent >= 0.6 && elapsed_percent <= 0.7)
				digitalWrite(BEEEP_PIN, LOW);
			else if (elapsed_percent >= 0.8 && elapsed_percent <= 0.9)
				digitalWrite(BEEEP_PIN, LOW);
			else
				digitalWrite(BEEEP_PIN, HIGH);

			// Turn brightness down
			FastLED.setBrightness((1-elapsed_percent) * 250 + 5 );
			break;


		case CDOWN2_END: 
			for (uint8_t i = 0; i < NUM_LEDS_RING; ++i)
			{
				if ( (elapsed_percent * NUM_LEDS_RING) >= i )
					leds_ring[i] = CRGB::Black;	
				else
					leds_ring[i] = CRGB::Blue;	
			}		
			break;


		case CDOWN2_IDLE:
			for (uint8_t i = 0; i < NUM_LEDS_RING; ++i)
			{
				if ( (elapsed_percent * NUM_LEDS_RING) >= i )
					leds_ring[i] = CRGB::Black;	
				else
					leds_ring[i] = CRGB::White;	
			}		

			// Turn brightness up
			FastLED.setBrightness(elapsed_percent * 250 + 5 );


			break;

		default:
			break;
	}

	return;
}


void set_exposureTime()
{
	// Steps through exposure time options
	// Call after check_BTNS()
	if (justpressed[BTN_TIME])
	{
		justpressed[BTN_TIME] = 0;										// Clear 'just Pressed' because debouncing inhibits this happening instantly
		if (++exposureSelection >= sizeof(exposureDuration))
			exposureSelection = 0;
	}


	// Set LEDs according to slected exposure
	for (uint8_t i = 1; i < NUM_LEDS_STAT; ++i)			// First LED is 'start' one
	{
		if ( exposureSelection == (i-1))							// Highlight current selection
			leds_stat[i] = CRGB::Blue;
		else
			leds_stat[i] = CRGB::Black;
	}


	

	return;
}


void check_BTNS()
{
  static byte previousstate[NUM_BTNS];
  static byte currentstate[NUM_BTNS];
  static long lasttime;

  if (millis() < lasttime) 
  {
     lasttime = millis(); // we wrapped around, lets just try again
  }
 
  if ((lasttime + DEBOUNCE) > millis()) 
  {
    return; // not enough time has passed to debounce
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
 
  for (uint8_t i = 0; i < NUM_BTNS; i++) 
  {
    justpressed[i] = 0;       // when we start, we clear out the "just" indicators
    justreleased[i] = 0;
 
    currentstate[i] = digitalRead(buttons[i]);   // read the button
    
    if (currentstate[i] == previousstate[i]) 
    {
      if ((pressed[i] == LOW) && (currentstate[i] == LOW)) 
          justpressed[i] = 1;
      else if ((pressed[i] == HIGH) && (currentstate[i] == HIGH)) 
          justreleased[i] = 1;

      pressed[i] = !currentstate[i];  // remember, digital HIGH means NOT pressed
    }
    // Serial.println(pressed[i], DEC);
    previousstate[i] = currentstate[i];   // keep a running tally of the buttons
  }
}