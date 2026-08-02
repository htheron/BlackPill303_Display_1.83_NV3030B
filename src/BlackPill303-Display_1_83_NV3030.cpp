/* Rotax Engine cylinder Pressure Display
*  LCD Integration with Blackpill 303
*  Processor: STM32F303CCT6
*  Board: Blackpill-F303
*  Display: Waveshare 1.54" SPI TFT LCD 240x240
*  Version: 1.00
 */

#include <Arduino.h>
#include <BlackPill303-Display_1_83_NV3030B.h>
#include <TFT_eSPI.h> // Hardware-specific library Version 2.3.70
#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BMP3XX.h>

TIM_TypeDef *Instance = TIM8;
HardwareTimer *MyTim = new HardwareTimer(Instance);
//Adafruit_BMP3XX BMP388;
//TwoWire I2CDef;

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TFT_eSprite stextP1 = TFT_eSprite(&tft); // Sprite object stextP1 - Secondary Pressure display
TFT_eSprite stextP2 = TFT_eSprite(&tft); // Sprite object stextP2 - Primary Pressure display
//TFT_eSprite sgraphP3 = TFT_eSprite(&tft); // Sprite object sgraphP3 - Graph display

//Variables
uint8_t DisplayHg = 0;   //inHg:1, kPa: 0 Select what pressure unit to display
uint8_t DisplayNumbers = 1; // 7Seg number:1, Graph:0
uint16_t Viewport_Height, Viewport_Width;
uint16_t LCDWidth, LCDHeight;
int16_t CursorX, CursorY;
char s_Dim[32],s_TempI[8],s_TempM[8];
char s_V[5],s_PM[6],s_PI[5];
uint16_t i_Counter=0; 
uint8_t DisplayID;
uint16_t RateCounter=0; 
uint8_t LEDFlag=0, SampleFlag=0;
int16_t ADCValue_P1=0, ADCValue_P2=0;
uint16_t i_y0=0, i_y1=0, i_x0=0, i_x1=0,i_xx1=0,i_yy1=0;
uint16_t P1x0, P1x1, P1x2, P1x3, P1y0, P1y1, P1y2, P1y3;
uint16_t P1x0_old, P1x1_old, P1x2_old, P1x3_old, P1y0_old, P1y1_old, P1y2_old, P1y3_old;
float f_P1 = 0.0,f_Vp = 0.0,f_VA0 = 0.0,f_Phg=0.0, f_P1sx0=0.0, f_P1sy0=0.0,f_P1sx1=0.0, f_P1sy1=0.0;
float f_P1sx2=0.0, f_P1sy2=0.0;
float sdeg=0, mdeg=0, hdeg=0, adeg=0, Dadeg=0, f_P1deg=0;
float f_sx = 0, f_sy = 0, mx = 1, my = 0, hx = -1, hy = 0;
float f_Altitude=0.0, f_atmospheric=0.0, f_Pb=1030.0, f_Altitude0=0.0;

uint16_t i_gx,i_gy,i_grid=0; // coordinates inside graph window



// Interrupts
// Timer 8 Interrupt
void Update_IT_callback(void)
{
  if (RateCounter++ > LEDRateDivisor)
      {
//      digitalWrite(LED_GREEN,!digitalRead(LED_GREEN));
      LEDFlag = 1;
      RateCounter = 0;
      }
  SampleFlag = 1;    
}

//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/

 //Define Dial
