/* 8 Field Display
*  LCD Integration with Blackpill 303
*  Processor: STM32F303CCT6
*  Board: Blackpill-F303
*  Display: Waveshare 1.83" SPI TFT LCD 240x280
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
TFT_eSprite stextP1 = TFT_eSprite(&tft); // Sprite object stextP1 - Display Field 1
TFT_eSprite stextP2 = TFT_eSprite(&tft); // Sprite object stextP2 - Display field 2
TFT_eSprite sgraphP3 = TFT_eSprite(&tft); // Sprite object sgraphP3 - Graph display

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

/******************************************************************************************************************/

  // Generate data fields and/or graph
  //stextP1.pushSprite((TFT_HEIGHT-sTextP1Width)>>1,TFT_WIDTH-sTextP1Height-1); // display Secondary Pressue block
  //stextP2.pushSprite((TFT_HEIGHT-sTextP2Width)>>1,TFT_WIDTH-sTextP1Height-sTextP2Height-2); // display Primary Pressure block
    


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
 

  // Create a sprites for data fields, TL, TR, 2L, 2R, 3L,3R, ...., BL, BR
  stextP1.setColorDepth(8);
  stextP1.createSprite(sTextP1Width, sTextP1Height); // top left data block
  stextP1.fillSprite(TFT_BLACK); // Fill sprite with black
  stextP1.setTextColor(TFT_WHITE); // White text, no background
  //stextP1.setTextDatum(C_BASELINE);  // Bottom right coordinate datum
  stextP1.drawRect(0,0,sTextP1Width,sTextP1Height,TFT_WHITE);

  // Create a sprite for P1 [Hg] values and graph
  stextP2.setColorDepth(8);
  stextP2.createSprite(sTextP2Width,sTextP2Height); // top right data block
  stextP2.fillSprite(TFT_BLACK); // Fill sprite with black
  stextP2.setTextColor(TFT_WHITE); // White text, no background
  //stextP2.setTextDatum(TL_DATUM);  // Bottom center coordinate datum
  stextP2.drawRect(0,0,sTextP2Width,sTextP2Height,TFT_WHITE);

//Enable STLink serial port
  Serial.begin(115200,SERIAL_8N1);
  Serial.println("BlackPill F303 1.83\" Display");

// Set the rotation
  tft.setRotation(1);
// Draw intro screen
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0,0,TFT_HEIGHT,TFT_WIDTH,TFT_WHITE);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(2);
  tft.setCursor(15,10); //Offset for rounded corner
  tft.print(Proj_Name);
  tft.setTextFont(2);
  tft.setTextSize(1);
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

  delay(1000);
  tft.fillScreen(TFT_BLACK); // clear screen
  tft.setTextDatum(CC_DATUM);
}

void loop()
{
if (SampleFlag) // interrupt every 1ms
{
SampleFlag = 0;
}
if (LEDFlag)
{
  
  // Set up the sprite blocks  
/*   stextP1.fillSprite(TFT_BLACK); // Fill sprite with black
  stextP1.setCursor(0,0);
  stextP1.setTextFont(2); // font for unit display
  stextP1.setTextSize(2);
  stextP1.drawRect(0,0,sTextP1Width,sTextP1Height,TFT_BLUE);
  stextP1.print("Data 1");
 
  stextP2.setCursor(sTextP2Width,sTextP2Height);
  stextP2.setTextFont(1);
  stextP2.setTextSize(2);
  stextP2.drawRect(0,0,sTextP2Width,sTextP2Height,TFT_DARKGREEN);
  stextP2.print("Data 2"); */


  //stextP1.pushSprite(0,0);
  //stextP2.pushSprite(sTextP1Width,0);

  
  tft.drawRect(1,1,DField1Width,DField1Height,TFT_DARKGREEN);
  tft.drawRect(DField1Width,1,DField2Width,DField2Height,TFT_DARKGREEN);
  tft.drawRect(1,DField1Height,DField2Width,DField2Height,TFT_DARKCYAN);
  tft.drawRect(DField2Width,DField1Height,DField1Width,DField1Height,TFT_DARKCYAN);

  tft.setTextFont(2);
  tft.setTextSize(2);
  tft.setCursor(DField1Width/2,DField1Height/2);
  tft.setTextDatum(BC_DATUM);
  tft.print("DATA 1");
  tft.setTextFont(2);
  tft.setTextSize(2);
  tft.setCursor(DField1Width+DField2Width/2,DField2Height/2);
  tft.setTextDatum(TC_DATUM);
  tft.print("DATA 2");

  




 if (DisplayHg == 1) sprintf(s_Dim,"%s",s_TempI);
  else sprintf(s_Dim,"%s",s_TempM);
  Serial.println(s_Dim);
  digitalWrite(LED_GREEN,!digitalRead(LED_GREEN));
  LEDFlag = 0;
}
}