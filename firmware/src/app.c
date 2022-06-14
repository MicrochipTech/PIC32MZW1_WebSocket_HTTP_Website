/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include <math.h>
#include <time.h>

// *****************************************************************************
/* Main timer resolution */
#define TIMER_RESOLUTION_MS         500

/* LEDs */
#define LED_SLOW_BLINK_PERIOD_MS        2000
#define LED_F_BLINK_PERIOD_MS           300

/* Sensors */
#define SENSORS_READ_FREQ_MS        2000

/* MCP9808 registers */
#define MCP9808_I2C_ADDRESS         0x18 
#define MCP9808_REG_CONFIG          0x01
#define MCP9808_REG_TAMBIENT		0x05
#define MCP9808_REG_MANUF_ID		0x06
#define MCP9808_REG_DEVICE_ID		0x07
#define MCP9808_REG_RESOLUTION		0x08

/* MCP9808 other settings */
#define MCP9808_CONFIG_DEFAULT		0x00
#define MCP9808_CONFIG_SHUTDOWN		0x0100
#define MCP9808_RES_DEFAULT         62500
#define MCP9808_MANUF_ID            0x54
#define MCP9808_DEVICE_ID           0x0400
#define MCP9808_DEVICE_ID_MASK		0xff00

/* OPT3001 registers */
#define OPT3001_I2C_ADDRESS             0x44
#define OPT3001_REG_RESULT              0x00
#define OPT3001_REG_CONFIG              0x01
#define OPT3001_REG_LOW_LIMIT           0x02
#define OPT3001_REG_HIGH_LIMIT          0x03
#define OPT3001_REG_MANUFACTURER_ID     0x7E
#define OPT3001_REG_DEVICE_ID           0x7F

/* MCP9808 other settings */
#define OPT3001_CONFIG_SHUTDOWN             0x00
#define OPT3001_CONFIG_CONT_CONVERSION		0xCE10        //continuous convesrion
#define OPT3001_MANUF_ID                    0x5449
#define OPT3001_DEVICE_ID                   0x3001

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
#define REPORT_MSG  "{\"time\": \"%d-%d-%dT%d:%d:%d\",\"temperature\": %d,\"lightIntensity\": %d}"

int gclient_index[SYS_WSS_MAX_NUM_CLIENTS];


// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/

/* I2C transfer callback */
static void i2cTransferCallback(DRV_I2C_TRANSFER_EVENT event, 
        DRV_I2C_TRANSFER_HANDLE transferHandle, 
        uintptr_t context){
    switch(event)
    {
        case DRV_I2C_TRANSFER_EVENT_COMPLETE:
            appData.i2c.transferStatus = I2C_TRANSFER_STATUS_SUCCESS;
            break;
        case DRV_I2C_TRANSFER_EVENT_ERROR:
            appData.i2c.transferStatus = I2C_TRANSFER_STATUS_ERROR;
            break;
        default:
            break;
    }
}


/* I2C read */
static bool i2cReadReg(uint8_t addr, uint16_t reg, uint8_t size){
    bool ret = false;
    appData.i2c.transferHandle = DRV_I2C_TRANSFER_HANDLE_INVALID;
    appData.i2c.txBuffer[0] = (uint8_t)reg;
    
    DRV_I2C_WriteReadTransferAdd(appData.i2c.i2cHandle, 
            addr, 
            (void*)appData.i2c.txBuffer, 1, 
            (void*)&appData.i2c.rxBuffer, size, 
            &appData.i2c.transferHandle);
    if(appData.i2c.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID)
    {
        SYS_CONSOLE_PRINT("I2C read reg %x error \r\n", reg);
        ret = false;
    }
    else
        ret = true;
    return ret;
}

