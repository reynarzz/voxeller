#include "GfxDevice.h"

void GfxDevice::SetPipelineData(const PipelineData* data)
{
   // _currentPipelineData = data;
}

bool GfxDevice::NeedChangePipeline(const PipelineData* data)
{
    return true;//_currentPipelineData.lock() == nullptr || _currentPipelineData.lock() != data.lock();
}
