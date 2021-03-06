/*
 * wiseagentlite.c
 *
 *  Created on: 2015¦~11¤ë13¤é
 *      Author: Fred.Chang
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "snail_version.h"
#include "wisememory.h"
#include "wiseutility.h"
//#include "wisemqtt.h"
#include "wiseagentlite.h"
#include "wiseaccess.h"
#include "WISECore.h"

/*===============================================================*/

#define INTERFACE "Ethernet"
#define VERSION "3.1.23"
#define MY_PRODUCT_NAME 		"sample001"
#define MY_MANUFACTURE_NAME 	"advantech"
#define MY_SEN_SN 				"111122220056"
#define MY_SEN_MAC 				"111122220057"

//[WillMessage]
///cagent/admin/000000049F0130E0/willmessage
//static const char *WILLMESSAGE_JSON = "{\"susiCommData\":{\"devID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\"mac\":\"%s\",\"version\":\""VERSOIN"\",\"type\":\"IoTGW\",\"product\":\"\",\"manufacture\":\"\",\"account\":\"%s\",\"passwd\":\"%s\",\"status\":%d,\"commCmd\":1,\"requestID\":21,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//@@@ gwMac[s], hostname[s], Mac[s], Mac[s], username[s]{anonymous}, password[s], 0|1(connect or disconnect), gwMac[s], timestamp[d]

//[Connect]
///cagent/admin/000000049F0130E0/agentinfoack
//static const char *CONNECT_JSON = "{\"susiCommData\":{\"devID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\"mac\":\"%s\",\"version\":\""VERSOIN"\",\"type\":\"IoTGW\",\"product\":\"\",\"manufacture\":\"\",\"account\":\"%s\",\"passwd\":\"%s\",\"status\":%d,\"commCmd\":1,\"requestID\":21,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//@@@ gwMac[s], hostname[s], Mac[s], Mac[s], username[s]{anonymous}, password[s], 0|1(connect or disconnect), gwMac[s], timestamp[d]

//[OSINFO]
///cagent/admin/000000049F0130E0/agentactionreq
//static const char *OSINFO_JSON = "{\"susiCommData\":{\"osInfo\":{\"cagentVersion\":\""VERSOIN"\",\"cagentType\":\"IoTGW\",\"osVersion\":\"\",\"biosVersion\":\"\",\"platformName\":\"\",\"processorName\":\"\",\"osArch\":\"RTOS\",\"totalPhysMemKB\":1026060,\"macs\":\"%s\",\"IP\":\"%s\"},\"commCmd\":116,\"requestID\":109,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//@@@ Mac[n], ip[s], gwMac[s], timestamp[d]

static const char *SENDATA_JSON = "\"SenData\":{\"e\":[%s],\"bn\":\"SenData\"}";
static const char *INFO_JSON = "\"Info\":{\"e\":[%s],\"bn\":\"Info\"}";
static const char *NET_JSON = "\"Net\":{\"e\":[%s],\"bn\":\"Net\"}";
static const char *ACTION_JSON = "\"Action\":{\"e\":[%s],\"bn\":\"Action\"}";

//[INFOSPEC]
///cagent/admin/000000049F0130E0/agentactionreq
//PAAS1
//static const char *INFOSPEC_JSON = "{\"susiCommData\":{\"infoSpec\":{\"IoTGW\":{\"%s\":{\"%s\":{\"Info\":{\"e\":[%s],\"bn\":\"Info\"},\"bn\":\"%s\",\"ver\":1},\"bn\":\"%s\",\"ver\":1},\"ver\":1}},\"commCmd\":2052,\"requestID\":2001,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//PAAS2
static const char *INFOSPEC_JSON = "{\"content\":{\"%s\":{\"%s\":{\"%s\":{\"Info\":{\"e\":[%s],\"bn\":\"Info\"},\"bn\":\"%s\",\"ver\":1},\"bn\":\"%s\",\"ver\":1},\"ver\":1}},\"commCmd\":2052,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":%lld}}";
//@@@ Interface[s], Interface[s], InterfaceNumber[d], SenHubList[sl], Topology[sl], Interface[s], InterfaceNumber[d], Health[d], wsnMac[s], Interface[s], gwMac[s], timestamp[d]


