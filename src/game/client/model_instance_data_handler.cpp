#include "cbase.h"
#include "model_instance_data_handler.h"
#include "cdll_client_int.h"

bool CPerInstanceModelDataMgr::Init()
{
	if (!modelrender)
		return false;

	modelrender->SetModelInstanceCallback(this);
	return true;
}

void CPerInstanceModelDataMgr::Shutdown()
{
	modelrender->SetModelInstanceCallback(nullptr);
}

void* CPerInstanceModelDataMgr::CreateInstanceData(ModelInstanceHandle_t handle, IClientRenderable* pRenderable)
{
	return nullptr;
}

void CPerInstanceModelDataMgr::DestroyInstanceData(void* pData, ModelInstanceHandle_t handle)
{
}

bool CPerInstanceModelDataMgr::OnChangeInstance(void* pData, ModelInstanceHandle_t handle, IClientRenderable* pRenderable)
{
	return true;
}

CPerInstanceModelDataMgr g_InstanceDataManager;
CPerInstanceModelDataMgr* ModelInstanceDataManager()
{
	return &g_InstanceDataManager;
}
