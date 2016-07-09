/***************************************************************************/
//LED sign for our home using RadioShack Tricolor LED Strip
//  By Richard Bailey
//
//
// Version 3.0.0
// complete recode to allow easier addition of patterns and features
// convert to HSV and redesign UI
//
// Version 2.2.3
// User interface fixed for now, brightness implemented.
// Future updates from this point will most likely to clean up the code and reduce size.
//
// Version 2.2.2
// Start implementation of menu options to adjust brightness and frame speed
// frame delay adjustment implemented, need to tweak user interface to be more
// user friendly.
//
// Version 2.2.1
// move kelley pattern out of main loop into subroutine as prep for menu system.
//
// Version 2.2.0
// physical prototyping board with  LED connector, analog color selector and circuits on it built.
// incorporate LCD sample code to begin working on menu system.
// reorganize code to be closer to standard practices.
//
//  Version 2.1.1
//  complete restucture to stop using delay()
//
// Version 2.1.0
// convert colors from RGB to HSV to allow simple addition of color picker
//
// Version 2.0.4
// June 24, 2014
// implement use of high_intensity variable, move blanking to subroutine, and bug fixes
//
//
// Version 2.0.3
// June 20, 2014
// setup color codes into two dimensional array and a code option to
// have random colors. Currently set to cycle through the preprogrammed colors.
// Need to get the display shield to program the
// interface for stand alone operation as well as a 12vdc power supply
// of at least 2 amps.
//
//
// Version 2.0.2
// June 20, 2014
// converted from using hex color codes to individual RGB colors
// to allow custom color selection with on board controls planned
// for later versions.
// now includes output to serial for debugging use.
//
//
// Version 2.0.1
// complete rebuild using FastLED library instead of the RadioShack code.
/***************************************************************************/
#include <avr/pgmspace.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

// Variable definitions start here

// How many leds are in the strip?
#define NUM_LEDS 12


// Data pin that led data will be written out over
#define DATA_PIN 11

// define analog color pins for color selector output
#define REDPIN   5
#define GREENPIN 6
#define BLUEPIN  3
// These #defines make it easy to set the LCD backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

// define the delay between frames in milliseconds.
long frame_delay = 50;

// define initial intensity levels
uint8_t high_intensity = 128;
uint8_t med_intensity = high_intensity * .66;
uint8_t low_intensity = high_intensity * .33;

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];

// predefined color arrays (hue,saturation)
PROGMEM unsigned int colors[][2] = {
	{
	64,255                                    }
	, // yellow
	{
	0,255                                    }
	, //red
	{
	160,255                                    }
	, //blue
	{
	96,255                                    }
	, //green
	{
	192,255                                    }
	, //purple
	{
	255,0                                    }
	, //white
};

// define menus

// main menu

//char* main_menu[] = {"Kelley Yellow","Kelley Red","Kelley Blue","Kelley Green","Kelley Purple","Kelley White","Kelley dial","Rainbow chase", "Rainbow cycle", "Frame Delay", "Brightness"};
//int menu_count = 11;
char* main_menu[] = {"Pattern","Color","Speed"};
int menu_count = 3;

const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to
int sensorValue = 0;        // value read from the pot

//initialize variables

