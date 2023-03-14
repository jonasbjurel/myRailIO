/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2023 Jonas Bjurel (jonasbjurel@hotmail.com)
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
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "fileSys.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: fileSys																																*/
/* Purpose: See fileSys.h                                                                                                                       */
/* Description: See fileSys.h                                                                                                                   */
/* Methods: See fileSys.h                                                                                                                       */
/* Data structures: See fileSys.h                                                                                                               */
/*==============================================================================================================================================*/
esp_vfs_spiffs_conf_t fileSys::spiffsPartitionConfig;

void fileSys::start(void) {
    Log.notice("fileSys::start: Trying to mount the first and only fs partition" CR);
    spiffsPartitionConfig.partition_label = NULL;
    char fsPath[30];
    strcpy(fsPath, FS_PATH);
    spiffsPartitionConfig.base_path = fsPath;
    spiffsPartitionConfig.max_files = 10;
    spiffsPartitionConfig.format_if_mount_failed = false;
    while (esp_vfs_spiffs_register(&spiffsPartitionConfig) ||
           esp_spiffs_check(NULL)) {
        char err[30];
        esp_err_to_name_r(esp_vfs_spiffs_register(&spiffsPartitionConfig), err, 30);
        Log.ERROR("fileSys::start: Could not mount fs partition: %s, Partition does "
                  "not exist-, or is corrupted - will (re-)format the partition, "
                  "all configurations will be lost..." CR, err);
        if (format())
            panic("fileSys::start: Failed to format the filesystem partition, "
                  "the flash may be weared or otherwise broken, rebooting..." CR);
    }
    Log.notice("fileSys::start: fileSys partition %s successfully mounted" CR, NULL);
}

rc_t fileSys::format(void) {
    Log.notice("fileSys::format: Formatting SPIFFS filesystem" CR);
    if (esp_spiffs_format(NULL)) {
        Log.ERROR("fileSys::format: Failed to format the SPIFS filesystem partition, "
                  "the flash may be weared or otherwise broken" CR);
        return RC_GEN_ERR;
    }
    Log.VERBOSE("fileSys::format: SPIFFS filesystem successfully formatted" CR);
    return RC_OK;
}

rc_t fileSys::getFile(const char* p_fileName, char* p_buff, uint p_buffSize,
                      uint* p_readSize) {
    FILE* fp;
    fp = fopen(p_fileName, "r");
    if (fp == NULL) {
        Log.ERROR("fileSys:getFile: Could not open file %s, %s" CR, p_fileName,
                  strerror(errno));
        return RC_NOT_FOUND_ERR;
    }
    *p_readSize = fread(p_buff, p_buffSize, 1, fp) * p_buffSize;
    fclose(fp);
    return RC_OK;
}

rc_t fileSys::putFile(const char* p_fileName, const char* p_buff, uint p_buffSize,
                      uint* p_writeSize) {
    FILE* fp;
    fp = fopen(p_fileName, "w");
    if (fp == NULL) {
        Log.ERROR("fileSys:putFile: Could not open or create file %s, %s" CR, p_fileName, strerror(errno));
        return RC_GEN_ERR;
    }
    *p_writeSize = fwrite(p_buff, p_buffSize, 1, fp) * p_buffSize;
    fclose(fp);
    return RC_OK;
}

bool fileSys::fileExists(const char* p_fileName) {
    FILE* fp;
    fp = fopen(p_fileName, "r");
    if (fp == NULL)
        return false;
    else
        return true;
}

rc_t fileSys::deleteFile(const char* p_fileName) {
    Log.VERBOSE("fileSys::deleteFile: Deleting file: %s" CR, p_fileName);

    if (remove(p_fileName)) {
        Log.ERROR("fileSys:deleteFile: Could not delete file %s, %s" CR, p_fileName, strerror(errno));
        return RC_GEN_ERR;
    }
    Log.VERBOSE("fileSys::deleteFile: File: %s successfully deleted" CR, p_fileName);
    return RC_OK;
}

rc_t fileSys::renameFile(const char* p_oldFileName, const char* p_newFileName) {
    Log.VERBOSE("fileSys:renameFile: Renaming file %s to %s" CR, p_oldFileName, p_newFileName);
    if (rename(p_oldFileName, p_newFileName)) {
        Log.ERROR("fileSys:renameFile: Could not rename file %s to %s, %s" CR, p_oldFileName, p_oldFileName,
                  strerror(errno));
        return RC_GEN_ERR;
    }
    Log.VERBOSE("fileSys:renameFile: File %s successfully renamed to %s" CR, p_oldFileName, p_newFileName);
    return RC_OK;
}

rc_t fileSys::listDir(QList<char*>* p_fileList, bool p_printOut) {
    DIR* d;
    struct dirent* dir;
    char* fileName;
    d = opendir(FS_PATH);
    if (d) {
        if (p_printOut)
            Log.INFO("fileSys::listDir: Following files exist:\n");
        while ((dir = readdir(d)) != NULL) {
            if (p_printOut)
                Log.INFO("fileSys::listDir: %s\n", dir->d_name);
            if (p_fileList) {
                fileName = new char[strlen(dir->d_name)];
                p_fileList->push_back(fileName);
            }
        }
        closedir(d);
    }
    else{
        Log.INFO("fileSys::listDir: No files found:\n");
        return RC_NOT_FOUND_ERR;
    }
    return(RC_OK);
}
/*==============================================================================================================================================*/
/* END Class fileSys                                                                                                                            */
/*==============================================================================================================================================*/
