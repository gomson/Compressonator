//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
/// \file PluginManager.cpp
/// \version 2.20
//
//=====================================================================

#include "stdafx.h"
#include "string.h"
#include "PluginInterface.h"
#include "PluginManager.h"
#include <boost/filesystem.hpp>


void PluginManager::registerStaticPlugin(char *pluginType, char *pluginName, void * makePlugin)
{
    PluginDetails * curPlugin = new PluginDetails();
    curPlugin->funcHandle = reinterpret_cast<PLUGIN_FACTORYFUNC>(makePlugin);
    curPlugin->isStatic = true;
    curPlugin->setType(pluginType);
    curPlugin->setName(pluginName);

    pluginRegister.push_back(curPlugin);
}

void PluginManager::registerStaticPlugin(char *pluginType, char *pluginName, char* uuid, void * makePlugin)
{
    PluginDetails * curPlugin = new PluginDetails();
    curPlugin->funcHandle = reinterpret_cast<PLUGIN_FACTORYFUNC>(makePlugin);
    curPlugin->isStatic = true;
    curPlugin->setType(pluginType);
    curPlugin->setName(pluginName);
    curPlugin->setUUID(uuid);

    pluginRegister.push_back(curPlugin);
}

void PluginManager::getPluginList(char * SubFolderName)
{      
#ifdef _WIN32  
    WIN32_FIND_DATAA fd;
    char fname[MAX_PATH];

    //----------------------------------
    // Load plugin List for processing
    // Use an Enviornment var, search through systems path
    // or use default app running folder.
    //----------------------------------
    char dirPath[MAX_PATH];

    // v2.1 change - to check if path exists in PATH or AMDCOMPRESS_PLUGINS is set
    // else use current exe directory
    char *pPath = getenv ("AMDCOMPRESS_PLUGINS") + '\0';
    if (pPath)
    {
        sprintf_s(dirPath,"%s",pPath);
    }
    else 
    {

        bool pathFound = false;

        //Get the exe directory
        DWORD pathsize;
        HMODULE hModule = GetModuleHandleA(NULL);
        pathsize = GetModuleFileNameA(hModule, dirPath, MAX_PATH);
        if (pathsize > 0)
        {
            char *appName = (strrchr(dirPath, '\\') + 1);
            int pathLen = strlen(dirPath);
            int appNameLen = strlen(appName);

            // Null terminate the dirPath so that FileName is removed
            pathLen = pathLen - appNameLen;
            dirPath[pathLen] = 0;
            if (dirPath[pathLen - 1] == '/' || dirPath[pathLen - 1] == '\\')
            {
                pathLen--;
                dirPath[pathLen] = 0;
            }

            strcat_s(dirPath, SubFolderName);

            sprintf_s(fname, "%s\\*.dll",dirPath);


            HANDLE hFind = FindFirstFileA(fname, &fd);

            if (hFind != INVALID_HANDLE_VALUE)
            {
                pathFound = true;
            }

            FindClose(hFind);
        }

        if (!pathFound)
        {
            char *pPath = getenv("PATH");
            if (pPath)
            {
                std::string s = pPath;
                std::string delimiter = ";";
                size_t pos = 0;
                std::string token;
                while ((pos = s.find(delimiter)) != std::string::npos) {
                    token = s.substr(0, pos);
                    sprintf(dirPath, "%s\\compressonatorCLI.exe", token.c_str());
                    if (boost::filesystem::exists(dirPath))
                    {
                        sprintf(dirPath, "%s", token.c_str());
                        strcat_s(dirPath, SubFolderName);
                        pathFound = true;
                        break;
                    }
                    s.erase(0, pos + delimiter.length());
                }
            }
        }
    }


    strcpy_s(fname,dirPath);
    size_t len=strlen(fname);
    if(fname[len-1]=='/' || fname[len-1]=='\\')    strcat_s(fname,"*.dll");
    else strcat_s(fname,"\\*.dll");
    HANDLE hFind = FindFirstFileA(fname, &fd); 

    if (hFind == INVALID_HANDLE_VALUE) 
    {
        FindClose(hFind);
        return;
    }

    do 
    { 
        HINSTANCE dllHandle = NULL;
        try
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {                    
                char fname[MAX_PATH];
                sprintf_s(fname,"%s\\%s",dirPath,fd.cFileName);
                dllHandle = LoadLibraryA(fname);
                if (dllHandle != NULL) 
                {
                    // Is this DLL a plugin for us if so keep its type and name details
                    PLUGIN_FACTORYFUNC funcHandle;
                    funcHandle = reinterpret_cast<PLUGIN_FACTORYFUNC>(GetProcAddress(dllHandle, "makePlugin"));
                    if(funcHandle !=NULL)
                    {                            
                        PluginDetails * curPlugin = new PluginDetails();
                        //printf("new: %s\n", fname);
                        curPlugin->setFileName(fname);

                        PLUGIN_TEXTFUNC textFunc;
                        textFunc = reinterpret_cast<PLUGIN_TEXTFUNC>(GetProcAddress(dllHandle, "getPluginType"));
                        if (textFunc)
                            curPlugin->setType(textFunc());

                        textFunc = reinterpret_cast<PLUGIN_TEXTFUNC>(GetProcAddress(dllHandle, "getPluginName"));
                        if (textFunc)
                            curPlugin->setName(textFunc());
                        
                        textFunc = reinterpret_cast<PLUGIN_TEXTFUNC>(GetProcAddress(dllHandle, "getPluginUUID"));
                        if (textFunc)
                            curPlugin->setUUID(textFunc());

                        textFunc = reinterpret_cast<PLUGIN_TEXTFUNC>(GetProcAddress(dllHandle, "getPluginCategory"));
                        if (textFunc)
                            curPlugin->setCategory(textFunc());

                        pluginRegister.push_back(curPlugin);
                    }
                    FreeLibrary(dllHandle);
                }
            }
        }
        catch(...)
        {
            if (dllHandle != NULL) FreeLibrary(dllHandle);
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
#endif 
}


//----------------------------------------------

PluginDetails::~PluginDetails()
{
#ifdef _WIN32
    if(dllHandle) FreeLibrary(dllHandle);
#endif
    clearMembers();
}

void PluginDetails::setFileName(char * nm)
{
#ifdef _WIN32
    strcpy_s(filename,MAX_PLUGIN_FILENAME_STR,nm);
#else
    strcpy(filename,nm);
#endif
}

void PluginDetails::setName(char * nm)
{
#ifdef _WIN32
    strcpy_s(pluginName,MAX_PLUGIN_NAME_STR,nm);
#else
    strcpy(pluginName,nm);
#endif
}

void PluginDetails::setUUID(char * nm)
{
#ifdef _WIN32
    strcpy_s(pluginUUID, MAX_PLUGIN_UUID_STR, nm);
#else
    strcpy(pluginUUID, nm);
#endif
}

void PluginDetails::setType(char * nm)
{
#ifdef _WIN32
    strcpy_s(pluginType,MAX_PLUGIN_TYPE_STR,nm);
#else
    strcpy(pluginType,nm);
#endif
}

void PluginDetails::setCategory(char * nm)
{
#ifdef _WIN32
    strcpy_s(pluginCategory, MAX_PLUGIN_CATEGORY_STR, nm);
#else
    strcpy(pluginCategory, nm);
#endif
}


void * PluginDetails::makeNewInstance()
{
    if (isStatic)
    {
        return funcHandle();
    }
    else
    {
#ifdef _WIN32
        if(!dllHandle) dllHandle = LoadLibraryA(filename);

        if(dllHandle != NULL) 
        {
            funcHandle = reinterpret_cast<PLUGIN_FACTORYFUNC>(GetProcAddress(dllHandle, "makePlugin"));
            if(funcHandle !=NULL)
            {
                return funcHandle();
            }
        }
#endif
    }
    return NULL;
}

