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
    LOG_INFO_NOFMT("Trying to mount the first and only fs partition" CR);
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
        LOG_ERROR("Could not mount fs partition: %s, Partition does " \
                  "not exist-, or is corrupted - will (re-)format the partition, " \
                  "all configurations will be lost..." CR, err);
        if (format()) {
            panic("Failed to format the filesystem partition, " \
                "the flash may be weared or otherwise broken");
            return;
        }
    }
    LOG_INFO_NOFMT("fileSys partition successfully mounted" CR);
}

rc_t fileSys::format(void) {
    LOG_INFO_NOFMT("Formatting SPIFFS filesystem" CR);
    if (esp_spiffs_format(NULL)) {
        LOG_ERROR_NOFMT("Failed to format the SPIFS filesystem partition, " \
                  "the flash may be weared or otherwise broken" CR);
        return RC_GEN_ERR;
    }
    LOG_VERBOSE_NOFMT("SPIFFS filesystem successfully formatted" CR);
    return RC_OK;
}

rc_t fileSys::getFile(const char* p_fileName, char* p_buff, uint p_buffSize,
                      uint* p_readSize) {
    FILE* fp;
    fp = fopen(p_fileName, "r");
    if (fp == NULL) {
        LOG_ERROR("Could not open file %s, %s" CR, p_fileName,
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
        LOG_ERROR("Could not open or create file \"%s\", %s" CR, p_fileName, strerror(errno));
        return RC_GEN_ERR;
    }
    if (p_writeSize)
        *p_writeSize = fwrite(p_buff, p_buffSize, 1, fp) * p_buffSize;
    else
        fwrite(p_buff, p_buffSize, 1, fp) * p_buffSize;
    fclose(fp);
    return RC_OK;
}

bool fileSys::fileExists(const char* p_fileName) {
    FILE* fp;
    fp = fopen(p_fileName, "r");
    if (fp == NULL)
        return false;
    else {
        fclose(fp);
        return true;
    }
}

rc_t fileSys::deleteFile(const char* p_fileName) {
    LOG_VERBOSE("Deleting file: %s" CR, p_fileName);

    if (remove(p_fileName)) {
        LOG_ERROR("fileSys:deleteFile: Could not delete file %s, %s" CR, p_fileName, strerror(errno));
        return RC_GEN_ERR;
    }
    LOG_VERBOSE("File: %s successfully deleted" CR, p_fileName);
    return RC_OK;
}

rc_t fileSys::renameFile(const char* p_oldFileName, const char* p_newFileName) {
    LOG_VERBOSE("Renaming file %s to %s" CR, p_oldFileName, p_newFileName);
    if (rename(p_oldFileName, p_newFileName)) {
        LOG_ERROR("fileSys:renameFile: Could not rename file %s to %s, %s" CR, p_oldFileName, p_oldFileName,
                  strerror(errno));
        return RC_GEN_ERR;
    }
    LOG_VERBOSE("fileSys:renameFile: File %s successfully renamed to %s" CR, p_oldFileName, p_newFileName);
    return RC_OK;
}

rc_t fileSys::listDir(QList<char*>* p_fileList, bool p_printOut) {
    DIR* d;
    struct dirent* dir;
    char* fileName;
    d = opendir(FS_PATH);
    if (d) {
        if (p_printOut)
            LOG_INFO_NOFMT("Following files exist:\n");
        while ((dir = readdir(d)) != NULL) {
            if (p_printOut)
                LOG_INFO("%s\n", dir->d_name);
            if (p_fileList) {
                fileName = new char[strlen(dir->d_name)];
                p_fileList->push_back(fileName);
            }
        }
        closedir(d);
    }
    else{
        LOG_INFO_NOFMT("No files found:\n");
        return RC_NOT_FOUND_ERR;
    }
    return(RC_OK);
}
/*==============================================================================================================================================*/
/* END Class fileSys                                                                                                                            */
/*==============================================================================================================================================*/
