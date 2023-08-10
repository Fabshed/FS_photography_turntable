#include <Arduino_BuiltIn.h>

////////////////////////////////////////////////////////////////
//        Arduino OLED / LCD / TFT Rotary Encoder Menu        //
//                          v1.0                              //
//                   Fabshed Designs 2022                     //
////////////////////////////////////////////////////////////////

// Comment / uncomment to choose required display
#define SSD1306_OLED
//#define PCD8544_LCD
//#define ST7735_TFT

#include <Adafruit_GFX.h>

#ifdef SSD1306_OLED
#include <Adafruit_SSD1306.h>
#else
#ifdef PCD8544_LCD
#include <Adafruit_PCD8544.h>
#else
#ifdef ST7735_TFT
#include <Adafruit_ST7735.h>
#endif
#endif
#endif

#include <ClickEncoder.h>
//ClickEncoder by karl@pitrich.com
//Install from https://github.com/0xPIT/encoder/tree/arduino

#ifdef ARDUINO_ARCH_MEGAAVR
#include "EveryTimerB.h"
#define Timer1 TimerB2 // use TimerB2 as a drop in replacement for Timer1
#else // assume architecture supported by TimerOne ....
#include "TimerOne.h"
#endif

#include <SPI.h>

#include "AccelStepper.h"
// Library created by Mike McCauley at
// http://www.airspayce.com/mikem/arduino/AccelStepper/

// EasyDriver connections
#define DIR_PIN 13  // Pin 13 connected to Direction pin
#define STP_PIN 12  // Pin 12 connected to Step pin
#define MS1_PIN 5       // Pin 5 connected to MS1 pin
#define MS2_PIN 6       // Pin 6 connected to MS2 pin
#define EN_PIN 8        // Pin 8 connected to enable pin


/* Configure type of Steps on Easy Driver:
  // MS1 MS2
  //
  // LOW LOW   = Full Step //
  // HIGH LOW  = Half Step //
  // LOW HIGH  = A quarter of Step //
  // HIGH HIGH = An eighth of Step //
*/
#define MS1_VAL HIGH  // An eighth of step
#define MS2_VAL HIGH  // An eighth of step

const long stepsPerRevolution = 1600L;  // For eighth step

// AccelStepper Setup
AccelStepper stepper(1, STP_PIN, DIR_PIN);
// 1 = Easy Driver interface

int Stepper_Speed;       // Used to set the travel speed of the stepper motor


// Define camera control pin (pin to take photo / start video)
#define CAMERA_CONTROL_PIN 7

#ifdef SSD1306_OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino NANO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; Usually 0x3D for 128x64 and 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#else
#ifdef PCD8544_LCD
#define SCREEN_WIDTH 84 // LCD display width, in pixels
#define SCREEN_HEIGHT 48 // LCD display height, in pixels

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
// Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);
// Note with hardware SPI MISO and SS pins aren't used but will still be read
// and written to during SPI transfer.  Be careful sharing these pins!
#else
#ifdef ST7735_TFT
#define SCREEN_WIDTH 128 // TFT display width, in pixels
#define SCREEN_HEIGHT 160 // TFT display height, in pixels

#define  ST7735_BG_COL ST7735_WHITE
#define  ST7735_TEXT_COL ST7735_BLUE

// OPTION 1 (recommended) is to use the HARDWARE SPI pins, which are unique
// to each board and not reassignable. For Arduino Uno: MOSI = pin 11 and
// SCLK = pin 13. This is the fastest mode of operation and is required if
// using the breakout board's microSD card.
#define TFT_CS        10
#define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8
// For 1.44" and 1.8" TFT with ST7735  use:
Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// OPTION 2 lets you interface the display using ANY TWO or THREE PINS,
// tradeoff being that performance is not as fast as hardware SPI above.
//#define TFT_MOSI 11  // Data out
//#define TFT_SCLK 13  // Clock out
//Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
#endif
#endif
#endif