unsigned int led_color[2];  //array that holds the current color codes for the LEDS
int c = 0;
long last_millis;
long current_millis;
int frame_count;
static uint8_t hue;
bool kpattern_selected = true;
bool chase_selected = false;
bool cycle_selected = false;
bool idle = true;
int kelley_menu_selection = 0;
int menu_active=0;
int menu_current = menu_active;
int menu_previous = menu_current;
int brightness = high_intensity;
long last_button;
int x;

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// ***********************************************************************************************************
// *
// *                            Power Up Init.
// *
// *
// ***********************************************************************************************************
void setup() {

	//	Serial.begin(115200);

	// set up the LCD's number of columns and rows:
	lcd.begin(16, 2);
	// Print a message to the LCD. We track how long it takes since
	// this library has been optimized a bit and we're proud of it :)
	//	int time = millis();
	lcd.print(F("Booting"));
	//	time = millis() - time;
	//	Serial.print(F("Took "));
	//	Serial.print(time);
	//	Serial.println(F(" ms"));
	lcd.setBacklight(WHITE);

	// sanity check delay - allows reprogramming if accidently blowing power w/leds
	delay(3000);


	// Change this line as needed to match the LED string type, control chip and color sequence
	FastLED.addLeds<TM1803, DATA_PIN, GBR>(leds, NUM_LEDS); // RadioShack LED String

	// turn off all leds
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Black);
	FastLED.show();
	delay (200);

	pinMode(REDPIN,   OUTPUT);
	pinMode(GREENPIN, OUTPUT);
	pinMode(BLUEPIN,  OUTPUT);

	// Flash the "hello" color sequence: R, G, B, black.
	colorBars();
	lcd.clear();
	lcd.setBacklight(GREEN);
	lcd.setCursor(0,0);
	lcd.print("Berkana");
	lcd.setCursor(0,1);
	lcd.print("Ready");
	delay (750);
	last_millis=millis();
	last_button=millis();
	frame_count=0;
	led_color[0] = pgm_read_dword_near(&colors[c][0]);
	led_color[1] = pgm_read_dword_near(&colors[c][1]);

}

// ***********************************************************************************************************
// *
// *                            Main Loop
// *
// *
// ***********************************************************************************************************
void loop()
{
	// read the analog in value: and adjust brightness
	sensorValue = analogRead(analogInPin);
	brightness = map(sensorValue, 0, 1023, 0, 255);
 if (high_intensity != brightness)
 {high_intensity == brightness;}
	// Use FastLED automatic HSV->RGB conversion
	showAnalogRGB( CHSV( hue, 255, high_intensity) );

	// set the cursor to column 0, line 1
	// (note: line 1 is the second row, since counting begins with 0):
	lcd.setCursor(0, 1);
	// print the number of seconds since reset:
	//lcd.print("Active: ");
	lcd.print(main_menu[menu_active]);
	//	  menu_previous = menu_current;
	if ((millis()-last_button) > 10000 && menu_active <9){
		if (idle == false){
			lcd.setCursor(0,0);
			lcd.print("                ");
			idle = true;
		}
		lcd.setCursor(0,0);
		lcd.print("Berkana");
	}
	uint8_t buttons = lcd.readButtons();
	if ((millis()-last_button) > 333) {
		if (buttons) {
			lcd.clear();
			lcd.setCursor(0,0);
			if (buttons & BUTTON_UP) {
				if (menu_current == 0){
					menu_current = menu_count-1;
				}
				else {
					menu_current = menu_current--;
				}
				last_button=millis();
				idle = false;
				//lcd.print("Menu: ");
				lcd.print(main_menu[menu_current]);
				//			lcd.setBacklight(GREEN);
				//lcd.print("up");
			}
			if (buttons & BUTTON_DOWN) {
				if (menu_current == menu_count-1) {
					menu_current = 0;
				}
				else {
					menu_current=menu_current++;
				}
				last_button=millis();
				idle = false;
				//lcd.print("Menu: ");
				lcd.print(main_menu[menu_current]);
			}
			if (menu_active==9){
				//lcd.clear();
				lcd.setCursor(0,0);
				//lcd.print(frame_delay);
				if (buttons & BUTTON_LEFT) {
					//lcd.setCursor(0,1);
					if (frame_delay > 50) {
						frame_delay=(frame_delay - 50);
						lcd.print(frame_delay);
					}
				}
				if (buttons & BUTTON_RIGHT) {
					//lcd.clear();
					//lcd.setCursor(0,0);
					//lcd.print(main_menu[menu_current]);
					//lcd.setCursor(0,1);
					if (frame_delay < 5000) {
						frame_delay= (frame_delay + 50);
						lcd.print(frame_delay);
					}
				}
				if (buttons & BUTTON_SELECT) {
					menu_active=menu_previous;
					lcd.clear();
					lcd.setCursor(0,0);
					lcd.print("Berkana");
				}
			}
			if (menu_active==10){
				lcd.setCursor(0,0);
				if (buttons & BUTTON_LEFT) {
					if (high_intensity > 72) {
						high_intensity=(high_intensity - 4);
						lcd.print(high_intensity);
					}
				}
				if (buttons & BUTTON_RIGHT) {
					if (high_intensity < 252) {
						high_intensity= (high_intensity + 4);
						lcd.print(high_intensity);
					}
				}
				if (buttons & BUTTON_SELECT) {
					menu_active=menu_previous;
					lcd.clear();
					lcd.setCursor(0,0);
					lcd.print("Berkana");
				}
			}
			if (buttons & BUTTON_SELECT) {
				menu_previous=menu_active;
				menu_active=menu_current;
				lcd.clear();
				lcd.setCursor(0,0);
				lcd.print("Berkana");
				//lcd.setCursor(0,0);
				//lcd.print("Menu: ");
				//lcd.print(main_menu[menu_current]);
				
			}
		}
	}
	
	choice_update();

	current_millis=millis();
	if (kpattern_selected)
	{ kelley_pattern();
	};
	if (chase_selected)
	{
		chase_sub();
	};
	if (cycle_selected)
	{
		cycle_sub();
	};


}


