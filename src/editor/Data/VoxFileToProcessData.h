#pragma once
#include <string>

struct VOXFileToProcessData
{
    std::string FileName = "Vox";
    std::string FileFolder = "/";
    std::string FullPath = "";
    std::string Extension = "";
    bool UseCustomConfig = false;
    bool Enabled = true;
};