/* I2C read complete */
static void i2cReadRegComp(uint8_t addr, uint8_t reg){
    appData.i2c.rxBuffer = (appData.i2c.rxBuffer << 8) | (appData.i2c.rxBuffer >> 8);
    //SYS_CONSOLE_PRINT("I2C read complete - periph addr %x val %x\r\n", addr, appData.i2c.rxBuffer);
    switch(addr)
    {   
        /* MCP9808 */
        case MCP9808_I2C_ADDRESS:
            if (reg == MCP9808_REG_TAMBIENT){
                uint8_t upperByte = (uint8_t)(appData.i2c.rxBuffer >> 8);
                uint8_t lowerByte = ((uint8_t)(appData.i2c.rxBuffer & 0x00FF));
                upperByte = upperByte & 0x1F;
                if ((upperByte & 0x10) == 0x10){         // Ta < 0 degC
                    upperByte = upperByte & 0x0F;       // Clear sign bit
                    appData.mcp9808.temperature = 256 - ((upperByte * 16) + lowerByte/16);
                }
                else{
                    appData.mcp9808.temperature = ((upperByte * 16) + lowerByte/16);
                }
#if APP_DEBUG                
                SYS_CONSOLE_PRINT("MCP9808 Temperature %d (C)\r\n", appData.mcp9808.temperature);   
#endif
            }
            else if (reg == MCP9808_REG_DEVICE_ID){
                appData.mcp9808.deviceID = appData.i2c.rxBuffer;
                SYS_CONSOLE_PRINT("MCP9808 Device ID %x\r\n", appData.mcp9808.deviceID);                
            }
            break;

        /* OPT3001 */
        case OPT3001_I2C_ADDRESS:
            if (reg == OPT3001_REG_RESULT){
                uint16_t m = appData.i2c.rxBuffer & 0x0FFF;
                uint16_t e = (appData.i2c.rxBuffer & 0xF000) >> 12;
                appData.opt3001.light = (m*pow(2,e))/100;
#if APP_DEBUG                 
                SYS_CONSOLE_PRINT("OPT3001 Light %d (lux)\r\n", appData.opt3001.light); 
#endif
            }
            else if (reg == OPT3001_REG_DEVICE_ID){
                appData.opt3001.deviceID = appData.i2c.rxBuffer;
                SYS_CONSOLE_PRINT("OPT3001 Device ID %x\r\n", appData.opt3001.deviceID);                
            }
            break;

        default:
            break;
    }
}

/* I2C write */
static bool i2cWriteReg(uint8_t addr, uint16_t reg, uint16_t val){
    bool ret = false;
    appData.i2c.transferHandle = DRV_I2C_TRANSFER_HANDLE_INVALID;
    appData.i2c.txBuffer[0] = (uint8_t)reg;
    appData.i2c.txBuffer[1] = (uint8_t)(val >> 8);
    appData.i2c.txBuffer[2] = (uint8_t)(val & 0x00FF);
    
    DRV_I2C_WriteTransferAdd(appData.i2c.i2cHandle, 
            addr, 
            (void*)appData.i2c.txBuffer, 3, 
            &appData.i2c.transferHandle);
    if(appData.i2c.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID)
    {
        SYS_CONSOLE_PRINT("I2C write reg %x error \r\n", reg);
        ret = false;
    }
    else
        ret = true;
    
    return ret;
}

/* I2C write complete */
static void i2cWriteRegComp(uint8_t addr, uint8_t reg){
#if APP_DEBUG 
    SYS_CONSOLE_PRINT("I2C write complete - periph addr %x\r\n", addr);
#endif
}

