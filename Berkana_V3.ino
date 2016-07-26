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
unsigned int frame_delay = 50;

// define initial intensity levels
byte high_intensity = 128;
byte med_intensity = high_intensity * .66;
byte low_intensity = high_intensity * .33;

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];

// predefined color arrays (hue,saturation)
PROGMEM byte colors[][2] = {
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
const String main_menu[] = {"Kelley Yellow","Kelley Red","Kelley Blue","Kelley Green","Kelley Purple","Kelley White","Kelley dial","Rainbow chase", "Rainbow cycle", "Frame Delay", "Brightness"};
byte main_menu_count = 11;
byte main_menu_active = 0;

// end of menus

// setup analog control
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to
unsigned int sensorValue = 512;        // value read from the pot

//initialize variables

byte led_color[2];  //array that holds the current color codes for the LEDS
byte c = 0;
unsigned long last_millis;
unsigned long current_millis;
int frame_count;
byte hue;
byte saturation = 255;
bool kpattern_selected = true;
bool chase_selected = false;
bool cycle_selected = false;
bool idle = true;
byte main_menu_current = main_menu_active;
byte menu_previous = main_menu_current;
byte pattern_menu_active=0;
byte pattern_mene_current = pattern_menu_active;
byte brightness = high_intensity;
unsigned long last_button;
int x;

// Kelley Pattern variables 
int F16pos = 0;
byte F16delta = 1;
byte Width = 3; // width of line
unsigned int InterframeDelay = 30; //ms

// logical order arrays

// start of logical array one
int logical_array_one_start = 2; //the range of this variable is 0 to NUM_LEDS-1
int logical_array_one[NUM_LEDS/2];
int logical_array_two[NUM_LEDS/2];



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
	lcd.setBacklight(RED);
	lcd.print("Booting");
	build_logical_arrays(logical_array_one_start);

	// sanity check delay - allows reprogramming if accidently blowing power w/leds
	delay(3000);


	// Change this line as needed to match the LED string type, control chip and color sequence
	FastLED.addLeds<TM1803, DATA_PIN, GBR>(leds, NUM_LEDS); // RadioShack LED String

	// turn off all leds
	FastLED.clear();
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
	frame_count=0;
	led_color[0] = pgm_read_dword_near(&colors[c][0]);
	led_color[1] = pgm_read_dword_near(&colors[c][1]);
	last_button=millis();
	last_millis=millis();

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
	{high_intensity = brightness;}
	// Use FastLED automatic HSV->RGB conversion
	showAnalogRGB( CHSV( hue, saturation, high_intensity) );

	// set the cursor to column 0, line 1
	// (note: line 1 is the second row, since counting begins with 0):
	lcd.setCursor(0, 1);
	lcd.print(main_menu[main_menu_active]);
	//	  menu_previous = main_menu_current;
	if ((millis()-last_button) > 30000){
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
				if (main_menu_current == 0){
					main_menu_current = main_menu_count-1;
				}
				else {
					main_menu_current = main_menu_current-1;
				}
				last_button=millis();
				idle = false;
				//lcd.print("Menu: ");
				lcd.print(main_menu[main_menu_current]);
				//			lcd.setBacklight(GREEN);
				//lcd.print("up");
			}
			if (buttons & BUTTON_DOWN) {
				if (main_menu_current == main_menu_count-1) {
					main_menu_current = 0;
				}
				else {
					main_menu_current=main_menu_current+1;
				}
				last_button=millis();
				idle = false;
				//lcd.print("Menu: ");
				lcd.print(main_menu[main_menu_current]);
			}
			/* if (menu_active==2){
			//lcd.clear();
			lcd.setCursor(0,0);
			//lcd.print(frame_delay);
			if (buttons & BUTTON_LEFT) {
			//lcd.setCursor(0,1);
			if (frame_delay > 50) {
			frame_delay=(frame_delay - 50);
			lcd.print(frame_delay);
			last_button=millis();
			}
			}
			if (buttons & BUTTON_RIGHT) {
			//lcd.clear();
			//lcd.setCursor(0,0);
			//lcd.print(main_menu[main_menu_current]);
			//lcd.setCursor(0,1);
			if (frame_delay < 5000) {
			frame_delay= (frame_delay + 50);
			lcd.print(frame_delay);
			last_button=millis();
			}
			}
			if (buttons & BUTTON_SELECT) {
			menu_active=menu_previous;
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("Berkana");
			}
			}*/
			/* if (menu_active==1){
			lcd.setCursor(0,0);
			if (buttons & BUTTON_LEFT) {
			hue=(hue - 1);
			lcd.print(hue);
			last_button=millis();
			}
			if (buttons & BUTTON_RIGHT) {
			hue = (hue + 1);
			lcd.print(hue);
			last_button=millis();
			}
			if (buttons & BUTTON_UP)
			{	saturation = (saturation+1) ;
			lcd.print(saturation);
			last_button=millis();
			}
			if (buttons & BUTTON_DOWN)
			{	saturation =(saturation -1);
			lcd.print(saturation);
			last_button=millis();
			}
			
			if (buttons & BUTTON_SELECT) {
			menu_active=menu_previous;
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("Berkana");
			}
			} */
			/*			if (buttons & BUTTON_SELECT) {
			menu_previous=main_menu_active;
			main_menu_active=main_menu_current;
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("Berkana");
			//lcd.setCursor(0,0);
			//lcd.print("Menu: ");
			//lcd.print(main_menu[main_menu_current]);
			
			} */
		}
	}
	

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
	if (main_menu_active < 6) {
		led_color[0] = pgm_read_dword_near(&colors[main_menu_active][0]);
		led_color[1] = pgm_read_dword_near(&colors[main_menu_active][1]);
	};

	if (main_menu_active == 6){
		// set color to analog picker color
		led_color[0]= hue;
		led_color[1]= saturation;
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
void kelley_pattern_new(){
		// Update the "Fraction Bar" by 1/16th pixel every time
		F16pos += F16delta;
		
		// wrap around at end
		// remember that F16pos contains position in "16ths of a pixel"
		// so the 'end of the strip' is ((NUM_LEDS/2) * 16)
		if( F16pos >= ((NUM_LEDS/2) * 16)) {
			F16pos -= ((NUM_LEDS/2) * 16);
		}
		
		
		// Draw everything:
		// clear the pixel buffer
		memset8( leds, 0, NUM_LEDS * sizeof(CRGB));
		
		
		// draw the Fractional Bar, length=4px
		drawFractionalBar( F16pos, Width, hue / 256);
		
		FastLED.show();
		#if defined(FASTLED_VERSION) && (FASTLED_VERSION >= 2001000)
		FastLED.delay(InterframeDelay);
		#else
		delay(InterframeDelay);
		#endif
}
void drawFractionalBar( int pos16, int width, uint8_t hue)
{
	int i = pos16 / 16; // convert from pos to raw pixel number
	uint8_t frac = pos16 & 0x0F; // extract the 'factional' part of the position
	
	// brightness of the first pixel in the bar is 1.0 - (fractional part of position)
	// e.g., if the light bar starts drawing at pixel "57.9", then
	// pixel #57 should only be lit at 10% brightness, because only 1/10th of it
	// is "in" the light bar:
	//
	//                       57.9 . . . . . . . . . . . . . . . . . 61.9
	//                        v                                      v
	//  ---+---56----+---57----+---58----+---59----+---60----+---61----+---62---->
	//     |         |        X|XXXXXXXXX|XXXXXXXXX|XXXXXXXXX|XXXXXXXX |
	//  ---+---------+---------+---------+---------+---------+---------+--------->
	//                   10%       100%      100%      100%      90%
	//
	// the fraction we get is in 16ths and needs to be converted to 256ths,
	// so we multiply by 16.  We subtract from 255 because we want a high
	// fraction (e.g. 0.9) to turn into a low brightness (e.g. 0.1)
	uint8_t firstpixelbrightness = 255 - (frac * 16);
	
	// if the bar is of integer length, the last pixel's brightness is the
	// reverse of the first pixel's; see illustration above.
	uint8_t lastpixelbrightness  = 255 - firstpixelbrightness;
	
	// For a bar of width "N", the code has to consider "N+1" pixel positions,
	// which is why the "<= width" below instead of "< width".
	
	uint8_t bright;
	for( int n = 0; n <= width; n++) {
		if( n == 0) {
			// first pixel in the bar
			bright = firstpixelbrightness;
			} else if( n == width ) {
			// last pixel in the bar
			bright = lastpixelbrightness;
			} else {
			// middle pixels
			bright = 255;
		}
		
		leds[logical_array_one[i]] += CHSV( hue, 255, bright);
		leds[logical_array_two[i]] += CHSV( hue, 255, bright);
		//    leds[i] += CHSV( hue, 255, bright);
		i++;
		if( i == NUM_LEDS/2) i = 0; // wrap around
	}
}

void build_logical_arrays(int _startPosition){
	// build logical arrays
	int logical_array_one_position = _startPosition;
	int logical_array_two_position = _startPosition-1;
	for (int a = 0;a <= (NUM_LEDS/2); a++){
		if (logical_array_one_position > NUM_LEDS-1)
		{logical_array_one_position=logical_array_one_position-NUM_LEDS;
		}
		if (logical_array_two_position < 0)
		{logical_array_two_position=logical_array_two_position+NUM_LEDS;
		}
		logical_array_one[a]=logical_array_one_position;
		logical_array_two[a]=logical_array_two_position;
		logical_array_one_position++;
		logical_array_two_position--;
	}
	logical_array_one[0]=_startPosition; //without this line, position 0 in array 1 does not display anything.
	//there is some sort of a problem in the for loop above that must be writing bad data to logical_array_one[0].

}