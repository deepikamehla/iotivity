//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "ocstack.h"
#include "logger.h"
#include "occlientbasicops.h"
#include "cJSON.h"
#include "common.h"

#define TAG "occlientbasicops"
static int UNICAST_DISCOVERY = 0;
static int TEST_CASE = 0;

static int IPV4_ADDR_SIZE = 16;
static char UNICAST_DISCOVERY_QUERY[] = "coap://%s:6298/oic/res";
static char MULTICAST_DISCOVERY_QUERY[] = "/oic/res";

static std::string putPayload = "{\"oic\":[{\"rep\":{\"state\":\"off\",\"power\":10}}]}";
static std::string coapServerIP;
static std::string coapServerPort;
static std::string coapServerResource;
static int coapSecureResource;
static OCConnectivityType ocConnType;


//Secure Virtual Resource database for Iotivity Client application
//It contains Client's Identity and the PSK credentials
//of other devices which the client trusts
static char CRED_FILE[] = "oic_svr_db_client.json";


int gQuitFlag = 0;

/* SIGINT handler: set gQuitFlag to 1 for graceful termination */
void handleSigInt(int signum)
{
    if (signum == SIGINT)
    {
        gQuitFlag = 1;
    }
}

static void PrintUsage()
{
    OC_LOG(INFO, TAG, "Usage : occlient -u <0|1> -t <1|2|3>");
    OC_LOG(INFO, TAG, "-u <0|1> : Perform multicast/unicast discovery of resources");
    OC_LOG(INFO, TAG, "-t 1 : Discover Resources");
    OC_LOG(INFO, TAG, "-t 2 : Discover Resources and"
            " Initiate Nonconfirmable Get/Put/Post Requests");
    OC_LOG(INFO, TAG, "-t 3 : Discover Resources and Initiate Confirmable Get/Put/Post Requests");
}

OCStackResult InvokeOCDoResource(std::ostringstream &query,
        OCMethod method, OCQualityOfService qos,
        OCClientResponseHandler cb, OCHeaderOption * options, uint8_t numOptions)
{
    OCStackResult ret;
    OCCallbackData cbData;

    cbData.cb = cb;
    cbData.context = NULL;
    cbData.cd = NULL;

    ret = OCDoResource(NULL, method, query.str().c_str(), 0,
            (method == OC_REST_PUT || method == OC_REST_POST) ? putPayload.c_str() : NULL,
            ocConnType, qos, &cbData, options, numOptions);

    if (ret != OC_STACK_OK)
    {
        OC_LOG_V(ERROR, TAG, "OCDoResource returns error %d with method %d", ret, method);
    }

    return ret;
}

OCStackApplicationResult putReqCB(void* ctx, OCDoHandle handle, OCClientResponse * clientResponse)
{
    OC_LOG(INFO, TAG, "Callback Context for PUT recvd successfully");

    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s",  getResult(clientResponse->result));
        OC_LOG_V(INFO, TAG, "JSON = %s =============> Put Response", clientResponse->resJSONPayload);
    }
    return OC_STACK_DELETE_TRANSACTION;
}

OCStackApplicationResult postReqCB(void *ctx, OCDoHandle handle, OCClientResponse *clientResponse)
{
    OC_LOG(INFO, TAG, "Callback Context for POST recvd successfully");

    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s",  getResult(clientResponse->result));
        OC_LOG_V(INFO, TAG, "JSON = %s =============> Post Response",
                clientResponse->resJSONPayload);
    }
    return OC_STACK_DELETE_TRANSACTION;
}

OCStackApplicationResult getReqCB(void* ctx, OCDoHandle handle, OCClientResponse * clientResponse)
{
    OC_LOG(INFO, TAG, "Callback Context for GET query recvd successfully");

    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s",  getResult(clientResponse->result));
        OC_LOG_V(INFO, TAG, "SEQUENCE NUMBER: %d", clientResponse->sequenceNumber);
        OC_LOG_V(INFO, TAG, "JSON = %s =============> Get Response",
                clientResponse->resJSONPayload);
    }
    return OC_STACK_DELETE_TRANSACTION;
}

// This is a function called back when a device is discovered
OCStackApplicationResult discoveryReqCB(void* ctx, OCDoHandle handle,
        OCClientResponse * clientResponse)
{
    OC_LOG(INFO, TAG, "Callback Context for DISCOVER query recvd successfully");

    if (clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s", getResult(clientResponse->result));
        OC_LOG_V(INFO, TAG,
                "Device =============> Discovered %s @ %s:%d",
                clientResponse->resJSONPayload, clientResponse->devAddr.addr, clientResponse->devAddr.port);

        ocConnType = clientResponse->connType;

        if (parseClientResponse(clientResponse) != -1)
        {
            switch(TEST_CASE)
            {
                case TEST_NON_CON_OP:
                    InitGetRequest(OC_LOW_QOS);
                    InitPutRequest(OC_LOW_QOS);
                    //InitPostRequest(OC_LOW_QOS);
                    break;
                case TEST_CON_OP:
                    InitGetRequest(OC_HIGH_QOS);
                    InitPutRequest(OC_HIGH_QOS);
                    //InitPostRequest(OC_HIGH_QOS);
                    break;
            }
        }
    }

    return (UNICAST_DISCOVERY) ? OC_STACK_DELETE_TRANSACTION : OC_STACK_KEEP_TRANSACTION ;

}