//   frame indicates which menu items should be displayed on the top level screen. It is used to keep track as the selected
//   item gets to the bottom of the screen and then increments as selection moves to the next screen.
//   Max value of frame is equal to the number of top level menu items - (no of lines screen can display - 1).
//   For example, for a screen which can display 3 menu items the relationship between frame and menuitems displayed is:
//   Frame 1:  MenuItem1, MenuItem2, MenuItem3
//   Frame 2:  MenuItem2, MenuItem3, MenuItem4
//   Frame 3:  MenuItem3, MenuItem4, MenuItem5
//   ....
//   Frame N: MenuItem(N), MenuItem(N+1), MenuItem(N+2)
//
//   menu_level inidcates which 'level' of the menu we are currently in. If menu_level == 1 then we are in top level, if menu_level == 2
//   then we are in second level (ie. we have selected one of the top level actions). It increments when the encoder button
//   is pressed.
//
//   menuitem keeps track of which top menu item the 'cursor' is on and should be highlighted. This will be the item
//   selected when the encoder button is pressed. It increments and decrements as the encoder is turned clockwise and counter
//   clockwise respectively
//
int frame = 1;
int menu_level = 1;
int menuitem = 1;

// Populate with text to display for top level menu items. First item is just 'title' and not an actual menuitem
String MenuList[] = { "Turntable V1.0", "Run", "Mode", "Direction", "Speed", "Stills", "Angle", "Defaults" };

#define RUN 1
#define MODE 2
#define DIRECTION 3
#define SPEED 4
#define STILLS 5
#define ANGLE 6
#define DEFAULTS 7

// Calculate how many menu items are in the top level menu (ignoring first item)
#define TOP_MENU_ITEMS (unsigned)((sizeof(MenuList)/sizeof(MenuList[0])) - 1)

// Calculate max number of  menu item lines that can be displayed on screen at once (assume lines take 10 pixels height)
#define MAX_NO_OF_MENU_ITEMS_TO_DISPLAY int((SCREEN_HEIGHT-10)/10)

int noOfMenuItemsToDisplay = (MAX_NO_OF_MENU_ITEMS_TO_DISPLAY > TOP_MENU_ITEMS) ? TOP_MENU_ITEMS : MAX_NO_OF_MENU_ITEMS_TO_DISPLAY;

// How many 'display' frames are there ?
#define FRAME_MAX  (TOP_MENU_ITEMS -  noOfMenuItemsToDisplay + 1)

String mode_opt[2] = { "PHOTO" , "VIDEO"};
int selectedMode = 0; //Photo

String dir_opt[2] = { "CCW", "CW " };
boolean clockwise = true;

int speed_opt = 500; //
int stills_opt = 20; // Number of photos to take (set to 1 in video mode)
int angle_opt = 180; // Degrees

boolean up = false;
boolean down = false;
boolean button_pressed = false;

ClickEncoder *myEncoder;
int16_t encoderLast, encoderCurrent;


void setup() {
  //Serial.begin(9600);

  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STP_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);

  pinMode(CAMERA_CONTROL_PIN, OUTPUT);

  // set stepping type to FULL Steps
  digitalWrite(MS1_PIN, MS1_VAL);
  digitalWrite(MS2_PIN, MS2_VAL);

  // set camera control pin to inactive
  digitalWrite(CAMERA_CONTROL_PIN, HIGH);


  // Use enable pin for stepper
  stepper.setEnablePin(EN_PIN);

  // Enable pin inverted
  stepper.setPinsInverted(false, false, true);

  // Disable motor puin outputs to save power
  stepper.disableOutputs();

#ifdef SSD1306_OLED
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    //Serial.Println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
#else
#ifdef PCD8544_LCD
  display.begin();
  display.setContrast(60); // Adjust as required
#else
#ifdef ST7735_TFT
  // Use this initializer if using a 1.8" TFT screen:
  display.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  display.setFont();
  display.setTextColor(ST7735_TEXT_COL);
