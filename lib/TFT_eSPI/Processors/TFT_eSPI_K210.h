        ////////////////////////////////////////////////////
        //       TFT_eSPI generic driver functions        //
        ////////////////////////////////////////////////////

// This is a generic driver for Arduino boards, it supports SPI interface displays
// 8 bit parallel interface to TFT is not supported for generic processors

#ifndef _TFT_eSPI_K210H_
#define _TFT_eSPI_K210H_

// Processor ID reported by getSetup()
#define PROCESSOR_ID 0x0210

// Include processor specific header
#include "st7789_k210.h"

#ifdef M5STICKV
#define RST_GPIONUM 21
#define DCX_GPIONUM 20
#define RST_PIN 6
#define DCX_PIN 7
#define SS_PIN 3
#define DMA_CH 3
#else
#define RST_GPIONUM 37
#define DCX_GPIONUM 38
#define RST_PIN 6
#define DCX_PIN 7
#define SS_PIN 3
#define DMA_CH 3
#endif

// Processor specific code used by SPI bus transaction startWrite and endWrite functions
#define SET_BUS_WRITE_MODE // Not used
#define SET_BUS_READ_MODE  // Not used

// Code to check if DMA is busy, used by SPI bus transaction startWrite and endWrite functions
#define DMA_BUSY_CHECK // Not used so leave blank

// To be safe, SUPPORT_TRANSACTIONS is assumed mandatory
#if !defined (SUPPORT_TRANSACTIONS)
  #define SUPPORT_TRANSACTIONS
#endif

// Initialise processor specific SPI functions, used by init()
#define INIT_TFT_DATA_BUS

// If smooth fonts are enabled the filing system may need to be loaded
#ifdef SMOOTH_FONT
  // Call up the filing system for the anti-aliased fonts
  //#define FS_NO_GLOBALS
  //#include <FS.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the DC (TFT Data/Command or Register Select (RS))pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TFT_DC
//  #define DC_C // No macro allocated so it generates no code
//  #define DC_D // No macro allocated so it generates no code
  #define DC_C set_dcx_control()
  #define DC_D set_dcx_data()
#else
  #define DC_C digitalWrite(TFT_DC, LOW)
  #define DC_D digitalWrite(TFT_DC, HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the CS (TFT chip select) pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TFT_CS
//  #define CS_L // No macro allocated so it generates no code
  #define CS_L tft_write_cs()
  #define CS_H // No macro allocated so it generates no code
#else
  #define CS_L digitalWrite(TFT_CS, LOW)
  #define CS_H digitalWrite(TFT_CS, HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the WR (TFT Write) pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#ifdef TFT_WR
  #define WR_L digitalWrite(TFT_WR, LOW)
  #define WR_H digitalWrite(TFT_WR, HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Define the touch screen chip select pin drive code
////////////////////////////////////////////////////////////////////////////////////////
#if !defined TOUCH_CS || (TOUCH_CS < 0)
  #define T_CS_L // No macro allocated so it generates no code
  #define T_CS_H // No macro allocated so it generates no code
#else
  #define T_CS_L digitalWrite(TOUCH_CS, LOW)
  #define T_CS_H digitalWrite(TOUCH_CS, HIGH)
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Make sure TFT_MISO is defined if not used to avoid an error message
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TFT_MISO
  #define TFT_MISO -1
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Macros to write commands/pixel colour data to an ILI9488 TFT
////////////////////////////////////////////////////////////////////////////////////////
#if  defined (ILI9488_DRIVER) // 16 bit colour converted to 3 bytes for 18 bit RGB

  // Write 8 bits to TFT
  #define tft_Write_8(C)   spi_.transfer(C)

  // Convert 16 bit colour to 18 bit and write in 3 bytes
  #define tft_Write_16(C)  spi_.transfer(((C) & 0xF800)>>8); \
                           spi_.transfer(((C) & 0x07E0)>>3); \
                           spi_.transfer(((C) & 0x001F)<<3)

  // Convert swapped byte 16 bit colour to 18 bit and write in 3 bytes
  #define tft_Write_16S(C) spi_.transfer((C) & 0xF8); \
                           spi_.transfer(((C) & 0xE000)>>11 | ((C) & 0x07)<<5); \
                           spi_.transfer(((C) & 0x1F00)>>5)
  // Write 32 bits to TFT
  #define tft_Write_32(C)  spi_.transfer16((C)>>16); spi_.transfer16((uint16_t)(C))

  // Write two address coordinates
  #define tft_Write_32C(C,D) spi_.transfer16(C); spi_.transfer16(D)

  // Write same value twice
  #define tft_Write_32D(C) spi_.transfer16(C); spi_.transfer16(C)

////////////////////////////////////////////////////////////////////////////////////////
// Macros to write commands/pixel colour data to other displays
////////////////////////////////////////////////////////////////////////////////////////
#else
  #if  defined (RPI_DISPLAY_TYPE) // RPi TFT type always needs 16 bit transfers
    #define tft_Write_8(C)     spi_.transfer(0); spi_.transfer(C)
  #else
    #ifdef __AVR__ // AVR processors do not have 16 bit transfer
      #define tft_Write_8(C)   {SPDR=(C); while (!(SPSR&_BV(SPIF)));}
      #define tft_Write_16(C)  tft_Write_8((uint8_t)((C)>>8));tft_Write_8((uint8_t)((C)>>0))
      #define tft_Write_16S(C) tft_Write_8((uint8_t)((C)>>0));tft_Write_8((uint8_t)((C)>>8))
    #else
      // Write 8 bits to TFT
      #define tft_Write_8(C)   tft_write_a_byte(C)

      // Convert 16 bit colour to 18 bit and write in 3 bytes
      #define tft_Write_16(C)  tft_write_a_half(C)
      #define tft_Write_16S(C) tft_write_a_half((C)>>8 & 0xFF | (C)<<8)

      // Write 32 bits to TFT
      #define tft_Write_32(C)  tft_write_a_word(C)

      // Write two address coordinates
      #define tft_Write_32C(C,D) tft_write_a_word((C)<<16 | (D))
      
      // Write same value twice
      #define tft_Write_32D(C) tft_write_a_word((C)<<16 | (C))


    #endif // AVR    
  #endif // RPI_DISPLAY_TYPE
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Macros to read from display using SPI or software SPI
////////////////////////////////////////////////////////////////////////////////////////
#if defined (TFT_SDA_READ)
  // Use a bit banged function call for STM32 and bi-directional SDA pin
  #define TFT_eSPI_ENABLE_8_BIT_READ // Enable tft_Read_8(void);
  #define SCLK_L digitalWrite(TFT_SCLK, LOW)
  #define SCLK_H digitalWrite(TFT_SCLK, LOW)
#else
  // Use a SPI read transfer
//  #define tft_Read_8() spi_.transfer(0)
  #define tft_Read_8() tft_read_a_byte()
#endif


#endif // Header end
