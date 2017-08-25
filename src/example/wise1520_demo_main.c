
//*****************************************************************************
//                        ------ Main Function ------
//
// Description  : 1. Main flowchart of WISE-1520 for demo
//                2. Web service
//                3. OTA example
//                4. AgentLite example
//
// Company      : Advantech Co., Ltd.
// Department   : Embedded Core Group
// Author       : Will Chen
// Create Date  : 2015/12/28
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup WISE-1520 Demo
//! @{
//
//*****************************************************************************

//*****************************************************************************
//                 Included files
//*****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simplelink includes
#include "simplelink.h"

//driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "utils.h"
#include "prcm.h"

//Free_rtos/ti-rtos includes
#include "osi.h"

//Common interface file in CC3200 SDK
#include "common.h"
#include "gpio_if.h"

//Common interface file in advantech SDK
#include "platform/ti/cc3200mod/adv_uart_if.h"

//Board related files
#include "board/wise1520/wise1520_demo_pinmux.h"
#include "platform/ti/cc3200mod/system.h"

#ifdef OTA_APP_EXAMPLE
#include "flc_api.h"
#include "ota_api.h"
#endif /* endif OTA_APP_EXAMPLE */

#ifdef AGENT_APP_EXAMPLE
#include <stdint.h>
#include "hw_memmap.h"
#include "pin.h"
#include "gpio.h"
#include "sl_mqtt_client.h"
#include "wiseutility.h"
#include "wiseagentlite.h"
#include "wiseaccess.h"
#ifdef USE_SENSOR
#ifdef USE_FREERTOS
#include "sensor/ti/hdc1050f.h"
#else
#include "sensor/ti/hdc1050.h"
#endif /* endif USE_FREERTOS */
#endif /* endif USE_SENSOR */
#endif /* endif AGENT_APP_EXAMPLE */


//*****************************************************************************
//                 MACROs and needed structures
//*****************************************************************************
#ifdef USE_FREERTOS
#define RTOS_TYPE                   "FreeRTOS"
#else
#define RTOS_TYPE                   "TI-RTOS"
#endif /* endif USE_FREERTOS */
#define DEMO_VERSION	            "WISE-1520 v1.1.04"
#define MODELNAME   	    	    "WISE-1520"
#define DOMAINNAME   	    	    "wise1520.net"
#define DEF_USERNAME_PWD    	    "admin"

#define APP_TASK_PRIORITY           1
#define OSI_STACK_SIZE              2048
#define CONNECTION_TIMEOUT_COUNT    10000  /* 10 seconds */
#define TOKEN_ARRAY_SIZE            10
#define STRING_TOKEN_SIZE           10
#define AP_SSID_LEN_MAX             32
#define SCAN_TABLE_SIZE             20
#define USERNAME_LEN_MAX            32
#define PASSWORD_LEN_MAX            32
#define SECURITY_KEY_LEN_MAX        63
#define APP_CONFIG_FILE             "/sys/appcfg.bin"
#define SLEEP_TIME_100_MS           100

#ifdef OTA_APP_EXAMPLE
#define OTA_SERVER_NAME                 "api.dropbox.com"
#define OTA_SERVER_IP_ADDRESS           0x00000000
#define OTA_SERVER_SECURED              1
#define OTA_SERVER_REST_UPDATE_CHK      "/1/metadata/auto/" // returns files/folder list
#define OTA_SERVER_REST_RSRC_METADATA   "/1/media/auto"     // returns A url that serves the media directly
#define OTA_SERVER_REST_HDR             "Authorization: Bearer "
#define LOG_SERVER_NAME                 "api-content.dropbox.com"
#define OTA_SERVER_REST_FILES_PUT       "/1/files_put/auto/"
#define OTA_VENDOR_STRING               DEMO_VERSION
#define OTA_KEY_LEN_MAX					96
#define OTA_SERVER_NAME_LEN_MAX			128
#define OTA_VENDOR_STRING_MAX			20
#define OTA_ENABLE						0x01050001

// For ota example
//#define OTA_SERVER_REST_HDR_VAL       "L2hmpjDaviQAAAAAAAAAD_20bgzvb3xsOO5DalBsI5aGCb9ogLG8jz8GFBkcKkuT"
// For wise-test
//#define OTA_SERVER_REST_HDR_VAL 		"L2hmpjDaviQAAAAAAAAAMdP1nJ9qNg8Lxt85zWkQLRDnB_l6TgS1Quxtb_J4UL8I"
#endif /* endif OTA_APP_EXAMPLE */

#ifdef AGENT_APP_EXAMPLE
#define TASK_PRIORITY                   3 		// Background receive task priority
#define AGENT_REFLASH_TIME              30
#define AGENT_REFLASH_TIME_GAP		    1000
#define AGENT_SERVER_NAME_LEN_MAX		64

//PAAS1
//#define AGENT_DEFAULT_SERVER_NAME		"m2com-standard.eastasia.cloudapp.azure.com"
//PAAS2
#define AGENT_DEFAULT_SERVER_NAME		"wise-msghub.eastasia.cloudapp.azure.com"
#define PAAS_ID                         "0e95b665-3748-46ce-80c5-bdd423d7a8a5:b69f6f36-ad5b-4126-abfb-be5443a07c73"
#define PAAS_PW                         "24s04jv4mtb0oq4eiruehf0r22"

#endif /* endif AGENT_APP_EXAMPLE */

// Application specific status/error codes
typedef enum{
    // Choosing this number to avoid overlap w/ host-driver's error codes
    eStat_ConnectionFailed = -2000,
    eStat_ClientConnectionFailed = -2001,
    eStat_DeviceNotInStationMode = -2002,
    eStat_DeviceNotInAPMode = -2003,
    eStat_ScanFailed = -2004,
	eStat_StopDHCPFailed = -2005,
	eStat_StartDHCPFailed = -2006,
	eStat_StopHTTPFailed = -2007,
	eStat_StartHTTPFailed = -2008,

    eStat_CodeMax = -3000
}e_AppStatusCodes;

// Supported connection mode
typedef enum{
    eMode_Ap,
    eMode_Station
}e_Mode;

// Supported command input from web page
typedef enum{
    eWebCmd_None,
    eWebCmd_SetUsernamePwd,
    eWebCmd_RemoteReset,
    eWebCmd_OTA,
    eWebCmd_AgentServerName,
    eWebCmd_TotalCmd
}e_WebCmd;

#ifdef OTA_APP_EXAMPLE
// OTA configuration
typedef struct sOTACfg {
    int iEnable;
    unsigned char aucKey[OTA_KEY_LEN_MAX];
    unsigned char aucServerName[OTA_SERVER_NAME_LEN_MAX];
}TOTACfg;

// OTA status
typedef enum{
    eOTA_S_Idle,
    eOTA_S_NoUpdate,
    eOTA_S_ServerError,
    eOTA_S_DownloadOK,
    eOTA_S_Commit,
    eOTA_S_TotalStatus
}e_OTA_Status;
#endif /* endif OTA_APP_EXAMPLE */

#ifdef AGENT_APP_EXAMPLE
typedef struct sAgentCfg {
    char aucServerName[AGENT_SERVER_NAME_LEN_MAX];
}TAgentCfg;
#endif /* endif AGENT_APP_EXAMPLE */

typedef struct sAppCfg {
#ifdef OTA_APP_EXAMPLE
    TOTACfg tOTACfg;
#endif /* endif OTA_APP_EXAMPLE */
#ifdef AGENT_APP_EXAMPLE
    TAgentCfg tAgentCfg;   
#endif /* endif AGENT_APP_EXAMPLE */
    int iDummy;
}TAppCfg;