void GenerateDialFace()
{
tft.fillScreen(TFT_BLACK); //clear screen
tft.drawRect(0,0,TFT_HEIGHT,TFT_WIDTH,TFT_WHITE); // draw border
tft.drawCircle(TFT_HEIGHT/2,2*TFT_WIDTH,2*TFT_WIDTH-5,TFT_LIGHTGREY);
  // Draw Tick lines 
   for(uint16_t i = 0; i<=(NT-1); i++) {
    f_sy = cos((i-(NT>>1))*(Dadeg)*0.017453); //Horizontal coordinate, angle in rad
    f_sx = sin((i-(NT>>1))*(Dadeg)*0.017453); //Vertical coordinate, angle in rad
    i_y0 = (int16_t)(-f_sy*DialRadius)+ArcYOrigin; 
    i_x0 = (int16_t)(f_sx*DialRadius)+ArcXOrigin;
    i_y1 = (int16_t)(-f_sy*(DialRadius-LongTick))+ArcYOrigin;
    i_x1 = (int16_t)(f_sx*(DialRadius-LongTick))+ArcXOrigin;
    i_yy1 = (int16_t)(-f_sy*(DialRadius-ShortTick))+ArcYOrigin;
    i_xx1 = (int16_t)(f_sx*(DialRadius-ShortTick))+ArcXOrigin;
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
   if (i%2 != 1) // modulo function
      {
      // Draw Long arc ticks
      tft.drawLine(i_x0,i_y0,i_x1,i_y1,TFT_LIGHTGREY);
      // Write Dial Pressure values
      if (i==0|i==(NT>>1)|i==NT-1)
        {
          if (DisplayHg==1)
          {
          sprintf(s_TempI,"%d",(Min_HgPressure+i*Int_HgPressure));
          }
          else
          {
          sprintf(s_TempI,"%d",(Min_kPaPressure+i*Int_kPaPressure)); 
          }
        if (i==(0)) tft.drawString(s_TempI,f_sx*(DialRadius-TextTick)+ArcXOrigin,-f_sy*(DialRadius-TextTick)+ArcYOrigin-3,1);
        else if (i==(NT>>1)) tft.drawCentreString(s_TempI,f_sx*(DialRadius-TextTick)+ArcXOrigin,-f_sy*(DialRadius-TextTick)+ArcYOrigin-3,1);
        else tft.drawRightString(s_TempI,f_sx*(DialRadius-TextTick)+ArcXOrigin,-f_sy*(DialRadius-TextTick)+ArcYOrigin-3,1);
        }
      } 
      else
      {
      // Draw Short arc ticks
      tft.drawLine(i_x0,i_y0,i_xx1,i_yy1,TFT_LIGHTGREY);
      }
  } // end draw tick lines

  // Generate data fields and/or graph
  if (DisplayNumbers == 1){
  stextP1.pushSprite((TFT_HEIGHT-sTextP1Width)>>1,TFT_WIDTH-sTextP1Height-1); // display Secondary Pressue block
  stextP2.pushSprite((TFT_HEIGHT-sTextP2Width)>>1,TFT_WIDTH-sTextP1Height-sTextP2Height-2); // display Primary Pressure block
  }
  else {
  //tft.setCursor((TFT_HEIGHT)>>1,TFT_WIDTH-sTextP1Height-sTextP2Height-2*sTextP1Height-2);
//  if (DisplayHg == 1) sprintf(s_Dim,"inHg"); // generate legend
//  else sprintf(s_Dim,"kPa");
//  tft.drawCentreString(s_Dim,TFT_HEIGHT>>1,TFT_WIDTH-2*sTextP1Height-sTextP2Height-2,1); // print legend
//  tft.fillRect(1,TFT_WIDTH-sTextP1Height-sGraphHeight-2,sGraphWidth,sGraphHeight,TFT_BLACK); // clear area
//  tft.drawRect(1,TFT_WIDTH-sTextP1Height-sGraphHeight-2,sGraphWidth,sGraphHeight,TFT_WHITE);
  stextP2.fillSprite(TFT_BLACK); // clear screen
//  stextP2.drawRect(0,0,sTextP2Width,sTextP2Height,TFT_WHITE); // generate border
  tft.drawLine(0,(TFT_WIDTH-sTextP1Height-2),TFT_HEIGHT,(TFT_WIDTH-sTextP1Height-2),TFT_WHITE);
  tft.drawLine(0,(TFT_WIDTH-sTextP1Height-sTextP2Height-3),TFT_HEIGHT,(TFT_WIDTH-sTextP1Height-sTextP2Height-3),TFT_WHITE);
  stextP2.drawLine(sTextP2Width-1,0,sTextP2Width-1,sTextP2Height,TFT_DARKGREY);
  //sgraphP3.fillSprite(TFT_BLACK); // Fill sprite with blue
  //sgraphP3.drawRect(0,0,sGraphWidth,sGraphHeight,TFT_WHITE); // generate border
  stextP1.pushSprite((TFT_HEIGHT-sTextP1Width)>>1,TFT_WIDTH-sTextP1Height-1); // display Secondary Pressure block
  stextP2.pushSprite((TFT_HEIGHT-sTextP2Width)>>1,TFT_WIDTH-sTextP1Height-sTextP2Height-2); // display Graph
  }
  if (DisplayHg == 1) sprintf(s_Dim,"inHg"); // generate main legend
  else sprintf(s_Dim,"kPa");
  tft.drawCentreString(s_Dim,TFT_HEIGHT>>1,TFT_WIDTH-2*sTextP1Height-sTextP2Height-2,1); // print main legend  
}

