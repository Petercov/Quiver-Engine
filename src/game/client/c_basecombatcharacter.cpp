//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter() :
	m_iv_ragPos("C_BaseCombatCharacter::m_iv_ragPos"),
	m_iv_ragAngles("C_BaseCombatCharacter::m_iv_ragAngles")
{
	for ( int i=0; i < m_iAmmo.Count(); i++ )
		m_iAmmo.Set( i, 0 );

	m_elementCount = 0;
	m_flLastBoneChangeTime = -FLT_MAX;
	m_bRagdollEnabled = false;

	AddVar(m_ragPos, &m_iv_ragPos, LATCH_SIMULATION_VAR);
	AddVar(m_ragAngles, &m_iv_ragAngles, LATCH_SIMULATION_VAR);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
}

/*
//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	C_BaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( g_pGameRules->GetAmmoDef()->Index(szName) );
}
*/

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->DoMuzzleFlash();
		//NOTENOTE: We do not chain to the base here
	}
	else
	{
		BaseClass::DoMuzzleFlash();
	}
}
IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE( C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	RecvPropTime( RECVINFO( m_flNextAttack ) ),
END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
RecvPropDataTable("bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive)),
RecvPropEHandle(RECVINFO(m_hActiveWeapon)),
RecvPropArray3(RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle(RECVINFO(m_hMyWeapons[0]))),

#ifdef INVASION_CLIENT_DLL
RecvPropInt(RECVINFO(m_iPowerups)),
#endif

RecvPropArray(RecvPropQAngles(RECVINFO(m_ragAngles[0])), m_ragAngles),
RecvPropArray(RecvPropVector(RECVINFO(m_ragPos[0])), m_ragPos),
RecvPropBool(RECVINFO(m_bRagdollEnabled)),

END_RECV_TABLE()


BEGIN_PREDICTION_DATA(C_BaseCombatCharacter)

DEFINE_PRED_ARRAY(m_iAmmo, FIELD_INTEGER, MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_ARRAY(m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE),

END_PREDICTION_DATA();

void C_BaseCombatCharacter::OnPreDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnPreDataChanged(updateType);

	m_bOldRagdollEnabled = m_bRagdollEnabled;
}

void C_BaseCombatCharacter::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (m_bOldRagdollEnabled != m_bRagdollEnabled)
	{
		// Enabling ragdoll mode
		if (m_bRagdollEnabled)
		{
			m_iv_ragPos.Reset();
			m_iv_ragAngles.Reset();
		}
		// Disabling ragdoll mode
		else
		{
			CStudioHdr* pHdr = GetModelPtr();
			if (pHdr)
			{
				AutoAllowBoneAccess autoAccess(true, false);

				// Temp re-enable ragdoll mode for bones
				m_bRagdollEnabled = true;
				matrix3x4_t mBoneToWorld[MAXSTUDIOBONES];
				SetupBones(mBoneToWorld, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, gpGlobals->curtime);
				m_bRagdollEnabled = false;

				CBoneAccessor bones(mBoneToWorld);
				bones.SetWritableBones(0);
				bones.SetReadableBones(BONE_USED_BY_ANYTHING);
				SaveRagdollInfo(pHdr->numbones(), EntityToWorldTransform(), bones);
			}
		}
	}
}

void C_BaseCombatCharacter::PostDataUpdate(DataUpdateType_t updateType)
{
	BaseClass::PostDataUpdate(updateType);

	m_iv_ragPos.NoteChanged(gpGlobals->curtime, true);
	m_iv_ragAngles.NoteChanged(gpGlobals->curtime, true);
	// this is the local client time at which this update becomes stale
	m_flLastBoneChangeTime = gpGlobals->curtime + GetInterpolationAmount(m_iv_ragPos.GetType());
}

int C_BaseCombatCharacter::InternalDrawModel(int flags)
{
	int ret = BaseClass::InternalDrawModel(flags);
	if (ret && m_bRagdollEnabled && vcollide_wireframe.GetBool())
	{
		vcollide_t* pCollide = modelinfo->GetVCollide(GetModelIndex());
		IMaterial* pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", TEXTURE_GROUP_OTHER);

		matrix3x4_t matrix;
		for (int i = 0; i < m_elementCount; i++)
		{
			static color32 debugColor = { 0,255,255,0 };

			AngleMatrix(m_ragAngles[i], m_ragPos[i], matrix);
			engine->DebugDrawPhysCollide(pCollide->solids[i], pWireframe, matrix, debugColor);
		}
	}
	return ret;
}

CStudioHdr* C_BaseCombatCharacter::OnNewModel(void)
{
	CStudioHdr* hdr = BaseClass::OnNewModel();

	if (hdr)
	{
		vcollide_t* pCollide = modelinfo->GetVCollide(GetModelIndex());
		if (!pCollide)
		{
			const char* pszName = modelinfo->GetModelName(modelinfo->GetModel(GetModelIndex()));
			Msg("*** ERROR: C_BaseCombatCharacter::InitModel: %s missing vcollide data ***\n", (pszName) ? pszName : "<null>");
			m_elementCount = 0;
		}
		else
		{
			m_elementCount = RagdollExtractBoneIndices(m_boneIndex, hdr, pCollide);
		}
		m_iv_ragPos.SetMaxCount(m_elementCount);
		m_iv_ragAngles.SetMaxCount(m_elementCount);
	}

	return hdr;
}

void C_BaseCombatCharacter::GetRenderBounds(Vector& theMins, Vector& theMaxs)
{
	if (m_bRagdollEnabled)
	{
		if (!CollisionProp()->IsBoundsDefinedInEntitySpace())
		{
			IRotateAABB(EntityToWorldTransform(), CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), theMins, theMaxs);
		}
		else
		{
			theMins = CollisionProp()->OBBMins();
			theMaxs = CollisionProp()->OBBMaxs();
		}
	}
	else
	{
		BaseClass::GetRenderBounds(theMins, theMaxs);
	}
}

float C_BaseCombatCharacter::LastBoneChangedTime()
{
	return (m_bRagdollEnabled) ? m_flLastBoneChangeTime : FLT_MAX;
}

void C_BaseCombatCharacter::SetupRagdollBones(CStudioHdr* hdr, bool* boneSimulated, CBoneAccessor& pBoneToWorld)
{
	if (!hdr)
		return;

	for (int i = 0; i < m_elementCount; i++)
	{
		int index = m_boneIndex[i];
		if (index >= 0)
		{
			boneSimulated[index] = true;
			matrix3x4_t& matrix = pBoneToWorld.GetBoneForWrite(index);
			AngleMatrix(m_ragAngles[i], m_ragPos[i], matrix);
		}
	}
}