//*****************************************************************************
//                 Global variables
//*****************************************************************************
volatile unsigned char  g_ulStatus = 0;
volatile unsigned char g_ucProfileAdded = 1;
volatile unsigned char g_ucChangeUsrPwd = 0;
volatile unsigned char g_ucWebCmd = eWebCmd_None;
unsigned char g_ucConnectedToConfAP = 0;
unsigned char g_ucPriority = 0;
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned long  g_ulDeviceIP = 0; //Device IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
signed char g_cWlanSSID[AP_SSID_LEN_MAX];
signed char g_cWlanSecurityKey[SECURITY_KEY_LEN_MAX];
signed char g_cUsername[USERNAME_LEN_MAX];
signed char g_cPassword[PASSWORD_LEN_MAX];
signed char g_cTmpBuf[SSID_LEN_MAX+1];
SlSecParams_t g_SecParams;
Sl_WlanNetworkEntry_t g_NetEntries[SCAN_TABLE_SIZE];
char g_token_get [TOKEN_ARRAY_SIZE][STRING_TOKEN_SIZE] = {
            "__SL_G_US0", "__SL_G_US1", "__SL_G_US2", "__SL_G_US3", "__SL_G_US4",
            "__SL_G_US5", "__SL_G_US6", "__SL_G_US7", "__SL_G_US8", "__SL_G_US9"
};
static TAppCfg g_tAppCfg;
static long g_lWlanMode;
#ifdef USE_FREERTOS
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#endif

#ifdef OTA_APP_EXAMPLE
static OtaOptServerInfo_t g_otaOptServerInfo;
static OsiSyncObj_t g_NetStatSyncObj;
static void *g_pvOtaApp;
static char g_cOTADone = 0;
#endif /* endif OTA_APP_EXAMPLE */

#ifdef AGENT_APP_EXAMPLE

//*****************************************************************************
void GPIO_Write(uint16_t GpioId, uint16_t Value)
{
    //if(GpioId>=0 && GpioId<=1)
    switch(GpioId)
    {
        case 1:
        //
        // Configure PIN_15 for GPIO Input, GPIO22  --> GPIO0
        //
        MAP_PinTypeGPIO(PIN_15, PIN_MODE_0, false);
        MAP_GPIODirModeSet(GPIOA2_BASE, 0x40, GPIO_DIR_MODE_OUT);
        MAP_GPIOPinWrite(GPIOA2_BASE,  0x40, (Value==1)?0x40:0 );
        break;
        case 2:
        //
        // Configure PIN_18 for GPIO Input, GPIO28  --> GPIO1
        //
        MAP_PinTypeGPIO(PIN_18, PIN_MODE_0, false);
        MAP_GPIODirModeSet(GPIOA3_BASE, 0x10, GPIO_DIR_MODE_OUT);
        MAP_GPIOPinWrite(GPIOA3_BASE,  0x10, (Value==1)?0x10:0 );
        break;
    };
}

//*****************************************************************************
static int gpio_1 = 0;
void SetGPIO_1(WiseAgentData *data) {
    gpio_1 = (data->value != 0) ? 1 : 0;
    UART_PRINT("*************Set GPIO 1  to %d\r\n",gpio_1);
    GPIO_Write(1, gpio_1);
}

int GetGPIO_1() {
    UART_PRINT("*************Get GPIO 1  to %d\r\n",gpio_1);
    return gpio_1;
}

static int gpio_2 = 0;
void SetGPIO_2(WiseAgentData *data) {
    gpio_2 = (data->value != 0) ? 1 : 0;
    UART_PRINT("*************Set GPIO 2  to %d\r\n",gpio_2);
    GPIO_Write(2, gpio_2);
}

int GetGPIO_2() {
    UART_PRINT("*************Get GPIO 2  to %d\r\n",gpio_2);
    return gpio_2;
}

//*****************************************************************************
WiseSnail_InfoSpec interface[] = {
		{
				WISE_BOOL,
				"/Info/reset",
				"",
				0,
				0,
				1,
				"",
				NULL
		}
};

WiseSnail_InfoSpec infospec[] = {
		{
				WISE_FLOAT,
				"Temperature",
				"Cel",
				0,
				-100,
				200,
				"ucum.Cel"
		},
		{
				WISE_VALUE,
				"Humidity",
				"%",
				0,
				0,
				100,
				"ucum.%"
#if 0   //#samlin-
        },
        {
                WISE_BOOL,
                "GPIO1",
                "",
                0,
                0,
                1,
                "",
                SetGPIO_1
        },
        {
                WISE_BOOL,
                "GPIO2",
                "",
                0,
                0,
                1,
                "",
                SetGPIO_2
#endif
        }
};

WiseSnail_Data data[] = {
		{
				WISE_FLOAT,
				"Temperature",
				100
		},
		{
				WISE_VALUE,
				"Humidity",
				55
#if 0 //samlin-
		},
        {
                WISE_BOOL,
                "GPIO1",
                0
        },
        {
                WISE_BOOL,
                "GPIO2",
                0
#endif
        }
};

#endif /* endif AGENT_APP_EXAMPLE */


//*****************************************************************************
//            Local functions prototypes
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState();
static void BoardInit(void);
static long WlanConnect(int _iMode);
static long lRunStationMode(void);
static long lRunAPMode(void);
static int iSetUserPwd(char *_strUsername, char *_strPwd);
static int iRunWebCmd(void);
static int iSetToDefault(void);
static int iReadAppCfg(TAppCfg *_pCfg);
static int iWriteAppCfg(TAppCfg *_pCfg);

#ifdef OTA_APP_EXAMPLE
static int OTAServerInfoSet(void **pvOtaApp, char *vendorStr);
static int OTARun(void);
#endif /* endif OTA_APP_EXAMPLE */

#ifdef USE_FREERTOS
//*****************************************************************************
// FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
//*****************************************************************************

//*****************************************************************************
//
//! \brief Application defined hook (or callback) function - assert
//!
//! \param[in]  pcFile - Pointer to the File Name
//! \param[in]  ulLine - Line Number
//!
//! \return none
//!
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    //Handle Assert here
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined idle task hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationIdleHook( void)
{
    //Handle Idle Hook for Profiling, Power Management etc
}

//*****************************************************************************
//
//! \brief Application defined malloc failed hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationMallocFailedHook()
{
    //Handle Memory Allocation Errors
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined stack overflow hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationStackOverflowHook( OsiTaskHandle *pxTask,
                                   signed char *pcTaskName)
{
    //Handle FreeRTOS Stack Overflow
    while(1)
    {
    }
}
#endif //USE_FREERTOS


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            
            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'sl_protocol_wlanConnectAsyncResponse_t'-Applications
            // can use it if required
            //
            //  sl_protocol_wlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //
            
            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s ,"
                        "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
        	slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request, 
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION 
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info 
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);
            
            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
            g_ulDeviceIP = pNetAppEvent->EventData.ipAcquiredV4.ip;
            
            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;
            
            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
            "Gateway=%d.%d.%d.%d\n\r", 
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! This function gets triggered when HTTP Server receives Application
//! defined GET and POST HTTP Tokens.
//!
//! \param pSlHttpServerEvent - Pointer indicating http server event
//! \param pSlHttpServerResponse - Pointer indicating http server response
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent,
        SlHttpServerResponse_t *pSlHttpServerResponse)
{

    switch (pSlHttpServerEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [0], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                if(g_ucConnectedToConfAP == 1)
                {
                    // Important - Connection Status
                    memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                            "TRUE",strlen("TRUE"));
                    pSlHttpServerResponse->ResponseData.token_value.len = \
                                                                strlen("TRUE");
                }
                else
                {
                    // Important - Connection Status
                    memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                            "FALSE",strlen("FALSE"));
                    pSlHttpServerResponse->ResponseData.token_value.len = \
                                                                strlen("FALSE");
                }
            }

            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [1], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[0].ssid,g_NetEntries[0].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                      g_NetEntries[0].ssid_len;
            }
            
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [2], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[1].ssid,g_NetEntries[1].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                       g_NetEntries[1].ssid_len;
            }
            
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [3], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[2].ssid,g_NetEntries[2].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                       g_NetEntries[2].ssid_len;
            }
            
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [4], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[3].ssid,g_NetEntries[3].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                       g_NetEntries[3].ssid_len;
            }
            
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                            g_token_get [5], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[4].ssid,g_NetEntries[4].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                      g_NetEntries[4].ssid_len;
            }
            
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                        	g_token_get [6], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_NetEntries[5].ssid,g_NetEntries[5].ssid_len);
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                      g_NetEntries[5].ssid_len;
            }
            
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                        	g_token_get [7], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_cUsername, strlen((const char*)g_cUsername));
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                      strlen((const char*)g_cUsername);
            }
            
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                        	g_token_get [8], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        DEMO_VERSION, strlen(DEMO_VERSION));
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                                      strlen(DEMO_VERSION);
            }

