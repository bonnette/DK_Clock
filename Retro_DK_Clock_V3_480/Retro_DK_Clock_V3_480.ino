/*  Arduino DK Clock Project
  V3 - Bug fixed where when reachs 95 mX value not reset to start. Reset Jump issue resolved.
*/

#include <UTFT.h>
#include <URTouch.h>
#include <EEPROM.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

// Alarm Variables
boolean alarmstatus = false; // flag where false is off and true is on
boolean soundalarm = false; // Flag to indicate the alarm needs to be initiated
int alarmhour = 0;  // hour of alarm setting
int alarmminute = 0; // Minute of alarm setting
byte ahour; //Byte variable for hour
byte amin; //Byte variable for minute
int actr = 300; // When alarm sounds this is a counter used to reset sound card until screen touched
int act = 0;


// Initializes RTC time values:
const int DS1307 = 0x68; // Address of DS1307 see data sheets

// Display Dimmer Variables
int dimscreen = 255; // This variable is used to drive the screen brightness where 255 is max brightness
int LDR = 100; // LDR variable measured directly from Analog 7

// JumpVariables
int jptr = 0; // Points to value in array
int jumpval[16] = {0, -7, -9, -11, -12, -13, -14, -15, -15, -14, -13, -11, -9, -7, -4, 0}; // Value used to subtract from mY during a jump
boolean jumptrigger = false; // Flag indicates if jump initiated
int jumpY = 0; // Temp storage of mY during jump
int jumpA = 0; // Initial jumping coodrinate before adjustment

// Mario Variables

int mX = 28;
int mY = 278;
int mimage = 1; // Used to determine which image should be used
int mdirection = 1; // Where Down = 0, Right = 1, Left = 2
int mprevdirection = 1; // Where Down = 0, Right = 1, Left = 2
boolean mtrigger = true; // switch used to trigger release of a barrel


//Barrel Variables
// Three Barrels are able to be sent down at a time
// Each require their own set of variables and a desicion tree to follow when released
// Monkey waives arms then rolls a barrell down which sets the tempo for all barrels released
int Barreldelay = 5;  // Number of main loop cycles before the Monkey graphic changes and the barrel drop is triggered
byte bd = 0; // Temp variable to make turning decisions

int b1X = 42;
int b1Y = 74;
int b1image = 1; // Used to determine which image should be used
int b1direction = 0; // Where Down = 0, Right = 1, Left = 2
int prevb1direction = 0; // Where Down = 0, Right = 1, Left = 2
boolean triggerbarrel1 = false; // switch used to trigger release of a barrel


int b2X = 42;
int b2Y = 74;
int b2image = 1; // Used to determine which image should be used
int b2direction = 0; // Where Down = 0, Right = 1, Left = 2
int prevb2direction = 0; // Where Down = 0, Right = 1, Left = 2
boolean triggerbarrel2 = false; // switch used to trigger release of a barrel

int b3X = 42;
int b3Y = 74;
int b3image = 1; // Used to determine which image should be used
int b3direction = 0; // Where Down = 0, Right = 1, Left = 2
int prevb3direction = 0; // Where Down = 0, Right = 1, Left = 2
boolean triggerbarrel3 = false; // switch used to trigger release of a barrel

int b4X = 42;
int b4Y = 74;
int b4image = 1; // Used to determine which image should be used
int b4direction = 0; // Where Down = 0, Right = 1, Left = 2
int prevb4direction = 0; // Where Down = 0, Right = 1, Left = 2
boolean triggerbarrel4 = false; // switch used to trigger release of a barrel

// scoreboard

int mscore = 0; // current mario score
int bscore = 0; // current DK score
int prevmscore = 0; // previous mario score
int prevbscore = 0; // previous DK score


//==== Creating Objects
UTFT    myGLCD(ILI9341_16, 38, 39, 40, 41); //Parameters should be adjusted to your Display/Schield model
URTouch  myTouch( 6, 5, 4, 3, 2);

//==== Defining Fonts
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t SevenSeg_XXXL_Num[];

// Define bitmaps


extern unsigned int Barrelr1[540]; // Barrel Bitmap
extern unsigned int Barrelr2[540]; // Barrel Bitmap
extern unsigned int Barrelr3[540]; // Barrel Bitmap
extern unsigned int Barrelr4[540]; // Barrel Bitmap
extern unsigned int Barrelh1[330]; // Barrel Bitmap
extern unsigned int Barrelh2[330]; // Barrel Bitmap


extern unsigned int DK_scaffold[578]; // Scaffold Bitmap
extern unsigned int Monkey1[0x5A0]; // Monkey1 Bitmap
extern unsigned int Monkey2[0x5A0]; // Monkey2 Bitmap
extern unsigned int Monkey3[0x5A0]; // Monkey3 Bitmap
extern unsigned int Monkeyleft[0x5A0]; // Monkey3 Bitmap
extern unsigned int MonkeyBarrel[0x5A0]; // MonkeyBarrel Bitmap
extern unsigned int Barrel1[0xA0]; // Barrel Bitmap
extern unsigned int Oil1[864]; // Oil1 Bitmap
extern unsigned int Oil2[864]; // Oil2 Bitmap
extern unsigned int Girl1[1364]; // Girl1 Bitmap
extern unsigned int Ladder1[330]; // Ladder1 Bitmap

extern unsigned int MarioL1[576]; // 24X24
extern unsigned int MarioL2[576]; // 24X24
extern unsigned int MarioL3[576]; // 24X24 Jump Left
extern unsigned int MarioR1[576]; // 24X24
extern unsigned int MarioR2[576]; // 24X24
extern unsigned int MarioR3[576]; // 24X24 Jump Right
extern unsigned int MarioU1[560]; // 24X24 Climb Ladder 1
extern unsigned int MarioU2[560]; // 24X24 Climb Ladder 2
extern unsigned int MarioU3[560]; // 24X24 Climb Ladder 3
extern unsigned int MarioU4[432]; // 24X18 Climb Ladder 4
extern unsigned int MarioStop[0x100]; // 16x16 Mario Stand Still

// Touch screen coordinates
boolean screenPressed = false;
int xT, yT;
int userT = 4; // flag to indicate directional touch on screen
boolean setupscreen = false; // used to access the setup screen

//Alarm setup variables
boolean xsetup = false; // Flag to determine if existing setup mode

// Monkey Character Graphic number

int monkeygraphic = 0;// Used to track the change in graphics

// Animation delay to slow movement down
int dly = 15; // Orignally 30

// Time Refresh counter
int rfcvalue = 900; // wait this long untiul check time for changes
int rfc = 1;


// Declare global variables for previous time,  to enable refesh of only digits that have changed
// There are four digits that bneed to be drawn independently to ensure consisitent positioning of time
int c1 = 20;  // Tens hour digit
int c2 = 20;  // Ones hour digit
int c3 = 20;  // Tens minute digit
int c4 = 20;  // Ones minute digit


void setup() {

  //Initialize RTC
  Serial.begin(9600);
  // while (!Serial) ; // wait until Arduino Serial Monitor opens
  delay(200);
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  setSyncInterval(60); // sync the time every 60 seconds (1 minutes)
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
    RTC.set(1408278800); // set the RTC to Aug 25 2014 9:00 am
    setTime(1408278800);
  }
  else {
    Serial.println("RTC has set the system time");
  }

  // Randomseed will shuffle the random function
  randomSeed(analogRead(0));

  // Setup Alarm enable pin to play back sound on the ISD1820 board
  pinMode(8, OUTPUT); // D8 used to toggle sound
  digitalWrite(8, LOW); // Set to low to turn off sound

  // Initiate display
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_LOW);


  drawscreen(); // Initiate the game
  UpdateDisp(); // update value to clock


}

