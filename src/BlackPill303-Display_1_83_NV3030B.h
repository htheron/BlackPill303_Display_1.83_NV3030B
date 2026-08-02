/*
* Only definitions pertaining to the application.
* Definitions of the platform and TFT done in pltformio.ini
*/

#ifndef INCLUDE_BlackPill303_Display_H
#define INCLUDE_BlackPill303_Display_H
#define Proj_Name "Mini Engine Monitor" 
#define Proc_Name "Blackpill STM32F303CCT6"
#define DRV_Name "NV3030B"
#define LCD_Num "Waveshare #26784"
#define SW_Version "1.0"
#define LEDRate 2
#define LEDPeriod 1000000/(2*LEDRate) //500000 //microsecond
#define SampleRate 1000 //S/s
#define SamplePeriod 1000000/SampleRate
#define LEDRateDivisor SampleRate/(LEDRate*2)
#define RateDivisor SampleRate/LEDRate 
#define LED_GREEN   PC13
#define Pressure1_Pin PA0
#define Pressure2_Pin PA1
#define EncoderA_Pin PC14
#define EncoderB_Pin PC15
#define EncoderButton_Pin PB1
/*
#define TFT_SPI_PORT 2 // SPI port 2 maximum clock rate is 27MHz
#define TFT_MOSI PB15
#define TFT_MISO PB14
#define TFT_SCLK PB13
#define TFT_CS   PB12
#define TFT_DC   PA8 
#define TFT_RST  PB10 
*/
// I2C pins for BMP388
#define BMP_SDA_Pin PB9
#define BMP_SCL_Pin PB8
// defines for pressure sensor
#define VCC 3.3 //V
#define VCC_Cal 5.1 //[V] Sensor Supply voltage for calibration curve
#define VCC_Sen 4.4 //[V] actual measure sensor supply voltage
#define VP_Offset 0.558
#define VP_Gain 18.5185 // 1/0.054
#define Kpa2Hg 0.2953
// define for dial face
#define ArcXOrigin TFT_HEIGHT/2 //X centre point of arc
#define ArcYOrigin 2*TFT_WIDTH+5 //Y centre point of arc
#define DialRadius 2*TFT_WIDTH
#define NT 13 //Amount of ticks from min to max
#define Min_HgPressure 0 // minimum displayed Hg value
#define Min_kPaPressure 0 // minimum displayed kPa value
#define Max_HgPressure 30 // maximum displayed Hg value
#define Max_kPaPressure 100 // maximum displayed kPa Value
#define Int_HgPressure (Max_HgPressure-Min_HgPressure)/(NT-1)
#define Int_kPaPressure (Max_kPaPressure-Min_kPaPressure)/(NT-1)
#define LongTick 6 // length of major ticks
#define ShortTick 3 // length of minor tick
#define TextTick 28 //
#define NeedleRadius (DialRadius-LongTick-2)
#define NeedleTipLength 14 // length of needle
#define NeedleHWidth 1 // angular base width of needle
#define sTextP1Height 15 // height of Auxiliary info sprite
#define sTextP2Height 50 // height of Main pressure sprite
#define sTextP1Width 30 // height of Auxiliary info sprite
#define sTextP2Width (TFT_HEIGHT-6) //110 // Width of Main pressure sprite
#define sGraphWidth (TFT_HEIGHT-2) // width of graph sprite
#define sGraphHeight sTextP2Height
#define GridlineInterval LEDRate*20
#define mtoft (3.2808399)

//https://www.mide.com/air-pressure-at-altitude-calculator
#define Tb 288 //[K]
#define Lb -0.0065 //[K/m]
#define M 0.0289644 //[kg/mol]
#define g0 9.80665 // [m/s^2]
#define R 8.31432 // [n.m/mol.K]

#define SEALEVELPRESSURE_HPA (1013.25)


void Update_IT_callback(void);
void GenerateDialFace();



#endif