//[DEVICEINFO]
///cagent/admin/000000049F0130E0/deviceinfo
//PAAS1
//static const char *DEVICEINFO_JSON = "{\"susiCommData\":{\"data\":{\"IoTGW\":{\"%s\":{\"%s\":{\"Info\":{\"e\":[%s],\"bn\":\"Info\"},\"bn\":\"%s\",\"ver\":1},\"bn\":\"%s\"},\"ver\":1}},\"commCmd\":2055,\"requestID\":2001,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//PAAS2
//static const char *DEVICEINFO_JSON = "{\"content\":{\"opTS\":{\"$date\":%lld},\"%s\":{\"%s\":{\"%s\":{%s,\"bn\":\"%s\",\"ver\":1},\"bn\":\"%s\"},\"ver\":1}},\"commCmd\":2055,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":%lld}}";
static const char *DEVICEINFO_JSON = "{\"content\":{\"opTS\":{\"$date\":1},\"%s\":{\"%s\":{\"%s\":{%s,\"bn\":\"%s\",\"ver\":1},\"bn\":\"%s\"},\"ver\":1}},\"commCmd\":2055,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":2}}";
//@@@ Interface[s], Interface[s], InterfaceNumber[d], SenHubList[sl], Topology[sl], Interface[s], InterfaceNumber[d], Health[d], wsnMac[s], Interface[s], gwMac[s], timestamp[d]


//[Sensor Connect]
///cagent/admin/00170d00006063c2/agentinfoack
//PAAS1
//static const char *SEN_CONNECT_JSON = "{\"susiCommData\":{\"devID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\"mac\":\"%s\",\"version\":\""VERSION"\",\"type\":\"SenHub\",\"product\":\"WISE-1020\",\"manufacture\":\"\",\"status\":\"1\",\"commCmd\":1,\"requestID\":30002,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//PAAS2
static const char *SEN_CONNECT_JSON = "{\"content\":{\"parentID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\"mac\":\"%s\",\"version\":\""VERSION"\",\"type\":\"SenHub\",\"product\":\""MY_PRODUCT_NAME"\",\"manufacture\":\""MY_MANUFACTURE_NAME"\",\"account\":\"\",\"passwd\":\"\",\"status\":1,\"tag\":\"device\"},\"commCmd\":1,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":%lld}}";
//@@@ sMac[s], hostname[s]{Agriculture}, sMac[s], sMac[s], sMac[s], timestamp[d]