void loop() {

  // Set Screen Brightness
  // Check the ambient light and adjust LED brightness to suit Ambient approx 500 dark is below 100
  LDR = analogRead(A7);

  /* Test value range of LDR
    myGLCD.setColor(237, 28, 36);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumI(LDR,250,60);
  */

  if (LDR >= 121) {
    dimscreen = 255;
  }

  if (LDR <= 120)   {
    dimscreen = 35;
  }

  analogWrite(9, dimscreen); // Controls brightness 0 is Dark, Ambient room is approx 25 and 70 is direct sunlight

  // Print Score
  if ((prevbscore != bscore) || (prevmscore != mscore)) {

    if ((bscore > 95) || ( mscore > 95)) {
      myGLCD.setColor(237, 28, 36);
      myGLCD.setBackColor(0, 0, 0);

      myGLCD.print("DK", 24, 160); 
      myGLCD.printNumI(bscore, 36, 160, 3);

      myGLCD.print("M", 204, 160); 
      myGLCD.printNumI(mscore, 216, 160, 3); 

      delay(3000);  // Delay then reset the game
      bscore = 0;
      mscore = 0;
      myGLCD.fillRect(mX - 1, mY + jumpA - 1, mX + 24, mY + jumpA + 24); // Clear trail off graphic before printing new position  24x24

      mX = 28;
      mY = 278;
      jptr = 0;
      jumpY = 0;
      mdirection = 1;
      mprevdirection = 1;
      jumptrigger = false;

      b1X = 42;
      b1Y = 74;
      b1image = 1; // Used to determine which image should be used
      b1direction = 0; // Where Down = 0, Right = 1, Left = 2
      prevb1direction = 0; // Where Down = 0, Right = 1, Left = 2
      triggerbarrel1 = false; // switch used to trigger release of a barrel

      b2X = 42;
      b2Y = 74;
      b2image = 1; // Used to determine which image should be used
      b2direction = 0; // Where Down = 0, Right = 1, Left = 2
      prevb2direction = 0; // Where Down = 0, Right = 1, Left = 2
      triggerbarrel2 = false; // switch used to trigger release of a barrel


      b3X = 42;
      b3Y = 74;
      b3image = 1; // Used to determine which image should be used
      b3direction = 0; // Where Down = 0, Right = 1, Left = 2
      prevb3direction = 0; // Where Down = 0, Right = 1, Left = 2
      triggerbarrel3 = false; // switch used to trigger release of a barrel


      b4X = 42;
      b4Y = 74;
      b4image = 1; // Used to determine which image should be used
      b4direction = 0; // Where Down = 0, Right = 1, Left = 2
      prevb4direction = 0; // Where Down = 0, Right = 1, Left = 2
      triggerbarrel4 = false; // switch used to trigger release of a barrel

      myGLCD.clrScr();

      c1 = 20;
      c2 = 20;
      c3 = 20;
      c4 = 20;

      drawscreen(); // Initiate the game
      UpdateDisp(); // update value to clock

    }

    myGLCD.setColor(237, 28, 36);
    myGLCD.setBackColor(0, 0, 0);

    myGLCD.print("DK", 24, 160); //
    myGLCD.printNumI(bscore, 36, 160, 3); //

    myGLCD.print("M", 204, 160); //
    myGLCD.printNumI(mscore, 216, 160, 3); //
  }

  // Read the current date and time from the RTC and reset board
  rfc++;
  if (rfc >= rfcvalue) { // count cycles and print time
    UpdateDisp(); // update value to clock then ...
    dly = 15; // reset delay
    rfc = 0;

  }

  //***********************************************************************************************
  // Mario Logic
  // **********************************************************************************************
  /*
    int mX = 30;
    int mY = 230;
    int mimage = 1; // Used to determine which image should be used
    int mdirection = 0; // Where Down = 0, Right = 1, Left = 2
    int mprevdirection = 0; // Where Down = 0, Right = 1, Left = 2
    boolean mtrigger = tru; // switch used to trigger release of a mario

  **************************************************************************************************

    Mario Movements
    1 - Right as expected left & right of touch screen
    2 - Left as expected left of touch screen
    3 - Jump straight Up = Top of touch screen
    4 - Right Jump = Top RHS of touch screem
    5 - Left Jump = Top LHS of touch screen
    6 - Climb Down = When at top of ladder Bottom of touch screen
    7 - Climb Up = When at bottom of ladder Top of touch screen
    8 - Stop = Stand upright when touch Bottom of touch screen

    Use Image Number to Toggle the correct image based on Direction
    Approach
    Divide the screen into rows and columns. Build a decision based on each row and then

  */


  // Assess Jump Position

  /*
    // JumpVariables
    int jptr = 0; // Points to value in array
    int jumpval[14]

  */
  if (jumptrigger == true) {

    jumpA = jumpval[jptr]; // Current position of graphic to be erased
    jptr = jptr + 1;
    jumpY = jumpval[jptr]; //future position of graphic


    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.fillRect(mX - 1, mY + jumpA - 1, mX + 24, mY + jumpA + 24); // Clear trail off graphic before printing new position  24x24

    if (jumpval[jptr] == 0) { // End of Jump
      // Reset flags
      jumptrigger = false;
      jptr = 0;
      jumpY = 0;
      // Clear up damage to background
      redraw(mX, mY, mdirection);

    }
  }

  // Capture previous score before changing to avoid priting scoreboard each cycle
  prevmscore = mscore;
  prevbscore = bscore;

  // ************************************************************************************************
  // Check for collision and reset Mario if so
  // ************************************************************************************************
  /* Mario no kill
    if ((b1X - mX >= 4) && (b1X - mX <= 12) && (b1Y - (mY+jumpA) <= 10) && (b1Y - (mY+jumpA) >= 0)){
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.fillRect(mX-1, mY+jumpA-1, mX+24, mY+jumpA+24); // clear mario
    // incremebnt monkey score
    bscore = bscore + 1;
    // Clear up damage to background
     redraw(mX, mY, mdirection);
    // reset coorinates
    mX = 28;
    mY = 278;
    jptr = 0;
    jumpY = 0;
    mdirection = 1;
    mprevdirection = 1;
    jumptrigger = false;
    }

    if ((b2X - mX >= 4) && (b2X - mX <= 12) && (b2Y - (mY+jumpA) <= 10)&& (b2Y - (mY+jumpA) >= 0)){
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.fillRect(mX-1, mY+jumpA-1, mX+24, mY+jumpA+24); // clear mario
    // incremebnt monkey score
    bscore = bscore + 1;
    // Clear up damage to background
     redraw(mX, mY, mdirection);
    // reset coorinates
    mX = 28;
    mY = 278;
    jptr = 0;
    jumpY = 0;
    mdirection = 1;
    mprevdirection = 1;
    jumptrigger = false;
    }
    if ((b3X - mX >= 4) && (b3X - mX <= 12) && (b3Y - (mY+jumpA) <= 10) && (b3Y - (mY+jumpA) >= 0)){
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.fillRect(mX-1, mY+jumpA-1, mX+24, mY+jumpA+24); // clear mario

    // incremebnt monkey score
    bscore = bscore + 1;
    // Clear up damage to background
     redraw(mX, mY, mdirection);
    // reset coorinates
    mX = 28;
    mY = 278;
    jptr = 0;
    jumpY = 0;
    mdirection = 1;
    mprevdirection = 1;
    jumptrigger = false;
    }
    if ((b4X - mX >= 4) && (b4X - mX <= 12) && (b4Y - (mY+jumpA) <= 10) && (b4Y - (mY+jumpA) >= 0)){
    myGLCD.setColor(0, 0, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.fillRect(mX-1, mY+jumpA-1, mX+24, mY+jumpA+24); // clear mario
    // incremebnt monkey score
    bscore = bscore + 1;
    // Clear up damage to background
     redraw(mX, mY, mdirection);
    // reset coorinates
    mX = 28;
    mY = 278;
    jptr = 0;
    jumpY = 0;
    mdirection = 1;
    mprevdirection = 1;
    jumptrigger = false;
    }
  */
  // ************************************************************************************************
  // Mario Scoring for jumping Barrell

  if (jumptrigger == false) { //If during  a Jump over a Barrel if there is not a collision then increment the Mario Points counter by 5 Larry Mod to false

    if ((b1X - mX >= 4) && (b1X - mX <= 8) && (b1Y - (mY + jumpA) <= 40) && (b1Y - (mY + jumpA) >= 0)) {
      mscore = mscore + 5;
    }
    if ((b2X - mX >= 4) && (b2X - mX <= 8) && (b2Y - (mY + jumpA) <= 40) && (b2Y - (mY + jumpA) >= 0)) {
      mscore = mscore + 5;
    }
    if ((b3X - mX >= 4) && (b3X - mX <= 8) && (b3Y - (mY + jumpA) <= 40) && (b3Y - (mY + jumpA) >= 0)) {
      mscore = mscore + 5;
    }
    if ((b4X - mX >= 4) && (b4X - mX <= 8) && (b4Y - (mY + jumpA) <= 40) && (b4Y - (mY + jumpA) >= 0)) {
      mscore = mscore + 5;
    }

  }

  // ************************************************************************************************
  // Mario Motion
  // ************************************************************************************************

  // Capture previous direction information
  mprevdirection = mdirection;

  if ((mtrigger == true)) {// Only draw if Mario in play

    drawMario(mX, mY + jumpY, mimage, mdirection, mprevdirection); // Draw mario1`


/*    //Test print of Mario X/Y coordinates
    myGLCD.setColor(255, 255, 255);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.setFont(BigFont);
    myGLCD.printNumI(mX, 220, 100, 3);
    myGLCD.printNumI(mY, 220, 115, 3);
*/

    // Right Mario ##################################################################################
    if ( mdirection == 1) {

      mX = mX + 2 ;  // First move mario down 2 pixels

      // Centralise Y function before decision logic

      if ((mX >= 240) && (mY <= 274) && (mY >= 260)) { // Validate actually on RHS half of Level 1 then apply formula for gradient
        mY = slpup(mX, mY); // Level 1 mY algorithm for RHS of platform
      } else if ((mX <= 239) && (mY >= 279)) { // Validate actually on LHS half of level 1
        mY = 278;  // Level 1 mY algorithm for LHS of platform
      } else if ((mY >= 220) && (mY <= 232)) { // On Level 2
        mY = slpdwn(mX, mY); //Level 2 mY algorithm
      } else if ((mY >= 168) && (mY <= 180)) { // On Level 3
        mY = slpup(mX, mY); //Level 3 mY algorithm
      } else if ((mY >= 113) && (mY <= 136)) { // On Level 4
        mY = slpdwn(mX, mY); // Level 4 mY algorithm
      }

      // For Right movement use these images sequentialy
      if (mprevdirection != mdirection) { // If change in direction reset image sequence
        mimage = 1;
      } else {
        mimage++;
      }
      if ( mimage >= 4) {  // Max 3 images
        mimage = 1;
      }

      // **************************************************
      //Level 1 - One Ladder, One end stop
      // **************************************************
      if ((mX >= 240) && (mY <= 278) && (mY >= 260)) { // Validate actually on RHS half of level 1 then apply formulae for gradient
        mY = slpup(mX, mY); // Level 1 mY algorithm for RHS of platform
        // RHS wall Level 1
        if (mX >= 448) {  // Make a decision on direction
          mdirection = 2; // Left
        }  else if (mX == 416) { // Ladder 6   Level 1
          bd = 7;//random(2);
          if ( bd == 1) { // Randomise selection
            mdirection = 1; // Right
          } else {
            if (jumptrigger == false) {
              mdirection = 7; // Up
            } else {
              mdirection = 1; // Right
            }
          }
        }
      } else


        // **************************************************
        //Level 2 - Three ladders and one end stop
        // **************************************************
        if ((mY >= 220) && (mY <= 233)) {

          //        mY = (((mX)/23)+173); // Level 2 mY Algorithm

          // Ladder 4 Level 2
          if (mX == 46) {  // Make a decision on direction
            bd = random(2);
            if ( bd == 1) { // Randomise selection
              mdirection = 1; // Right
            } else {
              if (jumptrigger == false) {
                mdirection = 7; // Up
              } else {
                mdirection = 1; // Right
              }
            }
          } else
            // Ladder 5 Level 2
            if (mX == 282) {  // Make a decision on direction
              bd = random(2);
              if ( bd == 1) { // Randomise selection
                mdirection = 1; // Right
              } else {
                if (jumptrigger == false) {
                  mdirection = 7; // Up
                } else {
                  mdirection = 1; // Right
                }
              }
            } else
              // Top of Ladder 6 Level 1
              if (mX == 416) {  // Make a decision on direction
                bd = random(2);
                if ( bd == 1) { // Randomise selection
                  mdirection = 1; // Right
                } else {
                  if (jumptrigger == false) {
                    mdirection = 6; // Down
                  } else {
                    mdirection = 1; // Right
                  }
                }
              } else
                // RHS end of scaffold Level 2
                if (mX == 422) {  // Make a decision on direction
                  mY = 232;
                  mdirection = 2; // Left
                }


        } else
          // **************************************************
          //Level 3 - Four Ladders and one end stop
          // **************************************************
          if ((mY >= 168) && (mY <= 180)) {

            //       mY = (((319 - mX)/23)+138);    // Level 3 mY algorithm

            // Top Ladder 4 Level 3
            if (mX == 46) {  // Make a decision on direction
              bd = random(2);
              if ( bd == 1) { // Randomise selection
                mdirection = 1; // Right
              } else {
                if (jumptrigger == false) {
                  mdirection = 6; // Down
                } else {
                  mdirection = 1; // Right
                }
              }
            } else
              // Top Ladder 5 Level 3
              if (mX == 282) {  // Make a decision on direction
                bd = random(2);
                if ( bd == 1) { // Randomise selection
                  mdirection = 1; // Right
                } else {
                  if (jumptrigger == false) {
                    mdirection = 6; // Down
                  } else {
                    mdirection = 1; // Right
                  }
                }
              } else
                // Bottom Ladder 2 Level 3
                if (mX == 148) {  // Make a decision on direction
                  bd = random(2);
                  if ( bd == 1) { // Randomise selection
                    mdirection = 1; // Right
                  } else {
                    if (jumptrigger == false) {
                      mdirection = 7; // Up
                    } else {
                      mdirection = 1; // Right
                    }
                  }
                } else
                 // Bottom Ladder 3 Level 3
                  if (mX == 416) {  // Make a decision on direction
                    bd = random(2);
                    if ( bd == 1) { // Randomise selection
                      mdirection = 1; // Right
                    } else {
                      if (jumptrigger == false) {
                        mdirection = 7; // Up
                      } else {
                        mdirection = 1; // Right
                      }
                    }
                  } else 
                    // RHS end of scaffold Level 3
                    if (mX == 444) {  // Make a decision on direction
                      mY = 168;
                      mdirection = 2; // Left
                    }


          } else
            // **************************************************
            //Level 4 - Three ladders and one end stop
            // **************************************************
            if ((mY >= 112) && (mY <= 124)) {

              //        mY = (((mX)/23)+106);  // Level 4 mY Algorithm

              // Bottom Ladder 1 Level 4
              if (mX == 12) {  // Make a decision on direction
                bd = random(2);
                if ( bd == 1) { // Randomise selection
                  mdirection = 1; // Right
                } else {
                  if (jumptrigger == false) {
                    mdirection = 7; // Up
                  } else {
                    mdirection = 1; // Right
                  }
                }
              } else
                // Top of Ladder 2 Level 4
                if (mX == 148) {  // Make a decision on direction
                  bd = random(2);
                  if ( bd == 1) { // Randomise selection
                    mdirection = 1; // Right
                  } else {
                    if (jumptrigger == false) {
                      mdirection = 6; // Down
                    } else {
                      mdirection = 1; // Right
                    }
                  }
                } else
                  // Top Ladder 3 Level 4
                  if (mX == 416) {  // Make a decision on direction
                    bd = random(2);
                    if ( bd == 1) { // Randomise selection
                      mdirection = 1; // Right
                    } else {
                      if (jumptrigger == false) {
                        mdirection = 6; // Down
                      } else {
                        mdirection = 1; // Right
                      }
                    }
                  } else
                    // RHS end of scaffold Level 4
                    if (mX >= 422) {  // Make a decision on direction
                      mY = 124;
                      mdirection = 2; // Left
                    }
            }

    } else
      // Left Mario  ##################################################################################
      if ( mdirection == 2) {

        mX = mX - 2 ;  // First move mario left 2 pixels

        // Centralise Y function before decision logic

        if ((mX >= 240) && (mY <= 274) && (mY >= 260)) { // Validate actually on RHS half of level 1 then apply formulae for gradient
          int P = mX - 24; // Because we move left we subtract Mario graphic size
          mY = slpdwn(P, mY);
        } else if ((mX <= 239) && (mY >= 279)) { // Validate actually on LHS half of level 1
          mY = 279;  // Level 1 mY algorithm for LHS of platform
        } else if ((mY >= 220) && (mY <= 232)) { // On Level 2
          int P = mX - 24; // Because we move left we subtract Mario graphic size
          mY = slpup(P, mY);
        } else if ((mY >= 168) && (mY <= 180)) { // On Level 3
          int P = mX - 24; // Because we move left we subtract Mario graphic size
          mY = slpdwn(P, mY);
        } else if ((mY >= 113) && (mY <= 136)) { // On Level 4
          int P = mX - 24; // Because we move left we subtract Mario graphic size
          mY = slpup(P, mY);
        }


        // For Left movement use these images sequentialy
        if (mprevdirection != mdirection) { // If change in direction reset image sequence
          mimage = 1;
        } else {
          mimage++;
        }
        if ( mimage >= 4) {  // Max 3 images
          mimage = 1;
        }



        // **************************************************
        //Level 1 - One Ladder, One end stop
        // **************************************************

        if ((mX >= 240) && (mY <= 277) && (mY >= 260)) { // Validate actually on RHS half of level 1 then apply formulae for gradient
          int P = mX - 24;
          mY = slpdwn(P, mY);

          if (mX == 448) {  // Bottom of Ladder 6   Level 1
            bd = random(2);
            if ( bd == 1) { // Randomise selection
              mdirection = 2; // Left
            } else {
              if (jumptrigger == false) {
                mdirection = 7; // Up
              } else {
                mdirection = 2; // Left
              }
            }
          }
        } else if ((mX <= 239) && (mY >= 270)) { // Validate actually on LHS half of level 1
          mY = 278;  // Level 1 mY algorithm for LHS of platform
          // RHS wall Level 1
          if (mX <= 22) {  // Make a decision on direction
            mdirection = 1; // Right
          }
        }
        // **************************************************
        //Level 2 - Three ladders and one end stop
        // **************************************************
        if ((mY >= 220) && (mY <= 233)) {

          //       mY = (((mX)/23)+173); // Level 2 mY Algorithm

          // Ladder 4 Level 2
          if (mX == 46) {  // Make a decision on direction
            bd = random(2);
            if ( bd == 1) { // Randomise selection
              mdirection = 2; // Left
            } else {
              if (jumptrigger == false) {
                mdirection = 7; // Up
              } else {
                mdirection = 2; // Left
              }
            }
          } else
            // Ladder 5 Level 2
            if (mX == 282) {  // Make a decision on direction
              bd = random(2);
              if ( bd == 1) { // Randomise selection
                mdirection = 2; // Left
              } else {
                if (jumptrigger == false) {
                  mdirection = 7; // Up
                } else {
                  mdirection = 2; // Left
                }
              }
            } else
              // Top of Ladder 6 Level 1
              if (mX == 416) {  // Make a decision on direction
                bd = random(2);
                if ( bd == 1) { // Randomise selection
                  mdirection = 2; // Left
                } else {
                  if (jumptrigger == false) {
                    mdirection = 6; // Down
                  } else {
                    mdirection = 2; // Left
                  }
                }
              } else
                // LHS end of scaffold Level 2
                if (mX == 2) {  // Make a decision on direction
                  mY = 221;
                  mdirection = 1; // Right
                }


        } else
          // **************************************************
          //Level 3 - Four Ladders and one end stop
          // **************************************************
          if ((mY >= 167) && (mY <= 181)) {


            //        mY = (((319 - mX)/23)+138);    // Level 3 mY algorithm

            // Top Ladder 4 Level 3
            if (mX == 46) {  // Make a decision on direction
              bd = random(2);
              if ( bd == 1) { // Randomise selection
                mdirection = 2; // Left
              } else {
                if (jumptrigger == false) {
                  mdirection = 6; // Down
                } else {
                  mdirection = 2; // Left
                }
              }
            } else
              // Top Ladder 5 Level 3
              if (mX == 282) {  // Make a decision on direction
                bd = random(2);
                if ( bd == 1) { // Randomise selection
                  mdirection = 2; // Left
                } else {
                  if (jumptrigger == false) {
                    mdirection = 6; // Down
                  } else {
                    mdirection = 2; // Left
                  }
                }
              } else
               // Bottom Ladder 2 Level 3
                if (mX == 148) {  // Make a decision on direction
                  bd = random(2);
                  if ( bd == 1) { // Randomise selection
                    mdirection = 2; // Left
                  } else {
                    if (jumptrigger == false) {
                      mdirection = 7; // Up
                    } else {
                      mdirection = 2; // Left
                    }
                  }
                } else
                 // Bottom Ladder 3 Level 3
                  if (mX == 416) {  // Make a decision on direction
                    bd = random(2);
                    if ( bd == 1) { // Randomise selection
                      mdirection = 2; // Left
                    } else {
                      if (jumptrigger == false) {
                        mdirection = 7; // Up
                      } else {
                        mdirection = 2; // Left
                      }
                    }
                  } else
                    // LHS end of scaffold Level 3
                    if (mX == 36) { // Make a decision on direction
                      mY = 180;
                      mdirection = 1; // Right
                    }


          } else
            // **************************************************
            //Level 4 - Three ladders and one end stop
            // **************************************************
            if ((mY >= 105) && (mY <= 116)) {

              //        mY = (((mX)/23)+106);  // Level 4 mY Algorithm

              // Bottom Ladder 1 Level 4
              if (mX == 12) {  // Make a decision on direction
                bd = random(2);
                if ( bd == 1) { // Randomise selection
                  mdirection = 2; // Left
                } else {
                  if (jumptrigger == false) {
                    mdirection = 7; // Up
                  } else {
                    mdirection = 2; // Left
                  }
                }
              } else
                // Top of Ladder 2 Level 4
                if (mX == 148) {  // Make a decision on direction
                  bd = random(2);
                  if ( bd == 1) { // Randomize selection
                    mdirection = 2; // Left
                  } else {
                    if (jumptrigger == false) {
                      mdirection = 6; // Down
                    } else {
                      mdirection = 2; // Left
                    }
                  }
                } else
                  // Top Ladder 3 Level 4
                  if (mX == 278) {  // Make a decision on direction
                    bd = random(2);
                    if ( bd == 1) { // Randomise selection
                      mdirection = 2; // Left
                    } else {
                      if (jumptrigger == false) {
                        mdirection = 6; // Down
                      } else {
                        mdirection = 2; // Left
                      }
                    }
                  } else
                    // LHS end of scaffold Level 4
                    if (mX == 2) {  // Make a decision on direction
                      mY = 112;
                      mdirection = 1; // Right
                    }
            }



      } else
        // Down Mario ##################################################################################
        if ( mdirection == 6) {

          mY = mY + 2 ;  // First move mario down 2 pixels

          // For Down movement use these images sequentialy
          if (mprevdirection != mdirection) { // If change in direction reset image sequence
            mimage = 1;
          } else {
            mimage++;
          }
          if ( mimage >= 5) {  // Max 4 images
            mimage = 1;
          }

          // Increment mario image needs to be included in the directional logic due to complexity

          if (mX == 12) { // Ladder 1
            if (mY >= 106) { // mario hits L3
              bd = random(2);
              if ( bd == 1) { // Randomise selection
                mdirection = 1;
              } else {
                mdirection = 2;
              }
            }
          } else

            if (mX == 148) { // Ladder 2
              if (mY >= 177) { // mario hits L3
                mY = 177;
                bd = random(2);
                if ( bd == 1) { // Randomise selection
                  mdirection = 1;
                } else {
                  mdirection = 2;
                }
              }
            } else if ((mX == 416) && (mY <= 172)) { // Ladder 3 Larry
              if (mY >= 169) { // mario hits L3
                mY = 169;
                mdirection = 2; // Only option here is left
              }
            } else

              if (mX <= 46) { // Ladder 4
                if (mY >= 222) { // mario hits L2
                  mY = 222;
                  mdirection = 1; // Only option here is Right
                }
              } else if (mX <= 282) { // Ladder 5
                if (mY >= 229) { // mario hits L2
                  mY = 229;
                  mdirection = 1; // Only option here is Right
                }
              } else if ((mX == 416) && (mY >= 232)) { // Ladder 6
                if (mY >= 272) { // mario hits L1
                  mY = 272;
                  mdirection = 2; // Only option here is left
                }
              }

        } else
          // Up Mario  ##################################################################################
          if ( mdirection == 7) {

            mY = mY - 2 ;  // First move mario down 2 pixels

            // For Up movement use these images sequentialy
            if (mprevdirection != mdirection) { // If change in direction reset image sequence
              mimage = 1;
            } else {
              mimage++;
            }
            if ( mimage >= 5) {  // Max 4 images
              mimage = 1;
            }

            // Increment mario image needs to be included in the directional logic due to complexity

            if (mX == 12) { // Ladder 1
              if (mY <= 84) { // mario hits L5
                //          mdirection = 6;  // Down
                // Reset Mario to beginning of game and award 30 points

                // Quick Pause
                delay(1000);
                // Clear Mario
                myGLCD.setColor(0, 0, 0);
                myGLCD.setBackColor(0, 0, 0);
                myGLCD.fillRect(mX - 1, mY + jumpA - 1, mX + 24, mY + jumpA + 24); // clear mario

                // Redraw Ladder
                myGLCD.drawBitmap (15, 99, 15, 22, Ladder1);
                myGLCD.drawBitmap (15, 116, 15, 22, Ladder1);
                myGLCD.drawBitmap (34, 86, 34, 17, DK_scaffold); //   Scaffold
                myGLCD.drawBitmap (0, 86, 34, 17, DK_scaffold); //   Scaffold


                // reset coorinates
                mX = 28;
                mY = 278;
                mdirection = 1;
                mprevdirection = 1;
                jumptrigger = false;
                mscore = mscore + 30;

              }
            } else

              if (mX == 148) { // Ladder 2
                if (mY <= 116) { // mario hits L4
                  mY = 116;
                  bd = random(2);
                  if ( bd == 1) { // Randomise selection
                    mdirection = 1;
                  } else {
                    mdirection = 2;
                  }
                }
              } else if ((mX == 416) && (mY <= 124) && (mY <= 128)) { // Ladder 3
                if (mY <= 124) { // mario hits L4
                  mY = 124;
                  bd = random(2);
                  if ( bd == 1) { // Randomise selection
                    mdirection = 1;
                  } else {
                    mdirection = 2;
                  }
                }
              } else

                if (mX <= 46) { // Ladder 4
                  if (mY <= 180) { // mario hits L3
                    mY = 180;
                    mdirection = 1; // Only option here is Right
                  }
                } else if (mX <= 284) { // Ladder 5
                  if (mY <= 173) { // mario hits L3
                    mY = 173;
                    bd = random(2);
                    if ( bd == 1) { // Randomise selection
                      mdirection = 1;
                    } else {
                      mdirection = 2;
                    }
                  }
                } else if ((mX == 416) && (mY <= 232) && (mY >= 230)) { // Ladder 6
                  if (mY <= 232) { // mario hits L2
                    mY = 232;
                    mdirection = 2; // Only option here is left
                  }
                }
          }
  }

  // **********************************************************************************************


  // **********************************************************************************************
  // Barrel 1 Logic
  // **********************************************************************************************


  /*Test print out

        myGLCD.setColor(255, 0, 0);
        myGLCD.setBackColor(0, 0, 0);
        myGLCD.setFont(BigFont);

        myGLCD.printNumI(b1X, 220, 100,3);
        myGLCD.printNumI(b1Y, 220, 115,3);
  */

  if ((triggerbarrel1 == true)) {// Only draw if Barrel in play

    drawbarrel(b1X, b1Y, b1image, b1direction, prevb1direction);  // Draw Barre1

    // Increment barrel image
    b1image++;
    if (b1image == 5) {
      b1image = 1;
    }

    // Capture previous direction to enable trail blanking
    prevb1direction = b1direction;

    // Direction and Movement Logic


    if (b1direction == 0) { // Barrel Down direction   ***************************************************
      b1Y = b1Y + 2 ;  // First move Barrel down 2 pixels

      if ((b1X == 42) && (b1Y <= 122)) { // Monkey drop L5 (additional test for level) #1
        if (b1Y == 122) { // Barrel hits L4
          b1direction = 1; // Only option here is right
        }
      } else if (b1X == 150) { // Middle Ladder L4 #2
        if (b1Y >= 186) { // Barrel hits L3
          b1direction = 2; // Only option here is left *
        }
      } else if ((b1X == 418) && (b1Y <= 179)) { // RHS Ladder L4 #3
        if (b1Y >= 179) { // Barrel hits L3
          b1direction = 2; // Only option here is left
        }
      } else if ((b1X == 444) && (b1Y <= 178)) { // RHS  L4 drop #4
        if (b1Y >= 178) { // Barrel hits L3
          b1direction = 2; // Only option here is left
        }
      } else

        if ((b1X == 14) && (b1Y <= 230)) { // LHS Drop L3 #5
          if (b1Y >= 230) { // Barrel hits L2
            b1direction = 1; // Only option here is Right
          }
        } else

          if ((b1X == 48) && (b1Y <= 232)) { // LHS Ladder L3 #6
            if (b1Y >= 231) { // Barrel hits L2
              b1Y = 231; // Needed to force Y position
              b1direction = 1; // Only option here is Right
            }

          } else if ((b1X == 286) && (b1Y <= 239)) { // Middle Ladder L3 #7
            if (b1Y >= 238) { // Barrel hits L2
              b1Y = 238; // Needed to force y position
              b1direction = 1; // Only option here is Right
            }
          } else if ((b1X == 418) && (b1Y <= 280)) { // RHS Ladder L2 #8
            if (b1Y >= 279) { // Barrel hits L1
              b1direction = 2; // Only option here is left
            }
          } else if ((b1X == 444) && (b1Y <= 280)) { // RHS drop from L2 #9
            if (b1Y >= 279) { // Barrel hits L1
              b1direction = 2; // Only option here is left
            }
          }
    } else if (b1direction == 1) { // Barrel Right direction  ***************************************************
      b1X = b1X + 2 ;  // First move Barrel down 2 pixels

      // Apply drop relative to position on levels 4 then 2
      // Level 4
      if ((b1Y >= 122) && (b1Y <= 135)) { // Validate actually on level 4 then apply formula for gradient
        b1Y = slpdwn(b1X, b1Y);
        // Now decide on direction at three points on level 4
        if (b1X == 150) { // Level 4 middle ladder decision
          b1direction = random(2);
        } else if (b1X == 418) { // Level 4 RHS ladder decision
          b1direction = random(2);
        } else if (b1X == 444) { // Level 4 end of scaffold
          b1direction = 0; // Only option down
        }


      } else    // Level 2
        if (((b1Y >= 230) && (b1Y <= 243))) { // Validate actually on level 2 then apply formula for gradient
          b1Y = slpdwn(b1X, b1Y);
          // Now decide on direction at two points on level 2
          if (b1X == 418) { // Level 2 RHS ladder decision
            b1direction = random(2);
          } else if (b1X == 444) { // Level 2 end of scaffold
            b1direction = 0; // Only option down
          }

        }


    } else if (b1direction == 2) { // Barrel Left direction **********************************************************
      b1X = b1X - 2 ;  // First move Barrel left 2 pixels

      // Level 3
      if (((b1Y <= 191) && (b1Y >= 178))) { // Validate actually on level 3 then apply formula for gradient
        int P = b1X - 18; // needed because barrel goiing left we must subtract the width of the barrel graphic
        b1Y = slpdwn(P, b1Y);
        // Now decide on direction at three points on level 3
        if (b1X == 286) { // Level 3 middle ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomise selection
            b1direction = 0;
          } else {
            b1direction = 2;
          }
        } else if (b1X == 48) { // Level 3 LHS ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomize selection
            b1direction = 0;
          } else {
            b1direction = 2;
          }
        } else if (b1X == 14) { // Level 3 end of scaffold
          b1direction = 0; // Only option down
        }

      } else   // Level 1
        if (((b1X >= 240) && (b1X <= 450)) && (b1Y <= 286)) { // Validate actually on RHS half of level 1 then apply formula for gradient
          int P = b1X - 18; // needed because barrel going left we must subtract the width of the barrel graphic
          b1Y = slpdwn(P, b1Y);
        }
      if ((b1X <= 239) && (b1Y >= 285)) { // Validate actually on LHS half of level 1
        b1Y = 287;  // Flat finish for Barrell

        if (b1X <= 35) { // Turn off the Barrell and blank graphic
          triggerbarrel1 = false;
          b1image = 1; // Used to determine which image should be used
          b1direction = 0; // Where Down = 0, Right = 1, Left = 2
          prevb1direction = 0;
          myGLCD.fillRect(b1X, b1Y, b1X + 20, b1Y + 15); // Clear Barrel
          b1X = 42;
          b1Y = 74;
        }
      }

    }
  }





  // **********************************************************************************************
  // Barrel 2 Logic
  // **********************************************************************************************

  /*
    // Test print out

        myGLCD.setColor(1, 73, 240);
        myGLCD.setBackColor(0, 0, 0);
        myGLCD.setFont(SmallFont);

        myGLCD.printNumI(b2X, 150, 200,3);
        myGLCD.printNumI(b2Y, 150, 220,3);
  */

  if ((triggerbarrel2 == true)) {// Only draw if Barrel in play

    drawbarrel(b2X, b2Y, b2image, b2direction, prevb2direction);  // Draw Barre1

    // Increment barrel image
    b2image++;
    if (b2image == 5) {
      b2image = 1;
    }

    // Capture previous direction to enable trail blanking
    prevb2direction = b2direction;

    // Direction and Movement Logic


    if (b2direction == 0) { // Barrel Down direction   ***************************************************
      b2Y = b2Y + 2 ;  // First move Barrel down 2 pixels

      if ((b2X == 42) && (b2Y <= 122)) { // Monkey drop L5 (additional test for level) #1
        if (b2Y == 122) { // Barrel hits L4
          b2direction = 1; // Only option here is right
        }
      } else if (b2X == 150) { // Middle Ladder L4 #2
        if (b2Y >= 186) { // Barrel hits L3
          b2direction = 2; // Only option here is left *
        }
      } else if ((b2X == 418) && (b2Y <= 179)) { // RHS Ladder L4 #3
        if (b2Y >= 179) { // Barrel hits L3
          b2direction = 2; // Only option here is left
        }
      } else if ((b2X == 444) && (b2Y <= 178)) { // RHS  L4 drop #4
        if (b2Y >= 178) { // Barrel hits L3
          b2direction = 2; // Only option here is left
        }
      } else

        if ((b2X == 14) && (b2Y <= 230)) { // LHS Drop L3 #5
          if (b2Y >= 230) { // Barrel hits L2
            b2direction = 1; // Only option here is Right
          }
        } else

          if ((b2X == 48) && (b2Y <= 232)) { // LHS Ladder L3 #6
            if (b2Y >= 231) { // Barrel hits L2
              b2Y = 231; // Needed to force Y position
              b2direction = 1; // Only option here is Right
            }

          } else if ((b2X == 286) && (b2Y <= 239)) { // Middle Ladder L3 #7
            if (b2Y >= 238) { // Barrel hits L2
              b2Y = 238; // Needed to force y position
              b2direction = 1; // Only option here is Right
            }
          } else if ((b2X == 418) && (b2Y <= 280)) { // RHS Ladder L2 #8
            if (b2Y >= 279) { // Barrel hits L1
              b2direction = 2; // Only option here is left
            }
          } else if ((b2X == 444) && (b2Y <= 280)) { // RHS drop from L2 #9
            if (b2Y >= 279) { // Barrel hits L1
              b2direction = 2; // Only option here is left
            }
          }
    } else if (b2direction == 1) { // Barrel Right direction  ***************************************************
      b2X = b2X + 2 ;  // First move Barrel down 2 pixels

      // Apply drop relative to position on levels 4 then 2
      // Level 4
      if ((b2Y >= 122) && (b2Y <= 135)) { // Validate actually on level 4 then apply formula for gradient
        b2Y = slpdwn(b2X, b2Y);
        // Now decide on direction at three points on level 4
        if (b2X == 150) { // Level 4 middle ladder decision
          b2direction = random(2);
        } else if (b2X == 418) { // Level 4 RHS ladder decision
          b2direction = random(2);
        } else if (b2X == 444) { // Level 4 end of scaffold
          b2direction = 0; // Only option down
        }


      } else    // Level 2
        if (((b2Y >= 230) && (b2Y <= 243))) { // Validate actually on level 2 then apply formula for gradient
          b2Y = slpdwn(b2X, b2Y);
          // Now decide on direction at two points on level 2
          if (b2X == 418) { // Level 2 RHS ladder decision
            b2direction = random(2);
          } else if (b2X == 444) { // Level 2 end of scaffold
            b2direction = 0; // Only option down
          }

        }


    } else if (b2direction == 2) { // Barrel Left direction **********************************************************
      b2X = b2X - 2 ;  // First move Barrel left 2 pixels

      // Level 3
      if (((b2Y <= 191) && (b2Y >= 178))) { // Validate actually on level 3 then apply formula for gradient
        int P = b2X - 18; // needed because barrel goiing left we must subtract the width of the barrel graphic
        b2Y = slpdwn(P, b2Y);
        // Now decide on direction at three points on level 3
        if (b2X == 286) { // Level 3 middle ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomise selection
            b2direction = 0;
          } else {
            b2direction = 2;
          }
        } else if (b2X == 48) { // Level 3 LHS ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomize selection
            b2direction = 0;
          } else {
            b2direction = 2;
          }
        } else if (b2X == 14) { // Level 3 end of scaffold
          b2direction = 0; // Only option down
        }

      } else   // Level 1
        if (((b2X >= 240) && (b2X <= 450)) && (b2Y <= 286)) { // Validate actually on RHS half of level 1 then apply formula for gradient
          int P = b2X - 18; // needed because barrel going left we must subtract the width of the barrel graphic
          b2Y = slpdwn(P, b2Y);
        }
      if ((b2X <= 239) && (b2Y >= 285)) { // Validate actually on LHS half of level 1
        b2Y = 287;  // Flat finish for Barrell

        if (b2X <= 35) { // Turn off the Barrell and blank graphic
          triggerbarrel2 = false;
          b2image = 1; // Used to determine which image should be used
          b2direction = 0; // Where Down = 0, Right = 1, Left = 2
          prevb2direction = 0;
          myGLCD.fillRect(b2X, b2Y, b2X + 20, b2Y + 15); // Clear Barrel
          b2X = 42;
          b2Y = 74;
        }
      }

    }
  }




  // **********************************************************************************************
  // Barrel 3 Logic
  // **********************************************************************************************

  /*
    // Test print out

        myGLCD.setColor(1, 73, 240);
        myGLCD.setBackColor(0, 0, 0);
        myGLCD.setFont(SmallFont);

        myGLCD.printNumI(b3X, 150, 200,3);
        myGLCD.printNumI(b3Y, 150, 220,3);
  */

  if ((triggerbarrel3 == true)) {// Only draw if Barrel in play

    drawbarrel(b3X, b3Y, b3image, b3direction, prevb3direction);  // Draw Barre1

    // Increment barrel image
    b3image++;
    if (b3image == 5) {
      b3image = 1;
    }

    // Capture previous direction to enable trail blanking
    prevb3direction = b3direction;

    // Direction and Movement Logic


    if (b3direction == 0) { // Barrel Down direction   ***************************************************
      b3Y = b3Y + 2 ;  // First move Barrel down 2 pixels

      if ((b3X == 42) && (b3Y <= 122)) { // Monkey drop L5 (additional test for level) #1
        if (b3Y == 122) { // Barrel hits L4
          b3direction = 1; // Only option here is right
        }
      } else if (b3X == 150) { // Middle Ladder L4 #2
        if (b3Y >= 186) { // Barrel hits L3
          b3direction = 2; // Only option here is left *
        }
      } else if ((b3X == 418) && (b3Y <= 179)) { // RHS Ladder L4 #3
        if (b3Y >= 179) { // Barrel hits L3
          b3direction = 2; // Only option here is left
        }
      } else if ((b3X == 444) && (b3Y <= 178)) { // RHS  L4 drop #4
        if (b3Y >= 178) { // Barrel hits L3
          b3direction = 2; // Only option here is left
        }
      } else

        if ((b3X == 14) && (b3Y <= 230)) { // LHS Drop L3 #5
          if (b3Y >= 230) { // Barrel hits L2
            b3direction = 1; // Only option here is Right
          }
        } else

          if ((b3X == 48) && (b3Y <= 232)) { // LHS Ladder L3 #6
            if (b3Y >= 231) { // Barrel hits L2
              b3Y = 231; // Needed to force Y position
              b3direction = 1; // Only option here is Right
            }

          } else if ((b3X == 286) && (b3Y <= 239)) { // Middle Ladder L3 #7
            if (b3Y >= 238) { // Barrel hits L2
              b3Y = 238; // Needed to force y position
              b3direction = 1; // Only option here is Right
            }
          } else if ((b3X == 418) && (b3Y <= 280)) { // RHS Ladder L2 #8
            if (b3Y >= 279) { // Barrel hits L1
              b3direction = 2; // Only option here is left
            }
          } else if ((b3X == 444) && (b3Y <= 280)) { // RHS drop from L2 #9
            if (b3Y >= 279) { // Barrel hits L1
              b3direction = 2; // Only option here is left
            }
          }
    } else if (b3direction == 1) { // Barrel Right direction  ***************************************************
      b3X = b3X + 2 ;  // First move Barrel down 2 pixels

      // Apply drop relative to position on levels 4 then 2
      // Level 4
      if ((b3Y >= 122) && (b3Y <= 135)) { // Validate actually on level 4 then apply formula for gradient
        b3Y = slpdwn(b3X, b3Y);
        // Now decide on direction at three points on level 4
        if (b3X == 150) { // Level 4 middle ladder decision
          b3direction = random(2);
        } else if (b3X == 418) { // Level 4 RHS ladder decision
          b3direction = random(2);
        } else if (b3X == 444) { // Level 4 end of scaffold
          b3direction = 0; // Only option down
        }


      } else    // Level 2
        if (((b3Y >= 230) && (b3Y <= 243))) { // Validate actually on level 2 then apply formula for gradient
          b3Y = slpdwn(b3X, b3Y);
          // Now decide on direction at two points on level 2
          if (b3X == 418) { // Level 2 RHS ladder decision
            b3direction = random(2);
          } else if (b3X == 444) { // Level 2 end of scaffold
            b3direction = 0; // Only option down
          }

        }


    } else if (b3direction == 2) { // Barrel Left direction **********************************************************
      b3X = b3X - 2 ;  // First move Barrel left 2 pixels

      // Level 3
      if (((b3Y <= 191) && (b3Y >= 178))) { // Validate actually on level 3 then apply formula for gradient
        int P = b3X - 18; // needed because barrel goiing left we must subtract the width of the barrel graphic
        b3Y = slpdwn(P, b3Y);
        // Now decide on direction at three points on level 3
        if (b3X == 286) { // Level 3 middle ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomise selection
            b3direction = 0;
          } else {
            b3direction = 2;
          }
        } else if (b3X == 48) { // Level 3 LHS ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomize selection
            b3direction = 0;
          } else {
            b3direction = 2;
          }
        } else if (b3X == 14) { // Level 3 end of scaffold
          b3direction = 0; // Only option down
        }

      } else   // Level 1
        if (((b3X >= 240) && (b3X <= 450)) && (b3Y <= 286)) { // Validate actually on RHS half of level 1 then apply formula for gradient
          int P = b3X - 18; // needed because barrel going left we must subtract the width of the barrel graphic
          b3Y = slpdwn(P, b3Y);
        }
      if ((b3X <= 239) && (b3Y >= 285)) { // Validate actually on LHS half of level 1
        b3Y = 287;  // Flat finish for Barrell

        if (b3X <= 35) { // Turn off the Barrell and blank graphic
          triggerbarrel3 = false;
          b3image = 1; // Used to determine which image should be used
          b3direction = 0; // Where Down = 0, Right = 1, Left = 2
          prevb3direction = 0;
          myGLCD.fillRect(b3X, b3Y, b3X + 20, b3Y + 15); // Clear Barrel
          b3X = 42;
          b3Y = 74;
        }
      }

    }
  }





  // **********************************************************************************************
  // Barrel 4 Logic
  // **********************************************************************************************

  /*
    // Test print out

        myGLCD.setColor(1, 73, 240);
        myGLCD.setBackColor(0, 0, 0);
        myGLCD.setFont(SmallFont);

        myGLCD.printNumI(b4X, 150, 200,3);
        myGLCD.printNumI(b4Y, 150, 220,3);
  */

  if ((triggerbarrel4 == true)) {// Only draw if Barrel in play

    drawbarrel(b4X, b4Y, b4image, b4direction, prevb4direction);  // Draw Barre1

    // Increment barrel image
    b4image++;
    if (b4image == 5) {
      b4image = 1;
    }

    // Capture previous direction to enable trail blanking
    prevb4direction = b4direction;

    // Direction and Movement Logic


    if (b4direction == 0) { // Barrel Down direction   ***************************************************
      b4Y = b4Y + 2 ;  // First move Barrel down 2 pixels

      if ((b4X == 42) && (b4Y <= 122)) { // Monkey drop L5 (additional test for level) #1
        if (b4Y == 122) { // Barrel hits L4
          b4direction = 1; // Only option here is right
        }
      } else if (b4X == 150) { // Middle Ladder L4 #2
        if (b4Y >= 186) { // Barrel hits L3
          b4direction = 2; // Only option here is left *
        }
      } else if ((b4X == 418) && (b4Y <= 179)) { // RHS Ladder L4 #3
        if (b4Y >= 179) { // Barrel hits L3
          b4direction = 2; // Only option here is left
        }
      } else if ((b4X == 444) && (b4Y <= 178)) { // RHS  L4 drop #4
        if (b4Y >= 178) { // Barrel hits L3
          b4direction = 2; // Only option here is left
        }
      } else

        if ((b4X == 14) && (b4Y <= 230)) { // LHS Drop L3 #5
          if (b4Y >= 230) { // Barrel hits L2
            b4direction = 1; // Only option here is Right
          }
        } else

          if ((b4X == 48) && (b4Y <= 232)) { // LHS Ladder L3 #6
            if (b4Y >= 231) { // Barrel hits L2
              b4Y = 231; // Needed to force Y position
              b4direction = 1; // Only option here is Right
            }

          } else if ((b4X == 286) && (b4Y <= 239)) { // Middle Ladder L3 #7
            if (b4Y >= 238) { // Barrel hits L2
              b4Y = 238; // Needed to force y position
              b4direction = 1; // Only option here is Right
            }
          } else if ((b4X == 418) && (b4Y <= 280)) { // RHS Ladder L2 #8
            if (b4Y >= 279) { // Barrel hits L1
              b4direction = 2; // Only option here is left
            }
          } else if ((b4X == 444) && (b4Y <= 280)) { // RHS drop from L2 #9
            if (b4Y >= 279) { // Barrel hits L1
              b4direction = 2; // Only option here is left
            }
          }
    } else if (b4direction == 1) { // Barrel Right direction  ***************************************************
      b4X = b4X + 2 ;  // First move Barrel down 2 pixels

      // Apply drop relative to position on levels 4 then 2
      // Level 4
      if ((b4Y >= 122) && (b4Y <= 135)) { // Validate actually on level 4 then apply formula for gradient
        b4Y = slpdwn(b4X, b4Y);
        // Now decide on direction at three points on level 4
        if (b4X == 150) { // Level 4 middle ladder decision
          b4direction = random(2);
        } else if (b4X == 418) { // Level 4 RHS ladder decision
          b4direction = random(2);
        } else if (b4X == 444) { // Level 4 end of scaffold
          b4direction = 0; // Only option down
        }


      } else    // Level 2
        if (((b4Y >= 230) && (b4Y <= 243))) { // Validate actually on level 2 then apply formula for gradient
          b4Y = slpdwn(b4X, b4Y);
          // Now decide on direction at two points on level 2
          if (b4X == 418) { // Level 2 RHS ladder decision
            b4direction = random(2);
          } else if (b4X == 444) { // Level 2 end of scaffold
            b4direction = 0; // Only option down
          }

        }


    } else if (b4direction == 2) { // Barrel Left direction **********************************************************
      b4X = b4X - 2 ;  // First move Barrel left 2 pixels

      // Level 3
      if (((b4Y <= 191) && (b4Y >= 178))) { // Validate actually on level 3 then apply formula for gradient
        int P = b4X - 18; // needed because barrel goiing left we must subtract the width of the barrel graphic
        b4Y = slpdwn(P, b4Y);
        // Now decide on direction at three points on level 3
        if (b4X == 286) { // Level 3 middle ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomise selection
            b4direction = 0;
          } else {
            b4direction = 2;
          }
        } else if (b4X == 48) { // Level 3 LHS ladder decision
          bd = random(2);
          if ( bd == 1) { // Randomize selection
            b4direction = 0;
          } else {
            b4direction = 2;
          }
        } else if (b4X == 14) { // Level 3 end of scaffold
          b4direction = 0; // Only option down
        }

      } else   // Level 1
        if (((b4X >= 240) && (b4X <= 450)) && (b4Y <= 286)) { // Validate actually on RHS half of level 1 then apply formula for gradient
          int P = b4X - 18; // needed because barrel going left we must subtract the width of the barrel graphic
          b4Y = slpdwn(P, b4Y);
        }
      if ((b4X <= 239) && (b4Y >= 285)) { // Validate actually on LHS half of level 1
        b4Y = 287;  // Flat finish for Barrell

        if (b4X <= 35) { // Turn off the Barrell and blank graphic
          triggerbarrel4 = false;
          b4image = 1; // Used to determine which image should be used
          b4direction = 0; // Where Down = 0, Right = 1, Left = 2
          prevb4direction = 0;
          myGLCD.fillRect(b4X, b4Y, b4X + 20, b4Y + 15); // Clear Barrel
          b4X = 42;
          b4Y = 74;
        }
      }

    }
  }








  // **********************************************************************************************


  //=== Check if Alarm needs to be sounded
  if (alarmstatus == true) {
    if ( (alarmhour == hour()) && (alarmminute == minute())) {  // Sound the alarm
      soundalarm = true;
    }
  }

  //=== Start Alarm Sound - Sound pays for 10 seconds then will restart at 20 second mark

  if ((alarmstatus == true) && (soundalarm == true)) { // Set off a counter and take action to restart sound if screen not touched

    if (act == 0) { // Set off alarm by toggling D8, recorded sound triggered by LOW to HIGH transition
      digitalWrite(8, HIGH); // Set high
      digitalWrite(8, LOW); // Set low
      UpdateDisp(); // update value to clock
    }
    act = act + 1;

    if (act == actr) { // Set off alarm by toggling D8, recorded sound triggered by LOW to HIGH transition
      digitalWrite(8, HIGH); // Set high
      digitalWrite(8, LOW); // Set low
      act = 0; // Reset counter hopfully every 20 seconds
    }

  }

  // Check if user input to touch screen
  // UserT sets direction 0 = right, 1 = down, 2 = left, 3 = up, 4 = no touch input


  myTouch.read();
  if (myTouch.dataAvailable() && !screenPressed) {
    xT = myTouch.getX();
    yT = myTouch.getY();

    // Capture direction request from user
    if ((xT >= 1) && (xT <= 80) && (yT >= 80) && (yT <= 160)) { // Left
      userT = 2; // Request to go left
    }
    if ((xT >= 240) && (xT <= 318) && (yT >= 80) && (yT <= 160)) { // Right
      userT = 1; // Request to go right
    }
    if ((xT >= 110) && (xT <= 210) && (yT >= 1) && (yT <= 80)) { // Up
      userT = 7; // Request to go Up or Jump
    }
    if ((xT >= 110) && (xT <= 210) && (yT >= 160) && (yT <= 238)) { // Down
      userT = 6; // Request to go Down
    }
    // **********************************
    // ******* Enter Setup Mode *********
    // **********************************

    if (((xT >= 120) && (xT <= 200) && (yT >= 105) && (yT <= 140)) &&  (soundalarm != true)) { // Call Setup Routine if alarm is not sounding
      xsetup = true;  // Toggle flag
      clocksetup(); // Call Clock Setup Routine
      UpdateDisp(); // update value to clock

    } else  // If centre of screen touched while alarm sounding then turn off the sound and reset the alarm to not set

      if (((xT >= 120) && (xT <= 200) && (yT >= 105) && (yT <= 140)) && ((alarmstatus == true) && (soundalarm == true))) {

        alarmstatus = false;
        soundalarm = false;
        digitalWrite(8, LOW); // Set low
      }

    if (userT == 2 && mdirection == 1 ) { // Going Right request to turn Left OK
      mdirection = 2;
    }
    if (userT == 1 && mdirection == 2 ) { // Going Left request to turn Right OK
      mdirection = 1;
    }
    if (userT == 6 && mdirection == 7 ) { // Going Up request to turn Down OK
      mdirection = 6;
    }
    if (userT == 7 && mdirection == 6 ) { // Going Down request to turn Up OK
      mdirection = 7;
    }

    // Set Flag for Jump only if going left or right
    if (userT == 7 && ((mprevdirection == 1) || (mprevdirection == 2 ))) { // Going Up request to turn Down OK
      jumptrigger = true;
    }
    screenPressed = true;
  }
  // Doesn't allow holding the screen / you must tap it
  else if ( !myTouch.dataAvailable() && screenPressed) {
    screenPressed = false;
  }







  // Burning Oil Barrels on ground level Platform
  myGLCD.drawBitmap (10, 267, 24, 36, Oil1); //   First Oil Barrel

  delay(dly);

  // Burning Oil Barrels on ground level Platform
  myGLCD.drawBitmap (10, 267, 24, 36, Oil2); //   Second Oil Barrel

  //Monkey graphic

  if (monkeygraphic == Barreldelay) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkey1); //   Monkey1 graphic
  }
  if (monkeygraphic == Barreldelay * 2) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkey2); //   Monkey2 graphic
  }
  if (monkeygraphic == Barreldelay * 3) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkey3); //   Monkey3 graphic
  }
  if (monkeygraphic == Barreldelay * 4) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkeyleft); //   Monkey4 graphic
  }

  if (monkeygraphic == Barreldelay * 5) {
    
      // trigger off the next Barrel
      if ((triggerbarrel1 == false) && (b1X == 42)&&(b1Y == 74)){
        triggerbarrel1 = true;
      }  else
      if ((triggerbarrel2 == false) && (b2X == 42)&&(b2Y == 74)){
        triggerbarrel2 = true;
      } else
      if ((triggerbarrel3 == false) && (b3X == 42)&&(b3Y == 74)){
        triggerbarrel3 = true;
      } else
        if ((triggerbarrel4 == false) && (b4X == 42)&&(b4Y == 74)){
        triggerbarrel4 = true;
      }
      
    if ((triggerbarrel1 == false) || (triggerbarrel2 == false) || (triggerbarrel3 == false) || (triggerbarrel4 == false))  { // Only display Monkey with Barrel if less than 3 barrels in play
      myGLCD.drawBitmap (27, 53, 45, 32, MonkeyBarrel); //   MonkeyBarrel graphic
    }
    // reset the Monkey Graphic to start sequence
    monkeygraphic = 0;
  }
  monkeygraphic++; // increment counter
}