#ifdef AGENT_APP_EXAMPLE
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                    	    g_token_get [9], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                        g_tAppCfg.tAgentCfg.aucServerName, \
                        strlen(g_tAppCfg.tAgentCfg.aucServerName));
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                    strlen(g_tAppCfg.tAgentCfg.aucServerName);
            }
#else
            if(0== memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, \
                    	    g_token_get [9], \
                            pSlHttpServerEvent->EventData.httpTokenName.len))
            {
                // Important - Token value len should be < MAX_TOKEN_VALUE_LEN
                memcpy(pSlHttpServerResponse->ResponseData.token_value.data, \
                		"rmm.wise-paas.com", \
                        strlen("rmm.wise-paas.com"));
                pSlHttpServerResponse->ResponseData.token_value.len = \
                                    strlen("rmm.wise-paas.com");
            }
#endif /* endif AGENT_APP_EXAMPLE */
            //else
            //    break;

        }
        break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {

            if((0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                          "__SL_P_USC", \
                 pSlHttpServerEvent->EventData.httpPostData.token_name.len)) && \
            (0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                     "Add", \
                     pSlHttpServerEvent->EventData.httpPostData.token_value.len)))
            {
                g_ucProfileAdded = 0;
            }
            
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                     "__SL_P_USD", \
                     pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                memcpy(g_cWlanSSID,  \
                pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                pSlHttpServerEvent->EventData.httpPostData.token_value.len);
                g_cWlanSSID[pSlHttpServerEvent->EventData.httpPostData.token_value.len] = 0;
            }

            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USE", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {

                if(pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] \
                                                                        == '0')
                {
                    g_SecParams.Type =  SL_SEC_TYPE_OPEN;
    
                }
                else if(pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] \
                                                                        == '1')
                {
                    g_SecParams.Type =  SL_SEC_TYPE_WEP;

                }
                else if(pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] == '2')
                {
                    g_SecParams.Type =  SL_SEC_TYPE_WPA;

                }
                else
                {
                    g_SecParams.Type =  SL_SEC_TYPE_OPEN;
                }
            }
            
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USF", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                memcpy(g_cWlanSecurityKey, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);
                g_cWlanSecurityKey[pSlHttpServerEvent->EventData.httpPostData.token_value.len] = 0;
                g_SecParams.Key = g_cWlanSecurityKey;
                g_SecParams.KeyLen = pSlHttpServerEvent->EventData.httpPostData.token_value.len;
            }
            
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                        "__SL_P_USG", \
                        pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                g_ucPriority = pSlHttpServerEvent->EventData.httpPostData.token_value.data[0] - 48;
                if(g_ucPriority > 7) g_ucPriority = 0;
            }

            // Apply button for username and password
            if((0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                          "__SL_P_USH", \
                 pSlHttpServerEvent->EventData.httpPostData.token_name.len)) && \
            (0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                     "Apply", \
                     pSlHttpServerEvent->EventData.httpPostData.token_value.len)))
            {
                //g_ucChangeUsrPwd = 1;
                g_ucWebCmd = eWebCmd_SetUsernamePwd;
            }

            // Saved username
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USI", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                memset(g_cUsername, 0, USERNAME_LEN_MAX);
                memcpy(g_cUsername, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);
            }

            // Saved password
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USJ", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                memset(g_cPassword, 0, PASSWORD_LEN_MAX);
                memcpy(g_cPassword, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);
            }

            // Remote reset button
            if((0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                          "__SL_P_USK", \
                 pSlHttpServerEvent->EventData.httpPostData.token_name.len)) && \
            (0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                     "Reset", \
                     pSlHttpServerEvent->EventData.httpPostData.token_value.len)))
            {
                g_ucWebCmd = eWebCmd_RemoteReset;
            }

#ifdef OTA_APP_EXAMPLE
            // Saved server name for OTA
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USM", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
            	memset(g_tAppCfg.tOTACfg.aucServerName, 0, OTA_SERVER_NAME_LEN_MAX);
                memcpy(g_tAppCfg.tOTACfg.aucServerName, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);
                    
            }

            // Saved key01 for OTA
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USN", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                memset(g_tAppCfg.tOTACfg.aucKey, 0, OTA_KEY_LEN_MAX);
                memcpy(g_tAppCfg.tOTACfg.aucKey, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);

            }

            // Saved key02 for OTA
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USO", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
                memcpy(&g_tAppCfg.tOTACfg.aucKey[1], \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);

            }

            // OTA button
            if((0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                        "__SL_P_USL", \
                        pSlHttpServerEvent->EventData.httpPostData.token_name.len)) && \
            (0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                        "Upgrade", \
                        pSlHttpServerEvent->EventData.httpPostData.token_value.len)))
            {
            	g_tAppCfg.tOTACfg.iEnable = OTA_ENABLE;
                g_ucWebCmd = eWebCmd_OTA;
            }
#endif /* endif OTA_APP_EXAMPLE */

#ifdef AGENT_APP_EXAMPLE
            // Saved server name for AgentLite
            if(0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                         "__SL_P_USQ", \
                         pSlHttpServerEvent->EventData.httpPostData.token_name.len))
            {
            	memset(g_tAppCfg.tAgentCfg.aucServerName, 0, AGENT_SERVER_NAME_LEN_MAX);
                memcpy(g_tAppCfg.tAgentCfg.aucServerName, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                    pSlHttpServerEvent->EventData.httpPostData.token_value.len);
                    
            }

            // AgentLite button
            if((0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, \
                        "__SL_P_USP", \
                        pSlHttpServerEvent->EventData.httpPostData.token_name.len)) && \
            (0 == memcmp(pSlHttpServerEvent->EventData.httpPostData.token_value.data, \
                        "Apply", \
                        pSlHttpServerEvent->EventData.httpPostData.token_value.len)))
            {
                g_ucWebCmd = eWebCmd_AgentServerName;
            }
#endif /* endif AGENT_APP_EXAMPLE */
        }
        break;
        
      default:
          break;
    }    
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status)
            {
                case SL_ECLOSE: 
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n", 
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default: 
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
        	UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
          break;
    }

}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************


//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     None
//
//****************************************************************************
static void InitializeAppVariables()
{
    g_ucProfileAdded = 1;
	g_ulGatewayIP = 0;
	g_ucChangeUsrPwd = 0;
    g_ucWebCmd = eWebCmd_None;
    memset(g_ucConnectionSSID, 0, sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID, 0, sizeof(g_ucConnectionBSSID));
}

//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy
//!           - Enable HTTP authentication
//!           - Check switch to run factory to default
//!           - Enable OTA
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. 
//!          On error, negative is returned.
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;
    _u8 u8Tmp;
    long lRetVal = -1;
    long lMode = -1;
    static int iDefaultDone = 0;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode 
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
            }
        }

        // Switch to STA role and restart 
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again 
        if (ROLE_STA != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in STA-mode 
            return eStat_DeviceNotInStationMode;
        }
    }
    
    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt, 
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);
    
    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);
    UART_PRINT("RTOS: %s\n\r", RTOS_TYPE);

    // Set connection policy 
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, 
                                SL_CONNECTION_POLICY(0, 0, 0, 0, 0), NULL, 0);
    							//SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal); 

    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore 
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(IS_CONNECTED(g_ulStatus))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
        }
    }

    //
    // Set default setting through checking application configuration file
    // whether exist or not.
    //
    memset(&g_tAppCfg, 0, sizeof(TAppCfg));
    lRetVal = iReadAppCfg(&g_tAppCfg);
    if(lRetVal <= 0 || (lDetectSW(eSW_BackupToDefault) && (!iDefaultDone))) {
#ifdef AGENT_APP_EXAMPLE
        strcpy((char *)g_tAppCfg.tAgentCfg.aucServerName, AGENT_DEFAULT_SERVER_NAME);
#endif /* endif AGENT_APP_EXAMPLE */
        lRetVal = iWriteAppCfg(&g_tAppCfg);
        ASSERT_ON_ERROR(lRetVal);
        lRetVal = iSetToDefault();
        ASSERT_ON_ERROR(lRetVal);
        iDefaultDone = 1;
        UART_PRINT("Backup to default setting!\n\r");
    }

    //
    // Enable http authentication
    //
    memset(g_cTmpBuf, 0, sizeof(g_cTmpBuf));
    u8Tmp = sizeof(g_cTmpBuf) - 1;
    lRetVal = sl_NetAppGet(SL_NET_APP_HTTP_SERVER_ID,
                 				NETAPP_SET_GET_HTTP_OPT_AUTH_CHECK,
           						&u8Tmp,
								(_u8*)g_cTmpBuf);

    if(g_cTmpBuf[0] != 't') {
    	u8Tmp = 1;
    	lRetVal = sl_NetAppSet(SL_NET_APP_HTTP_SERVER_ID,
             					NETAPP_SET_GET_HTTP_OPT_AUTH_CHECK,
								u8Tmp,
								(_u8*)"true");
    }
    ASSERT_ON_ERROR(lRetVal);


    //
    // Get username for web command
    //
    memset(g_cUsername, 0, USERNAME_LEN_MAX);
    u8Tmp = sizeof(g_cUsername) - 1;
    lRetVal = sl_NetAppGet(SL_NET_APP_HTTP_SERVER_ID,
                    NETAPP_SET_GET_HTTP_OPT_AUTH_NAME,
        		    (_u8*)&u8Tmp,
        			(_u8*)g_cUsername);
    ASSERT_ON_ERROR(lRetVal);

    //
    // Set HTTP port number
    //
    memset(g_cTmpBuf, 0, sizeof(g_cTmpBuf));
    u8Tmp = 2;
    if(g_lWlanMode) {
    	*((_u16*)g_cTmpBuf) = 80;
	}
    else {
    	*((_u16*)g_cTmpBuf) = 10000;
    }
    lRetVal = sl_NetAppSet(SL_NET_APP_HTTP_SERVER_ID,
       	    				NETAPP_SET_GET_HTTP_OPT_PORT_NUMBER,
       	           			u8Tmp,
       						(_u8*)g_cTmpBuf);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();
    
    return lRetVal; // Success
}