//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/

void setup()
{
  //Initialise LCD
  tft.init();
  pinMode(LED_GREEN, OUTPUT); // initialize LED digital pin as an output.
  pinMode(EncoderA_Pin,INPUT); //Input phase A Rotary encoder
  pinMode(EncoderB_Pin,INPUT);//Input phase B Rotary encoder
  pinMode(EncoderButton_Pin,INPUT);
  analogReadResolution(12); // 12bit adc
 
  //Timer setup
  MyTim->setMode(2,TIMER_OUTPUT_COMPARE);
  MyTim->setOverflow(SamplePeriod,MICROSEC_FORMAT);
  MyTim->attachInterrupt(Update_IT_callback);
  MyTim->resume(); 

  //BMP388 setup
  //I2CDef.setSDA(BMP_SDA_Pin);
  //I2CDef.setSCL(BMP_SCL_Pin);
  //BMP388.begin_I2C(BMP3XX_DEFAULT_ADDRESS,&I2CDef);
 

  // Create a sprite for ADC values
  stextP1.setColorDepth(8);
  stextP1.createSprite(sTextP1Width, sTextP1Height);
  stextP1.fillSprite(TFT_BLACK); // Fill sprite with black
  stextP1.setTextColor(TFT_WHITE); // White text, no background
  stextP1.setTextDatum(BC_DATUM);  // Bottom right coordinate datum

  // Create a sprite for P1 [Hg] values and graph
  stextP2.setColorDepth(8);
  stextP2.createSprite(sTextP2Width,sTextP2Height); // (Height/2,Fontsize*2)
  stextP2.fillSprite(TFT_BLACK); // Fill sprite with black
  //stextP2.setScrollRect(0, 0, sTextP2Width, sTextP2Height, TFT_BLACK);     // here we set scroll gap fill color to blue
  stextP2.setTextColor(TFT_WHITE); // White text, no background
  stextP2.setTextDatum(BC_DATUM);  // Bottom center coordinate datum

//Enable STLink serial port
  Serial.begin(115200,SERIAL_8N1);
  Serial.println("Nucleo-F303 1.83 Inch Display");

// Set the rotation before we calibrate
  tft.setRotation(1);
  LCDHeight = tft.height();
  LCDWidth = tft.width();
//  tft.drawLine(0,0,TFT_WIDTH,TFT_HEIGHT,TFT_BLUE);
// Draw intro screen
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0,0,TFT_HEIGHT,TFT_WIDTH,TFT_WHITE);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextFont(1);
  tft.setTextSize(2);
  tft.setCursor(15,10);
  tft.print(Proj_Name);
  tft.setCursor(1,50);
  tft.print(Proc_Name);
  tft.setCursor(1,75);
  sprintf(s_Dim, "LCD: %s", LCD_Num);
  tft.print(s_Dim);
  tft.setCursor(1,100);
  sprintf(s_Dim, "LCD Driver: %s", DRV_Name);
  tft.print(s_Dim);
  tft.setCursor(1,125);
  sprintf(s_Dim, "Res: %d x %d",TFT_WIDTH,TFT_HEIGHT);
  tft.print(s_Dim);
  tft.setCursor(1,150);
  tft.printf("SW Version: %s", SW_Version);
  //tft.print(s_Dim);

  // calculate dial face angles
  adeg = 180/PI*asin((1.0*ArcXOrigin)/(1.0*DialRadius)); //half angle of the dispay arc range
  Dadeg = 2.0*adeg/(NT-1); // angle increments for ticks
  delay(5000);
  GenerateDialFace();
 //delay(5000);
}

