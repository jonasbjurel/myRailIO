/*============================================================================================================================================= =*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonasbjurel@hotmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law and agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*==============================================================================================================================================*/
/* END License                                                                                                                                  */
/*==============================================================================================================================================*/

#ifndef SYSTEM_ARCH_CONF_H
#define SYSTEM_ARCH_CONF_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
//-
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* ESP32 system architecture configuration                                                                                                      */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
// CPU Cores
// System constants
#define MAX_CPU_CORES					2
#define CORE_0							0
#define CORE_1							1

// SATLINK RMT configuration 
const uint8_t SATLINK_RMT_RX_CHAN[] = { 1, 3 };
const uint8_t SATLINK_RMT_TX_CHAN[] = { 2, 4 };
const uint8_t SATLINK_RMT_RX_MEMBANK[] = { 1, 3 };
const uint8_t SATLINK_RMT_TX_MEMBANK[] = { 2, 4 };
/*==============================================================================================================================================*/
/* End ESP32 system architecture configuration                                                                                                  */
/*==============================================================================================================================================*/

#endif //SYSTEM_ARCH_CONF_H
