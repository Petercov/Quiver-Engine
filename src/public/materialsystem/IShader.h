//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ISHADER_H
#define ISHADER_H

#ifdef _WIN32
#pragma once
#endif


#include "materialsystem/imaterialsystem.h"
#include "materialsystem/ishaderapi.h"
#include "materialsystem/ishadersystem_declarations.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IMaterialVar;
class IShaderShadow;
class IShaderDynamicAPI;
class IShaderInit;
class CBasePerMaterialContextData;

//-----------------------------------------------------------------------------
// Information about each shader parameter
//-----------------------------------------------------------------------------
struct ShaderParamInfo_t
{
	const char *m_pName;
	const char *m_pHelp;
	ShaderParamType_t m_Type;
	const char *m_pDefaultValue;
	int m_nFlags;
};



#define VERTEX_SHADER_BONE_TRANSFORM( k )	( VERTEX_SHADER_MODEL + 3 * (k) )


// The public methods exposed by each shader
//-----------------------------------------------------------------------------
abstract_class IShader
{
public:
	// Returns the shader name
	virtual char const* GetName( ) const = 0;

	// returns the shader fallbacks
	virtual char const* GetFallbackShader( IMaterialVar** params ) const = 0;

	// Shader parameters
	virtual int GetNumParams( ) const = 0;

	// These functions must be implemented by the shader
	virtual void InitShaderParams( IMaterialVar** ppParams, const char *pMaterialName ) = 0;
	virtual void InitShaderInstance( IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName, const char *pTextureGroupName ) = 0;
	virtual void DrawElements( IMaterialVar **params, int nModulationFlags,
		IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr ) = 0;

	virtual char const* GetParamName( int paramIndex ) const = 0;
	virtual char const* GetParamHelp( int paramIndex ) const = 0;
	virtual ShaderParamType_t GetParamType( int paramIndex ) const = 0;
	virtual char const* GetParamDefault( int paramIndex ) const = 0;

	// FIXME: Figure out a better way to do this?
	virtual int ComputeModulationFlags( IMaterialVar** params, IShaderDynamicAPI* pShaderAPI ) = 0;
	virtual bool NeedsPowerOfTwoFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame = true ) const = 0;
	virtual bool NeedsFullFrameBufferTexture( IMaterialVar **params, bool bCheckSpecificToThisFrame ) const = 0;
	virtual bool IsTranslucent( IMaterialVar **params ) const = 0;

	virtual int GetParamFlags( int paramIndex ) const = 0;

	virtual int GetFlags() const = 0;

	// FIXME: Remove GetParamName, etc. above
//	virtual const ShaderParamInfo_t& GetParamInfo( int paramIndex ) const = 0;
};

#endif // ISHADER_H