#endif
#endif
#endif

  displayClear();


  // A1 - DT
  // A0 - Clk
  // A2 - Button
  myEncoder = new ClickEncoder(A1, A0, A2);
  myEncoder->setAccelerationEnabled(false);

  Timer1.initialize();
  Timer1.attachInterrupt(timerIsr);
  Timer1.setPeriod(500);

  encoderLast = myEncoder->getValue();
}



void loop() {

  displayMenu();

  // Check for rotary encoder updates
  updateRotaryEncoder();

  // Turned rotary encoder Anti Clockwise ?
  if (up)
  {
    up = false;

    // If in the top level....
    if (menu_level == 1 )
    {
      // Decrement frame if we need to scroll up a screen
      if ((menuitem == frame) && (frame > 1))
      {
        frame--;
        displayClear();
      }

      menuitem--;

      // Don't go beyond first menu item
      if (menuitem == 0)
      {
        menuitem = 1;
      }
    }


    // If in the 2nd level update menu variables where appropriate (add all menu categories where variables are changed by turning
    // the rotary encoder (but don't include categories where selection runs a function or categories with boolean type variables
    // as these are toggled as soon as selected)
    else if (menu_level == 2)
    {
      switch (menuitem) {
        case MODE:
          selectedMode--;
          if (selectedMode <= -1)
          {
            selectedMode = 1;
          }
          break;
        case SPEED:
          speed_opt -= 50;
          if (speed_opt <= -1)
          {
            speed_opt = 2000;
          }
          break;
        case STILLS:
          stills_opt--;
          if (stills_opt <= 0)
          {
            stills_opt = 36;
          }
          break;
        case ANGLE:
          angle_opt -= 5;
          if (angle_opt <= 0)
          {
            angle_opt = 360;
          }
          break;
        default:
          break;
      }
    }
  }


  // Turned rotary encoder Clockwise ?
  if (down)
  {
    down = false;

    if (menu_level == 1) // In top level menu and we have turned the Rotary Encoder Clockwise
    {

      // Increment frame if we need to scroll down a screen
      if (menuitem >= noOfMenuItemsToDisplay)
      {
        if (frame <  FRAME_MAX)
        {
          frame++;
          displayClear();
        }
      }

      menuitem++;

      // Don't go beyond last menu item
      if (menuitem > TOP_MENU_ITEMS)
      {
        menuitem--;
      }
    }


    // If in the 2nd level update menu variables where appropriate (add all menu categories where variables are changed by turning
    // the rotary encoder (but don't include categories where selection runs a function or categories with boolean type variables
    // as these are toggled as soon as selected)
    else if (menu_level == 2)
    {
      switch (menuitem) {
        case MODE:
          selectedMode++;
          if (selectedMode == 2)
          {
            selectedMode = 0;
          }
          break;
        case SPEED:
          speed_opt += 50;
          if (speed_opt > 2000)
          {
            speed_opt = 50;
          }
          break;
        case STILLS:
          stills_opt++;
          if (stills_opt >= 37)
          {
            stills_opt = 1;
          }
          break;
        case ANGLE:
          angle_opt += 5;
          if (angle_opt > 360)
          {
            angle_opt = 5;
          }
          break;
        default:
          break;
      }
    }
  }

  if (button_pressed)
  {
    button_pressed = false;
    displayClear();

    // We are in the first level of the menu and the encoder button has been pressed. Add all menu items which either run a function or toggle a
    // boolean variable
    if (menu_level == 1) {
      switch (menuitem) {
        case RUN:
          // Put call to run function here
          turntableRun();
          break;
        case DIRECTION:
          clockwise = !clockwise; // Toggle clockwise boolean
          break;
        case DEFAULTS:
          resetDefaults();
          break;
        default:  // For all level menu items next menu level was selected so increment it
          menu_level = 2;
          break;
      }
    }
    else if (menu_level == 2)
    {
      menu_level = 1;
    }
  }
}



