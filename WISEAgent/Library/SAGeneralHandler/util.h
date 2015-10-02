#ifndef _CORE_UTIL_H
#define _CORE_UTIL_H

bool GetFileMD5(char * filePath, char * retMD5Str);
bool IsFileExist(const char * pFilePath);
bool ExecuteInstaller(const char * pFilePath);

#endif