//****************************************************************************
//
//! Confgiures the mode in which the device will work
//!
//! \param iMode is the current mode of the device
//!
//!
//! \return   SlWlanMode_t
//! 				       
//
//****************************************************************************
static int ConfigureMode(int iMode)
{
    long   lRetVal = -1;

    lRetVal = sl_WlanSetMode(iMode);
    ASSERT_ON_ERROR(lRetVal);

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    //ASSERT_ON_ERROR(lRetVal);

    // reset status bits
    CLR_STATUS_BIT_ALL(g_ulStatus);

    return sl_Start(NULL,NULL,NULL);
}

//****************************************************************************
//
//! \brief Connecting to a WLAN Accesspoint
//!
//!  This function connects to the required AP (SSID_NAME)
//!
//! \param  _iMode: eMode_Ap or eMode_Station
//! 
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//!
//! \warning    If the WLAN connection fails or we don't aquire an IP 
//!            address, It will be stuck in this function forever.
//
//****************************************************************************
static long WlanConnect(int _iMode)
{
	long lRetVal = -1;
	unsigned int uiConnectTimeoutCnt = 0;
	
    if(_iMode == eMode_Ap) {
    	// Connecting to the Access point
    	lRetVal = sl_WlanConnect(g_cWlanSSID, strlen((char*)g_cWlanSSID), 0,
    		    	           	   &g_SecParams, 0);
    }
    else {
    	// Set AUTO policy
    	lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION ,
    	                    		  SL_CONNECTION_POLICY(1,0,0,0,0), 0, 0);
    }

  	UART_PRINT("Connecting...... \n\r");
    ASSERT_ON_ERROR(lRetVal);

    // Wait for WLAN Event and acquire IP address
    while(uiConnectTimeoutCnt < CONNECTION_TIMEOUT_COUNT &&
		((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))) 
    { 
        osi_Sleep(1); //Sleep 1 millisecond
        uiConnectTimeoutCnt++;
    }

    // Connection timeout
    if(uiConnectTimeoutCnt >= CONNECTION_TIMEOUT_COUNT)
    	return -1;

    // Add WiFi connectivity profile
    if(_iMode == eMode_Ap) {
    	lRetVal = sl_WlanProfileAdd(g_cWlanSSID, strlen((char*)g_cWlanSSID), 0,
                                        &g_SecParams, 0, g_ucPriority, 0);
    	if(lRetVal < 0) {
    		UART_PRINT("Add profile failed! \n\r");
    	    return -1;
    	}
    }

    return SUCCESS;
}



//*****************************************************************************
//
//! \brief This function Get the Scan Result
//!
//! \param[out]      netEntries - Scan Result 
//!
//! \return  On success, size of scan result array is returned.
//!          On error, negative is returned.
//!
//! \note
//!
//
//*****************************************************************************
static int GetScanResult(Sl_WlanNetworkEntry_t* netEntries )
{
    unsigned char   policyOpt;
    unsigned long IntervalVal = 60;
    int lRetVal;

    policyOpt = SL_CONNECTION_POLICY(0, 0, 0, 0, 0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION , policyOpt, NULL, 0);
	ASSERT_ON_ERROR(lRetVal);

    // enable scan
    policyOpt = SL_SCAN_POLICY(1);

    // set scan policy - this starts the scan
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt,
                            (unsigned char *)(IntervalVal), sizeof(IntervalVal));
	ASSERT_ON_ERROR(lRetVal);
		
	
    // delay 1 second to verify scan is started
    osi_Sleep(1000);

    // lRetVal indicates the valid number of entries
    // The scan results are occupied in netEntries[]
    lRetVal = sl_WlanGetNetworkList(0, SCAN_TABLE_SIZE, netEntries);
	ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    policyOpt = SL_SCAN_POLICY(0);

    // set scan policy - this stops the scan
    sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt,
                            (unsigned char *)(IntervalVal), sizeof(IntervalVal));
	ASSERT_ON_ERROR(lRetVal);	

    return lRetVal;

}

//****************************************************************************
//
//! Task function implements the ConnectionTask functionality
//!
//! \param none
//! 
//! This function  
//!    1. Starts device in STA Mode
//!    2. Scans and stores all the AP
//!    3. Switch to AP mode and wait for AP configuration from browser
//!    4. Switch to STA mode and connect to configured AP
//!
//! \return None.
//
//****************************************************************************
void ConnectionTask(void* pTaskParams)
{
	long lRetVal = -1;

	//
	// Get WLAN mode
	//
	g_lWlanMode = lGetModeStatus();

	//
	// Main loop
	//
    while(1)
    {
        InitializeAppVariables();
        
        lRetVal = ConfigureSimpleLinkToDefaultState();
        if(lRetVal < 0)
        {
            if (eStat_DeviceNotInStationMode == lRetVal)
            {
                UART_PRINT("Failed to configure the device in its default state\n\r");
            }

            osi_Sleep(SLEEP_TIME_100_MS);
            continue;
        }
        
        UART_PRINT("Device is configured in default state \n\r");
        
	    //
	    // Assumption is that the device is configured in station mode already
	    // and it is in its default state
	    //
	    lRetVal = sl_Start(0, 0, 0);
	    if (lRetVal < 0 || ROLE_STA != lRetVal)
	    {
	        UART_PRINT("Failed to start the device \n\r");
            osi_Sleep(SLEEP_TIME_100_MS);
            continue;
	    }
        
	    UART_PRINT("Device started as default setting \n\r");
        
	    //
	    // Check switch to detect which mode we are going to
	    //
        if(g_lWlanMode) {
            UART_PRINT("Running AP mode\n\r");
            lRetVal = lRunAPMode();
        }
        else {
            UART_PRINT("Running station mode\n\r");
            lRetVal = lRunStationMode();
        }

        if(lRetVal < 0)
            UART_PRINT("Return code:%d in main loop\n\r", lRetVal);

    } /* end while(1) */
}

