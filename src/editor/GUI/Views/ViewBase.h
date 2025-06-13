#pragma once

class ViewBase
{
public:
    virtual void OnShowView() = 0;
    virtual void UpdateGUI() = 0;
    virtual void OnCloseView() = 0;
protected:
};