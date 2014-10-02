#include "commands.h"
#include <stm32l1xx.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <string.h>
#include "options.h"
#include "spi.h"

/* -------- defines -------- */
#define SPI_DATA_LEN 256
/* -------- variables -------- */
uint8_t SPI1_tx_data[SPI_DATA_LEN];
uint8_t SPI1_rx_data[SPI_DATA_LEN];

/* -------- constants -------- */
static const char * const pcVersion = "0.0.1\r\n";
/* -------- functions -------- */

/**
  * @brief  Get value from single 0-F hex digit
  * @param  hex char
  * @retval integer value, -1 if not hex at input
  */

int8_t get_hex_val(char chr)
{
   if ((chr >= '0') && (chr <= '9'))
   {
      return chr-'0';
   }
   else if ((chr >= 'A') && (chr <= 'F'))
   {
      return chr-'A'+0x0A;
   }
   else if ((chr >= 'a') && (chr <= 'f'))
   {
      return chr-'a'+0x0A;
   }
   return -1;
}

/**
  * @brief  Converts two digit hex string to byte
  * @param  two hex chars (validity not checked)
  * @retval integer value.
  */

inline uint8_t get_hex_str_val(const char* str)
{
   return (get_hex_val(str[0])<<4) | get_hex_val(str[1]);
}

/**
  * @brief  Prints one byte hex value to string
  * @param  hex value, pointer to output string
  * @retval number of chars copied to string.
  */

static uint8_t print_hex_val(uint8_t data, char* dest)
{
   char t;
   t = data>>4;
   dest[0] = t > 9 ? t+'A'-0x0A : t+'0';
   t = data&0x0F;
   dest[1] = t > 9 ? t+'A'-0x0A : t+'0';
   return 2;
}

/**
  * @brief  Command ver: prints version string.
  * @param  CLI template
  * @retval CLI template
  */
static portBASE_TYPE prvVerCommand( char *pcWriteBuffer,
                             size_t xWriteBufferLen,
                             const char *pcCommandString )
{
   strcpy((char*)pcWriteBuffer, pcVersion);
   return pdFALSE;
}

/**
  * @brief  Command reset.
  * @param  CLI template
  * @retval CLI template
  */
static portBASE_TYPE prvResetCommand( char *pcWriteBuffer,
                             size_t xWriteBufferLen,
                             const char *pcCommandString )
{
   NVIC_SystemReset();
   return pdFALSE;
}

/**
  * @brief  Command set_cons_speed: sets console UART speed.
  * @param  CLI template
  * @retval CLI template
  */
static portBASE_TYPE prvSetConsSpeedCommand( char *pcWriteBuffer,
                             size_t xWriteBufferLen,
                             const char *pcCommandString )
{
   const char* param;
   BaseType_t  param_len;
   char        param_valid = 0;
   uint32_t    new_speed;

   param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);
   if (!strcmp(param, "4800"))
   {
      new_speed = 4800;
      param_valid = 1;
   }
   else if (!strcmp(param, "115200"))
   {
      new_speed = 115200;
      param_valid = 1;
   }

   if (param_valid)
   {
      SetOption(OPT_CONS_SPEED, &new_speed);
      sprintf(pcWriteBuffer, "New speed set, please reset CPU.\r\n");
   }
   else
   {
      sprintf(pcWriteBuffer, "Incorrect speed, supported: 4800|115200\r\n");
   }
   return pdFALSE;
}

/**
  * @brief  Command set_gps_speed: sets GPS UART speed.
  * @param  CLI template
  * @retval CLI template
  */
static portBASE_TYPE prvSetGPSSpeedCommand( char *pcWriteBuffer,
                             size_t xWriteBufferLen,
                             const char *pcCommandString )
{
   const char* param;
   BaseType_t  param_len;
   char        param_valid = 0;
   uint32_t    new_speed;

   param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);
   if (!strcmp(param, "4800"))
   {
      new_speed = 4800;
      param_valid = 1;
   }
   else if (!strcmp(param, "9600"))
   {
      new_speed = 9600;
      param_valid = 1;
   }

   if (param_valid)
   {
      SetOption(OPT_GPS_SPEED, &new_speed);
      sprintf(pcWriteBuffer, "New speed set, please reset CPU.\r\n");
   }
   else
   {
      sprintf(pcWriteBuffer, "Incorrect speed, supported: 4800|9600\r\n");
   }
   return pdFALSE;
}

/**
  * @brief  Command cons_speed: prints console UART speed.
  * @param  CLI template
  * @retval CLI template
  */
static portBASE_TYPE prvConsSpeedCommand( char *pcWriteBuffer,
                             size_t xWriteBufferLen,
                             const char *pcCommandString )
{
   uint32_t* cons_speed = (uint32_t*)GetOption(OPT_CONS_SPEED);

   if (cons_speed)
   {
      switch (*cons_speed)
      {
         case 4800:
         {
            sprintf(pcWriteBuffer, "4800\r\n");
            break;
         }
         case 115200:
         {
            sprintf(pcWriteBuffer, "115200\r\n");
            break;
         }
         default:
         {
            sprintf(pcWriteBuffer, "Invalid console speed.\r\n");
         }
      }
   }
   else
   {
       sprintf(pcWriteBuffer, "Invalid parameter.\r\n");
   }
   return pdFALSE;
}