//****************************************************************************
//
//! Task function implementing the OTA update functionality
//!
//! \param none
//!
//! \return None.
//
//****************************************************************************
#ifdef OTA_APP_EXAMPLE
void OTAUpdateTask(void *pvParameters)
{
    long lRetVal = -1;
    long OptionLen;
    unsigned char OptionVal;
    int SetCommitInt;

    if((strlen(DEMO_VERSION)) >= OTA_VENDOR_STRING_MAX) {
        UART_PRINT("OTA: Vendor string is too long! \n\r");
        LOOP_FOREVER();
    }

    while(1)
    {
        UART_PRINT("OTA: Waiting for next update! \n\r");
        osi_SyncObjWait(&g_NetStatSyncObj, OSI_WAIT_FOREVER);
        
        OTAServerInfoSet(&g_pvOtaApp, DEMO_VERSION);
        
        //
        // Check if this image is booted in test mode
        //
        sl_extLib_OtaGet(g_pvOtaApp, EXTLIB_OTA_GET_OPT_IS_PENDING_COMMIT,
                                 &OptionLen, &OptionVal);
        UART_PRINT("EXTLIB_OTA_GET_OPT_IS_PENDING_COMMIT? %d \n\r", OptionVal);
        if(OptionVal == true) {
        	UART_PRINT("OTA: PENDING COMMIT & WLAN OK ==> PERFORM COMMIT  \n\r");
	    	SetCommitInt = OTA_ACTION_IMAGE_COMMITED;
        	sl_extLib_OtaSet(g_pvOtaApp, EXTLIB_OTA_SET_OPT_IMAGE_COMMIT,
        						sizeof(int), (_u8 *)&SetCommitInt);
            g_cOTADone = eOTA_S_Commit;
        }
        else {
        	UART_PRINT("Starting OTA \n\r");
        	lRetVal = 0;
        
        	while(!lRetVal) {
        		lRetVal = sl_extLib_OtaRun(g_pvOtaApp);
        	}
        
        	UART_PRINT("OTA run = %d \n\r", lRetVal);
        	if(lRetVal < 0) {
        		UART_PRINT("OTA: Error with OTA server \n\r");
                g_cOTADone = eOTA_S_ServerError;
        	}
        	else if(lRetVal == RUN_STAT_NO_UPDATES) {
        		UART_PRINT("OTA: RUN_STAT_NO_UPDATES \n\r");
                g_cOTADone = eOTA_S_NoUpdate;
        	}
        	else if((lRetVal & RUN_STAT_DOWNLOAD_DONE)) {
        		// Set OTA File for testing
        		lRetVal = sl_extLib_OtaSet(g_pvOtaApp, EXTLIB_OTA_SET_OPT_IMAGE_TEST,
        									sizeof(int), (_u8 *)&SetCommitInt);
        
        		UART_PRINT("OTA: NEW Image download complete \n\r");
        
        		lRetVal = sl_WlanDisconnect();
        		if(0 == lRetVal)
        		{
        			// Wait
        		    while(IS_CONNECTED(g_ulStatus))
        		    {
#ifndef SL_PLATFORM_MULTI_THREADED
        		    	_SlNonOsMainLoopTask();
#endif
        		    }
        		}
        
        		//
        		// Stop the simplelink host
        		//
        		sl_Stop(SL_STOP_TIMEOUT);
        		UART_PRINT("OTA: Disconnect WLAN \n\r");
        
        		g_cOTADone = eOTA_S_DownloadOK;
        	}
        }
    } /* end while(1) */
}

//****************************************************************************
//
//! Sets the OTA server info and vendor ID
//!
//! \param pvOtaApp pointer to OtaApp handler
//! \param ucVendorStr vendor string
//!
//! This function sets the OTA server info and vendor ID.
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//****************************************************************************
static int OTAServerInfoSet(void **pvOtaApp, char *vendorStr)
{
	unsigned char macAddressLen = SL_MAC_ADDR_LEN;

	//
	// Set OTA server info
	//
	g_otaOptServerInfo.ip_address = OTA_SERVER_IP_ADDRESS;
	g_otaOptServerInfo.secured_connection = OTA_SERVER_SECURED;
	strcpy((char *)g_otaOptServerInfo.server_domain, OTA_SERVER_NAME);
	strcpy((char *)g_otaOptServerInfo.rest_update_chk, OTA_SERVER_REST_UPDATE_CHK);
	strcpy((char *)g_otaOptServerInfo.rest_rsrc_metadata, OTA_SERVER_REST_RSRC_METADATA);
	strcpy((char *)g_otaOptServerInfo.rest_hdr, OTA_SERVER_REST_HDR);
    strcpy((char *)g_otaOptServerInfo.rest_hdr_val, (const char*)g_tAppCfg.tOTACfg.aucKey);
	strcpy((char *)g_otaOptServerInfo.log_server_name, (const char*)g_tAppCfg.tOTACfg.aucServerName);
	strcpy((char *)g_otaOptServerInfo.rest_files_put, OTA_SERVER_REST_FILES_PUT);
	sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &macAddressLen, (_u8 *)g_otaOptServerInfo.log_mac_address);

	//
	// Set OTA server Info
	//
	sl_extLib_OtaSet(*pvOtaApp, EXTLIB_OTA_SET_OPT_SERVER_INFO,
	              	  sizeof(g_otaOptServerInfo), (_u8 *)&g_otaOptServerInfo);
	//
	// Set vendor ID.
	//
	sl_extLib_OtaSet(*pvOtaApp, EXTLIB_OTA_SET_OPT_VENDOR_ID, strlen(vendorStr),
                	(_u8 *)vendorStr);
	//
	// Return ok status
	//
	return RUN_STAT_OK;
}