//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/

void PressureCalculation()
{
ADCValue_P1 = analogRead(Pressure1_Pin); // read MAP sensor inputs
//ADCValue_P2 = analogRead(Pressure2_Pin);
f_VA0 = VCC*ADCValue_P1/4095;
f_Vp = 1.5*f_VA0*VCC_Cal/VCC_Sen;
f_P1 = (f_Vp+VP_Offset)*VP_Gain; //P=(Vp+0.558)/0.054 from VSen=0.054*P-0.558 @ VSen=5.1V 
f_Phg = f_P1*0.2953; // 1kPa = 0.2953 inHg
//ToDo: Smooth
f_VA0 = VCC*ADCValue_P1/4095; // calculate voltage on ADC pin
f_Vp = 1.5*f_VA0*VCC_Cal/VCC_Sen; // Calculate inferred sensor voltage
// Calculate pressure from general MAP sensor data
f_P1 = (f_Vp+0.558)*18.5185; //P=(Vp+0.558)/0.054 from VSen=0.054*P-0.558 @ VSen=5.1V 
if (f_P1 >100.0){f_P1 = 100.0;} // limit reading to sensor maximum
f_Phg = f_P1*Kpa2Hg; // 1kPa = 0.2953 inHg
} // end PressureCalculation()


//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/
//******************************************************************************************************************/

