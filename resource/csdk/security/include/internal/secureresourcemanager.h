//******************************************************************
//
// Copyright 2015 Intel Mobile Communications GmbH All Rights Reserved.
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

#ifndef SECURITYRESOURCEMANAGER_H_
#define SECURITYRESOURCEMANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Register Persistent storage callback.
 * @param   persistentStorageHandler [IN] Pointers to open, read, write, close & unlink handlers.
 * @return
 *     OC_STACK_OK    - No errors; Success
 *     OC_STACK_INVALID_PARAM - Invalid parameter
 */
OCStackResult SRMRegisterPersistentStorageHandler(OCPersistentStorage* persistentStorageHandler);

/**
 * @brief   Get Persistent storage handler pointer.
 * @return
 *     The pointer to Persistent Storage callback handler
 */
OCPersistentStorage* SRMGetPersistentStorageHandler();

/**
 * @brief   Register request and response callbacks.
 *          Requests and responses are delivered in these callbacks.
 * @param   reqHandler   [IN] Request handler callback ( for GET,PUT ..etc)
 * @param   respHandler  [IN] Response handler callback.
 * @param   errHandler   [IN] Error handler callback.
 * @return
 *     OC_STACK_OK    - No errors; Success
 *     OC_STACK_INVALID_PARAM - Invalid parameter
 */
OCStackResult SRMRegisterHandler(CARequestCallback reqHandler,
                                 CAResponseCallback respHandler,
                                 CAErrorCallback errHandler);

/**
 * @brief   Initialize all secure resources ( /oic/sec/cred, /oic/sec/acl, /oic/sec/pstat etc).
 * @return  OC_STACK_OK for Success, otherwise some error value
 */
OCStackResult SRMInitSecureResources();

/**
 * @brief   Perform cleanup for secure resources ( /oic/sec/cred, /oic/sec/acl, /oic/sec/pstat etc).
 * @return  none
 */
void SRMDeInitSecureResources();

/**
 * @brief   Initialize Policy Engine context.
 * @return  OC_STACK_OK for Success, otherwise some error value.
 */
OCStackResult SRMInitPolicyEngine();

/**
 * @brief   Cleanup Policy Engine context.
 * @return  none
 */
void SRMDeInitPolicyEngine();

#ifdef __cplusplus
}
#endif

#endif /* SECURITYRESOURCEMANAGER_H_ */