//*****************************************************************************
//
//! Main flowchart on OTA
//!
//! \param  None
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//*****************************************************************************
static int OTARun(void)
{
    long lRetVal = -1;
    unsigned char ucConnected;

    // Read OTA configuration from flash
    memset(g_tAppCfg.tOTACfg.aucKey, 0, OTA_KEY_LEN_MAX);
    lRetVal = iReadAppCfg(&g_tAppCfg);
    if(lRetVal <= 0) {
        UART_PRINT("Reading OTA configuration failed! \n\r");
    }

    if(g_tAppCfg.tOTACfg.iEnable == OTA_ENABLE && lRetVal > 0) {
    	//
        // Configure to STA Mode
        // 
        if(ConfigureMode(ROLE_STA) != ROLE_STA)
        {
            UART_PRINT("Unable to set STA mode...\n\r");
            lRetVal = sl_Stop(SL_STOP_TIMEOUT);
            CLR_STATUS_BIT_ALL(g_ulStatus);
            return -1;
        }

        //
        // Connect to the Configured Access Point
        //
        UART_PRINT("Start to Wlan connect AP router ...\n\r");
        lRetVal = WlanConnect(eMode_Station);
        if(lRetVal < 0)
        {
        	ERR_PRINT(lRetVal);
            g_tAppCfg.tOTACfg.iEnable = 0;
            lRetVal = iWriteAppCfg(&g_tAppCfg);
            if(lRetVal < 0) {
                UART_PRINT("OTA: Write configuration failed! \n\r");
            }
            return -1;
        }

        ucConnected = IS_CONNECTED(g_ulStatus);
        CLR_STATUS_BIT_ALL(g_ulStatus);

        if(ucConnected) {
            g_cOTADone = eOTA_S_Idle;
            osi_SyncObjSignal(&g_NetStatSyncObj);
    	    UART_PRINT("Sync flag with task to firmware upgrade ...\n\r");
            while(!g_cOTADone) {
                osi_Sleep(100);
            }

            switch(g_cOTADone)
            {
                case eOTA_S_NoUpdate:
                    UART_PRINT("OTA: No new version firmware to upgrade ... \n\r");
                    g_tAppCfg.tOTACfg.iEnable = 0;
                    lRetVal = iWriteAppCfg(&g_tAppCfg);
                    if(lRetVal < 0) {
                        UART_PRINT("OTA: Write configuration failed! \n\r");
                        LOOP_FOREVER();
                    }
                    break;

                case eOTA_S_DownloadOK:
                    UART_PRINT("OTA: Rebooting after download complete... \n\r");
                    Sys_vRebootMCU();
                    break;

                case eOTA_S_Commit:
                    g_tAppCfg.tOTACfg.iEnable = 0;
                    lRetVal = iWriteAppCfg(&g_tAppCfg);
                    if(lRetVal < 0) {
                        UART_PRINT("OTA: Write configuration failed! \n\r");
                        LOOP_FOREVER();
                    }
                    break;

                case eOTA_S_ServerError:
                    UART_PRINT("OTA: Communication failed with OTA server! \n\r");
                    g_tAppCfg.tOTACfg.iEnable = 0;
                    lRetVal = iWriteAppCfg(&g_tAppCfg);
                    if(lRetVal < 0) {
                        UART_PRINT("OTA: Write configuration failed! \n\r");
                        LOOP_FOREVER();
                    }
                    break;

                default:
                    UART_PRINT("OTA: Unknow status... \n\r");
                    break;
            }
        }
    }

    return 0;
}
#endif /* endif OTA_APP_EXAMPLE */

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void BoardInit(void)
{

#ifdef USE_FREERTOS
//
// Set vector table base
//
#if defined(ccs)
        IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
        IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

void sleepOneSecond() {
	osi_Sleep(AGENT_REFLASH_TIME_GAP);
}
//*****************************************************************************
//
//! Running into station mode
//!
//! \param  None
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//*****************************************************************************
static long lRunStationMode(void)
{
    long lRetVal = -1;
#ifdef AGENT_APP_EXAMPLE
    int second = 0;
    float fTemperature;
    unsigned short int usiHumidity;
    unsigned char ucIsDhcp;
    unsigned char ucLen = sizeof(SlNetCfgIpV4Args_t);
    SlNetCfgIpV4Args_t tIPV4 = {0};
    _u8 au8MacAddress[SL_MAC_ADDR_LEN];
    _u8 u8MacAddressLen = SL_MAC_ADDR_LEN;
    char acIp[16] = {0};
    char mac[16] = {0};
    char snmac[16] = {0};
#endif /* AGENT_APP_EXAMPLE endif */

    //
    // Configure to STA Mode
    // 
    if(ConfigureMode(ROLE_STA) != ROLE_STA)
    {
        UART_PRINT("Unable to set STA mode...\n\r");
        lRetVal = sl_Stop(SL_STOP_TIMEOUT);
        CLR_STATUS_BIT_ALL(g_ulStatus);
        return eStat_DeviceNotInStationMode;
    }

    //
    // Connect to the Configured Access Point
    //
    UART_PRINT("Starting wlan to connect AP router...\n\r");
    lRetVal = WlanConnect(eMode_Station);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        return eStat_ConnectionFailed;
    }

    //
    // Stop Internal HTTP Server
    //
    lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
    if(lRetVal < 0)
    {
    	ERR_PRINT(lRetVal);
        return eStat_StopHTTPFailed;
    }

    //Start Internal HTTP Server
    lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
    if(lRetVal < 0)
    {
    	ERR_PRINT(lRetVal);
        return eStat_StartHTTPFailed;
    }

#ifdef AGENT_APP_EXAMPLE
    //
    // Initiation for sensor
    //
#ifdef USE_SENSOR
    HDC1050_Init();
#endif /* endif USE_SENSOR */

    //
    // Get IP address and MAC and pass it to agent
    //
    lRetVal = sl_NetCfgGet(SL_MAC_ADDRESS_GET,
                                NULL,
                                &u8MacAddressLen,
                                (_u8 *)au8MacAddress);
    ASSERT_ON_ERROR(lRetVal);

    GPIO_Write(1, 0); //Set GPIO default to 0
    GPIO_Write(2, 0); //Set GPIO default to 0

	sprintf(acIp, "%d.%d.%d.%d",
				SL_IPV4_BYTE(g_ulDeviceIP, 3), SL_IPV4_BYTE(g_ulDeviceIP, 2),
				SL_IPV4_BYTE(g_ulDeviceIP, 1), SL_IPV4_BYTE(g_ulDeviceIP, 0));

    //
    // Initiation for WISE Agent and creating connection to broker 
    //
	UART_PRINT("Agent server name:%s\n\r", g_tAppCfg.tAgentCfg.aucServerName);
	UART_PRINT("Agent product name:%s\n\r", MODELNAME);

	sprintf(mac, "%02X%02X%02X%02X%02X%02X", au8MacAddress[0], au8MacAddress[1],
			au8MacAddress[2], au8MacAddress[3], au8MacAddress[4],
			au8MacAddress[5]);

	WiseSnail_Init(MODELNAME,NULL, NULL, NULL, 0);
	WiseSnail_RegisterInterface(mac, "Ethernet", 0, interface, 1);

    //samlin+ forcibly use WISE-PaaS 2.0 url instead of reading config
	//if(WiseSnail_Connect(g_tAppCfg.tAgentCfg.aucServerName, 1883, PAAS_ID, PAAS_PW, NULL, 0) == 0) {
	if(WiseSnail_Connect(AGENT_DEFAULT_SERVER_NAME, 1883, PAAS_ID, PAAS_PW, NULL, 0) == 0) {
        //
        // No succesful connection to broker
        //
        return -1;
    }
    else {
		WiseSnail_RegisterSensor(mac, "OnBoard", infospec, sizeof(infospec)/sizeof(WiseSnail_InfoSpec));
    }

    while(1)
    {
        if(second == 0) {
#ifdef USE_SENSOR
            HDC1050_GetSensorData(&fTemperature, &usiHumidity);
#endif /* endif USE_SENSOR */
            data[0].value = fTemperature;
            data[1].value = usiHumidity;
#if 0   //samlin-
            data[2].value = GetGPIO_1();
            data[3].value = GetGPIO_2();
#endif
            //WiseSnail_Update(mac, data, 4);   //samlin-
            WiseSnail_Update(mac, data, 2);     //samlin+
        }
        WiseSnail_MainLoop(sleepOneSecond);
    	second = (second+1)%AGENT_REFLASH_TIME;

    	if(!IS_CONNECTED(g_ulStatus)) {
    	    CLR_STATUS_BIT_ALL(g_ulStatus);
    		return -1;
    	}
    } /* end while(1) */
    
#else /* No defined AGENT_APP_EXAMPLE */

    UART_PRINT("Loop forever in station mode ...\n\r");
    while(1)
    {
    	osi_Sleep(100);
    }
#endif /* AGENT_APP_EXAMPLE endif */
}

//*****************************************************************************
//
//! Set username and password
//!
//! \param _strUsername username string
//! \param _strPwd password string
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//*****************************************************************************
static int iSetUserPwd(char *_strUsername, char *_strPwd)
{
    int iRetVal = -1;
    int iLen;

    if(_strUsername == NULL || _strPwd == NULL) {
        UART_PRINT("Input string of username or password is null\n\r"); 
        return -1;
    }

    iLen = strlen(_strUsername);
    iRetVal = sl_NetAppSet(SL_NET_APP_HTTP_SERVER_ID,
            		      	  NETAPP_SET_GET_HTTP_OPT_AUTH_NAME,
							  iLen,
							  (_u8*)_strUsername);

    if(iRetVal != 0) 
            return iRetVal;

    iLen = strlen(_strPwd);
    iRetVal = sl_NetAppSet(SL_NET_APP_HTTP_SERVER_ID,
        		       	   	   NETAPP_SET_GET_HTTP_OPT_AUTH_PASSWORD,
							   iLen,
							   (_u8*)_strPwd);

    return iRetVal;
}