// ***********************************************************************************************************
// *
// *                            Subroutines
// *
// *
// ***********************************************************************************************************

void showAnalogRGB( const CRGB& rgb)
{
	// showAnalogRGB: this is like FastLED.show(), but outputs on
	// analog PWM output pins instead of sending data to an intelligent,
	// pixel-addressable LED strip.
	//
	// This function takes the incoming RGB values and outputs the values
	// on three analog PWM output pins to the r, g, and b values respectively.

	analogWrite(REDPIN,   rgb.r );
	analogWrite(GREENPIN, rgb.g );
	analogWrite(BLUEPIN,  rgb.b );
}
void kelley_pattern() {
	// subroutine to display kelley pattern in selected color
	if (kelley_menu_selection < 6) {
		led_color[0] = pgm_read_dword_near(&colors[kelley_menu_selection][0]);
		led_color[1] = pgm_read_dword_near(&colors[kelley_menu_selection][1]);
	};

	if (kelley_menu_selection == 6){
		// set color to analog picker color
		led_color[0]= hue;
		led_color[1]= 255;
	};
	if (current_millis-last_millis > frame_delay) {
		delay(0);
		last_millis=current_millis;
/* run Kelley Pattern */
kelley_frame();

	}
}
void kelley_frame() {
	// generates frames for kelley_pattern

//	FastLED.show();
}
void kelley_blank () {
	// blanking subroutine for kelley_pattern
//	leds[(x+NUM_LEDS) % NUM_LEDS]=CHSV(led_color[0],led_color[1],0);
//	leds[y] = leds[(x+NUM_LEDS) % NUM_LEDS];
//	FastLED.show();
}
void colorBars()
{
	// colorBars: flashes Red, then Green, then Blue, then Black.
	// Helpful for diagnosing if you've mis-wired which is which.

	showAnalogRGB( CRGB::Red );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Red);
	FastLED.show();
	delay(500);
	showAnalogRGB( CRGB::Green );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Green);
	FastLED.show();
	delay(500);
	showAnalogRGB( CRGB::Blue );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Blue);
	FastLED.show();
	delay(500);
	showAnalogRGB( CRGB::Black );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Black);
	FastLED.show();
	delay(500);
}
void chase_sub() {
	if (current_millis-last_millis > frame_delay * .15) {
		delay(0);
		last_millis=current_millis;
		static uint8_t hue = 64;
		leds[x % NUM_LEDS]=CHSV(hue--, 255, high_intensity);
		FastLED.show();
		x++;
	}
}
void cycle_sub() {
	if (current_millis-last_millis > (frame_delay * .5)) {
		delay(0);
		last_millis=current_millis;

		static uint8_t hue = 64;
		FastLED.showColor(CHSV(hue--, 255, high_intensity));
		// delay (frame_delay);
	}
}
void choice_update()
{
	if (menu_active < 7) {
		kpattern_selected = true;
		chase_selected = false;
		cycle_selected = false;
		kelley_menu_selection = menu_active;
		// Serial.println(kelley_menu_selection);
		
	}
	if (menu_active == 7) {
		kpattern_selected = false;
		chase_selected = true;
		cycle_selected = false;
	}
	if (menu_active == 8) {
		kpattern_selected = false;
		chase_selected = false;
		cycle_selected = true;
	}
}