// Update menu display
void displayMenu()
{
  // If in top menu level print the top level options
  if (menu_level == 1)
  {
    display.setTextSize(1);


#ifdef SSD1306_OLED
    //displayClear();
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Draw white text
    display.setCursor(0, 0);
    display.print(MenuList[0]);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
#else
#ifdef PCD8544_LCD
    //displayClear();
    display.setTextColor(BLACK, WHITE);
    display.setCursor(0, 0);
    display.print(MenuList[0]);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, BLACK);
#else
#ifdef ST7735_TFT
    display.setTextColor(ST7735_TEXT_COL, ST7735_BG_COL);
    display.setCursor(0, 0);
    display.print(MenuList[0]);
    display.drawFastHLine(0, 10, SCREEN_WIDTH, ST7735_TEXT_COL);
#endif
#endif
#endif


    for (int i = 0; i < noOfMenuItemsToDisplay; i++)
    {
      displayMenuItem(frame + i, 15 + (10 * i) , (frame + i == menuitem) ? true : false);
    }

    displayUpdate();
  }

  // Else if in second menu level display the menu options being changed (add appropriate case statement for each integer or string array option)
  else if (menu_level == 2)
  {
    switch (menuitem) {
      case MODE:
        displayStringMenuOption(MODE, mode_opt[selectedMode]);
        break;
      case SPEED:
        displayNumMenuOption(SPEED, speed_opt);
        break;
      case STILLS:
        displayNumMenuOption(STILLS, stills_opt);
        break;
      case ANGLE:
        displayNumMenuOption(ANGLE, angle_opt);
        break;
      default:
        break;
    }
  }

}



// Reset all values to defaults
void resetDefaults()
{
  selectedMode = 0; // Photo
  clockwise = true;
  speed_opt = 30; // RPM
  stills_opt = 20; // Number of photos to take (set to 1 in video mode)
  angle_opt = 180; // Angle
}




void timerIsr() {
  myEncoder->service();
}



// Display menu numeric otpions being updated
void displayNumMenuOption(int index, long numeric_value)
{
  display.setTextSize(1);

#ifdef SSD1306_OLED
  //displayClear();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Draw white text
  display.setCursor(0, 0);
  display.print(MenuList[index]);
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
#else
#ifdef PCD8544_LCD
  //displayClear();
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 0);
  display.print(MenuList[index]);
  display.drawFastHLine(0, 10, SCREEN_WIDTH, BLACK);
#else
#ifdef ST7735_TFT
  display.setTextColor(ST7735_TEXT_COL, ST7735_BG_COL);
  display.setCursor(0, 0);
  display.print(MenuList[index]);
  display.drawFastHLine(0, 10, SCREEN_WIDTH, ST7735_TEXT_COL);
#endif
#endif
#endif

  display.setTextSize(2);
  display.setCursor(5, 25);
  display.print(numeric_value);
  display.print(F("   "));  // Print few extra spaces to ensure previous calue overwritten  (quicker than clearing whole display)

  displayUpdate();
}


// Display menu string array options being updated
void displayStringMenuOption(int index, String string_option)
{
  display.setTextSize(1);

#ifdef SSD1306_OLED
  //displayClear();
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Draw white text
  display.setCursor(0, 0);
  display.print(MenuList[index]);
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
#else
#ifdef PCD8544_LCD
  //displayClear();
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 0);
  display.print(MenuList[index]);
  display.drawFastHLine(0, 10, SCREEN_WIDTH, BLACK);
#else
#ifdef ST7735_TFT
  display.setTextColor(ST7735_TEXT_COL, ST7735_BG_COL);
  display.setCursor(0, 0);
  display.print(MenuList[index]);
  display.drawFastHLine(0, 10, SCREEN_WIDTH, ST7735_TEXT_COL);
#endif
#endif
#endif

  display.setTextSize(2);
  display.setCursor(5, 25);
  display.print(string_option);
  display.print(F("   "));  // Print few extra spaces to ensure previous calue overwritten  (quicker than clearing whole display)

  displayUpdate();
}