//*****************************************************************************
//
//! Running into AP mode
//!
//! \param  None
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//*****************************************************************************
static long lRunAPMode(void)
{
    volatile unsigned char ucWebCmd;
    long lCountSSID;
    long lRetVal = -1;

    unsigned char ucIsDhcp;
    unsigned char ucLen;
    int iIp0, iIp1, iIp2, iIp3, iGwIp0, iGwIp1, iGwIp2, iGwIp3;
    SlNetCfgIpV4Args_t tIPV4 = {0};

#ifdef OTA_APP_EXAMPLE
    lRetVal = OTARun();
    if(lRetVal != 0)
        return -1;
#endif /* endif OTA_APP_EXAMPLE */

    //
    // DHCP addresses must be in the subnet of AP IP address
    // Check subnet whether changed or not
    //
    ucIsDhcp = 0;
    ucLen = sizeof(SlNetCfgIpV4Args_t);
    lRetVal = sl_NetCfgGet(SL_IPV4_AP_P2P_GO_GET_INFO,
    	                    &ucIsDhcp,
    	                    &ucLen,
    	                    (unsigned char *)&tIPV4);
  	ASSERT_ON_ERROR(lRetVal);

    iIp3 = SL_IPV4_BYTE(tIPV4.ipV4, 3);
    iIp2 = SL_IPV4_BYTE(tIPV4.ipV4, 2);
    iIp1 = SL_IPV4_BYTE(tIPV4.ipV4, 1);
    iIp0 = SL_IPV4_BYTE(tIPV4.ipV4, 0);
    iGwIp3 = SL_IPV4_BYTE(tIPV4.ipV4Gateway, 3);
    iGwIp2 = SL_IPV4_BYTE(tIPV4.ipV4Gateway, 2);
    iGwIp1 = SL_IPV4_BYTE(tIPV4.ipV4Gateway, 1);
    iGwIp0 = SL_IPV4_BYTE(tIPV4.ipV4Gateway, 0);

    if(iIp3 != iGwIp3 || iIp2 != iGwIp2 || iIp1 != iGwIp1 || iIp0 != iGwIp0) {
        UART_PRINT("IP is changed on AP mode!\n\r");
        tIPV4.ipV4          = (_u32)SL_IPV4_VAL(iIp3, iIp2, iIp1, iIp0);
        tIPV4.ipV4Mask      = (_u32)SL_IPV4_VAL(255, 255, 255, 0);
        tIPV4.ipV4Gateway   = (_u32)SL_IPV4_VAL(iIp3, iIp2, iIp1, iIp0);
        tIPV4.ipV4DnsServer = (_u32)SL_IPV4_VAL(iIp3, iIp2, iIp1, iIp0);
        lRetVal = sl_NetCfgSet(SL_IPV4_AP_P2P_GO_STATIC_ENABLE,
        	                    IPCONFIG_MODE_ENABLE_IPV4,
        	                    sizeof(SlNetCfgIpV4Args_t),
        	                    (_u8 *)&tIPV4);
        ASSERT_ON_ERROR(lRetVal);
    }

    while(1)
    {     
        //Get Scan Result
        lCountSSID = GetScanResult(&g_NetEntries[0]);
        if(lCountSSID < 0)
        {
            ERR_PRINT(lRetVal);
            return eStat_ScanFailed;
        }
        
        //
        // Configure to AP Mode
        // 
        if(ConfigureMode(ROLE_AP) != ROLE_AP)
        {
            UART_PRINT("Unable to set AP mode...\n\r");
            lRetVal = sl_Stop(SL_STOP_TIMEOUT);
            CLR_STATUS_BIT_ALL(g_ulStatus);
            ERR_PRINT(lRetVal);
            return eStat_DeviceNotInAPMode;
        }

        while(!IS_IP_ACQUIRED(g_ulStatus))
        {
          //looping till ip is acquired
        }

        //Stop Internal DHCP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_DHCP_SERVER_ID);
        if(lRetVal < 0)
        {
            ERR_PRINT(lRetVal);
            return eStat_StopDHCPFailed;
        }

        //Start Internal DHCP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_DHCP_SERVER_ID);
        if(lRetVal < 0)
        {
            ERR_PRINT(lRetVal);
            return eStat_StartDHCPFailed;
        }

        //Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        if(lRetVal < 0)
        {
            ERR_PRINT(lRetVal);
            return eStat_StopHTTPFailed;
        }

        //Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        if(lRetVal < 0)
        {
            ERR_PRINT(lRetVal);
            return eStat_StartHTTPFailed;
        }

        //
        // Wait for AP Configuraiton, Open Browser and Configure AP
        //
        while(g_ucProfileAdded)
        {
            ucWebCmd = g_ucWebCmd;
            lRetVal = iRunWebCmd();             
            if(lRetVal < 0) {
                g_ucWebCmd = eWebCmd_None;
                UART_PRINT("Running web command:%d failed! \n\r", ucWebCmd);
            }
            else if(lRetVal > eWebCmd_None && lRetVal < eWebCmd_TotalCmd) {
                g_ucWebCmd = eWebCmd_None;
                UART_PRINT("Running web command:%d success! \n\r", ucWebCmd);

                if(lRetVal == eWebCmd_OTA) {
                    return 0;
                }
            }
                
            osi_Sleep(100); //Sleep 100 millisecond
        }

        g_ucProfileAdded = 1;
        g_ucConnectedToConfAP = 0;

        //
        // Configure to STA Mode
        // 
        if(ConfigureMode(ROLE_STA) != ROLE_STA)
        {
            UART_PRINT("Unable to set STA mode...\n\r");
            lRetVal = sl_Stop(SL_STOP_TIMEOUT);
            CLR_STATUS_BIT_ALL(g_ulStatus);
            ERR_PRINT(lRetVal);
            return eStat_DeviceNotInStationMode;
        }

        //
        // Connect to the Configured Access Point
        //
        UART_PRINT("Start to Wlan connect...\n\r");
        lRetVal = WlanConnect(eMode_Ap);
        if(lRetVal < 0)
        {
        	ERR_PRINT(lRetVal);
            return eStat_ConnectionFailed;
        }

        g_ucConnectedToConfAP = IS_CONNECTED(g_ulStatus);
        CLR_STATUS_BIT_ALL(g_ulStatus);

        break;
    }

	return 0;
}

//****************************************************************************
//
//! Running commands from web page. 
//!
//! \param none
//!
//! \return  On success, positive is returned.
//!          On error, negative is returned.
//
//****************************************************************************
static int iRunWebCmd(void)
{
    int iRet;

    switch(g_ucWebCmd)
    {
        case eWebCmd_SetUsernamePwd:
        	iRet = iSetUserPwd((char*)g_cUsername, (char*)g_cPassword);
            break;

        case eWebCmd_RemoteReset:
            Sys_vRebootMCU();
            break;

#ifdef OTA_APP_EXAMPLE
        case eWebCmd_OTA:
            iRet = iWriteAppCfg(&g_tAppCfg);
            iRet = (iRet > 0) ? 0 : -1;
            break;
#endif /* endif OTA_APP_EXAMPLE */

#ifdef AGENT_APP_EXAMPLE
        case eWebCmd_AgentServerName:
            iRet = iWriteAppCfg(&g_tAppCfg);
            iRet = (iRet > 0) ? 0 : -1;
            break;
#endif /* endif AGENT_APP_EXAMPLE */

        case eWebCmd_None:
            iRet = 0;
            break;
    }

    if(iRet != 0) 
        return -1;
    else 
        return (int)g_ucWebCmd;
}

