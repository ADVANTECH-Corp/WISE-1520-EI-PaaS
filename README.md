# WISE-1520-EI-PaaS
This repository aims to patch WISE-1520 SDK v1.1.04.0239 in order to connect to EI-PaaS. Source code is based on wise1520_sdk_v1.1.04.0239.zip.

Patch Step:

1. Refer to [WISE-1520 SDK](http://ess-wiki.advantech.com.tw/view/MCU/WISE-1520_SDK) wiki to setup development environment.
2. git clone this repository
3. Then, copy all files in src directory and overwrite src directory in WISE-1520 SDK in your development environment. 

If EI-PaaS server's url and authentication need to be modified, please edit src/example/wise1520_demo_main.c as below defines:

```
#define AGENT_DEFAULT_SERVER_NAME		"wise-msghub.eastasia.cloudapp.azure.com"
#define PAAS_ID                         "0e95b665-3748-46ce-80c5-bdd423d7a8a5:b69f6f36-ad5b-4126-abfb-be5443a07c73"
#define PAAS_PW                         "24s04jv4mtb0oq4eiruehf0r22"
```