//[Sensor INFOSPEC]
///cagent/admin/00170d00006063c2/agentactionreq
//PAAS1
//static const char *SEN_INFOSPEC_JSON = "{\"susiCommData\":{\"infoSpec\":{\"SenHub\":{\"SenData\":{\"e\":[%s],\"bn\":\"SenData\"},\"Info\":{\"e\":[%s],\"bn\":\"Info\"},\"Net\":{\"e\":[%s],\"bn\":\"Net\"},\"Action\":{\"e\":[%s],\"bn\":\"Action\"},\"ver\":1}},\"commCmd\":2052,\"requestID\":2001,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//PAAS2
static const char *SEN_INFOSPEC_JSON = "{\"content\":{\"SenHub\":{\"SenData\":{\"e\":[%s],\"bn\":\"SenData\"},\"Info\":{\"e\":[%s],\"bn\":\"Info\"},\"Net\":{\"e\":[%s],\"bn\":\"Net\"},\"Action\":{\"e\":[%s],\"bn\":\"Action\"},\"ver\":1}},\"commCmd\":2052,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":{\"$date\":%lld}}";
//@@@ hostname[s]{Agriculture}, senData[ss], Health[d], Topology[sl], sMac[s], timestamp[d]

static const char *SEN_INFOSPEC_SENDATA_V_JSON = "{\"n\":\"%s\",\"u\":\"%s\",\"v\":%d,\"min\":%d,\"max\":%d,\"asm\":\"%s\",\"type\":\"d\",\"rt\":\"%s\",\"st\":\"ipso\",\"exten\":\"\"}";
static const char *SEN_INFOSPEC_SENDATA_FV_JSON = "{\"n\":\"%s\",\"u\":\"%s\",\"v\":%f,\"min\":%f,\"max\":%f,\"asm\":\"%s\",\"type\":\"d\",\"rt\":\"%s\",\"st\":\"ipso\",\"exten\":\"\"}";
static const char *SEN_INFOSPEC_SENDATA_SV_JSON = "{\"n\":\"%s\",\"u\":\"%s\",\"sv\":\"%s\",\"min\":%d,\"max\":%d,\"asm\":\"%s\",\"type\":\"s\",\"rt\":\"%s\",\"st\":\"ipso\",\"exten\":\"\"}";
static const char *SEN_INFOSPEC_SENDATA_BV_JSON = "{\"n\":\"%s\",\"u\":\"%s\",\"bv\":%s,\"min\":false,\"max\":true,\"asm\":\"%s\",\"type\":\"b\",\"rt\":\"%s\",\"st\":\"ipso\",\"exten\":\"\"}";

/*
@@@@@ senData
{"n":"Visible","u":"lx","v":0,"min":0,"max":2000,"asm":"r","type":"d","rt":"ucum.lx","st":"ipso","exten":""}
{"n":"Infrared","u":"lx","v":0,"min":0,"max":600,"asm":"r","type":"d","rt":"ucum.lx","st":"ipso","exten":""}
=========================
{\"n\":\"%s\",\"u\":\"%s\",\"v\":%d,\"min\":%d,\"max\":%d,\"asm\":\"r\",\"type\":\"d\",\"rt\":\"%s\",\"st\":\"ipso\",\"exten\":\"\"}
{\"n\":\"%s\",\"u\":\"%s\",\"sv\":%s,\"min\":%d,\"max\":%d,\"asm\":\"r\",\"type\":\"d\",\"rt\":\"%s\",\"st\":\"ipso\",\"exten\":\"\"}
{\"n\":\"%s\",\"u\":\"%s\",\"bv\":%d,\"min\":%d,\"max\":%d,\"asm\":\"r\",\"type\":\"d\",\"rt\":\"%s\",\"st\":\"ipso\",\"exten\":\"\"}
*/


//[Sensor DEVICEINFO]
///cagent/admin/00170d00006063c2/deviceinfo
//PAAS1
//static const char *SEN_DEVINFO_JSON = "{\"susiCommData\":{\"data\":{\"SenHub\":{\"SenData\":{\"e\":[%s],\"bn\":\"SenData\"},\"Info\":{\"e\":[%s],\"bn\":\"Info\"},\"Net\":{\"e\":[%s],\"bn\":\"Net\"},\"Action\":{\"e\":[%s],\"bn\":\"Action\"},\"ver\":1}},\"commCmd\":2055,\"requestID\":2001,\"agentID\":\"%s\",\"handlerName\":\"general\",\"sendTS\":%d}}";
//PAAS2
static const char *SEN_DEVINFO_JSON = "{\"agentID\":\"%s\",\"commCmd\":2055,\"handlerName\":\"general\",\"content\":{\"opTS\":{\"$date\":3},\"SenHub\":{\"%s\":{\"bn\":\"%s\",\"e\":[%s]},\"ver\":1}},\"sendTS\":{\"$date\":4}}";
//@@@ senData[ss], Health[d], Topology[sl], sMac[s], timestamp[d]

static const char *SEN_DEVINFO_SENDATA_V_JSON = "{\"n\":\"%s\",\"v\":%d}";
static const char *SEN_DEVINFO_SENDATA_FV_JSON = "{\"n\":\"%s\",\"v\":%f}";
static const char *SEN_DEVINFO_SENDATA_SV_JSON = "{\"n\":\"%s\",\"sv\":\"%s\"}";
static const char *SEN_DEVINFO_SENDATA_BV_JSON = "{\"n\":\"%s\",\"bv\":%s}";
/*
@@@@@
{"n":"Visible","v":2000}
{"n":"Infrared","v":312}
==============================
{\"n\":\"%s\",\"v\":%d}
{\"n\":\"%s\",\"sv\":%s}
{\"n\":\"%s\",\"bv\":%d}
*/

/*=====================================================================================*/

/*=====================================================================================*/

/*Defining Broker IP address and port Number*/
static char projectName[65] = {0};
static char groupName[65] = {0};
static char parentId[DEVICE_ID_MAX_LEN] = {0};
static char gatewayId[GATEWAY_ID_MAX_LEN] = {0};
static char interfaceId[INTERFACE_ID_MAX_LEN] = {0};
static char interfaceMac[INTERFACE_MAC_MAX_LEN] = {0};
static char interfaceName[INTERFACE_NAME_MAX_LEN] = {0};
static int interfaceNumber = -1;
static char interfaceTag[64] = {0};

static int timestamp = 160081020;


void DeviceIdRework (char *idin, char*idout)
{
	snprintf(idout,DEVICE_ID_MAX_LEN,"00000000-0000-0000-0000-%s",idin);
	ToUpper(idout);
}


void InterfaceIdRework (char *idin, char*idout)
{
	snprintf(idout,INTERFACE_ID_MAX_LEN,"00000000-0000-0000-0000-%s",idin);
	ToUpper(idout);
}

void GatewayIdRework (char *idin, char*idout)
{
	//snprintf(idout,GATEWAY_ID_MAX_LEN,"%s",idin);
	//snprintf(idout,GATEWAY_ID_MAX_LEN,"0000%s",idin);
	snprintf(idout,GATEWAY_ID_MAX_LEN,"00000000-0000-0000-0000-%s",idin);
	ToUpper(idout);
}

void WiseAgent_Init(char *productionName, char *wanIp, unsigned char *parentMac, WiseAgentInfoSpec *infospec, int count) {
	strncpy(projectName,productionName, sizeof(projectName));
	if(parentMac != NULL) {
		DeviceIdRework (parentMac, parentId);
	} else {
		memset(parentId,0,sizeof(parentId));
	}
	SetDeviceIpAddress(wanIp);
	memset(groupName,0,sizeof(groupName));	//samlin+
	strcpy(groupName, "IoTGW");				//samlin+
}

void WiseAgent_RegisterInterface(char *ifMac, char *ifName, int ifNumber, WiseAgentInfoSpec *infospec, int count) {
	int index = 0;
	WiseAgentInfoSpec *is;
    char *topic = NULL;
	if(interfaceMac[0] != 0) {
		return;
	}
	
	snprintf(interfaceMac,INTERFACE_MAC_MAX_LEN,"%s",ifMac);
	ToUpper(interfaceMac);
	GatewayIdRework (interfaceMac, gatewayId);
	
	//WiseAccess_Init(projectName,interfaceMac);
	WiseAccess_Init(projectName,gatewayId);
	//SetDeviceMacAddress(ifMac);

	InterfaceIdRework (interfaceMac, interfaceId);
	
	strncpy(interfaceName, ifName, INTERFACE_NAME_MAX_LEN);
	interfaceNumber = ifNumber;
	if(interfaceNumber == -1) {
		sprintf(interfaceTag,"%s",interfaceName);
	} else {
		sprintf(interfaceTag,"%s%d",interfaceName,interfaceNumber);
	}
    
	topic = (char *)WiseMem_Alloc(128);
    
	sprintf(topic,WA_SUB_CBK_TOPIC, interfaceId);
	core_subscribe(topic, 0);
	
	WiseAccess_InterfaceInit(interfaceId, interfaceTag);
	
	for(index = 0 ; index < count ; index++) {
		is = &infospec[index];
		if(is->name[0] == '/')
		{
			switch(is->type) {
				case WISE_VALUE:
                case WISE_FLOAT:
				case WISE_BOOL:
				case WISE_STRING:
					WiseAccess_AddItem(interfaceId, is->name, is);
					break;
				default:
					wiseprint("Infospec datatype error!!\n");
					infiniteloop();
					break;
			}
		}
	}
    
    WiseMem_Release();
}

/************************************************************/

void on_connect_cb(int socketfd, char* localip, int iplength)
{
	/*char strRecvTopic[256] = {0};
	sprintf(strRecvTopic, "/cagent/admin/%s/+", GW_MACADDRESS);
	core_subscribe(strRecvTopic, 0);*/

	wiseprint("CB_Connected \n");
}

void on_lostconnect_cb(int rc)
{
	wiseprint("CB_Lostconnect \n");
}

void on_disconnect_cb()
{
	wiseprint("CB_Disconnect \n");
}

void on_rename(const char* name, const int cmdid, const char* sessionid)
{
	wiseprint("rename to: %s\r\n", name);
	WiseAgentData data;
	data.string = (char *)name;
	SetGWName(&data);
	WiseAccess_AssignCmd(cmdid, -1, 0, 200, NULL, NULL, (char *)sessionid, NULL, NULL);
	return;
}

void on_update(const char* loginID, const char* loginPW, const int port, const char* path, const char* md5, const int cmdid, const char* sessionid)
{
	wiseprint("Update: %s, %s, %d, %s, %s\n", loginID, loginPW, port, path, md5);
	return;
}

void on_server_reconnect(const char* devid) {
	int d = WiseAccess_FindDevice(devid);
	if(WiseAccess_FindDevice(devid) > 0) {
		WiseAccess_AssignCmd(9999, d, 0, 200, devid, NULL, NULL, NULL, NULL);		
	} else {
		if(strncmp(gatewayId,devid,sizeof(gatewayId)) == 0) {
			WiseAccess_AssignCmd(9999, -1, 0, 200, devid, NULL, NULL, NULL, NULL);		
		}
	}
}

void on_query_heartbeatrate(const char* sessionid,const char* devid) {
	WiseAccess_AssignCmd(127, -1, 0, 200, NULL, devid, sessionid, NULL, NULL);	
}

void on_update_heartbeatrate(const int heartbeatrate, const char* sessionid, const char* devid) {
	WiseAgentData data;
	data.value = heartbeatrate;
	SetHeartBeatRate(&data);
	WiseAccess_AssignCmd(129, -1, 0, 200, NULL, devid, sessionid, NULL, NULL);
}

static int WiseAgent_ConnectBySSL(char *server_url, int port, char *username, char *password, WiseAgentInfoSpec *infospec, int count) {
	int iRet = 0;
	int i = 0;
	char *cafile = NULL;
	char *clientCertificate = NULL;
	char *clientKey = NULL;
	char *keyPassword = NULL;
    char *topic = NULL;
	char *message = NULL;
    char *senhublist = NULL;
    char *infoString = NULL;
    
    iRet = core_initialize(gatewayId, (char *)GetGWName(), interfaceMac);
    if(!iRet)
	{
    	wiseprint("Unable to initialize AgentCore.\n");
		wiseprint("WiseCore Error: %s\n", core_error_string_get());
		return 0;
	}

    core_connection_callback_set(on_connect_cb, on_lostconnect_cb, on_disconnect_cb, CmdReceive);

    core_action_callback_set(on_rename, on_update);

    core_server_reconnect_callback_set(on_server_reconnect);
	
	core_heartbeat_callback_set(on_query_heartbeatrate, on_update_heartbeatrate);

	core_product_info_set(interfaceMac, parentId, VERSION, "IoTGW", "", "");

	core_os_info_set("SnailOS", "Snail", 123, interfaceMac);

	core_platform_info_set("", "", "Snail");

	core_local_ip_set((interfaceNumber == -1 ? GetIp() : ""));

	for(i = 0 ; i < count ; i++) {
		if(strcmp(infospec[i].name, "@cafile") == 0) {
			cafile = infospec[i].string;
		}
		if(strcmp(infospec[i].name, "@clientCertificate") == 0) {
			clientCertificate = infospec[i].string;
		}
		if(strcmp(infospec[i].name, "@clientKey") == 0) {
			clientKey = infospec[i].string;
		}
		if(strcmp(infospec[i].name, "@keyPassword") == 0) {
			keyPassword = infospec[i].string;
		}
	}

	if(clientCertificate != NULL && clientKey != NULL) {
		core_tls_set(cafile, "", clientCertificate, clientKey, keyPassword);
	}
	
	iRet = core_connect(server_url, port, username, password);
	//iRet = core_connect("172.22.12.60", 1122, "", "");
	if(iRet){
		wiseprint("Connect to broker: %s\n", server_url);
	} else {
		wiseprint("Unable to connect to broker.\n");
		wiseprint("WiseCore Error: %s\n", core_error_string_get());
		return 0;
	}

	core_device_register();
	core_platform_register();
	
    //topic = (char *)WiseMem_Alloc(128);
	//Sub
    //sprintf(topic,WA_SUB_CBK_TOPIC, gatewayId);
    //WiseAccess_Register(topic);
    //core_subscribe(topic, 0);

	//
	// publish
	//
	/*sprintf(topic, WA_PUB_CONNECT_TOPIC, GW_MACADDRESS);
	sprintf(message,CONNECT_JSON, GW_MACADDRESS, GetGWName(), MACADDRESS, MACADDRESS, "anonymous", "", 1, GW_MACADDRESS, timestamp++);
	core_publish(topic, message, strlen(message), 0, 0);*/

	/*sprintf(topic, WA_PUB_ACTION_TOPIC, GW_MACADDRESS);
	sprintf(message,OSINFO_JSON, MACADDRESS_N, GetIp(), GW_MACADDRESS, timestamp++);
	core_publish(topic, message, strlen(message), 0, 0);*/

    topic = (char *)WiseMem_Alloc(128);
    message = (char *)WiseMem_Alloc(8192);
    senhublist = (char *)WiseMem_Alloc(4096);
    infoString = (char *)WiseMem_Alloc(1024);
    
	sprintf(topic, WA_PUB_ACTION_TOPIC, gatewayId);
	sprintf(senhublist, "%s", interfaceId);
	WiseAccess_GenerateTokenCapability(interfaceId, "Info", infoString, WiseMem_Size(infoString));
	//sprintf(message,INFOSPEC_JSON, interfaceName, interfaceTag, infoString, interfaceId, interfaceName, gatewayId, timestamp++);
	sprintf(message,INFOSPEC_JSON, groupName, interfaceName, interfaceId, infoString, interfaceId, interfaceName, gatewayId, timestamp++);
	core_publish(topic, message, strlen(message), 0, 0);
	//PAAS1
	//sprintf(topic, WA_PUB_DEVINFO_TOPIC, gatewayId);
	//PAAS2
	sprintf(topic, DEF_AGENTREPORT_TOPIC, gatewayId);

	WiseAccess_GenerateTokenDataInfo(interfaceId, "Info", infoString, WiseMem_Size(infoString));

	//PAAS1
	//sprintf(message,DEVICEINFO_JSON, interfaceName, interfaceTag, infoString, interfaceId, interfaceName, gatewayId, timestamp++);
	//PAAS2
	//sprintf(message,DEVICEINFO_JSON, timestamp++, groupName, interfaceName, interfaceId, infoString, interfaceId, interfaceName, gatewayId, timestamp++);
	sprintf(message,DEVICEINFO_JSON, groupName, interfaceName, interfaceId, infoString, interfaceId, interfaceName, gatewayId);

	core_publish(topic, message, strlen(message), 0, 0);
    
    WiseMem_Release();
	return 1;
}

int WiseAgent_Connect(char *server_url, int port, char *username, char *password, WiseAgentInfoSpec *infospec, int count) {
	return WiseAgent_ConnectBySSL(server_url, port, username, password, infospec, count);
}

int WiseAgent_PublishInterfaceInfoSpecMessage(char *gatewayId) {
    char *topic = (char *)WiseMem_Alloc(128);
    char *message = (char *)WiseMem_Alloc(8192);
    char *senhublist = (char *)WiseMem_Alloc(4096);
    char *infoString = (char *)WiseMem_Alloc(1024);
	sprintf(topic, WA_PUB_ACTION_TOPIC, gatewayId);
	sprintf(senhublist, "%s", interfaceId);
	WiseAccess_GenerateTokenCapability(interfaceId, "Info", infoString, WiseMem_Size(infoString));
	sprintf(message,INFOSPEC_JSON, interfaceName, interfaceTag, infoString, interfaceId, interfaceName, gatewayId, timestamp++);
	core_publish(topic, message, strlen(message), 0, 0);
    WiseMem_Release();
}

int WiseAgent_PublishInterfaceDeviceInfoMessage(char *gatewayId) {
    WiseAccess_GetTopology();
    
    char *topic = (char *)WiseMem_Alloc(128);
    char *message = (char *)WiseMem_Alloc(8192);
    char *infoString = (char *)WiseMem_Alloc(1024);
    
	//PAAS1
	//sprintf(topic, WA_PUB_DEVINFO_TOPIC, gatewayId);
	//PAAS2
	sprintf(topic, DEF_AGENTREPORT_TOPIC, gatewayId);
	WiseAccess_GenerateTokenDataInfo(interfaceId, "Info", infoString, WiseMem_Size(infoString));
	//PAAS1
	//sprintf(message,DEVICEINFO_JSON, interfaceName, interfaceTag, infoString, interfaceId, interfaceName, gatewayId, timestamp++);
	//PAAS2
	//sprintf(message,DEVICEINFO_JSON, timestamp++, groupName, interfaceName, interfaceId, infoString, interfaceId, interfaceName, gatewayId, timestamp++);
	sprintf(message,DEVICEINFO_JSON, groupName, interfaceName, interfaceId, infoString, interfaceId, interfaceName, gatewayId);
	core_publish(topic, message, strlen(message), 0, 0);
	WiseMem_Release();
}

int WiseAgent_PublishSensorConnectMessage(char *deviceId) {
    char *topic = (char *)WiseMem_Alloc(128);
    char *message = (char *)WiseMem_Alloc(8192);

	WiseAgentData shname;

	sprintf(topic, WA_PUB_CONNECT_TOPIC, deviceId);

	WiseAccess_Get(deviceId, "/Info/Name", &shname);
	//PAAS1
	//sprintf(message,SEN_CONNECT_JSON, deviceId, shname.string, deviceId, deviceId, deviceId, timestamp++);
	//PAAS2
	sprintf(message,SEN_CONNECT_JSON, parentId, shname.string, MY_SEN_SN, MY_SEN_MAC, deviceId, timestamp++);
	core_publish(topic, message, strlen(message), 0, 0);
    
    sprintf(topic,WA_SUB_CBK_TOPIC, deviceId);
	core_subscribe(topic, 0);
    
    WiseMem_Release();
}

void WiseAgent_RegisterSensor(char *deviceMac, char *defaultName, WiseAgentInfoSpec *infospec, int count) {
	int number = 0;
	int index = 0;
    char *topic = NULL;
    char *message = NULL;
    char *senhublist = NULL;
    char *infoString = NULL;
    char *netString = NULL;
    char *actionString = NULL;
	char *pos = senhublist;
	char *format;
	char *access = "rw";
	char deviceId[DEVICE_ID_MAX_LEN] = {0};
    
	WiseAgentInfoSpec *is;

	DeviceIdRework (deviceMac, deviceId);
	
    
	//sprintf(topic,WA_SUB_CBK_TOPIC, deviceId);
	//core_subscribe(topic, 0);
	
	WiseAccess_SensorInit(deviceId, defaultName);
    WiseMem_Release();
	WiseAgent_PublishInterfaceDeviceInfoMessage(gatewayId);

    for(index = 0 ; index < count ; index++) {
		is = &infospec[index];
		if(is->name[0] == '/') {
			switch(is->type) {
				case WISE_VALUE:
                case WISE_FLOAT:
				case WISE_BOOL:
					WiseAccess_AddItem(deviceId, is->name, is);
					break;
				case WISE_STRING:
					WiseAccess_AddItem(deviceId, is->name, is);
					break;
				default:
					wiseprint("Infospec datatype error!!\n");
					infiniteloop();
					break;
			}
		}
	}
    
    WiseAccess_ConnectionStatus(deviceId, 1);

	WiseAgent_PublishSensorConnectMessage(deviceId);

    topic = (char *)WiseMem_Alloc(128);
    message = (char *)WiseMem_Alloc(8192);
    senhublist = (char *)WiseMem_Alloc(4096);
    infoString = (char *)WiseMem_Alloc(1024);
    netString = (char *)WiseMem_Alloc(1024);
    actionString = (char *)WiseMem_Alloc(1024);
	pos = senhublist;
    
	for(index = 0 ; index < count ; index++) {
		is = &infospec[index];
		if(is->name[0] != '/') {
			if(number >= 1) pos += sprintf(pos,",");
			number++;
			switch(is->type) {
				case WISE_VALUE:
						format = (char *)SEN_INFOSPEC_SENDATA_V_JSON;
					break;
                case WISE_FLOAT:
						format = (char *)SEN_INFOSPEC_SENDATA_FV_JSON;
					break;
				case WISE_STRING:
						format = (char *)SEN_INFOSPEC_SENDATA_SV_JSON;
					break;
				case WISE_BOOL:
						format = (char *)SEN_INFOSPEC_SENDATA_BV_JSON;
					break;

			}

			if(is->setValue == NULL) {
				access = "r";
			} else {
				access = "rw";
				if(is->type == WISE_STRING) {
					if(is->getValue == NULL) {
						access = "r";
					}
				}
			}

			switch(is->type) {
				case WISE_VALUE:
					WiseAccess_AddItem(deviceId, is->name, is);
					pos += sprintf(pos, format, is->name, is->unit, (int)is->value, (int)is->min, (int)is->max, access, is->resourcetype);
					break;
                case WISE_FLOAT:
					WiseAccess_AddItem(deviceId, is->name, is);
					pos += sprintf(pos, format, is->name, is->unit, is->value, is->min, is->max, access, is->resourcetype);
					break;
				case WISE_BOOL:
					WiseAccess_AddItem(deviceId, is->name, is);
					pos += sprintf(pos, format, is->name, is->unit, is->value > 0 ? "true" : "false", access, is->resourcetype);
					break;
				case WISE_STRING:
					WiseAccess_AddItem(deviceId, is->name, is);
					pos += sprintf(pos, format, is->name, is->unit, is->string, (int)is->min, (int)is->max, access, is->resourcetype);
					break;
				default:
					wiseprint("Infospec datatype error!!\n");
					infiniteloop();
					break;
			}
		} 
	}

	sprintf(topic, WA_PUB_ACTION_TOPIC, deviceId);
	wiseprint("senhublist:\033[33m\"%s\"\033[0m\r\n", senhublist);
	WiseAccess_GenerateTokenCapability(deviceId, "Info", infoString, WiseMem_Size(infoString));
	WiseAccess_GenerateTokenCapability(deviceId, "Net", netString, WiseMem_Size(netString));
	WiseAccess_GenerateTokenCapability(deviceId, "Action", actionString, WiseMem_Size(actionString));
	wiseprint("infoString:\033[33m\"%s\"\033[0m\r\n", infoString);
	wiseprint("netString:\033[33m\"%s\"\033[0m\r\n", netString);
	wiseprint("actionString:\033[33m\"%s\"\033[0m\r\n", actionString);
	sprintf(message,SEN_INFOSPEC_JSON, senhublist, infoString, netString, actionString, deviceId, timestamp++);
	wiseprint("message:\033[33m\"%s\"\033[0m\r\n", message);
	core_publish(topic, message, strlen(message), 0, 0);
	WiseAccess_SetInfoSpec(deviceId, message, strlen(message)+1);
    WiseMem_Release();
}

void WiseAgent_SenHubDisconnect(char *deviceMac) {
	char deviceId[DEVICE_ID_MAX_LEN] = {0};

	DeviceIdRework (deviceMac, deviceId);

	if(WiseAccess_FindDevice(deviceId) < 0) return;
	
	WiseAccess_ConnectionStatus(deviceId, 0);

	WiseAgent_PublishInterfaceDeviceInfoMessage(gatewayId);
}

void WiseAgent_SenHubReConnected(char *deviceMac) {
	char deviceId[DEVICE_ID_MAX_LEN] = {0};

	DeviceIdRework (deviceMac, deviceId);

	if(WiseAccess_FindDevice(deviceId) < 0) return;
	
	WiseAccess_ConnectionStatus(deviceId, 1);

	WiseAgent_PublishInterfaceDeviceInfoMessage(gatewayId);
}


void WiseAgent_Write(char *deviceMac, WiseAgentData* data, int count) {
	int number = 0;
	int index = 0;
	char *pos = NULL;
	WiseAgentData* d;
	int otherInfo = 0;
	char deviceId[DEVICE_ID_MAX_LEN] = {0};
    char *topic = NULL;
	char *message = NULL;
    char *senhublist = NULL;
    char *infoString = NULL;
    char *netString = NULL;
    char *actionString = NULL;
	DeviceIdRework (deviceMac, deviceId);
	if(WiseAccess_FindDevice(deviceId) < 0) return;
	
    topic = (char *)WiseMem_Alloc(128);
    message = (char *)WiseMem_Alloc(8192);
    senhublist = (char *)WiseMem_Alloc(4096);
    infoString = (char *)WiseMem_Alloc(1024);
    netString = (char *)WiseMem_Alloc(1024);
    actionString = (char *)WiseMem_Alloc(1024);
    pos = senhublist;
    
	for(index = 0 ; index < count ; index++) {
        d = &data[index];
		if(d->name[0] != '/') {
			//sprintf(message,"/SenData/%s",d->name);
			sprintf(message,"%s",d->name);
		} else {
			sprintf(message,"%s",d->name);
			otherInfo++;
		}
        WiseAccess_Update(deviceId, message, d);
    }
	
	message[0] = 0;
	

	for(index = 0 ; index < count ; index++) {
		d = &data[index];
		if(d->name[0] != '/') {
			if(number != 0) pos += sprintf(pos,",");
			switch(d->type) {
			case WISE_VALUE:
				pos += sprintf(pos, SEN_DEVINFO_SENDATA_V_JSON, d->name, (int)d->value);
				break;
            case WISE_FLOAT:
				pos += sprintf(pos, SEN_DEVINFO_SENDATA_FV_JSON, d->name, d->value);
				break;
            case WISE_BOOL:
				pos += sprintf(pos, SEN_DEVINFO_SENDATA_BV_JSON, d->name, d->value > 0 ? "true" : "false");
				break;
			case WISE_STRING:
				pos += sprintf(pos, SEN_DEVINFO_SENDATA_SV_JSON, d->name, d->string);
				break;
			default:
				wiseprint("Datatype error!!\n");
				infiniteloop();
				break;
			}
			number++;
		}
	}

	

	if(otherInfo) {
		WiseAccess_GenerateTokenDataInfo(deviceId, "Info", infoString, WiseMem_Size(infoString));
		WiseAccess_GenerateTokenDataInfo(deviceId, "Net", netString, WiseMem_Size(netString));
		WiseAccess_GenerateTokenDataInfo(deviceId, "Action", actionString, WiseMem_Size(actionString));
		//PAAS1
		//sprintf(message,SEN_DEVINFO_JSON, senhublist, infoString, netString, actionString, deviceId, timestamp++);
		//PAAS2
		//sprintf(message,SEN_DEVINFO_JSON, deviceId, timestamp++, senhublist, timestamp++);
		//sprintf(message,SEN_DEVINFO_JSON, deviceId, senhublist);
		sprintf(message,SEN_DEVINFO_JSON, deviceId, "test", "test", senhublist);
	} else {
		//PAAS1
		//sprintf(message,SEN_DEVINFO_JSON, senhublist, "", "", "", deviceId, timestamp++);
		//PAAS2
		//sprintf(message,SEN_DEVINFO_JSON, deviceId, timestamp++, senhublist, timestamp++);
		//sprintf(message,SEN_DEVINFO_JSON, deviceId, senhublist);
		sprintf(message,SEN_DEVINFO_JSON, deviceId, "test", "test", senhublist);
	}
	wiseprint("message:\033[33m\"%s\"\033[0m\r\n", message);
	//PAAS1
   	//sprintf(topic, WA_PUB_DEVINFO_TOPIC, deviceId);
	//PAAS2
	sprintf(topic, DEF_AGENTREPORT_TOPIC, deviceId);

	core_publish(topic, message, strlen(message), 0, 0);
    
    WiseMem_Release();
}

void WiseAgent_Get(char *deviceMac, char *name, WiseAgentData *data) {
	char deviceId[DEVICE_ID_MAX_LEN] = {0};

	DeviceIdRework (deviceMac, deviceId);

	if(WiseAccess_FindDevice(deviceId) < 0) return;
	
    WiseAccess_Get(deviceId, name,data);
}

void WiseAgent_Cmd_Handler() {
    WiseAccess_Handler();
}

void WiseAgent_Close() {
	//WiseMQTT_Close();
}

//Advance Functions
void WiseAgent_Publish(const char *topic, const char *msg, const int msglen, const int retain, const int qos) {
	WiCar_MQTT_Publish(topic, msg, msglen, retain, qos);
}
