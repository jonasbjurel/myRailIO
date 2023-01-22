/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2023 Jonas Bjurel (jonas.bjurel@hotmail.com)
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



/*==============================================================================================================================================*/
/* .h Definitions                                                                                                                               */
/*==============================================================================================================================================*/
#ifndef FILESYS_H
#define FILESYS_H
/*==============================================================================================================================================*/
/* END .h Definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <FS.h>
#include <esp_spiffs.h>
#include <dirent.h>
#include <errno.h>
#include "config.h"
#include "panic.h"
#include "rc.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "libraries/QList/src/QList.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: fileSys                                                                                                                               */
/* Purpose: Provides a persistant SPIFFS file system                                                                                            */
/* Description: fileSys::start() tries to mount the SPIFFS file system, if fail it will try to re-format it and re-mount it, if it fails the    */
/*              genJMRIdecoder will reboot. format() will format the SPIFFS file system, getFile() will read a file into the p_buff buffer,     */
/*              putFile() will write a file with the content of p_buff, fileExists() checks if a file exists, deleteFile() deletes a file,      */
/*              renameFile() renames a file, listDir(QList<char*>* p_fileList, bool p_printOut=true) lists the SPIFFS file system directory and */
/*              provides the file names in a *QList<char*>* p_fileList unless unless p_fileList is set to NULL. If p_printOut is set to true    */
/*              the list of files will be written to the console output.                                                                        */
/* Methods: See below                                                                                                                           */
/* Data structures: -                                                                                                                           */
/*==============================================================================================================================================*/
class fileSys {
public:
    //Public methods
    static void start(void);                                                            // Start the SPIFFS file system and mount it...
    static rc_t format(void);                                                           // Format the SPIFFS file system and mount it
    static rc_t getFile(const char* p_fileName, char* p_buff, uint p_buffSize,          // Get a file p_fileName and store the content to p_buff sized
                                                                                        //   p_buffSize
                        uint* p_readSize);
    static rc_t putFile(const char* p_fileName, const char* p_buff, uint p_buffSize,    // Store a file p_fileName from  the content to p_buff sized
                        uint* p_writeSize);                                             //   p_buffSize
    static bool fileExists(const char* p_fileName);                                     // Check if a file named p_fileName exists, returns true if
                                                                                        //   exist, otherwise false
    static rc_t deleteFile(const char* p_fileName);                                     // Delete a file named p_fileName exists, returns RC_OK if 
                                                                                        //   successfully deleted
    static rc_t renameFile(const char* p_oldFileName, const char* p_newFileName);       // Rename a file named p_oldFileName to p_newFileName,
                                                                                        //   returns RC_OK if successfully renamed
    static rc_t listDir(QList<char*>* p_fileList, bool p_printOut=true);                // list the SPIFFS directory, list of filenames in the
                                                                                        //   QList<char*>* p_fileList pointer unless p_fileList
                                                                                        //   is set to NULL, if p_printOut is set to tru the list
                                                                                        //   is echoed to the console


    //Public data structures
    //-

private:
    //Private methods
    //-

    //Private data structures
    static esp_vfs_spiffs_conf_t spiffsPartitionConfig;                                 // SPIFFS partition descriptor
};
/*==============================================================================================================================================*/
/* END Class fileSys                                                                                                                            */
/*==============================================================================================================================================*/
#endif //FILESYS_H
