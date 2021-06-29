#ifndef MODEL_INSTANCE_DATA_HANDLER_H
#define MODEL_INSTANCE_DATA_HANDLER_H

#pragma once

#include <igamesystem.h>
#include <engine/ivmodelrender.h>

class CPerInstanceModelDataMgr : public CBaseGameSystem, public IVModelInstanceClientCallback
{
public:
	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init();
	virtual void Shutdown();

	// Inherited via IVModelInstanceClientCallback
	virtual void* CreateInstanceData(ModelInstanceHandle_t handle, IClientRenderable* pRenderable) override;
	virtual void DestroyInstanceData(void* pData, ModelInstanceHandle_t handle) override;
	virtual bool OnChangeInstance(void* pData, ModelInstanceHandle_t handle, IClientRenderable* pRenderable) override;
};

CPerInstanceModelDataMgr* ModelInstanceDataManager();

#endif