/* Main timer handler */
static void timeCallback(uintptr_t param) {
    
    appData.state = APP_STATE_READ_TEMP;
    
}
static void sntpCallback(TCPIP_SNTP_EVENT evType, const void* evParam)
{
	struct tm *lt;
    char       buf[80];
    
    if (evType == TCPIP_SNTP_EVENT_TSTAMP_OK)
    {
        TCPIP_SNTP_EVENT_TIME_DATA   *ntpData;
        ntpData = (TCPIP_SNTP_EVENT_TIME_DATA   *)evParam;
		
        lt = localtime((time_t *)&ntpData->tUnixSeconds);
        strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", lt);
        SYS_CONSOLE_PRINT("UTC time %s\n", buf);

        if (RTCC_TimeSet(lt) == false) {
            SYS_CONSOLE_PRINT("Error setting time\r\n");
            return;
        }
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    SYS_WSS_RESULT result;
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    /* TODO: Initialize your application's state machine and other
     * parameters.
     */

    result=SYS_WSS_register_callback(wss_user_callback, 0);
    if(SYS_WSS_SUCCESS== result){
        SYS_CONSOLE_PRINT("Registered call back with WSS service successfully\r\n");
    }
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */


void APP_Tasks ( void )
{
    char str[200];
    struct tm  time;
    int i = 0;

    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;

            /* initialize client index pool value*/
            for (i = 0; i < SYS_WSS_MAX_NUM_CLIENTS; i++)
            {
                gclient_index[i] = -1;
            }
            
            
            /* Open I2C driver client */
            appData.i2c.i2cHandle = DRV_I2C_Open( DRV_I2C_INDEX_0, DRV_IO_INTENT_READWRITE );
            if (appData.i2c.i2cHandle == DRV_HANDLE_INVALID)
            {
                SYS_CONSOLE_PRINT("Failed to open I2C driver for sensors reading\r\n");
                appData.state = APP_STATE_ERROR;
                
                return;
            }
           
            DRV_I2C_TransferEventHandlerSet(appData.i2c.i2cHandle, i2cTransferCallback, 0);


            /* Start a periodic timer to handle periodic events*/
            appData.timeHandle = SYS_TIME_CallbackRegisterMS(timeCallback, 
                                                                    (uintptr_t) 0, 
                                                                    TIMER_RESOLUTION_MS, 
                                                                    SYS_TIME_PERIODIC);
            if (appData.timeHandle == SYS_TIME_HANDLE_INVALID) {
                SYS_CONSOLE_PRINT("Failed creating a periodic timer \r\n");
                appData.state = APP_STATE_ERROR;
                return;
            }

            TCPIP_SNTP_HandlerRegister(sntpCallback);
            
            if (appInitialized)
            {

                appData.state = APP_STATE_TURN_ON_MCP9808;
            }

            break;
        }
      
        /* MCP9808 turn on */
        case APP_STATE_TURN_ON_MCP9808:
        {
#if APP_DEBUG             
            SYS_CONSOLE_PRINT("APP_CTRL_TURN_ON_MCP9808 \r\n");
#endif
            appData.i2c.transferStatus = I2C_TRANSFER_STATUS_IN_PROGRESS;
            if(i2cWriteReg(MCP9808_I2C_ADDRESS, MCP9808_REG_CONFIG, MCP9808_CONFIG_DEFAULT))
                appData.state = APP_STATE_WAIT_TURN_ON_MCP9808;
            else
                appData.state = APP_STATE_TURN_ON_OPT3001;
            break;
        }
        
        /* MCP9808 wait for turn on */
        case APP_STATE_WAIT_TURN_ON_MCP9808:
        {
#if APP_DEBUG             
            SYS_CONSOLE_PRINT("APP_CTRL_WAIT_TURN_ON_MCP9808 \r\n");
#endif
            if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_SUCCESS){
                appData.mcp9808.IsShutdown = false;
                i2cWriteRegComp(MCP9808_I2C_ADDRESS, MCP9808_REG_CONFIG);
                appData.state = APP_STATE_TURN_ON_OPT3001;
            }
            else if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_ERROR){
                SYS_CONSOLE_PRINT("I2C write MCP9808_REG_CONFIG error \r\n");
                appData.state = APP_STATE_TURN_ON_OPT3001;
            }
            break;
        }
        
        case APP_STATE_TURN_ON_OPT3001:
        {
            appData.i2c.transferStatus = I2C_TRANSFER_STATUS_IN_PROGRESS;
            appData.turnOnSensors = false;
            if(i2cWriteReg(OPT3001_I2C_ADDRESS, OPT3001_REG_CONFIG, OPT3001_CONFIG_CONT_CONVERSION))
                appData.state = APP_STATE_WAIT_TURN_ON_OPT3001;
            else
                appData.state = APP_STATE_READ_TEMP;
            break;
        }
        
        /* OPT3001 wait for turn on */
        case APP_STATE_WAIT_TURN_ON_OPT3001:
        {
            if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_SUCCESS){
                appData.opt3001.IsShutdown = false;
                i2cWriteRegComp(OPT3001_I2C_ADDRESS, OPT3001_REG_CONFIG);
                appData.state = APP_STATE_READ_TEMP;
            }
            else if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_ERROR){
                SYS_CONSOLE_PRINT("I2C write OPT3001_REG_CONFIG error \r\n");
                appData.state = APP_STATE_READ_TEMP;
            }
            break;
        }
        
        
        /* MCP9808 read ambient temperature */
        case APP_STATE_READ_TEMP:
        {
            /* Schedule MCP9808 temperature reading */
            appData.i2c.transferStatus = I2C_TRANSFER_STATUS_IN_PROGRESS;
            if(i2cReadReg(MCP9808_I2C_ADDRESS, MCP9808_REG_TAMBIENT, 2))
                appData.state = APP_STATE_WAIT_READ_TEMP;
            else
                appData.state = APP_STATE_READ_LIGHT;
            break;
        }
        
        /* MCP9808 wait for read ambient temperature */
        case APP_STATE_WAIT_READ_TEMP:
        {
            /* MCP9808 Temperature reading operation done */
            if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_SUCCESS){
                i2cReadRegComp(MCP9808_I2C_ADDRESS, MCP9808_REG_TAMBIENT);
                appData.state = APP_STATE_READ_LIGHT;
            }
            else if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_ERROR){
                SYS_CONSOLE_PRINT("I2C read temperature error \r\n");
                appData.state = APP_STATE_READ_LIGHT;
            }
            break;
        }
        /* OPT3001 read ambient light */
        case APP_STATE_READ_LIGHT:
        {
            /* Schedule OPT3001 light reading */
            appData.i2c.transferStatus = I2C_TRANSFER_STATUS_IN_PROGRESS;
            if(i2cReadReg(OPT3001_I2C_ADDRESS, OPT3001_REG_RESULT, 2))
                appData.state = APP_STATE_WAIT_READ_LIGHT;
            else
                appData.state = APP_STATE_WS_SEND;
            break;
        }
        
        /* OPT3001 wait for read ambient light */
        case APP_STATE_WAIT_READ_LIGHT:
        {
            /* OPT3001 Light reading operation done */
            if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_SUCCESS){
                i2cReadRegComp(OPT3001_I2C_ADDRESS, OPT3001_REG_RESULT);
                appData.state = APP_STATE_WS_SEND;
            }
            else if(appData.i2c.transferStatus == I2C_TRANSFER_STATUS_ERROR){
                SYS_CONSOLE_PRINT("I2C read light error \r\n");
                appData.state = APP_STATE_WS_SEND;
            }
            break;
        }
        
        case APP_STATE_WS_SEND:
        {

            
            RTCC_TimeGet(&time);
            sprintf(str, REPORT_MSG, time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, appData.mcp9808.temperature, appData.opt3001.light);
                
            for (i = 0; i < SYS_WSS_MAX_NUM_CLIENTS; i++)
            {
                if (gclient_index[i] != -1)
                {
#if APP_DEBUG                     
                    SYS_CONSOLE_PRINT("wss send msg: %s\r\n", str);
#endif
                    SYS_WSS_sendMessage(1, SYS_WSS_FRAME_TEXT, (uint8_t *) str , strlen(str), gclient_index[i]);

                }
            }       
            appData.state = APP_STATE_IDLE;
            
            break;
        }
        case APP_STATE_IDLE:
        {
            break;
        }   

        /* TODO: implement your application state machine.*/


        /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }


}


/*******************************************************************************
 End of File
 */