/**
  * @brief  Command gps_speed: prints GPS UART speed.
  * @param  CLI template
  * @retval CLI template
  */
static portBASE_TYPE prvGPSSpeedCommand( char *pcWriteBuffer,
                             size_t xWriteBufferLen,
                             const char *pcCommandString )
{
   uint32_t* gps_speed = (uint32_t*)GetOption(OPT_GPS_SPEED);

   if (gps_speed)
   {
      switch (*gps_speed)
      {
         case 4800:
         {
            sprintf(pcWriteBuffer, "4800\r\n");
            break;
         }
         case 9600:
         {
            sprintf(pcWriteBuffer, "9600\r\n");
            break;
         }
         default:
         {
            sprintf(pcWriteBuffer, "Invalid GPS speed.\r\n");
         }
      }
   }
   else
   {
       sprintf(pcWriteBuffer, "Invalid parameter.\r\n");
   }
   return pdFALSE;
}

/**
  * @brief  Command spi1_send: send data over SPI1 bus.
  * @param  CLI template
  * @retval CLI template
  */
static portBASE_TYPE prvSPI1SendCommand( char *pcWriteBuffer,
                             size_t xWriteBufferLen,
                             const char *pcCommandString )
{
   const char* param;
   BaseType_t  param_len;
   int i;
   uint8_t     ctr;

   param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);

   /* check number of provided hex values */
   if (param_len%2 != 0)
   {
      sprintf(pcWriteBuffer, "Error: provide round bytes.\r\n");
      return pdFALSE;
   }

   /* check provided hex values */
   for (i=0; i<param_len; i++)
   {
       if (get_hex_val(param[i]) == -1)
       {
           sprintf(pcWriteBuffer, "Error: provide hex values only.\r\n");
           return pdFALSE;
       }
   }

   /* convert string to array */
   for (i=0; i<param_len; i=i+2)
   {
      SPI1_tx_data[i>>1] = get_hex_str_val(&param[i]);
   }

   SPI1_Send(SPI1_tx_data, SPI1_rx_data, param_len>>1);

   /* Print MISO output after transfer */
   ctr = 0;
   for (i=0 ; i < param_len>>1; i++)
   {
      ctr+= print_hex_val(SPI1_rx_data[i], &pcWriteBuffer[ctr]);
   }
   pcWriteBuffer[ctr++] = '\r'; pcWriteBuffer[ctr++] = '\n';
   pcWriteBuffer[ctr++] = '\0';

   return pdFALSE;
}

/* -------- additional command constants -------- */
static const CLI_Command_Definition_t ConsSpeedCommand =
{
    "cons_speed",
    "cons_speed: console USART speed\r\n",
    prvConsSpeedCommand,
    0
};

/* -------- additional command constants -------- */
static const CLI_Command_Definition_t SetConsSpeedCommand =
{
    "set_cons_speed",
    "set_cons_speed: set console USART speed: 4800|115200\r\n",
    prvSetConsSpeedCommand,
    1
};

static const CLI_Command_Definition_t GPSSpeedCommand =
{
    "gps_speed",
    "gps_speed: GPS USART speed\r\n",
    prvGPSSpeedCommand,
    0
};

static const CLI_Command_Definition_t SetGPSSpeedCommand =
{
    "set_gps_speed",
    "set_gps_speed: set GPS USART speed: 4800|9600\r\n",
    prvSetGPSSpeedCommand,
    1
};

static const CLI_Command_Definition_t ResetCommand =
{
    "reset",
    "reset: CPU reset\r\n",
    prvResetCommand,
    0
};

static const CLI_Command_Definition_t VerCommand =
{
    "ver",
    "ver: version number\r\n",
    prvVerCommand,
    0
};

static const CLI_Command_Definition_t SPI1SendCommand =
{
    "spi1",
    "spi1 hex_vals: send data over SPI1\r\n",
    prvSPI1SendCommand,
    1
};

/**
  * @brief  Function registers all console commands.
  * @param  None
  * @retval None
  */
void RegisterCommands(void)
{
   /* The commands are displayed in help in the order provided here */
   FreeRTOS_CLIRegisterCommand(&ConsSpeedCommand);
   FreeRTOS_CLIRegisterCommand(&GPSSpeedCommand);
   FreeRTOS_CLIRegisterCommand(&ResetCommand);
   FreeRTOS_CLIRegisterCommand(&SetConsSpeedCommand);
   FreeRTOS_CLIRegisterCommand(&SetGPSSpeedCommand);
   FreeRTOS_CLIRegisterCommand(&SPI1SendCommand);
   FreeRTOS_CLIRegisterCommand(&VerCommand);
}