// ************************************************************************************************************
// ===== slope down calculator
// ************************************************************************************************************
// Calculates the amount of barrel or mario drop per the "X" location of the barrel or mario
int slpdwn(int x, int y) {

  if (x == 34) {
    y++;
    return y;
  } else if (x == 68) {
    y++;
    return y;
  } else if (x == 102) {
    y++;
    return y;
  } else if (x == 136) {
    y++;
    return y;
  } else if (x == 170) {
    y++;
    return y;
  } else if (x == 204) {
    y++;
    return y;
  } else if (x == 238) {
    y++;
    return y;
  } else if (x == 272) {
    y++;
    return y;
  } else if (x == 306) {
    y++;
    return y;
  } else if (x == 340) {
    y++;
    return y;
  } else if (x == 374) {
    y++;
    return y;
  } else if (x == 408) {
    y++;
    return y;
  } else if (x == 442) {
    y++;
    return y;
  } else
    return y;
}

// ************************************************************************************************************
// ===== slope up calculator
// ************************************************************************************************************
// Calculates the amount of barrel or mario climb per the "X" location of the barrel or mario
int slpup(int x, int y) {

  if (x == 34) {
    y--;
    return y;
  } else if (x == 68) {
    y--;
    return y;
  } else if (x == 102) {
    y--;
    return y;
  } else if (x == 136) {
    y--;
    return y;
  } else if (x == 170) {
    y--;
    return y;
  } else if (x == 204) {
    y--;
    return y;
  } else if (x == 238) {
    y--;
    return y;
  } else if (x == 272) {
    y--;
    return y;
  } else if (x == 306) {
    y--;
    return y;
  } else if (x == 340) {
    y--;
    return y;
  } else if (x == 374) {
    y--;
    return y;
  } else if (x == 408) {
    y--;
    return y;
  } else if (x == 442) {
    y--;
    return y;
  } else
    return y;
}



