#pragma once
#include <string>

struct VOXFileToProcessData
{
    std::string FileName = "Vox";
    std::string FileFolder = "/";
    bool UseCustomConfig = false;
    bool Enabled = true;
};