//****************************************************************************
//
//! Running this function is to factory to default.
//!
//! \param none
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//****************************************************************************
static int iSetToDefault(void)
{
    int iRet;
    _u8 u8Val;
    _u8 au8_MacAddress[SL_MAC_ADDR_LEN];
    _u8 strSSID[32];
    _u8 u8Len;
    SlNetCfgIpV4Args_t tIPV4;
    SlNetAppDhcpServerBasicOpt_t tDhcpParams; 

    // 
    // Get mac address
    //
    u8Len = SL_MAC_ADDR_LEN;
    iRet = sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &u8Len, (_u8 *)au8_MacAddress);
    ASSERT_ON_ERROR(iRet);
    
    //
    // Set device name
    // Maximum length of 33 characters
    // Allowed characters in device name are: 'a - z' , 'A - Z' , '0-9' and '-'
    //
    iRet = sl_NetAppSet(SL_NET_APP_DEVICE_CONFIG_ID, 
                    NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN, 
                    strlen(MODELNAME), 
                    (_u8 *) MODELNAME);
    ASSERT_ON_ERROR(iRet);
    
    //
    // Set domain name
    //
    iRet = sl_NetAppSet(SL_NET_APP_DEVICE_CONFIG_ID,
    				NETAPP_SET_GET_DEV_CONF_OPT_DOMAIN_NAME,
					strlen(DOMAINNAME),
                    (_u8 *) DOMAINNAME);
    ASSERT_ON_ERROR(iRet);

    //
    // Set SSID
    // SSID string of 32 characters
    //
    memset(strSSID, 0, sizeof(strSSID));
    sprintf((char*)strSSID, "%s-%02X%02X%02X",
    		(char*)MODELNAME, au8_MacAddress[3], au8_MacAddress[4], au8_MacAddress[5]);

    iRet = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, 
                        (_u16)strlen((const char*)strSSID), 
                        strSSID);
    ASSERT_ON_ERROR(iRet);

    //
    // Set security type to open
    //
    u8Val = SL_SEC_TYPE_OPEN;
    iRet = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1, &u8Val);
    ASSERT_ON_ERROR(iRet);

    //
    // Set channel to  6
    //
    u8Val = 6;
    iRet = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_CHANNEL, 1, &u8Val);
    ASSERT_ON_ERROR(iRet);

    //
    // Set static network setting to 0.0.0.0 for station mode
    //
    tIPV4.ipV4          = (_u32)SL_IPV4_VAL(0, 0, 0, 0);
    tIPV4.ipV4Mask      = (_u32)SL_IPV4_VAL(0, 0, 0, 0);
    tIPV4.ipV4Gateway   = (_u32)SL_IPV4_VAL(0, 0, 0, 0);
    tIPV4.ipV4DnsServer = (_u32)SL_IPV4_VAL(0, 0, 0, 0);
    iRet = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_STATIC_ENABLE, 
                            IPCONFIG_MODE_ENABLE_IPV4,
                            sizeof(SlNetCfgIpV4Args_t),
                            (_u8 *)&tIPV4);
    ASSERT_ON_ERROR(iRet);

    //
    // Set IP address for AP mode
    // Take affect after reset
    //
    tIPV4.ipV4          = (_u32)SL_IPV4_VAL(192, 168, 1, 1);
    tIPV4.ipV4Mask      = (_u32)SL_IPV4_VAL(255, 255, 255, 0);
    tIPV4.ipV4Gateway   = (_u32)SL_IPV4_VAL(192, 168, 1, 1);
    tIPV4.ipV4DnsServer = (_u32)SL_IPV4_VAL(192, 168, 1, 1);

    iRet = sl_NetCfgSet(SL_IPV4_AP_P2P_GO_STATIC_ENABLE, 
                        IPCONFIG_MODE_ENABLE_IPV4,
                        sizeof(SlNetCfgIpV4Args_t),
                        (_u8 *)&tIPV4);
    ASSERT_ON_ERROR(iRet);

    //
    // Set DHCP Server (AP mode) parameters
    // Take affect after reset
    //
    u8Len = sizeof(SlNetAppDhcpServerBasicOpt_t); 
    tDhcpParams.lease_time      = 86400;
    tDhcpParams.ipv4_addr_start = SL_IPV4_VAL(192, 168, 1, 2);
    tDhcpParams.ipv4_addr_last  = SL_IPV4_VAL(192, 168, 1, 254);
    iRet = sl_NetAppSet(SL_NET_APP_DHCP_SERVER_ID, 
                        NETAPP_SET_DHCP_SRV_BASIC_OPT, 
                        u8Len, 
                        (_u8* )&tDhcpParams);
    ASSERT_ON_ERROR(iRet);

    //
    // Enable DHCP client
    //
    u8Val = 1;
    iRet = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE, 1, 1, &u8Val);
    ASSERT_ON_ERROR(iRet);

    //
    // Set username and passwoard
    //
    iRet = iSetUserPwd(DEF_USERNAME_PWD, DEF_USERNAME_PWD);
    ASSERT_ON_ERROR(iRet);

    //
    // Remove all WiFi connectivity profiles
    //
    iRet = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(iRet);
    
    return 0;
}

//*****************************************************************************
//
//! Read application configuration
//!
//! \param  _pCfg pointer to configuration
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//*****************************************************************************
static int iReadAppCfg(TAppCfg *_pCfg)
{
    int iRetVal;
    long lFileHandle;

    if(_pCfg == NULL) {
        UART_PRINT("Input argument is null\n\r");
        return -1;
    }

    //
    // Open OTA configuration for reading
    //
    iRetVal = sl_FsOpen((unsigned char *)APP_CONFIG_FILE,
                            FS_MODE_OPEN_READ,
                            NULL,
                            &lFileHandle);

    //
    // If successful, Reading OTA configuration
    //
    if(iRetVal == 0) {
        iRetVal = sl_FsRead(lFileHandle,
                            0,
                            (unsigned char *)_pCfg,
                            sizeof(TAppCfg));
        sl_FsClose(lFileHandle, NULL, NULL , 0);
        return iRetVal;
    }

    return -1;
}

//*****************************************************************************
//
//! Write application configuration
//!
//! \param  _pCfg pointer to configuration
//!
//! \return  On success, zero is returned.
//!          On error, negative is returned.
//
//*****************************************************************************
static int iWriteAppCfg(TAppCfg *_pCfg)
{
    int iRetVal;
    long lFileHandle;

    if(_pCfg == NULL) {
        UART_PRINT("Input argument is null\n\r");
        return -1;
    }

    //
    // Open OTA configuration for writing
    //
    iRetVal = sl_FsOpen((unsigned char *)APP_CONFIG_FILE,
                            FS_MODE_OPEN_WRITE,
                            NULL,
                            &lFileHandle); 

    //
    // If successful, Writing OTA configuration
    //
    if(iRetVal == 0) {
        iRetVal = sl_FsWrite(lFileHandle, 
                                0, 
                                (unsigned char *)_pCfg, 
                                sizeof(TAppCfg));
        sl_FsClose(lFileHandle, NULL, NULL , 0);
        return iRetVal;
    }

    //
    // If failed, Create a new OTA configuration
    //
    iRetVal = sl_FsOpen((unsigned char *)APP_CONFIG_FILE,
                                FS_MODE_OPEN_CREATE(sizeof(TAppCfg),
                                		_FS_FILE_OPEN_FLAG_COMMIT |
										_FS_FILE_PUBLIC_WRITE |
                                		_FS_FILE_PUBLIC_READ),
                                NULL,
                                &lFileHandle);

    if(iRetVal == 0) {
        iRetVal = sl_FsWrite(lFileHandle, 
                                0, 
                                (unsigned char *)_pCfg, 
                                sizeof(TAppCfg));
        sl_FsClose(lFileHandle, NULL, NULL , 0);
        return iRetVal;
    }

    return -1;
}

//*****************************************************************************
//
//! Check WiFi connectivity profile whether exist or not
//!
//! \param  none
//!
//! \return  On success, zero or positive is returned.
//!          On error, negative is returned.
//
//*****************************************************************************
#if 0
static int iCheckProfileExist(void)
{
    int i, iRet;
    SlSecParams_t tSecParams;
    _i16 i16Idx = 0, i16Len;
    _u32 u32Priority;

    for(i=0; i<7; i++) {
        iRet = sl_WlanProfileGet(i16Idx,
                                (_i8*)g_cTmpBuf,
                                &i16Len,
                                0,
                                &tSecParams, 
                                0,
                                &u32Priority);
        if(iRet >= 0) 
            return 0;
    }

    return -1;
}
#endif

//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//! This function  
//!    1. Invokes the connection task
//!    2. Invokes the OTA Task
//!
//! \return None.
//
//****************************************************************************
void main()
{
	long lRetVal = -1;

    //
    // Initialize Board configurations
    //
    BoardInit();
    
    //
    // Configure the pinmux settings for the peripherals exercised
    //
    PinMuxConfig();

    //
    // Configuring UART
    //
    InitTerm();

    // Display version information
    UART_PRINT("\f");
    UART_PRINT("\t\t *************************************************\n\r");
    UART_PRINT("\t\t\t\t");
    UART_PRINT(DEMO_VERSION);
    UART_PRINT("\n\r");
    UART_PRINT("\t\t *************************************************\n\r");

    //
    // Simplelinkspawntask
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }	

#ifdef OTA_APP_EXAMPLE
    //
    // Initialize OTA
    //
    g_pvOtaApp = sl_extLib_OtaInit(RUN_MODE_NONE_OS | RUN_MODE_BLOCKING,0);

    //
    // Create sync object to signal Sl_Start and Wlan Connect complete
    //
    osi_SyncObjCreate(&g_NetStatSyncObj);

    //
    // OTA Update Task
    //
    lRetVal = osi_TaskCreate(OTAUpdateTask,
                   	   	   	   (const signed char *)"OTA Update",
							   OSI_STACK_SIZE,
							   NULL,
							   APP_TASK_PRIORITY,
							   NULL);
    if(lRetVal < 0)
    {
    	ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }
#endif /* endif OTA_APP_EXAMPLE */

    //
    // Start the ConnectionTask task
    //        
	lRetVal = osi_TaskCreate(ConnectionTask,
			                 (signed char*)"Connection Setup",
                             OSI_STACK_SIZE,
							 NULL,
							 APP_TASK_PRIORITY,
							 NULL);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }
    
    //
    // Start the task scheduler
    //	
    osi_start();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