void displayMenuItem(int index, int position, boolean selected)
{

#ifdef SSD1306_OLED
  if (selected)
  {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);  // Inverse
  }
  else
  {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  }
#else
#ifdef PCD8544_LCD
  if (selected)
  {

    display.setTextColor(WHITE, BLACK);  // Inverse
  }
  else
  {
    display.setTextColor(BLACK, WHITE);
  }
#else
#ifdef ST7735_TFT
  if (selected)
  {
    display.setTextColor(ST7735_BG_COL, ST7735_TEXT_COL);  // Inverse
  }
  else
  {
    display.setTextColor(ST7735_TEXT_COL, ST7735_BG_COL);
  }
#endif
#endif
#endif


  display.setCursor(0, position);
  display.print(F(">"));
  display.print(MenuList[index]);
  if (index == DIRECTION) {
    display.print(F(":"));
    display.print(dir_opt[clockwise]);
  }
}


void displayUpdate()
{
#if defined  (SSD1306_OLED) || defined (PCD8544_LCD)
  display.display();
#endif
}


void displayClear()
{
#ifdef SSD1306_OLED
  display.clearDisplay();
#else
#ifdef PCD8544_LCD
  display.clearDisplay();
#else
#ifdef ST7735_TFT
  display.fillScreen(ST7735_BG_COL);
#endif
#endif
#endif
}



// Check for rotary encoder updates
void updateRotaryEncoder()
{
  encoderCurrent += myEncoder->getValue();

  // Check for rotary encoder being turned
  if (encoderCurrent / 2 > encoderLast) { // Turned clockwise ?
    encoderLast = encoderCurrent / 2;
    down = true;
    delay(150);
  } else   if (encoderCurrent / 2 < encoderLast) { // Turned anti clockwise
    encoderLast = encoderCurrent / 2;
    up = true;
    delay(150);
  }

  // Check for encoder button pressed
  ClickEncoder::Button b = myEncoder->getButton();
  if (b != ClickEncoder::Open) {
    switch (b) {
      case ClickEncoder::Clicked:
        button_pressed = true;
        break;
    }
  }
}


// Rotate turntable according to menu parameters selected
void turntableRun()
{
  // Set location to move stepper to depending upon angle selected
  long Stepper_Location = (angle_opt * stepsPerRevolution) / 360;

  // If mode == video then still_opt = 1 (only one photo, ie. only need to start video at beginning of rotation)
  if (selectedMode == 1)
  {
    stills_opt = 1;
  }

  // Then adjust according to how many times it has to stop to take still photo
  Stepper_Location = Stepper_Location / stills_opt;

  stepper.enableOutputs();
  stepper.setCurrentPosition(0);

  displayNumMenuOption(RUN, Stepper_Location);

  displayClear();

  takePhoto(); // Start video or take first photo
  delay(1000); // Wait 1 second

  for (int photo_index = 0; photo_index < stills_opt; photo_index++)
  {
    for (int l_index = 0; l_index < Stepper_Location; l_index++)
    {
      stepper.move(clockwise ? 1 : -1); // Select clockwise or anti clockwise depending upon selected value of clockwise boolean flag
      stepper.setSpeed(speed_opt);  // Set speed according to selected value
      stepper.setMaxSpeed(4000);
      // Do this until the stepper has reached the new destination
      while (stepper.distanceToGo() != 0) {  // if stepper hasn't reached new position
        stepper.runSpeedToPosition(
          
          
          
          
          );  // move the stepper until new position reached
      }
    }
    takePhoto(); // take photoq 
    delay (1000);
  }

  // Disable motor pin outputs to save power
  stepper.disableOutputs();

}



void takePhoto()
{
  //Toggle camera control pin low (active) then high to take a photo (or start video)
  digitalWrite(CAMERA_CONTROL_PIN, LOW);
  delay(200);
  digitalWrite(CAMERA_CONTROL_PIN, HIGH);
}