void loop()
{
if (SampleFlag) // interrupt every 1ms
{
PressureCalculation();
SampleFlag = 0;
}
if (LEDFlag)
{
  
// Write Pressure values to strings
  /*if (f_P1>=100.0) dtostrf(f_P1,3,0,s_TempM);
    else dtostrf(f_P1,3,1,s_TempM);
  //dtostrf(f_Phg,3,1,s_TempI);
  if (BMP388.performReading()) 
  {//f_atmospheric = BMP388.readPressure() *0.01; //
  //f_Altitude = 44330.0 * (1.0 - pow(BMP388.pressure*0.01 / SEALEVELPRESSURE_HPA, 0.1903));
  f_Altitude = f_Altitude0 + 44308 * (1-pow((BMP388.pressure*0.01/f_Pb),0.190263));
   dtostrf(f_Altitude*mtoft,4,0,s_TempI);
  }
  else dtostrf(f_Phg,3,1,s_TempI);
*/
  // Set up the sprite blocks  
  stextP1.fillSprite(TFT_BLACK); // Fill sprite with black
  stextP1.setCursor(1,3);
  stextP1.setTextFont(1); // font for unit display
  stextP1.setTextSize(1);
  //
  //stextP2.fillSprite(TFT_BLACK); // Fill sprite with black
  //stextP2.drawRect(0,0,sTextP2Width,sTextP2Height,TFT_WHITE);
  stextP2.setCursor(20,1);
  stextP2.setTextFont(7);
  stextP2.setTextSize(1);

// Write pressure field values
  if (DisplayNumbers == 1){ 
  stextP2.fillSprite(TFT_BLACK); // Fill sprite with black
    if (DisplayHg == 1) // main display in inHg
    { 
      stextP2.print(s_TempI);
      stextP1.print(s_TempM);
    }
    else // main display in kPa
    {
      stextP2.print(s_TempM);
      stextP1.print(s_TempI);
    }
    //  stextP1.pushSprite((TFT_HEIGHT-sTextP1Width)>>1,TFT_WIDTH-sTextP1Height-1);
    //  stextP2.pushSprite((TFT_HEIGHT-sTextP2Width)>>1,(TFT_WIDTH-sTextP1Height-sTextP2Height-2)); // write to screen
  } 
  else {
   if (DisplayHg == 1)
    { 
      i_gy = (uint16_t)((sTextP2Height)*(1-f_Phg/(Max_HgPressure-Min_HgPressure))); // determine y-coordinate
      stextP1.print(s_TempI);
    }
    else // main display in kPa
    {
      i_gy = (uint16_t)((sTextP2Height)*(1-f_P1/(Max_kPaPressure-Min_kPaPressure))); // determine y-coordinate
      stextP1.print(s_TempM);
    }   
  if (i_gx > sTextP2Width) // fold back pixel draw at the end of the window
    {
    i_gx = 0;
    //tft.fillRect(1,TFT_WIDTH-sTextP1Height-sGraphHeight-2,sGraphWidth,sGraphHeight,TFT_BLACK); // clear area
    //tft.drawRect(1,TFT_WIDTH-sTextP1Height-sGraphHeight-2,sGraphWidth,sGraphHeight,TFT_WHITE);
   // stextP2.fillSprite(TFT_BLACK); // clear screen
  //  stextP2.drawRect(0,0,sTextP2Width,sTextP2Height,TFT_WHITE); // generate border
//  tft.drawLine(0,(TFT_WIDTH-sTextP1Height-2),TFT_HEIGHT,(TFT_WIDTH-sTextP1Height-2),TFT_WHITE);
//  tft.drawLine(0,(TFT_WIDTH-sTextP1Height-sTextP2Height-3),TFT_HEIGHT,(TFT_WIDTH-sTextP1Height-sTextP2Height-3),TFT_WHITE);
    }
    //stextP2.drawPixel(i_gx,i_gy+1,TFT_WHITE);
    stextP2.scroll(-1,0);
    if (i_grid >= GridlineInterval)
    {
      stextP2.drawLine(sTextP2Width-1,0,sTextP2Width-1,sTextP2Height,TFT_DARKGREY);
      i_grid=0;
    } else
    {    
    stextP2.drawPixel(sTextP2Width-1,i_gy+1,TFT_WHITE);
    }
    i_gx++;
    i_grid++;
//    stextP2.pushSprite((TFT_HEIGHT-sTextP2Width)>>1,(TFT_WIDTH-sTextP1Height-sTextP2Height-2));
//    stextP1.pushSprite((TFT_HEIGHT-sTextP1Width)>>1,TFT_WIDTH-sTextP1Height-1);
  }
    stextP1.pushSprite((TFT_HEIGHT-sTextP1Width)>>1,(TFT_WIDTH-sTextP1Height-1));
    stextP2.pushSprite((TFT_HEIGHT-sTextP2Width)>>1,(TFT_WIDTH-sTextP1Height-sTextP2Height-2));

// Generate the needle tip
if (DisplayHg == 1) f_P1deg = adeg*((2*(f_Phg-Min_HgPressure)/(Max_HgPressure-Min_HgPressure))-1);
    else f_P1deg = adeg*((2*(f_P1-Min_kPaPressure)/(Max_kPaPressure-Min_kPaPressure))-1);
  f_P1sx0 = sin(f_P1deg*0.017453); //Horizontal unit vektor of P1 needle centre, angle in rad
  f_P1sy0 = -cos(f_P1deg*0.017453); //Vertical unit vektor of P1 needle centre, angle in rad
  f_P1sx1 = sin((f_P1deg+NeedleHWidth)*0.017453); //Horizontal unit vektor of P1 needle right, angle in rad
  f_P1sy1 = -cos((f_P1deg+NeedleHWidth)*0.017453); //Vertical unit vektor of P1 needle right, angle in rad
  f_P1sx2 = sin((f_P1deg-NeedleHWidth)*0.017453); //Horizontal unit vektor of P1 needle left, angle in rad
  f_P1sy2 = -cos((f_P1deg-NeedleHWidth)*0.017453); //Vertical unit vektor of P1 needle left, angle in rad

  P1x0 = (f_P1sx0*NeedleRadius)+ArcXOrigin;
  P1y0 = (f_P1sy0*NeedleRadius)+ArcYOrigin;
  P1x1 = f_P1sx1*(NeedleRadius-NeedleTipLength)+ArcXOrigin;
  P1y1 = f_P1sy1*(NeedleRadius-NeedleTipLength)+ArcYOrigin;
  P1x2 = f_P1sx2*(NeedleRadius-NeedleTipLength)+ArcXOrigin;
  P1y2 = f_P1sy2*(NeedleRadius-NeedleTipLength)+ArcYOrigin;
  tft.fillTriangle(P1x0_old,P1y0_old,P1x1_old,P1y1_old,P1x2_old,P1y2_old,TFT_BLACK);
  tft.fillTriangle(P1x0,P1y0,P1x1,P1y1,P1x2,P1y2,TFT_RED);
  P1x0_old = P1x0; P1y0_old = P1y0; P1x1_old = P1x1; P1y1_old = P1y1; P1x2_old = P1x2; P1y2_old = P1y2;

  // Print values to STLink Serial Port
  // Elaborate string population to print floats to serial port
  // Can use s_Tempx strings above if memory runs out
  //dtostrf(f_VA0,4,2,s_V);
 // dtostrf(f_P1,3,2,s_PM);
 // dtostrf(f_Phg,3,2,s_PI);
 if (DisplayHg == 1) sprintf(s_Dim,"%s",s_TempI);
  else sprintf(s_Dim,"%s",s_TempM);
  Serial.println(s_Dim);
  digitalWrite(LED_GREEN,!digitalRead(LED_GREEN));
  LEDFlag = 0;
}
}
/* 
#include <Arduino.h>
#include <BlackPill303-Display.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
TIM_TypeDef *Instance = TIM8;
HardwareTimer *MyTim = new HardwareTimer(Instance);

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TFT_eSprite stextP1 = TFT_eSprite(&tft); // Sprite object stextP1
TFT_eSprite stextP2 = TFT_eSprite(&tft); // Sprite object stextP2

//Variables
uint16_t Viewport_Height, Viewport_Width;
uint16_t LCDWidth, LCDHeight;
int16_t CursorX, CursorY;
char s_Dim[32];
uint16_t i_Counter=0; 
uint8_t DisplayID;
uint16_t RateCounter=0; 
uint8_t LEDFlag=0, SampleFlag=0;
int16_t ADCValue_P1=0, ADCValue_P2=0;

//Starfield stuff
// With 1024 stars the update rate is ~65 frames per second
#define NSTARS 1024
uint8_t sx[NSTARS] = {};
uint8_t sy[NSTARS] = {};
uint8_t sz[NSTARS] = {};

uint8_t za, zb, zc, zx;

// Fast 0-255 random number generator from http://eternityforest.com/Projects/rng.php:
uint8_t __attribute__((always_inline)) rng()
{
  zx++;
  za = (za^zc^zx);
  zb = (zb+za);
  zc = ((zc+(zb>>1))^za);
  return zc;
}

// Interrupts
// Timer 8 Interrupt
void Update_IT_callback(void)
{
  if (RateCounter++ > LEDRateDivisor)
      {
      digitalWrite(LED_BLUE,!digitalRead(LED_BLUE));
      LEDFlag = 1;
      RateCounter = 0;
      }
  SampleFlag = 1;    
}

void setup()
{

  za = random(256);
  zb = random(256);
  zc = random(256);
  zx = random(256);

  // initialize LED digital pin as an output.
  pinMode(LED_BLUE, OUTPUT);
  //pinMode(Pressure1_Pin,INPUT);
  //pinMode(Pressure2_Pin,INPUT);
  pinMode(EncoderA_Pin,INPUT); //Input phase A Rotary encoder
  pinMode(EncoderB_Pin,INPUT);//Input phase B Rotary encoder
  pinMode(EncoderButton_Pin,INPUT);
  analogReadResolution(12); // 12bit adc
 
  //Timer setup
  MyTim->setMode(2,TIMER_OUTPUT_COMPARE);
  MyTim->setOverflow(SamplePeriod,MICROSEC_FORMAT);
  MyTim->attachInterrupt(Update_IT_callback);
  MyTim->resume(); 

  // Create a sprite for P1 values
  stextP1.setColorDepth(8);
  stextP1.createSprite(32, 10);
  stextP1.fillSprite(TFT_BLACK); // Fill sprite with blue
  //stext1.setScrollRect(0, 0, 24, 16, TFT_BLUE);     // here we set scroll gap fill color to blue
  stextP1.setTextColor(TFT_WHITE); // White text, no background
  stextP1.setTextDatum(BR_DATUM);  // Bottom right coordinate datum

    // Create a sprite for P1 values
  stextP2.setColorDepth(8);
  stextP2.createSprite(32, 10);
  stextP2.fillSprite(TFT_BLACK); // Fill sprite with blue
  //stextP2.setScrollRect(0, 0, 24, 16, TFT_BLUE);     // here we set scroll gap fill color to blue
  stextP2.setTextColor(TFT_WHITE); // White text, no background
  stextP2.setTextDatum(BR_DATUM);  // Bottom right coordinate datum

//  Serial.begin(115200,SERIAL_8N1);
//  Serial.println("MIKROE-STM32 Mini M4");

  //Initialise
  tft.init();
  // Set the rotation before we calibrate
  tft.setRotation(3);
  LCDHeight = tft.height();
  LCDWidth = tft.width();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextSize(1);
  tft.setCursor(0,tft.fontHeight());
  tft.println("STM32 Black Pill");
  tft.println("STM32F303CCT6");
  tft.setTextFont(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.print("1.8\" LCD Module V"); tft.println(FW_Version);
//  tft.setFreeFont(&Yellowtail_32);
  sprintf(s_Dim,"ST7735S %d x %d",TFT_WIDTH,TFT_HEIGHT);
  tft.print(s_Dim);

  //tft.println("Starfield Simulation");
  delay(5000);
 // StarField();
 //Define Dial


}
void PressureCalculation()
{
ADCValue_P1 = analogRead(Pressure1_Pin); // read MAP sensor inputs
ADCValue_P2 = analogRead(Pressure2_Pin);
// Smooth

//Calculate pressure in inHg

}

void loop()
{
if (SampleFlag) // interrupt every 1ms
{
PressureCalculation();
SampleFlag = 0;
}
if (LEDFlag)
{
//  tft.setCursor(0,100,1);
//  tft.print(s_Dim);
  stextP1.fillSprite(TFT_BLACK); // Fill sprite with black
  stextP1.pushSprite(0,90);
  stextP1.setCursor(0,0);
  sprintf(s_Dim,"P1:%X",ADCValue_P1);
  stextP1.print(s_Dim);
  stextP1.pushSprite(0,90);
  stextP2.fillSprite(TFT_BLACK); // Fill sprite with black
  stextP2.pushSprite(50,90);
  stextP2.setCursor(0,0);
  sprintf(s_Dim,"P2:%X",ADCValue_P2);
  stextP2.print(s_Dim);
  stextP2.pushSprite(50,90);

  LEDFlag = 0;
}
}

void StarField(){
  uint8_t spawnDepthVariation = 255;
while(1){
  for(int i = 0; i < NSTARS; ++i)
  {
    if (sz[i] <= 1)
    {
      sx[i] = 160 - 120 + rng();
      sy[i] = rng();
      sz[i] = spawnDepthVariation--;
    }
    else
    {
      int old_screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
      int old_screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

      // This is a faster pixel drawing function for occassions where many single pixels must be drawn
      tft.drawPixel(old_screen_x, old_screen_y,TFT_BLACK);

      sz[i] -= 2;
      if (sz[i] > 1)
      {
        int screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
        int screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;
  
        if (screen_x >= 0 && screen_y >= 0 && screen_x < 320 && screen_y < 240)
        {
          uint8_t r, g, b;
          r = g = b = 255 - sz[i];
          tft.drawPixel(screen_x, screen_y, tft.color565(r,g,b));
        }
        else
          sz[i] = 0; // Out of screen, die.
      }
    }
  }
}
}

//====================================================================================

uint32_t readRegister(uint8_t reg, int16_t bytes, uint8_t index)
{
  uint32_t  data = 0;

  while (bytes > 0) {
    bytes--;
    data = (data << 8) | tft.readcommand8(reg, index);
    index++;
  }

  tft.print("Register 0x");
  if (reg < 0x10) tft.print("0");
  tft.print(reg , HEX);

  tft.print(": 0x");

  // Add leading zeros as needed
  uint32_t mask = 0x1 << 28;
  while (data < mask && mask > 0x1) {
    tft.print("0");
    mask = mask >> 4;
  }

  tft.println(data, HEX);

  return data;
} */