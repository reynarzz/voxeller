#pragma once

namespace VoxellerEditor
{
class ImGuiApp
{
public:
    ImGuiApp() = default;
    ~ImGuiApp();
    void Init(void* internalWindow);
    void Update();
   
private:
};
}
