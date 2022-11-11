/*
Sunhayato Corp.
MM-ENV01 I2C Sample for I2C

    MM-ENV01 ------ Arduino
    VDD ----------- 5.0V
    GND ----------- GND
    SDA ----------- SDA(A4) *pullup to VDD
    SCL ----------- SCL(A5) *pullup to VDD
    OPT_INT  -X
    MPL_INT1 -X
    MPL_INT2 -X
    NC       -X
*/

#include <Wire.h>

// I2C Address(7bit)
#define HDC1080_ADDR	(0x40)
#define VEML6075_ADDR	(0x10)
#define OPT3001_ADDR	(0x44)
#define MPL331_ADDR		(0x60)

// endian
#define MSB_LSB		(0)
#define LSB_MSB		(1)


void setup()
{
    Serial.begin(9600);
    
    Wire.begin();
}


void loop()
{
    Serial.println("<<MM-ENV01>>");   
    printHDC1080();
    printVEML6075();
    printOPT3001();
    printMPL331();
    
    delay(1000);
}


void printMPL331()
{
    
    // setting
    writeReg(MPL331_ADDR, 0x26, 0x39);
    writeReg(MPL331_ADDR, 0x13, 0x07);
    
    // read data
    int16_t temp_raw;
    temp_raw = readReg(MPL331_ADDR, 0x04);
    temp_raw <<= 8;
    temp_raw |= readReg(MPL331_ADDR, 0x05);
    temp_raw >>= 4;
    
    float tempC;
    tempC = (float)temp_raw / 16.0;
    
    uint32_t press_raw;
    press_raw  = readReg(MPL331_ADDR, 0x01);
    press_raw <<= 8;
    press_raw |= readReg(MPL331_ADDR, 0x02);
    press_raw <<= 8;
    press_raw |= readReg(MPL331_ADDR, 0x03);
    press_raw >>= 4;
    
    float press;
    press = (float)press_raw / 4.0;
    press /= 100;	// Pa->hPa
    
    Serial.println("MPL331:");
    Serial.print("Temp = ");
    Serial.print(tempC);
    Serial.print("deg C, Pressure = ");
    Serial.print(press);
    Serial.println("hPa");
}


void printOPT3001()
{
    int16_t result;
    byte exponent;
    int16_t lux_raw;
    float lux;
    
    // setting
    write16(OPT3001_ADDR, 0x01, 0xC610, MSB_LSB);
    
    // read data
    result = read16(OPT3001_ADDR, 0x00, MSB_LSB);
    
    exponent = (byte)(0x0F & (result >> 12));
    lux_raw  = 0x0FFF & result;
    
    lux = 0.01 * pow(2, exponent) * lux_raw;
    
    Serial.println("OPT3001:");
    Serial.print("LUX = ");
    Serial.print(lux);
    Serial.println(" lux");   
}


void printVEML6075()
{
    int16_t uva_raw, uvb_raw, uvcomp1, uvcomp2, devID;
    float uva_calc, uvb_calc;
    
    // setting
    write16(VEML6075_ADDR, 0x00, 0x0010, LSB_MSB);
    
    // read data
    uva_raw = read16(VEML6075_ADDR, 0x07, LSB_MSB);
    uvb_raw = read16(VEML6075_ADDR, 0x09, LSB_MSB);
    uvcomp1 = read16(VEML6075_ADDR, 0x0A, LSB_MSB);
    uvcomp2 = read16(VEML6075_ADDR, 0x0B, LSB_MSB);
    
    uva_calc = (float)uva_raw - 2.22f * (float)uvcomp1 - 1.33f * (float)uvcomp2;
    uvb_calc = (float)uvb_raw - 2.95f * (float)uvcomp1 - 1.74f * (float)uvcomp2;
    
    Serial.println("VEML6075:");
    Serial.print("UVA = ");
    Serial.print(uva_calc);
    Serial.print(", UVB = ");
    Serial.print(uvb_calc);
    Serial.println("");
}


void printHDC1080()
{
    int16_t temp_raw;
    float tempC;
    
    int16_t rh_raw;
    float rh;
    
    // setting
    write16(HDC1080_ADDR, 0x02, 0x0000, MSB_LSB);
    
    // read temperature
    temp_raw = read16(HDC1080_ADDR, 0x00, MSB_LSB);
    tempC = (((float)temp_raw * 165) / 64436) - 40;
    
    // read humidity
    rh_raw = read16(HDC1080_ADDR, 0x01, MSB_LSB);
    rh = ((float)rh_raw * 100) / 65536;
    
    Serial.println("HDC1080:");
    Serial.print("Temp = ");
    Serial.print(tempC);
    Serial.print("deg C, RH = ");   
    Serial.print(rh);
    Serial.println("%");   
}

void writeReg(byte addr, byte reg, byte value)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

byte readReg(byte addr, byte reg)
{
    byte data;
    
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
	delay(10);
    Wire.requestFrom(addr, (byte)1);
    data = Wire.read();
    Wire.endTransmission();
    
    return (data);
}


void write16(byte addr, byte reg, uint16_t value, byte endian)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    
    if (endian == MSB_LSB){
	    Wire.write((uint8_t)(0xFF & (value >> 8)));		// MSB
	    Wire.write((uint8_t)(0xFF & (value >> 0)));		// LSB
	} else {
	    Wire.write((uint8_t)(0xFF & (value >> 0)));		// LSB
	    Wire.write((uint8_t)(0xFF & (value >> 8)));		// MSB
	}
    Wire.endTransmission();
}

uint16_t read16(byte addr, byte reg, byte endian)
{
	byte msb, lsb;
	
	Wire.beginTransmission(addr);
	Wire.write(reg);
	Wire.endTransmission(false);
	delay(10);
	Wire.requestFrom(addr, (byte)2, true);
	
	if (endian == MSB_LSB){
		msb = Wire.read();
		lsb = Wire.read();
	} else {
		lsb = Wire.read();
		msb = Wire.read();
	}
	
    Wire.endTransmission();
    
	return ((msb << 8) | lsb);
}
