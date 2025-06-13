#pragma once
#include "ViewBase.h"

class VoxToProcessView : public ViewBase
{
public:
        void OnShowView() override;
     void UpdateGUI() override;
     void OnCloseView() override;
private:
};