//******************************************************************************
// File:        main.c
// Description: Main entry point for SDC41 I2C DLL test using CH347
// Author:      J. Laborda (Versa Design S.L.)
//******************************************************************************

//******************************************************************************
// INCLUDES
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <stdint.h>

#include "CH347DLL.H"
//******************************************************************************
// DEFINES & TYPEDEFS
#define I2C_MODE_100KHZ      (1)

#define SCD41_I2C_ADDR_7BIT  (0x62)
#define SCD41_I2C_ADDR_WRITE (0xC4)
#define SCD41_I2C_ADDR_READ  (0xC5)

#define SCD41_START_MEAS     (0x21B1)
#define SCD41_STOP_MEAS      (0x3F86)
#define SCD41_READ_MEAS      (0xEC05)

#define I2C_RX_MAX           (9)

#define NUMBER_OF_MEAS       (20)

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xff

//******************************************************************************
// VARIABLES
static mDeviceInforS deviceInfo;
static unsigned int ch347Index = 0;
static uint8_t rxBuffer[I2C_RX_MAX] = {0};

static const uint8_t scd41StartMeasCmd[] = {SCD41_I2C_ADDR_WRITE, (SCD41_START_MEAS >> 8), (SCD41_START_MEAS & 0xFF)};
static const uint8_t scd41StopMeasCmd[]  = {SCD41_I2C_ADDR_WRITE, (SCD41_STOP_MEAS >> 8),  (SCD41_STOP_MEAS & 0xFF)};
static const uint8_t scd41ReadMeasCmd[]  = {SCD41_I2C_ADDR_WRITE, (SCD41_READ_MEAS >> 8),  (SCD41_READ_MEAS & 0xFF)};

//******************************************************************************
// FUNCTIONS

//******************************************************************************
static uint8_t CalculateCrc(const uint8_t* data, uint16_t count) 
//******************************************************************************
// Description: Calculate and get CRC8
// Parameters:  const uint8_t* data: pointer to buffer
//              uint16_t count: size of data (number of bytes)
// Returns:     uint8_t calculated CRC
//******************************************************************************
{
  uint16_t current_byte;
  uint8_t crc = CRC8_INIT;
  uint8_t crc_bit;

  // Calculates 8-Bit checksum with given polynomial
  for (current_byte = 0; current_byte < count; ++current_byte) 
  {
    crc ^= (data[current_byte]);
    for (crc_bit = 8; crc_bit > 0; --crc_bit) 
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ CRC8_POLYNOMIAL;
      else
        crc = (crc << 1);
    }
  }
  return crc;
}

//******************************************************************************
int main()
//******************************************************************************
// Description: Main entry point
// Parameters:  None
// Returns:     int
//******************************************************************************
{
  int i, j;
  uint16_t co2Ppm;
  uint16_t tempReg, humReg;
  float temp, hum;

  CH347OpenDevice(ch347Index);

  // Get device information
  CH347GetDeviceInfor(ch347Index, &deviceInfo);
  printf("Device Info: %s\n", deviceInfo.DeviceID);

  // Configure I2C
  CH347I2C_Set(ch347Index, I2C_MODE_100KHZ);

  // Send start measurement command
  printf("scd41StartMeasCmd: ");
  for (i = 0; i < sizeof(scd41StartMeasCmd); i++) printf("%02X ", scd41StartMeasCmd[i]);
  printf("\n");
  CH347StreamI2C(ch347Index, sizeof(scd41StartMeasCmd), (uint8_t *)scd41StartMeasCmd, 0, NULL);
  Sleep(5000);

  // Read measurement and print temperature
  for (j = 0; j < NUMBER_OF_MEAS; j++)
  {
    printf("scd41ReadMeasCmd: ");
    for (i = 0; i < sizeof(scd41ReadMeasCmd); i++) printf("%02X ", scd41ReadMeasCmd[i]);
    printf("\n");

    CH347StreamI2C(ch347Index, sizeof(scd41ReadMeasCmd), (uint8_t *)scd41ReadMeasCmd, I2C_RX_MAX, rxBuffer);
    printf("rxBuffer: ");
    for (i = 0; i < I2C_RX_MAX; i++) printf("%02X ", rxBuffer[i]);
    printf("\n");

    // CO2 (ppm)
    co2Ppm = rxBuffer[1];
    co2Ppm |= (rxBuffer[0] << 8);
    printf("CO2: %u ppm\n", co2Ppm);

    // Temperature
    // printf("Temp reg: 0x%02X%02X\n", rxBuffer[3], rxBuffer[4]);
    tempReg = rxBuffer[4];
    tempReg |= (rxBuffer[3] << 8);
    temp = tempReg * 175.0;
    temp /= 65535.0;
    temp -= 45.0;
    printf("Temp: %.2f C\n", temp);

    // Humidity
    humReg = rxBuffer[7];
    humReg |= (rxBuffer[6] << 8);
    hum = humReg * 100.0;
    hum /= 65535.0;
    printf("Hum: %.1f \n", hum);

    Sleep(5000);
  }

  CH347StreamI2C(ch347Index, sizeof(scd41StopMeasCmd), (uint8_t *)scd41StopMeasCmd, 0, NULL);

  CH347CloseDevice(0);


  return 0;
}
