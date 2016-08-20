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
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <FastLED.h>


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
unsigned int frame_delay = 6;
unsigned int menuDelay = 250;
// define initial intensity levels
byte high_intensity = 128;

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
const String main_menu[] =
{
	"Kelley Yellow",
	"Kelley Red",
	"Kelley Blue",
	"Kelley Green",
	"Kelley Purple",
	"Kelley White",
	"Kelley custom",
	"Rainbow chase",
	"Rainbow cycle",
	"Custom Saturation",
	"Custom Hue"
};
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
byte hue = 64;
byte saturation = 255;
byte stripHue = 64;
byte stripSaturation = 255;
byte customHue = 0;
byte customSaturation = 255;
bool idle = true;
byte main_menu_current = main_menu_active;
byte main_menu_previous = main_menu_current;
byte brightness = high_intensity;
unsigned long last_button;
unsigned long lastLoop;
int x;

// Kelley Pattern variables
int F16pos = 0;
byte F16delta = 2;
byte Width = 3; // width of line
unsigned int InterframeDelay = 2; //ms

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
	FastLED.setBrightness(32);
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
	led_color[0] = pgm_read_dword_near(&colors[c][0]);
	led_color[1] = pgm_read_dword_near(&colors[c][1]);
	last_button=millis();
	last_millis=millis();
	lastLoop = millis();

}

// ***********************************************************************************************************
// *
// *                            Main Loop
// *
// *
// ***********************************************************************************************************
void loop()
{
	unsigned long loopSpeed=millis()-lastLoop;
	lastLoop = millis();
	// read the analog in value: and adjust brightness
	sensorValue = analogRead(analogInPin);
	brightness = constrain(map(sensorValue, 0, 1023, 90, 255),90,255);
	if (high_intensity != brightness)
	{
		high_intensity = brightness;
	}

	if (main_menu_current == 9 || main_menu_current == 10)
	{
		showAnalogRGB( CHSV( customHue, customSaturation, high_intensity) );
		//		leds[0]=CHSV(customHue,customSaturation,high_intensity);
		//		FastLED.show ();
	}
	else
	{
		showAnalogRGB( CHSV( stripHue, stripSaturation, high_intensity) );
	}
	FastLED.setBrightness(brightness);

	// set the cursor to column 0, line 1
	// (note: line 1 is the second row, since counting begins with 0):
	lcd.setCursor(0, 1);
	lcd.print(main_menu[main_menu_current]);
	//	  menu_previous = main_menu_current;
	if ((millis()-last_button) > 10000 && !idle)
	{
	//	if (idle == false)
	//	{
			lcd.setCursor(0,0);
			lcd.print("                ");
			lcd.setCursor(0,0);
			lcd.print("Berkana");
			idle = true;
	//	}
//		lcd.setCursor(0,0);
//		lcd.print("Berkana");
	}
	
	switch (main_menu_current)
	{
		case 0: // kelley pattern yellow
		led_color[0] = pgm_read_dword_near(&colors[main_menu_current][0]);
		led_color[1] = pgm_read_dword_near(&colors[main_menu_current][1]);
		stripHue = led_color[0];
		stripSaturation = led_color[1];
		lcd.setBacklight(YELLOW);
		kelley_pattern_new();
		menuButtonHandling();
		break;
		
		case 1: // kelley pattern red
		led_color[0] = pgm_read_dword_near(&colors[main_menu_current][0]);
		led_color[1] = pgm_read_dword_near(&colors[main_menu_current][1]);
		stripHue = led_color[0];
		stripSaturation = led_color[1];
		lcd.setBacklight(RED);
		kelley_pattern_new();
		menuButtonHandling();
		break;
		
		case 2: // kelley pattern blue
		led_color[0] = pgm_read_dword_near(&colors[main_menu_current][0]);
		led_color[1] = pgm_read_dword_near(&colors[main_menu_current][1]);
		stripHue = led_color[0];
		stripSaturation = led_color[1];
		lcd.setBacklight(BLUE);
		kelley_pattern_new();
		menuButtonHandling();
		break;
		
		case 3: // kelley pattern green
		led_color[0] = pgm_read_dword_near(&colors[main_menu_current][0]);
		led_color[1] = pgm_read_dword_near(&colors[main_menu_current][1]);
		stripHue = led_color[0];
		stripSaturation = led_color[1];
		lcd.setBacklight(GREEN);
		kelley_pattern_new();
		menuButtonHandling();
		break;
		
		case 4: // kelley pattern purple
		led_color[0] = pgm_read_dword_near(&colors[main_menu_current][0]);
		led_color[1] = pgm_read_dword_near(&colors[main_menu_current][1]);
		stripHue = led_color[0];
		stripSaturation = led_color[1];
		lcd.setBacklight(VIOLET);
		kelley_pattern_new();
		menuButtonHandling();
		break;
		
		case 5: // kelley pattern white
		led_color[0] = pgm_read_dword_near(&colors[main_menu_current][0]);
		led_color[1] = pgm_read_dword_near(&colors[main_menu_current][1]);
		stripHue = led_color[0];
		stripSaturation = led_color[1];
		lcd.setBacklight(WHITE);
		kelley_pattern_new();
		menuButtonHandling();
		break;
		
		case 6:		// kelley variable color
		stripHue=customHue;
		stripSaturation=customSaturation;
		lcd.setBacklight(TEAL);
		kelley_pattern_new ();
		menuButtonHandling();
		break;
		
		case 7:		// rainbow chase
		chase_sub();
		lcd.setBacklight(TEAL);
		menuButtonHandling();
		break;
		
		case 8:		//raimbow cycle
		cycle_sub();
		lcd.setBacklight(TEAL);
		menuButtonHandling();
		break;
		
		case 9:		//custom saturation
		lcd.setBacklight(WHITE);
		lcd.setCursor (0,0);
		lcd.print("                ");
		lcd.setCursor (0,0);
		lcd.print (customSaturation);
		customSaturationbuttons ();
		break;
		
		case 10:		//custom hue
		lcd.setBacklight(WHITE);
		lcd.setCursor (0,0);
		lcd.print("                ");
		lcd.setCursor (0,0);
		lcd.print (customHue);
		customHuebuttons ();
		break;
	}
	current_millis=millis();
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

void colorBars()
{
	// colorBars: flashes Red, then Green, then Blue, then Black.
	// Helpful for diagnosing if you've mis-wired which is which.

	showAnalogRGB( CRGB::Red );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Red);
	FastLED.show();
	delay(300);
	showAnalogRGB( CRGB::Green );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Green);
	FastLED.show();
	delay(300);
	showAnalogRGB( CRGB::Blue );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Blue);
	FastLED.show();
	delay(300);
	showAnalogRGB( CRGB::Black );
	fill_solid ( &(leds[0]), NUM_LEDS, CRGB::Black);
	FastLED.show();
}