// ************************************************************************************************************
// ===== Update Digital Clock
// ************************************************************************************************************
void UpdateDisp() {

  // Clear the time area
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(0, 0, 0);
  //  myGLCD.fillRect(60, 80 ,262, 166);


  int h; // Hour value in 24 hour format
  int e; // Minute value in minute format
  int pm = 0; // Flag to detrmine if PM or AM

  // There are four digits that need to be drawn independently to ensure consisitent positioning of time
  int d1;  // Tens hour digit
  int d2;  // Ones hour digit
  int d3;  // Tens minute digit
  int d4;  // Ones minute digit


  h = hour(); // 24 hour RT clock value
  e = minute();

  /* TEST
    h = 12;
    e = 8;
  */


  // Calculate hour digit values for time

  if ((h >= 10) && (h <= 12)) {     // AM hours 10,11,12
    d1 = 1; // calculate Tens hour digit
    d2 = h - 10;  // calculate Ones hour digit 0,1,2
  } else if ( (h >= 22) && (h <= 24)) {   // PM hours 10,11,12
    d1 = 1; // calculate Tens hour digit
    d2 = h - 22;  // calculate Ones hour digit 0,1,2
  } else if ((h <= 9) && (h >= 1)) {  // AM hours below ten
    d1 = 0; // calculate Tens hour digit
    d2 = h;  // calculate Ones hour digit 0,1,2
  } else if ( (h >= 13) && (h <= 21)) { // PM hours below 10
    d1 = 0; // calculate Tens hour digit
    d2 = h - 12;  // calculate Ones hour digit 0,1,2
  } else {
    // If hour is 0
    d1 = 1; // calculate Tens hour digit
    d2 = 2;  // calculate Ones hour digit 0,1,2
  }


  // Calculate minute digit values for time

  if ((e >= 10)) {
    d3 = e / 10 ; // calculate Tens minute digit 1,2,3,4,5
    d4 = e - (d3 * 10); // calculate Ones minute digit 0,1,2
  } else {
    // e is less than 10
    d3 = 0;
    d4 = e;
  }


  if (h >= 12) { // Set
    //  h = h-12; // Work out value
    pm = 1;  // Set PM flag
  }

  // *************************************************************************
  // Print each digit if it has changed to reduce screen impact/flicker

  // Set digit font color to white
  myGLCD.setColor(255, 255, 255);


  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SevenSeg_XXXL_Num);

  // First Digit
  if (((d1 != c1) || (xsetup == true)) && (d1 != 0)) { // Do not print zero in first digit position
    myGLCD.printNumI(d1, 154, 0); // Printing thisnumber impacts LFH walls so redraw impacted area

    // ---------------- Clear lines on Outside wall
    //    myGLCD.setColor(0,0,0);
    //    myGLCD.drawRoundRect(1, 238, 318, 1);

  }

  //If prevous time 12:59 or 00:59 and change in time then blank First Digit

  if ((c1 == 1) && (c2 == 2) && (c3 == 5) && (c4 == 9) && (d2 != c2) ) { // Clear the previouis First Digit and redraw wall

    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(54, 0, 118, 100);


  }

  if ((c1 == 0) && (c2 == 0) && (c3 == 5) && (c4 == 9) && (d2 != c2) ) { // Clear the previouis First Digit and redraw wall

    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(54, 0, 118, 100);
  }
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SevenSeg_XXXL_Num);

  // Second Digit
  if ((d2 != c2) || (xsetup == true)) {
    myGLCD.printNumI(d2, 221, 0); // Print 0
  }

  // Third Digit
  if ((d3 != c3) || (xsetup == true)) {
    myGLCD.printNumI(d3, 288, 0); // Was 145
  }

  // Fourth Digit
  if ((d4 != c4) || (xsetup == true)) {
    myGLCD.printNumI(d4, 355, 0); // Was 205
  }

  if (xsetup == true) {
    xsetup = false; // Reset Flag now leaving setup mode
  }
  // Print PM or AM

  //    myGLCD.setColor(1, 73, 240);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  if (pm == 0) {
    myGLCD.print("am", 440, 22);
  } else {
    myGLCD.print("pm", 440, 22);
  }
  myGLCD.drawBitmap (306, 300, 34, 17, DK_scaffold); //   Scaffold
  // ----------- Alarm Set on LHS lower pillar
  if (alarmstatus == true) { // Print AS on fron screenleft hand side
    myGLCD.print("as", 0, 135);
  }


  // Round dots
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(0, 0, 0);

  myGLCD.fillRect(283, 26, 288, 35);
  myGLCD.fillRect(283, 65, 288, 74);

  // Cover any blanks caused by digits

  //Fifth   Level
  myGLCD.drawBitmap (0, 86, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (34, 86, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 86, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (102, 86, 34, 17, DK_scaffold); //   Scaffold

  //Sixth   Level
  myGLCD.drawBitmap (0, 38, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (34, 38, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 38, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (102, 38, 34, 17, DK_scaffold); //   Scaffold

  //Redraw Monkey
  if (monkeygraphic <= Barreldelay) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkey1); //   Monkey1 graphic
  } else if (monkeygraphic <= Barreldelay * 2) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkey2); //   Monkey2 graphic
  } else if (monkeygraphic <= Barreldelay * 3) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkey3); //   Monkey3 graphic
  } else if (monkeygraphic <= Barreldelay * 4) {
    myGLCD.drawBitmap (27, 53, 45, 32, Monkeyleft); //   Monkey3 graphic
  } else if (monkeygraphic < Barreldelay * 5) {
    myGLCD.drawBitmap (27, 53, 45, 32, MonkeyBarrel); //   Monkey3 graphic
    monkeygraphic = 0;
  }


  // Four Barrels on Platform
  myGLCD.drawBitmap (0, 69, 10, 16, Barrel1); //   First Barrel
  myGLCD.drawBitmap (10, 69, 10, 16, Barrel1); //   First Barrel
  myGLCD.drawBitmap (0, 53, 10, 16, Barrel1); //   First Barrel
  myGLCD.drawBitmap (10, 53, 10, 16, Barrel1); //   First Barrel

  // Girl on top level Platform
  myGLCD.drawBitmap (20, 7, 22, 31, Girl1); //   Damsel

  // Level 5 Ladders
  myGLCD.drawBitmap (116, 0, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 20, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 40, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 60, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 68, 15, 22, Ladder1); //   Ladders
  //myGLCD.drawBitmap (116, 60, 15, 22, Ladder1); //   Ladders

  // Level 6 Ladders
  myGLCD.drawBitmap (50, 0, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (50, 12, 15, 22, Ladder1); //   Ladders

  //--------------------- copy exising time digits to global variables so that these can be used to test which digits change in future

  c1 = d1;
  c2 = d2;
  c3 = d3;
  c4 = d4;

}




