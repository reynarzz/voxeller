#include "GfxDevice.h"

void GfxDevice::SetPipelineData(const PipelineData* data, const RendererState& state, const RenderableObject* obj)
{
   // _currentPipelineData = data;
}

bool GfxDevice::NeedChangePipeline(const PipelineData* data)
{
    return true;//_currentPipelineData.lock() == nullptr || _currentPipelineData.lock() != data.lock();
}