int InitPutRequest(OCQualityOfService qos)
{
    OC_LOG_V(INFO, TAG, "\n\nExecuting %s", __func__);
    std::ostringstream query;
    query << (coapSecureResource ? "coaps://" : "coap://") << coapServerIP
        << ":" << coapServerPort  << coapServerResource;
    return (InvokeOCDoResource(query, OC_REST_PUT,
            ((qos == OC_HIGH_QOS) ? OC_HIGH_QOS: OC_LOW_QOS), putReqCB, NULL, 0));
}

int InitPostRequest(OCQualityOfService qos)
{
    OCStackResult result;
    OC_LOG_V(INFO, TAG, "\n\nExecuting %s", __func__);
    std::ostringstream query;
    query << (coapSecureResource ? "coaps://" : "coap://") << coapServerIP
        << ":" << coapServerPort << coapServerResource;

    // First POST operation (to create an LED instance)
    result = InvokeOCDoResource(query, OC_REST_POST,
            ((qos == OC_HIGH_QOS) ? OC_HIGH_QOS: OC_LOW_QOS),
            postReqCB, NULL, 0);
    if (OC_STACK_OK != result)
    {
        // Error can happen if for example, network connectivity is down
        OC_LOG(INFO, TAG, "First POST call did not succeed");
    }

    // Second POST operation (to create an LED instance)
    result = InvokeOCDoResource(query, OC_REST_POST,
            ((qos == OC_HIGH_QOS) ? OC_HIGH_QOS: OC_LOW_QOS),
            postReqCB, NULL, 0);
    if (OC_STACK_OK != result)
    {
        OC_LOG(INFO, TAG, "Second POST call did not succeed");
    }

    // This POST operation will update the original resourced /a/led
    return (InvokeOCDoResource(query, OC_REST_POST,
                ((qos == OC_HIGH_QOS) ? OC_HIGH_QOS: OC_LOW_QOS),
                postReqCB, NULL, 0));
}

int InitGetRequest(OCQualityOfService qos)
{
    OC_LOG_V(INFO, TAG, "\n\nExecuting %s", __func__);
    std::ostringstream query;
    query << (coapSecureResource ? "coaps://" : "coap://") << coapServerIP
        << ":" << coapServerPort << coapServerResource;

    return (InvokeOCDoResource(query, OC_REST_GET, (qos == OC_HIGH_QOS)?
            OC_HIGH_QOS:OC_LOW_QOS, getReqCB, NULL, 0));
}

int InitDiscovery()
{
    OCStackResult ret;
    OCMethod method;
    OCCallbackData cbData;
    char szQueryUri[MAX_URI_LENGTH] = { 0 };
    OCConnectivityType discoveryReqConnType;

    if (UNICAST_DISCOVERY)
    {
        char ipv4addr[IPV4_ADDR_SIZE];
        printf("Enter IPv4 address of the Server hosting secure resource (Ex: 11.12.13.14)\n");
        if (fgets(ipv4addr, IPV4_ADDR_SIZE, stdin))
        {
            //Strip newline char from ipv4addr
            StripNewLineChar(ipv4addr);
            snprintf(szQueryUri, sizeof(szQueryUri), UNICAST_DISCOVERY_QUERY, ipv4addr);
        }
        else
        {
            OC_LOG(ERROR, TAG, "!! Bad input for IPV4 address. !!");
            return OC_STACK_INVALID_PARAM;
        }
        discoveryReqConnType = CT_ADAPTER_IP;
        method = OC_REST_GET;
    }
    else
    {
        //Send discovery request on Wifi and Ethernet interface
        discoveryReqConnType = CT_DEFAULT;
        strcpy(szQueryUri, MULTICAST_DISCOVERY_QUERY);
        method = OC_REST_DISCOVER;
    }

    cbData.cb = discoveryReqCB;
    cbData.context = NULL;
    cbData.cd = NULL;

    /* Start a discovery query*/
    OC_LOG_V(INFO, TAG, "Initiating %s Resource Discovery : %s\n",
        (UNICAST_DISCOVERY) ? "Unicast" : "Multicast",
        szQueryUri);

    ret = OCDoResource(NULL, method, szQueryUri, 0, 0,
            discoveryReqConnType, OC_LOW_QOS,
            &cbData, NULL, 0);
    if (ret != OC_STACK_OK)
    {
        OC_LOG(ERROR, TAG, "OCStack resource error");
    }
    return ret;
}