// ===== initiateGame - Custom Function
void drawscreen() {


  // Setup Clock Background
  //Draw Background lines

  myGLCD.setColor(1, 73, 240);

  // ---------------- Outside wall
  //        myGLCD.drawRoundRect(0, 239, 319, 0);
  //        myGLCD.drawRoundRect(2, 237, 317, 2);

  //First Level
  myGLCD.drawBitmap (0, 303, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (34, 303, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 303, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (102, 303, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (136, 303, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (170, 303, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (204, 303, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (238, 302, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (272, 301, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (306, 300, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (340, 299, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (374, 298, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (408, 297, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (442, 296, 34, 17, DK_scaffold); //   Scaffold

  //Second   Level
  myGLCD.drawBitmap (0, 246, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (34, 247, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 248, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (102, 249, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (136, 250, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (170, 251, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (204, 252, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (238, 253, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (272, 254, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (306, 255, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (340, 256, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (374, 257, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (408, 258, 34, 17, DK_scaffold); //   Scaffold


  //Third   Level
  // myGLCD.drawBitmap (0, 191, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (34, 206, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 205, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (102, 204, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (136, 203, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (170, 202, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (204, 201, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (238, 200, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (272, 199, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (306, 198, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (340, 197, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (374, 196, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (408, 195, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (442, 194, 34, 17, DK_scaffold); //   Scaffold


  //Fourth   Level
  myGLCD.drawBitmap (0, 138, 34, 17, DK_scaffold); //   Scaffold modified to accomodate ladder
  myGLCD.drawBitmap (34, 139, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 140, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (102, 141, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (170, 143, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (204, 144, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (238, 145, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (272, 146, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (306, 147, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (340, 148, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (374, 149, 34, 17, DK_scaffold); //   Scaffold

  myGLCD.drawBitmap (408, 150, 34, 17, DK_scaffold); //   Scaffold

  //Fifth   Level
  myGLCD.drawBitmap (0, 86, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (34, 86, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 86, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (102, 86, 34, 17, DK_scaffold); //   Scaffold


  //Sixth   Level
  myGLCD.drawBitmap (0, 38, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (34, 38, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (68, 38, 34, 17, DK_scaffold); //   Scaffold
  myGLCD.drawBitmap (102, 38, 34, 17, DK_scaffold); //   Scaffold

  //Monkey
  myGLCD.drawBitmap (27, 53, 45, 32, Monkey1); //   Monkey1 graphic

  // Four Barrels on Platform
  myGLCD.drawBitmap (0, 69, 10, 16, Barrel1); //   First Barrel
  myGLCD.drawBitmap (10, 69, 10, 16, Barrel1); //   First Barrel
  myGLCD.drawBitmap (0, 53, 10, 16, Barrel1); //   First Barrel
  myGLCD.drawBitmap (10, 53, 10, 16, Barrel1); //   First Barrel

  // Burning Oil Barrels on ground level Platform
  myGLCD.drawBitmap (10, 267, 24, 36, Oil1); //   First Oil Barrel
  //myGLCD.drawBitmap (10, 267, 24, 36, Oil2); //   Second Oil Barrel

  // Girl on top level Platform
  myGLCD.drawBitmap (20, 6, 22, 31, Girl1); //   Damsel

  // Level 6 Ladders
  myGLCD.drawBitmap (50, 0, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (50, 20, 15, 22, Ladder1); //   Ladders

  // Level 5 Ladders
  myGLCD.drawBitmap (116, 0, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 20, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 40, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 60, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (116, 68, 15, 22, Ladder1); //   Ladders
  //myGLCD.drawBitmap (116, 60, 15, 22, Ladder1); //   Ladders

  // Level 4 Ladders
  myGLCD.drawBitmap (15, 99, 15, 22, Ladder1); //   Ladders
  myGLCD.drawBitmap (15, 116, 15, 22, Ladder1); //   Ladders
  //myGLCD.drawBitmap (15, 108, 15, 22, Ladder1); //   Ladders

  // Level 3 Ladders
  myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); //   Middle Ladders
  myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); //   Middle Ladders
  myGLCD.drawBitmap (419, 167, 15, 22, Ladder1); //   RHS Ladders
  myGLCD.drawBitmap (419, 173, 15, 22, Ladder1); //   RHS Ladders

  // Level 2 Ladders
  myGLCD.drawBitmap (287, 214, 15, 22, Ladder1); //   Middle Ladders
  myGLCD.drawBitmap (287, 233, 15, 22, Ladder1); //   Middle Ladders
  myGLCD.drawBitmap (49, 223, 15, 22, Ladder1); //   LHS Ladders

  // Level 1 Ladder
  myGLCD.drawBitmap (419, 275, 15, 22, Ladder1); //   RHS Ladders


}



// **********************************
// ******* Enter Setup Mode *********
// **********************************
// Use up down arrows to change time and alrm settings

void clocksetup() {

  int timehour = hour();
  int timeminute = minute();

  // Read Alarm Set Time from Eeprom

  // read a byte from the current address of the EEPROM
  ahour = EEPROM.read(100);
  alarmhour = (int)ahour;
  if (alarmhour > 24 ) {
    alarmhour = 0;
  }

  amin = EEPROM.read(101);
  alarmminute = (int)amin;
  if (alarmminute > 60 ) {
    alarmminute = 0;
  }


  boolean savetimealarm = false; // If save button pushed save the time and alarm

  // Setup Screen
  myGLCD.clrScr();
  // ---------------- Outside wall

  //      myGLCD.setColor(255, 255, 0);
  //      myGLCD.setBackColor(0, 0, 0);

  //   myGLCD.drawRoundRect(0, 239, 319, 0);
  //   myGLCD.drawRoundRect(2, 237, 317, 2);

  //Reset screenpressed flag
  screenPressed = false;

  // Read in current clock time and Alarm time
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(0, 0, 0);


  // Setup buttons
  myGLCD.setFont(BigFont);

  // Time Set buttons
  myGLCD.print("+  +", 135, 38);
  myGLCD.print("-  -", 135, 82);
  myGLCD.drawRoundRect(132, 35, 152, 55); // time hour +
  myGLCD.drawRoundRect(180, 35, 200, 55); // time minute +

  myGLCD.drawRoundRect(132, 80, 152, 100); // time hour -
  myGLCD.drawRoundRect(180, 80, 200, 100); // time minute -

  // Alarm Set buttons
  myGLCD.print("+  +", 135, 138);
  myGLCD.print("-  -", 135, 182);
  myGLCD.drawRoundRect(132, 135, 152, 155); // alarm hour +
  myGLCD.drawRoundRect(180, 135, 200, 155); // alarm minute +

  myGLCD.drawRoundRect(132, 180, 152, 200);  // alarm hour -
  myGLCD.drawRoundRect(180, 180, 200, 200); // alarm minute -




  myGLCD.print("SAVE", 13, 213);
  myGLCD.print("EXIT", 245, 213);
  myGLCD.drawRoundRect(10, 210, 80, 230);
  myGLCD.drawRoundRect(243, 210, 310, 230);

  // Get your Ghost on
  myGLCD.drawBitmap (5, 100, 24, 24, MarioR1); //   Closed Ghost
  myGLCD.drawBitmap (240, 100, 45, 32, Monkeyleft); //   Closed Ghost
  //    myGLCD.drawBitmap (154, 208, 40, 40, scissors); //   Closed Ghost

  // Begin Loop here

  while (xsetup == true) {


    if (alarmstatus == true) { // flag where false is off and true is on
      myGLCD.print("SET", 220, 160);
    } else {
      myGLCD.print("OFF", 220, 160);
    }
    myGLCD.drawRoundRect(218, 157, 268, 177);

    // Draw Sound Button

    myGLCD.print("TEST", 50, 110);  // Triggers alarm sound
    myGLCD.drawRoundRect(48, 108, 116, 128);

    // Display Current Time

    myGLCD.print("Time", 40, 60);


    //    myGLCD.printNumI(timehour, 130, 60);
    if (timehour >= 10) { // Annoyingly if number less than 10 doesnt print on RHS and misses zero so need to compensate
      myGLCD.printNumI(timehour, 130, 60);   // If >= 10 just print minute
    } else {
      myGLCD.print("0", 130, 60);
      myGLCD.printNumI(timehour, 146, 60);
    }

    myGLCD.print(":", 160, 60);

    if (timeminute >= 10) { // Annoyingly if number less than 10 doesnt print on RHS and misses zero so need to compensate
      myGLCD.printNumI(timeminute, 175, 60);   // If >= 10 just print minute
    } else {
      myGLCD.print("0", 175, 60);
      myGLCD.printNumI(timeminute, 193, 60);
    }


    //Display Current Alarm Setting

    myGLCD.print("Alarm", 40, 160);


    //    myGLCD.printNumI(alarmhour, 130, 160);
    if (alarmhour >= 10) { // Annoyingly if number less than 10 doesnt print on RHS and misses zero so need to compensate
      myGLCD.printNumI(alarmhour, 130, 160);   // If >= 10 just print minute
    } else {
      myGLCD.print("0", 130, 160);
      myGLCD.printNumI(alarmhour, 146, 160);
    }



    myGLCD.print(":", 160, 160);

    if (alarmminute >= 10) { // Annoyingly if number less than 10 doesnt print on RHS and misses zero so need to compensate
      myGLCD.printNumI(alarmminute, 175, 160);   // If >= 10 just print minute
    } else {
      myGLCD.print("0", 175, 160);
      myGLCD.printNumI(alarmminute, 193, 160);
    }

    // Read input to determine if buttons pressed
    myTouch.read();
    if (myTouch.dataAvailable()) {
      xT = myTouch.getX();
      yT = myTouch.getY();

      // Capture input command from user
      if ((xT >= 230) && (xT <= 319) && (yT >= 200) && (yT <= 239)) { // (243, 210, 310, 230)  Exit Button
        xsetup = false; // Exit setupmode
      }

      else if ((xT >= 0) && (xT <= 90) && (yT >= 200) && (yT <= 239)) { // (243, 210, 310, 230)  Save Alarm and Time Button
        savetimealarm = true; // Exit and save time and alarm
        xsetup = false; // Exit setupmode
      }


      else if ((xT >= 130) && (xT <= 154) && (yT >= 32) && (yT <= 57)) { // Time Hour +  (132, 35, 152, 55)
        timehour = timehour + 1; // Increment Hour
        if (timehour == 24) {  // reset hour to 0 hours if 24
          timehour = 0 ;

        }
      }

      else if ((xT >= 130) && (xT <= 154) && (yT >= 78) && (yT <= 102)) { // (132, 80, 152, 100); // time hour -
        timehour = timehour - 1; // Increment Hour
        if (timehour == -1) {  // reset hour to 23 hours if < 0
          timehour = 23 ;

        }
      }

      else if ((xT >= 178) && (xT <= 202) && (yT >= 32) && (yT <= 57)) { // Time Minute +  (180, 35, 200, 55)
        timeminute = timeminute + 1; // Increment Hour
        if (timeminute == 60) {  // reset minute to 0 minutes if 60
          timeminute = 0 ;
        }
      }

      else if ((xT >= 178) && (xT <= 202) && (yT >= 78) && (yT <= 102)) { // (180, 80, 200, 100); // time minute -
        timeminute = timeminute - 1; // Increment Hour
        if (timeminute == -1) {  // reset minute to 0 minutes if 60
          timeminute = 59 ;
        }
      }

      else if ((xT >= 130) && (xT <= 154) && (yT >= 133) && (yT <= 157)) { // (132, 135, 152, 155); // alarm hour +
        alarmhour = alarmhour + 1; // Increment Hour
        if (alarmhour == 24) {  // reset hour to 0 hours if 24
          alarmhour = 0 ;

        }
      }

      else if ((xT >= 130) && (xT <= 154) && (yT >= 178) && (yT <= 202)) { // (132, 180, 152, 200);  // alarm hour -
        alarmhour = alarmhour - 1; // Increment Hour
        if (alarmhour == -1) {  // reset hour to 23 hours if < 0
          alarmhour = 23 ;

        }
      }

      else if ((xT >= 178) && (xT <= 202) && (yT >= 133) && (yT <= 157)) { // (180, 135, 200, 155); // alarm minute +
        alarmminute = alarmminute + 1; // Increment Hour
        if (alarmminute == 60) {  // reset minute to 0 minutes if 60
          alarmminute = 0 ;
        }
      }

      else if ((xT >= 178) && (xT <= 202) && (yT >= 178) && (yT <= 202)) { // (180, 180, 200, 200); // alarm minute -
        alarmminute = alarmminute - 1; // Increment Hour
        if (alarmminute == -1) {  // reset minute to 0 minutes if 60
          alarmminute = 59 ;
        }
      }

      else if ((xT >= 216) && (xT <= 270) && (yT >= 155) && (yT <= 179)) { // (218, 157, 268, 177); // alarm set button pushed
        if (alarmstatus == true) {
          alarmstatus = false; // Turn off Alarm
        } else {
          alarmstatus = true; // Turn on Alarm
        }
      }
      else if ((xT >= 46) && (xT <= 118) && (yT >= 106) && (yT <= 130)) { // ((48, 108, 116, 128); // alarm test button pushed
        // Set off alarm by toggling D8, recorded sound triggered by LOW to HIGH transition
        digitalWrite(8, HIGH); // Set high
        digitalWrite(8, LOW); // Set low
      }

      // Should mean changes should scroll if held down
      delay(250);
    }

  }




  if ( savetimealarm == true) {
    // The following codes transmits the data to the RTC
    Wire.beginTransmission(DS1307);
    Wire.write(byte(0));
    Wire.write(decToBcd(0));
    Wire.write(decToBcd(timeminute));
    Wire.write(decToBcd(timehour));
    Wire.write(decToBcd(0));
    Wire.write(decToBcd(0));
    Wire.write(decToBcd(0));
    Wire.write(decToBcd(0));
    Wire.write(byte(0));
    Wire.endTransmission();
    // Ends transmission of data

    // Write the Alarm Time to EEPROM so it can be stored when powered off

    //alarmhour = (int)ahour;
    ahour = (byte)alarmhour;
    amin = (byte)alarmminute;
    EEPROM.write(100, ahour);
    EEPROM.write(101, amin);

    // Now time and alarm data saved reset flag
    savetimealarm = false;
  }


  //* Clear Screen
  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.fillRect(0, 239, 319, 0);
  xsetup = true; // Set Flag now leaving setup mode in order to draw Clock Digits
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  setSyncInterval(60); // sync the time every 60 seconds (1 minutes)
  drawscreen(); // Initiate the screen
  UpdateDisp(); // update value to clock

}



// **********************************************************************************************
// Draw Barrel
// **********************************************************************************************
/*
  int b1X = 30;
  int b1Y = 79;
  int b1image = 1; // Used to determine which image should be used
  int b1direction = 0; // Where Down = 0, Right = 1, Left = 2
  int prevb1direction = 0; // Where Down = 0, Right = 1, Left = 2
  extern unsigned int Barrelroll1[0x78]; // 12x10
  extern unsigned int Barrelroll2[0x78]; // 12x10
  extern unsigned int Barrelroll3[0x78]; // 12x10
  extern unsigned int Barrelroll4[0x78]; // 12x10
  extern unsigned int Barrelh1[0x96]; // 15x10
  extern unsigned int Barrel1h2[0x96]; // 15x10
  boolean triggerbarrel3 = false; // switch used to trigger release of a barrel
  if direction is down then use Barrelh1 and Barrellh2
*/

void drawbarrel(int x, int y, int image, int cdir, int prevdir) { // Make provision for 3 barrels

  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(0, 0, 0);

  // firstly based on previous direction blank out 2 pixels worth of screen
  // Where Down = 0, Right = 1, Left = 2

  if (prevdir == 0) { // Down  *************************************************************************************************
    myGLCD.fillRect(x, y, x + 22, y - 3); // Clear trail off graphic before printing new position  22x15


    // Cleanup Scaffold and Ladder Damage from downward activties

    // Monkey Drops Barrel

    if (x == 42 && y == 76) {
      myGLCD.drawBitmap (27, 53, 45, 32, Monkey1); //   Monkey1 graphic
    }
    if (x == 42  && y == 104) {
      myGLCD.drawBitmap (34, 86, 34, 17, DK_scaffold); //   Scaffold below monkey feet
    }

    // Level 3 middle Ladder
    if (x == 286 && y == 238) {
      myGLCD.drawBitmap (287, 214, 15, 22, Ladder1); // Top of ladder
      myGLCD.drawBitmap (272, 199, 34, 17, DK_scaffold); //   Scaffold
      myGLCD.drawBitmap (306, 198, 34, 17, DK_scaffold); //   Scaffold
    }
    if (x == 150 && y == 187) {
      myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); // Top L3 ladder  Scaffold
      myGLCD.drawBitmap (170, 143, 34, 17, DK_scaffold); // Top L3 ladder  Scaffold
      myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); //   Top of L3 Middle Ladder
    }
    if (x == 138 && y == 192) {
      myGLCD.drawBitmap (137, 160, 15, 22, Ladder1); //   Top of L3 Middle Ladder
      myGLCD.drawBitmap (137, 180, 15, 22, Ladder1); //   Bottom of L3 Middle Ladder
    }
    if (x == 138 && y <= 197) {
      myGLCD.drawBitmap (137, 180, 15, 22, Ladder1); //   Bottom of L3 Middle Ladder
    }

    // Level 2 middle Ladder
    if (x == 132 && y == 173) {
      myGLCD.drawBitmap (114, 164, 34, 17, DK_scaffold); //   Scaffold
      myGLCD.drawBitmap (137, 163, 34, 17, DK_scaffold); //   Scaffold
    }
    if (x == 132 && y == 183) {
      myGLCD.drawBitmap (134, 170, 15, 22, Ladder1); //   Top of L2 Middle Ladder
    }

    if (x == 418 && y == 179) {
      myGLCD.drawBitmap (408, 150, 34, 17, DK_scaffold);  //   Scaffold above L4 RHS Ladder
    }
    if (x == 418 && y == 280) {
      myGLCD.drawBitmap (408, 258, 34, 17, DK_scaffold);  //   Scaffold above L1 RHS Ladder
    }

    if (x == 48 && y == 231) {
      myGLCD.drawBitmap (34, 206, 34, 17, DK_scaffold);  //   Scaffold above L2 LHS Ladder
      myGLCD.drawBitmap (68, 205, 34, 17, DK_scaffold);  //   Scaffold above L2 LHS Ladder
    }

  } else if (prevdir == 1) { // Right  *************************************************************************************************
    myGLCD.fillRect(x + 2, y - 2, x - 3, y + 15); // Clear trail off graphic before printing new position 18X15

    if (x == 306 && y == 239) {
      myGLCD.drawBitmap (287, 233, 15, 22, Ladder1); //   Bottom of L2 Middle Ladder
    } else

      if (x == 70 && y == 232) {
        myGLCD.drawBitmap (49, 223, 15, 22, Ladder1); //   LHS Ladder Level 2
      }


  } else if (prevdir == 2) { // Left *************************************************************************************************
    myGLCD.fillRect(x + 14, y - 2, x + 23, y + 15); // Clear trail off graphic before printing new position 18X15



    if (x == 128 && y == 187) {
      myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); //   Bottom of L3 Middle Ladder
    }
    if (x == 396 && y == 179) {
      myGLCD.drawBitmap (419, 167, 15, 22, Ladder1); //   L3 RHS Ladder
      myGLCD.drawBitmap (419, 173, 15, 22, Ladder1); //   L3 RHS Ladder
    }

    if (x == 396 && y == 280) {
      myGLCD.drawBitmap (419, 275, 15, 22, Ladder1); //   L1 RHS Ladder
    }
  }


  // Special Circumstances where barrel does Down/Left turn

  if ((cdir == 2) && (prevdir == 0)) {

    if (x == 298 && y == 212) {
      myGLCD.fillRect(x + 19, y - 1, x + 22, y + 15); // Clear trail off graphic before printing new position 18X15
    } else if (x == 148 && y == 151) {
      myGLCD.fillRect(x + 19, y - 1, x + 22, y + 15); // Clear trail off graphic before printing new position 18X15
    } else if (x == 298 && y == 145) {
      myGLCD.fillRect(x + 19, y - 1, x + 22, y + 15); // Clear trail off graphic before printing new position 18X15
    }  else if (x == 278 && y == 215) {
      myGLCD.fillRect(x + 19, y - 1, x + 22, y + 15); // Clear trail off graphic before printing new position 18X15
    }  else if (x == 278 && y == 146) {
      myGLCD.fillRect(x + 19, y - 1, x + 22, y + 15); // Clear trail off graphic before printing new position 18X15
    }
  }


  /*
    // Special Circumstance where Barrel Y value drops by 1 leaving a trail

    myGLCD.setColor(200, 0, 0);

      if (prevb1X != x ){ // Ifin transition wipe the trail

          if (cdir == 2) { // If travelling left
              myGLCD.fillRect(x-2, y-2, x+14, y); // Clear trail off graphic before printing new position 12x10
          } else
          if (cdir == 1) { // If travelling right
              myGLCD.fillRect(x, y+2, x+14, y); // Clear trail off graphic before printing new position 12x10
          }
      }
  */

  // Now display Barrel at X,Y coordinates using the correct graphic image

  if (cdir != 0) { // only display barrelroll image if travelling left or right
    if (image == 1) {
      myGLCD.drawBitmap(x, y, 18, 15, Barrelr1); //   Rolling Barrel image 1

    } else if (image == 2) {
      myGLCD.drawBitmap(x, y, 18, 15, Barrelr2); //   Rolling Barrel image 2
    } else if (image == 3) {
      myGLCD.drawBitmap(x, y, 18, 15, Barrelr3); //   Rolling Barrel image 3
    } else if (image == 4) {
      myGLCD.drawBitmap(x, y, 18, 15, Barrelr4); //   Rolling Barrel image 4
    }
  } else { // Display horizontal barrel images

    if ((image == 1) || (image == 3)) {
      myGLCD.drawBitmap(x, y, 22, 15, Barrelh1); //  H Rolling Barrel image 1
    } else if ((image == 2) || (image == 4)) {
      myGLCD.drawBitmap(x, y, 22, 15, Barrelh2); //  H Rolling Barrel image 2

    }
  }
}


// **********************************************************************************************
// Draw Mario
// **********************************************************************************************
/*
  int mX = 30;
  int mY = 230;
  int mimage = 1; // Used to determine which image should be used
  int mdirection = 0; // Where Down = 0, Right = 1, Left = 2, Up = 3, Jump Right = 4, Jump Left = 6
  int mprevdirection = 0; // Where Down = 0, Right = 1, Left = 2
  boolean mtrigger = false; // switch used to trigger release of a mario

  extern unsigned int MarioL1[0x100]; // 16x16
  extern unsigned int MarioL2[0x100]; // 16x16
  extern unsigned int MarioL3[0x100]; // 16x16 Jump Left
  extern unsigned int MarioR1[0x100]; // 16x16
  extern unsigned int MarioR2[0x100]; // 16x16
  extern unsigned int MarioR3[0x100]; // 16x16 Jump Right
  extern unsigned int MarioU1[0x100]; // 16x16 Climb Ladder 1
  extern unsigned int MarioU2[0x100]; // 16x16 Climb Ladder 2
  extern unsigned int MarioU3[0x100]; // 16x16 Climb Ladder 3
  extern unsigned int MarioU4[0x100]; // 16x16 Climeb Ladder 4
  extern unsigned int MarioStop[0x100]; // 16x16 Mario Stand Still


  boolean mtrigger = false; // switch used to trigger release of a Mario direction is




*/

void drawMario(int x, int y, int image, int cdir, int prevdir) { // Draw Mario function

  myGLCD.setColor(0, 0, 0);
  myGLCD.setBackColor(0, 0, 0);

  /* **************************************************************************************************************************
    Firstly based on previous direction blank out 2 pixels worth of screen where image is 16 x 16
  *****************************************************************************************************************************
    Mario Movements
    1 - Left as expected left of touch screen
    2 - Right as expected left & right of touch screen
    3 - Jump straight Up = Top of touch screen
    4 - Right Jump = Top RHS of touch screem
    5 - Left Jump = Top LHS of touch screen
    6 - Climb Down = When at top of ladder Bottom of touch screen
    7 - Climb Up - Whn at bottom of a ladder touch bottom of screen
    8 - Stop = Bottom of touch screen
  */
  if (prevdir == 2) { // Left *************************************************************************************************
    myGLCD.fillRect(x + 11, y - 1, x + 26, y + 24); // Clear trail off graphic before printing new position 24x24

    if (x == 394 && y == 231) { // Top Left Ladder 6
      myGLCD.drawBitmap (419, 275, 15, 22, Ladder1); //    Ladder 6
      myGLCD.drawBitmap (408, 258, 34, 17, DK_scaffold); //   Scaffold above RHS ladder 6
      myGLCD.drawBitmap (408, 297, 34, 17, DK_scaffold); //   Scaffold below RHS ladder 6
    }
    if (x == 388 && y == 274) { // Bottom Left Ladder 6
      myGLCD.drawBitmap (419, 275, 15, 22, Ladder1); //    Ladder 6
      myGLCD.drawBitmap (408, 258, 34, 17, DK_scaffold); //   Scaffold above RHS ladder 6
      myGLCD.drawBitmap (408, 297, 34, 17, DK_scaffold); //   Scaffold below RHS ladder 6
    }

    if (x == 260 && y == 227) { // Bottom Left Ladder 5 
      myGLCD.drawBitmap (287, 233, 15, 22, Ladder1); // ladder 5 bottom rungs
      myGLCD.drawBitmap (287, 214, 15, 22, Ladder1); // ladder 5 top rungs
      myGLCD.drawBitmap (272, 199, 34, 17, DK_scaffold); //Scaffold above ladder 5
    }

    if (x == 256 && y == 174) { // Top Left Ladder 5 
      myGLCD.drawBitmap (287, 233, 15, 22, Ladder1); // ladder 5 bottom rungs
      myGLCD.drawBitmap (287, 214, 15, 22, Ladder1); // ladder 5 top rungs
      myGLCD.drawBitmap (272, 199, 34, 17, DK_scaffold); //Scaffold above ladder 5
      myGLCD.drawBitmap (306, 198, 34, 17, DK_scaffold); //Scaffold above and right ladder 5
    }

    if (x == 64 && y == 222 ) {
      myGLCD.drawBitmap (34, 206, 34, 17, DK_scaffold); //   Scaffold above ladder 4
      myGLCD.drawBitmap (49, 223, 15, 22, Ladder1); //   Ladder 4
      myGLCD.drawBitmap (34, 247, 34, 17, DK_scaffold); //   Scaffold belowladder 4
    }

    if (x == 18 && y == 221 ) {
      myGLCD.drawBitmap (34, 206, 34, 17, DK_scaffold); //   Scaffold above ladder 4
      myGLCD.drawBitmap (49, 223, 15, 22, Ladder1); //   Ladder 4
      myGLCD.drawBitmap (68, 205, 34, 17, DK_scaffold); //   Scaffold above right of ladder 4
    }
   if (x == 386 && y == 123) { // Top Left of Ladder 3
      myGLCD.drawBitmap (408, 150, 34, 17, DK_scaffold); //   Scaffold above ladder 3
      myGLCD.drawBitmap (419, 167, 15, 22, Ladder1); // ladder 3 Top rung
      myGLCD.drawBitmap (419, 173, 15, 22, Ladder1); // ladder 3 bottom rungs
      myGLCD.drawBitmap (408, 195, 34, 17, DK_scaffold); //   Scaffold below ladder 3
    }

   if (x == 392 && y == 170) { // Bottom Left of Ladder 3
      myGLCD.drawBitmap (408, 150, 34, 17, DK_scaffold); //   Scaffold above ladder 3
      myGLCD.drawBitmap (419, 167, 15, 22, Ladder1); // ladder 3 Top rung
      myGLCD.drawBitmap (419, 173, 15, 22, Ladder1); // ladder 3 bottom rungs
      myGLCD.drawBitmap (408, 195, 34, 17, DK_scaffold); //   Scaffold below ladder 3
    }
    if (x == 122 && y == 115) { // Top Left of ladder 2
      myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); //   Scaffold above ladder 2
      myGLCD.drawBitmap (170, 143, 34, 17, DK_scaffold); //   Scaffold above right ladder 2
      myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); // ladder 2 top rungs
      myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); // ladder 2 bottom rungs
    }
    if (x == 126 && y == 178) { // Bottom Left of ladder 2
      myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); //   Scaffold above ladder 2
      myGLCD.drawBitmap (170, 143, 34, 17, DK_scaffold); //   Scaffold above right ladder 2
      myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); // ladder 2 top rungs
      myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); // ladder 2 bottom rungs
    }
    if (x == 134 && y == 146) {// test
      myGLCD.drawBitmap (150, 148, 15, 22, Ladder1); // ladder 4 lower
      myGLCD.drawBitmap (150, 136, 15, 22, Ladder1); // ladder 4 uper
    }



    if (x == 278 && y == 117) {
      myGLCD.drawBitmap (275, 134, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
    }
    if (x == 274 && y == 139) {
      myGLCD.drawBitmap (275, 134, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
    }
    if (x == 260 && y == 140) {
      myGLCD.drawBitmap (275, 134, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
    }
    if (x == 202 && y == 143) {
      myGLCD.drawBitmap (252, 133, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
      myGLCD.drawBitmap (275, 134, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
    }
    if (x == 240 && y == 141) {
      myGLCD.drawBitmap (252, 133, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
    }

    if (x == 258 && y == 140) {
      myGLCD.drawBitmap (280, 141, 15, 22, Ladder1); // ladder 3
    }

    if (x == 4 && y == 104) {
      myGLCD.drawBitmap (15, 96, 15, 22, Ladder1); // ladder 1 middle
      myGLCD.drawBitmap (15, 108, 15, 22, Ladder1); // ladder 1 bottom
    }
    if (x == 264 && y == 116) {
      myGLCD.drawBitmap (252, 133, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
    }

    // Clear at top of ladder 2
    if (x == 144 && y == 111) {
      myGLCD.fillRect(x, y, x + 24, y + 3); // Clear trail off graphic before printing new position  24x24
    }

    // Clear at top of ladder 5
    if (x == 130 && y == 146) {
      myGLCD.fillRect(x, y, x + 24, y + 3); // Clear trail off graphic before printing new position  24x24
    }


  } else if (prevdir == 1) { // Right  *************************************************************************************************
    myGLCD.fillRect(x + 3, y - 1, x - 2, y + 24); // Clear trail off graphic before printing new position 24x24

    if (x == 442 && y == 273) {
      myGLCD.drawBitmap (408, 258, 34, 17, DK_scaffold); //Scaffold above ladder 6
      myGLCD.drawBitmap (419, 275, 15, 22, Ladder1); //   Ladder 6
    }
    if (x == 306 && y == 230) { // Bottom right ladder 5 
      myGLCD.drawBitmap (287, 233, 15, 22, Ladder1); // ladder 5 bottom rungs
      myGLCD.drawBitmap (287, 214, 15, 22, Ladder1); // ladder 5 top rungs
      myGLCD.drawBitmap (272, 199, 34, 17, DK_scaffold); //Scaffold above ladder 5
      myGLCD.drawBitmap (306, 198, 34, 17, DK_scaffold); //Scaffold above right ladder 5
      myGLCD.drawBitmap (272, 254, 34, 17, DK_scaffold); //Scaffold bottom ladder 5
    }
    if (x == 308 && y == 172) { // Top right Ladder 5 
      myGLCD.drawBitmap (287, 233, 15, 22, Ladder1); // ladder 5 bottom rungs
      myGLCD.drawBitmap (287, 214, 15, 22, Ladder1); // ladder 5 top rungs
      myGLCD.drawBitmap (272, 199, 34, 17, DK_scaffold); //Scaffold above ladder 5
      myGLCD.drawBitmap (306, 198, 34, 17, DK_scaffold); //Scaffold above right ladder 5
      myGLCD.drawBitmap (272, 254, 34, 17, DK_scaffold); //Scaffold bottom ladder 5
    }
    if (x == 66 && y == 222 ) { // Bottom rigt of ladder 4
      myGLCD.drawBitmap (34, 206, 34, 17, DK_scaffold); //   Scaffold above ladder 4
      myGLCD.drawBitmap (68, 205, 34, 17, DK_scaffold); //   Scaffold above and right ladder 4
      myGLCD.drawBitmap (49, 223, 15, 22, Ladder1); //   Ladder 4
      myGLCD.drawBitmap (34, 247, 34, 17, DK_scaffold); //   Scaffold belowladder 4
    }
    if (x == 68 && y == 179 ) { // Top rigt of ladder 4
      myGLCD.drawBitmap (34, 206, 34, 17, DK_scaffold); //   Scaffold above ladder 4
      myGLCD.drawBitmap (68, 205, 34, 17, DK_scaffold); //   Scaffold above and right ladder 4
      myGLCD.drawBitmap (49, 223, 15, 22, Ladder1); //   Ladder 4
      myGLCD.drawBitmap (34, 247, 34, 17, DK_scaffold); //   Scaffold belowladder 4
    }

    if (x == 436 && y == 169 ) { // Bottom right Ladder 3 
      myGLCD.drawBitmap (419, 167, 15, 22, Ladder1); // ladder 3
    }

    if (x == 172 && y == 117) { // Top right of ladder 2
      myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); //   Ladder 2 top  Scaffold
      myGLCD.drawBitmap (170, 143, 34, 17, DK_scaffold); //   Ladder 2 top  Scaffold
      myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); //  Top ladder 2
      myGLCD.drawBitmap (136, 203, 34, 17, DK_scaffold); //   Ladder 2 bottom  Scaffold
      myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); //  bottom ladder 2
          }
    if (x == 168 && y == 177) { // Bottom right of ladder 2
      myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); //   Ladder 2 top  Scaffold
      myGLCD.drawBitmap (170, 143, 34, 17, DK_scaffold); //   Ladder 2 top  Scaffold
      myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); //  Top ladder 2
      myGLCD.drawBitmap (136, 203, 34, 17, DK_scaffold); //   Ladder 2 bottom  Scaffold
      myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); //  bottom ladder 2
          }
    if (x == 38 && y == 112) { // Bottom right Ladder 1
      myGLCD.drawBitmap (15, 99, 15, 22, Ladder1); // ladder 1 top
      myGLCD.drawBitmap (15, 116, 15, 22, Ladder1); // ladder 1 bottom
      myGLCD.drawBitmap (0, 138, 34, 17, DK_scaffold); //   Scaffold left bottom ladder 1
      myGLCD.drawBitmap (22, 139, 34, 17, DK_scaffold); //   Scaffold right bottom ladder 1
    }


    // Trail appears aobve Marios Head when dropping a level right to left like barrels

  } else if (prevdir == 3) { // Jump straight Up then down  *****************************************************************************
    myGLCD.fillRect(x - 1, y + 16, x + 17, y + 18); // When jump trajectory going up use 3 this then When jump trajectory going down use 6 Climb Down
  } else

    if (prevdir == 4) { // Right Jump *************************************************************************************************
      myGLCD.fillRect(x, y - 2, x - 2, y + 16); // Clear trail off graphic before printing new position 16x16
    } else

      if (prevdir == 5) { // Left Jump*************************************************************************************************
        myGLCD.fillRect(x + 15, y - 2, x + 18, y + 16); // Clear trail off graphic before printing new position 16x16
      } else

        if (prevdir == 6) { // Climb Down  ********************************************************************************************
          myGLCD.fillRect(x - 2, y, x + 25, y - 2); // Clear trail off graphic before printing new position  24X24

          if (x == 132 && y == 176) {
            myGLCD.drawBitmap (137, 163, 34, 17, DK_scaffold); //   Ladder 5 top right Scaffold
            myGLCD.drawBitmap (114, 164, 34, 17, DK_scaffold); //   Ladder 5 top left Scaffold
          }

          if (x == 148 && y == 139) {
            myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); //   Ladder 2 top  Scaffold
            myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); // Ladder 2
            myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); // ladder 2
            myGLCD.drawBitmap (136, 203, 34, 17, DK_scaffold); //   Ladder 2 bottom  Scaffold
          }

          if (x == 12 && y == 97) {
            myGLCD.drawBitmap (0, 86, 34, 17, DK_scaffold); //   Ladder 1 top left Scaffold
            myGLCD.drawBitmap (34, 86, 34, 17, DK_scaffold); //   Ladder 1 top right Scaffold

            myGLCD.drawBitmap (15, 84, 15, 22, Ladder1); // ladder 1 top
          }

          if (x == 12 && y == 107) {
            myGLCD.drawBitmap (15, 96, 15, 22, Ladder1); // ladder 1 middle
          }








        } else if (prevdir == 7) { // Climb Up  ********************************************************************************************
          myGLCD.fillRect(x - 2, y + 16, x + 25, y + 27); // Clear trail off graphic before printing new position  24x24

          if (x == 416 && y == 232) {
            myGLCD.drawBitmap (419, 275, 15, 22, Ladder1); //    Ladder 6
            myGLCD.drawBitmap (408, 258, 34, 17, DK_scaffold); //   Scaffold above RHS ladder 6
            myGLCD.drawBitmap (408, 297, 34, 17, DK_scaffold); //   Scaffold below RHS ladder 6
          }

          if (x == 132 && y == 146) {
            myGLCD.drawBitmap (137, 163, 34, 17, DK_scaffold); //   Ladder 5 top right Scaffold
            myGLCD.drawBitmap (114, 164, 34, 17, DK_scaffold); //   Ladder 5 top left Scaffold
            myGLCD.drawBitmap (137, 197, 34, 17, DK_scaffold); //   Ladder 5 top left Scaffold

          }
          if (x == 46 && y == 180 ) {
            myGLCD.drawBitmap (34, 206, 34, 17, DK_scaffold); //   Scaffold above ladder 4
            myGLCD.drawBitmap (49, 223, 15, 22, Ladder1); //   Ladder 4
            myGLCD.drawBitmap (68, 205, 34, 17, DK_scaffold); //   Scaffold above right of ladder 4
          }

          if (x == 278 && y == 123) {
            myGLCD.drawBitmap (280, 141, 15, 22, Ladder1); // ladder 3
          }

          if (x == 278 && y == 117) {
            myGLCD.drawBitmap (275, 134, 34, 17, DK_scaffold); //   Ladder 3 top right Scaffold
          }

          if (x == 278 && y == 135) {
            myGLCD.drawBitmap (275, 157, 34, 17, DK_scaffold); //   Ladder 3 bottom Scaffold
          }

          if (x == 148 && y == 116) {
            myGLCD.drawBitmap (136, 142, 34, 17, DK_scaffold); //   Ladder 2 top  Scaffold
            myGLCD.drawBitmap (170, 143, 34, 17, DK_scaffold); //   Ladder 2 top  Scaffold
            myGLCD.drawBitmap (151, 159, 15, 22, Ladder1); //  Top ladder 2
            myGLCD.drawBitmap (136, 203, 34, 17, DK_scaffold); //   Ladder 2 bottom  Scaffold
            myGLCD.drawBitmap (151, 180, 15, 22, Ladder1); //  bottom ladder 2
          }
          
          if (x == 278 && y == 123) {
            myGLCD.drawBitmap (280, 141, 15, 22, Ladder1); // ladder 3
          }

          if (x == 12 && y == 103) {
            myGLCD.drawBitmap (0, 138, 34, 17, DK_scaffold); //   Ladder 1 bottom Scaffold
          }
          if (x == 12 && y == 95) {
            myGLCD.drawBitmap (15, 116, 15, 22, Ladder1); // ladder 1 bottom
          }
          if (x == 12 && y == 85) {
            myGLCD.drawBitmap (15, 99, 15, 22, Ladder1); // ladder 1 top
            myGLCD.drawBitmap (15, 116, 15, 22, Ladder1); // ladder 1 bottom

          }

        }





  // 8 - If previous direction was stopped then no action so ignore

  // Now display Mario at X,Y coordinates using the correct graphic image
  /*
    Mario Movements
    1 - Right as expected left & right of touch screen
    2 - Left as expected left of touch screen
    3 - Jump straight Up = Top of touch screen
    4 - Right Jump = Top RHS of touch screem
    5 - Left Jump = Top LHS of touch screen
    6 - Climb Down = When at top of ladder Bottom of touch screen
    7 - Climb Up = When at bottom of ladder Top of touch screen
    8 - Stop = Stand upright when touch Bottom of touch screen

    Use Image Number to Toggle the correct image based on Direction

  */

  /*
    Mario Left
    I# cdir   Image
    1    2    MarioL1
    2    2    MarioL2
    3    2    MarioL3
  */
  if (cdir == 2) { // only display image based on current direction
    if (image == 1) {
      myGLCD.drawBitmap(x, y, 24, 24, MarioL1); //   Rolling Mario image 1
    } else if (image == 2) {
      myGLCD.drawBitmap(x, y, 24, 24, MarioL2); //   Rolling Mario image 1
    } else if (image == 3) {
      myGLCD.drawBitmap(x, y, 24, 24, MarioL3); //   Rolling Mario image 1
    }
  } else
    /*
      Mario Right
      I# cdir    Image
      1    1     MarioR1
      2    1     MarioR2
      3    1     MarioR3
    */
    if (cdir == 1) { // Mario Right
      if (image == 1) {
        myGLCD.drawBitmap(x, y, 24, 24, MarioR1); //    Mario image 1
      } else if (image == 2) {
        myGLCD.drawBitmap(x, y, 24, 24, MarioR2); //    Mario image 1
      } else if (image == 3) {
        myGLCD.drawBitmap(x, y, 24, 24, MarioR3); //    Mario image 1
      }
    } else

      if (cdir == 2) { // Mario Left
        if (image == 1) {
          myGLCD.drawBitmap(x, y, 24, 24, MarioL1); //    Mario image 1
        } else if (image == 2) {
          myGLCD.drawBitmap(x, y, 24, 24, MarioL2); //    Mario image 1
        } else if (image == 3) {
          myGLCD.drawBitmap(x, y, 24, 24, MarioL3); //    Mario image 1
        }
      } else

        if (cdir == 3) { // Mario Jump based on previous direction
          if (prevdir == 1) {
            myGLCD.drawBitmap(x, y, 24, 24, MarioL3); //   Face left
          } else {
            myGLCD.drawBitmap(x, y, 24, 24, MarioR3); //   Otherwise face right
          }
        } else if (cdir == 4) { // Right Jump
          myGLCD.drawBitmap(x, y, 24, 24, MarioR3); //   Otherwise face right
        } else if (cdir == 5) { // Left Jump
          myGLCD.drawBitmap(x, y, 24, 24, MarioL3); //   Face left
        } else if (cdir == 6) { // Climb Down
          if (image == 1) {
            myGLCD.drawBitmap(x, y, 24, 18, MarioU4); //    Mario climb left
          } else if (image == 2) {
            myGLCD.drawBitmap(x, y, 24, 24, MarioU3); //    Mario climb right
          } else if (image == 3) {
            myGLCD.drawBitmap(x, y, 24, 24, MarioU2); //    Mario top of ladder  left
          } else if (image == 4) {
            myGLCD.drawBitmap(x, y, 24, 24, MarioU1); //    Mario top of ladder  right
          }
        } else if (cdir == 7) { // Climb Up
          if (image == 1) {
            myGLCD.drawBitmap(x, y, 24, 24, MarioU1); //    Mario climb left
          } else if (image == 2) {
            myGLCD.drawBitmap(x, y, 24, 24, MarioU2); //    Mario climb right
          } else if (image == 3) {
            myGLCD.drawBitmap(x, y, 24, 24, MarioU3); //    Mario top of ladder  left
          } else if (image == 4) {
            myGLCD.drawBitmap(x, y, 24, 18, MarioU4); //    Mario top of ladder  right
          }
        } else if (cdir == 8) { // Stop and stand straignt
          myGLCD.drawBitmap(x, y, 24, 24, MarioStop); //   Face left
        }
}

// ********************************************
// Redraw damage to screen during Mario Jump
void redraw(int x, int y, int d) {

  if (y < 134) { // Level 4

    if (x < 68) {            // Sector 0

      myGLCD.drawBitmap (15, 99, 15, 22, Ladder1);
      myGLCD.drawBitmap (15, 116, 15, 22, Ladder1);

    }

    if (x >= 252) {          // Above Sector 3

      myGLCD.drawBitmap (275, 134, 34, 17, DK_scaffold);

      myGLCD.drawBitmap (280, 141, 15, 22, Ladder1);


    }

  } else if (y < 168 && y > 134) { // Level 3




    if (x >= 68 && x <= 182) { // Sector 1


      myGLCD.drawBitmap (91, 126, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (114, 127, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (137, 128, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (160, 129, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (183, 130, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (206, 131, 34, 17, DK_scaffold);


      myGLCD.drawBitmap (150, 136, 15, 22, Ladder1);
      myGLCD.drawBitmap (150, 148, 15, 22, Ladder1);


    }
    if (x >= 183 && x <= 251) { // Sector 2

      myGLCD.drawBitmap (160, 129, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (183, 130, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (206, 131, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (229, 132, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (252, 133, 34, 17, DK_scaffold);

      myGLCD.drawBitmap (280, 141, 15, 22, Ladder1);


    }
    if (x >= 252) {          // Sector 3

      myGLCD.drawBitmap (229, 132, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (252, 133, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (275, 134, 34, 17, DK_scaffold);

      myGLCD.drawBitmap (280, 141, 15, 22, Ladder1);


    }


  } else if (y <= 205 && y > 170) { // Level 2

    if (x < 45) {             // Sector 4

      myGLCD.drawBitmap (22, 168, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (45, 167, 34, 17, DK_scaffold);

      myGLCD.drawBitmap (32, 176, 15, 22, Ladder1);

    }
    if (x >= 45 && x <= 113) { // Sector 5

      myGLCD.drawBitmap (22, 168, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (32, 176, 15, 22, Ladder1);

      myGLCD.drawBitmap (45, 167, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (68, 166, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (91, 165, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (114, 164, 34, 17, DK_scaffold);
    }
    if (x >= 114 && x <= 183) { // Sector 6

      myGLCD.drawBitmap (91, 165, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (114, 164, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (137, 163, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (160, 162, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (183, 161, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (206, 160, 34, 17, DK_scaffold);


      myGLCD.drawBitmap (134, 170, 15, 22, Ladder1);
      myGLCD.drawBitmap (134, 182, 15, 22, Ladder1);
    }
    if (x >= 184 && x <= 251) { // Sector 7

      myGLCD.drawBitmap (160, 162, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (183, 161, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (206, 160, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (229, 159, 34, 17, DK_scaffold);

    }

  } else if (y > 205) { // Level 1

    if (x < 45) {             // Sector 8

      myGLCD.drawBitmap (22, 192, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (45, 193, 34, 17, DK_scaffold);
    }
    if (x >= 45 && x <= 113) { // Sector 9

      myGLCD.drawBitmap (22, 192, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (45, 193, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (68, 194, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (91, 195, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (114, 196, 34, 17, DK_scaffold);


    }
    if (x >= 114 && x <= 183) { // Sector 10

      myGLCD.drawBitmap (91, 195, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (114, 196, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (137, 197, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (160, 198, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (183, 199, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (206, 200, 34, 17, DK_scaffold);

    }
    if (x >= 184 && x <= 251) { // Sector 11

      myGLCD.drawBitmap (160, 198, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (183, 199, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (206, 200, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (229, 201, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (252, 202, 34, 17, DK_scaffold);

    }
    if (x >= 252) { // Sector 12

      myGLCD.drawBitmap (183, 199, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (206, 200, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (229, 201, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (252, 202, 34, 17, DK_scaffold);
      myGLCD.drawBitmap (275, 203, 34, 17, DK_scaffold);

      myGLCD.drawBitmap (280, 210, 15, 22, Ladder1);
    }

  }

}



// ================= Decimal to BCD converter

byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}