void chase_sub()
{
	if (current_millis-last_millis > frame_delay)
	{
		delay(0);
		last_millis=current_millis;
		static uint8_t hue = 64;
		leds[x % NUM_LEDS]=CHSV(hue--, 255, high_intensity);
		stripHue  = hue;
		stripSaturation = 255;
		FastLED.show();
		x++;
	}
}

void cycle_sub()
{
	if (current_millis-last_millis > (frame_delay * 4 ))
	{
		delay(0);
		last_millis=current_millis;

		static uint8_t hue = 64;
		FastLED.showColor(CHSV(hue--, 255, high_intensity));
		stripHue = hue;
		stripSaturation = 255;
	}
}
void kelley_pattern_new()
{
	// Update the "Fraction Bar" by 1/16th pixel every time
	F16pos += F16delta;
	
	// wrap around at end
	// remember that F16pos contains position in "16ths of a pixel"
	// so the 'end of the strip' is ((NUM_LEDS/2) * 16)
	if( F16pos >= ((NUM_LEDS/2) * 16))
	{
		F16pos -= ((NUM_LEDS/2) * 16);
	}
	
	
	// Draw everything:
	// clear the pixel buffer
	//	memset8( leds, 0, NUM_LEDS * sizeof(CRGB));
	FastLED.clear();
	
	
	// draw the Fractional Bar, length=4px
	drawFractionalBar( F16pos, Width, hue / 256);
	
	FastLED.show();
	//	FastLED.delay(InterframeDelay);
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
	for( int n = 0; n <= width; n++)
	{
		if( n == 0)
		{
			// first pixel in the bar
			bright = firstpixelbrightness;
			} else if( n == width ) {
			// last pixel in the bar
			bright = lastpixelbrightness;
		}
		else
		{
			// middle pixels
			bright = 255;
		}
		
		leds[logical_array_one[i]] += CHSV( stripHue, stripSaturation, bright);
		leds[logical_array_two[i]] += CHSV( stripHue, stripSaturation, bright);
		//    leds[i] += CHSV( hue, 255, bright);
		i++;
		if( i == NUM_LEDS/2)
		{
			i = 0; // wrap around
		}
	}
}
void build_logical_arrays(int _startPosition){
	// build logical arrays
	int logical_array_one_position = _startPosition;
	int logical_array_two_position = _startPosition-1;
	for (int a = 0;a <= ((NUM_LEDS/2)-1); a++)
	{
		if (logical_array_one_position > NUM_LEDS-1)
		{
			logical_array_one_position=logical_array_one_position-NUM_LEDS;
		}
		if (logical_array_two_position < 0)
		{
			logical_array_two_position=logical_array_two_position+NUM_LEDS;
		}
		logical_array_one[a]=logical_array_one_position;
		logical_array_two[a]=logical_array_two_position;
		logical_array_one_position++;
		logical_array_two_position--;
	}

}
void menuButtonHandling()
{
	if (millis ()-last_button > menuDelay)
	{
		uint8_t buttons = lcd.readButtons();
		switch (buttons) {
			
			case (BUTTON_DOWN):
			main_menu_active++;
			if (main_menu_active > main_menu_count-1)
			{
				main_menu_active = 0;
			}
			lcd.setCursor(0,0);
			lcd.print("                ");
			lcd.setCursor(0,0);
			lcd.print(main_menu[main_menu_active]);
			idle = false;
			last_button = millis();
			break;
			
			case (BUTTON_UP):
			if (main_menu_active == 0)
			{
				main_menu_active = main_menu_count-1;
			}
			else
			{
				main_menu_active--;
			}
			lcd.setCursor(0,0);
			lcd.print("                ");
			lcd.setCursor(0,0);
			lcd.print(main_menu[main_menu_active]);
			idle = false;
			last_button = millis();
			break;
			
			case (BUTTON_LEFT):
			//<move menu up one level>
			break;
			
			case (BUTTON_RIGHT):
			//<move menu down one level>
			break;
			
			case (BUTTON_SELECT):
			main_menu_previous = main_menu_current;
			main_menu_current = main_menu_active;
			lcd.clear ();
			idle = false;
			FastLED.clear();
			FastLED.show ();
			last_button = millis();
			break;
			
			default:
			break;
		}
	}
}
void customSaturationbuttons()
{
	if (millis ()-last_button > menuDelay/3)
	{
		uint8_t buttons = lcd.readButtons();
		switch (buttons) {
			
			case (BUTTON_DOWN):
			break;
			
			case (BUTTON_UP):
			break;
			
			case (BUTTON_LEFT):
			if (customSaturation == 0)
			{
				customSaturation = 255;
			}
			else
			{
				customSaturation--;
			}
			idle = false;
			last_button = millis ();
			break;
			
			case (BUTTON_RIGHT):
			if (customSaturation == 255)
			{
				customSaturation = 0;
			}
			else
			{
				customSaturation++;
			}
			idle = false;
			last_button = millis ();
			break;
			
			case (BUTTON_SELECT):
			main_menu_current = main_menu_previous;
			lcd.clear ();
			idle = false;
			last_button = millis();
			break;
		}
	}
}
void customHuebuttons()
{
	if (millis ()-last_button > menuDelay/3)
	{
		
		uint8_t buttons = lcd.readButtons();
		switch (buttons)
		{
			case (BUTTON_DOWN):
			break;
			
			case (BUTTON_UP):
			break;
			
			case (BUTTON_LEFT):
			if (customHue == 0)
			{
				customHue = 255;
			}
			else
			{
				customHue--;
			}
			idle = false;
			last_button = millis ();
			break;
			
			case (BUTTON_RIGHT):
			if (customHue == 255)
			{
				customHue = 0;
			}
			else
			{
				customHue++;
			}
			idle = false;
			last_button = millis ();
			break;
			
			case (BUTTON_SELECT):
			main_menu_current = main_menu_previous;
			lcd.clear ();
			idle = false;
			last_button = millis();
			break;
		}
	}
}