FILE* client_fopen(const char *path, const char *mode)
{
    (void)path;
    return fopen(CRED_FILE, mode);
}

int main(int argc, char* argv[])
{
    int opt;
    struct timespec timeout;

    while ((opt = getopt(argc, argv, "u:t:")) != -1)
    {
        switch(opt)
        {
            case 'u':
                UNICAST_DISCOVERY = atoi(optarg);
                break;
            case 't':
                TEST_CASE = atoi(optarg);
                break;
            default:
                PrintUsage();
                return -1;
        }
    }

    if ((UNICAST_DISCOVERY != 0 && UNICAST_DISCOVERY != 1) ||
            (TEST_CASE < TEST_DISCOVER_REQ || TEST_CASE >= MAX_TESTS) )
    {
        PrintUsage();
        return -1;
    }

    // Initialize Persistent Storage for SVR database
    OCPersistentStorage ps = {};
    ps.open = client_fopen;
    ps.read = fread;
    ps.write = fwrite;
    ps.close = fclose;
    ps.unlink = unlink;
    OCRegisterPersistentStorageHandler(&ps);

    /* Initialize OCStack*/
    if (OCInit(NULL, 0, OC_CLIENT_SERVER) != OC_STACK_OK)
    {
        OC_LOG(ERROR, TAG, "OCStack init error");
        return 0;
    }

    InitDiscovery();

    timeout.tv_sec  = 0;
    timeout.tv_nsec = 100000000L;

    // Break from loop with Ctrl+C
    OC_LOG(INFO, TAG, "Entering occlient main loop...");
    signal(SIGINT, handleSigInt);
    while (!gQuitFlag)
    {
        if (OCProcess() != OC_STACK_OK)
        {
            OC_LOG(ERROR, TAG, "OCStack process error");
            return 0;
        }

        nanosleep(&timeout, NULL);
    }
    OC_LOG(INFO, TAG, "Exiting occlient main loop...");

    if (OCStop() != OC_STACK_OK)
    {
        OC_LOG(ERROR, TAG, "OCStack stop error");
    }

    return 0;
}

std::string getPortTBServer(OCClientResponse * clientResponse)
{
    if(!clientResponse) return "";
    std::ostringstream ss;
    ss << clientResponse->devAddr.port;
    return ss.str();
}

int parseClientResponse(OCClientResponse * clientResponse)
{
    int port = -1;
    cJSON * root = NULL;
    cJSON * oc = NULL;

    // Initialize all global variables
    coapServerResource.clear();
    coapServerPort.clear();
    coapServerIP.clear();
    coapSecureResource = 0;

    root = cJSON_Parse((char *)(clientResponse->resJSONPayload));
    if (!root)
    {
        return -1;
    }

    oc = cJSON_GetObjectItem(root,"oic");
    if (!oc)
    {
        return -1;
    }

    if (oc->type == cJSON_Array)
    {
        int numRsrcs =  cJSON_GetArraySize(oc);
        for(int i = 0; i < numRsrcs; i++)
        {
            cJSON * resource = cJSON_GetArrayItem(oc, i);
            if (cJSON_GetObjectItem(resource, "href"))
            {
                coapServerResource.assign(cJSON_GetObjectItem(resource, "href")->valuestring);
            }
            else
            {
                coapServerResource = "";
            }
            OC_LOG_V(INFO, TAG, "Uri -- %s", coapServerResource.c_str());

            cJSON * prop = cJSON_GetObjectItem(resource,"prop");
            if (prop)
            {
                cJSON * policy = cJSON_GetObjectItem(prop,"p");
                if (policy)
                {
                    // If this is a secure resource, the info about the port at which the
                    // resource is hosted on server is embedded inside discovery JSON response
                    if (cJSON_GetObjectItem(policy, "sec"))
                    {
                        if ((cJSON_GetObjectItem(policy, "sec")->valueint) == 1)
                        {
                            coapSecureResource = 1;
                        }
                    }
                    OC_LOG_V(INFO, TAG, "Secure -- %s", coapSecureResource == 1 ? "YES" : "NO");
                    if (cJSON_GetObjectItem(policy, "port"))
                    {
                        port = cJSON_GetObjectItem(policy, "port")->valueint;
                        OC_LOG_V(INFO, TAG, "Hosting Server Port (embedded inside JSON) -- %u", port);

                        std::ostringstream ss;
                        ss << port;
                        coapServerPort = ss.str();
                    }
                }
            }

            // If we discovered a secure resource, exit from here
            if (coapSecureResource)
            {
                break;
            }
        }
    }
    cJSON_Delete(root);

    coapServerIP = clientResponse->devAddr.addr;
    if (port == -1)
    {
        coapServerPort = getPortTBServer(clientResponse);
        OC_LOG_V(INFO, TAG, "Hosting Server Port -- %s", coapServerPort.c_str());
    }
    return 0;
}

