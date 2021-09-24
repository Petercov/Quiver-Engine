//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Base combat character with no AI
//
//=============================================================================//

#include "cbase.h"
#include "basecombatcharacter.h"
#include "basecombatweapon.h"
#include "animation.h"
#include "gib.h"
#include "entitylist.h"
#include "gamerules.h"
#include "ai_basenpc.h"
#include "ai_squadslot.h"
#include "ammodef.h"
#include "ndebugoverlay.h"
#include "player.h"
#include "physics.h"
#include "physics_saverestore.h"
#include "engine/IEngineSound.h"
#include "tier1/strtools.h"
#include "sendproxy.h"
#include "EntityFlame.h"
#include "CRagdollMagnet.h"
#include "IEffects.h"
#include "iservervehicle.h"
#include "igamesystem.h"
#include "globals.h"
#include "physics_prop_ragdoll.h"
#include "physics_impact_damage.h"
#include "saverestore_utlvector.h"
#include "eventqueue.h"
#include "world.h"
#include "globalstate.h"
#include "items.h"
#include "movevars_shared.h"
#include "RagdollBoogie.h"
#include "rumble_shared.h"
#include "saverestoretypes.h"
#include "bone_setup.h"
#include <datacache\imdlcache.h>

#ifdef HL2_DLL
#include "weapon_physcannon.h"
#include "hl2_gamerules.h"
#endif

#ifdef PORTAL
	#include "portal_util_shared.h"
	#include "prop_portal_shared.h"
	#include "portal_shareddefs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef HL2_DLL
extern int	g_interactionBarnacleVictimReleased;
#endif //HL2_DLL

extern ConVar weapon_showproficiency;

ConVar ai_show_hull_attacks( "ai_show_hull_attacks", "0" );
ConVar ai_force_serverside_ragdoll( "ai_force_serverside_ragdoll", "0" );

#ifndef _RETAIL
ConVar ai_use_visibility_cache( "ai_use_visibility_cache", "1" );
#define ShouldUseVisibilityCache() ai_use_visibility_cache.GetBool()
#else
#define ShouldUseVisibilityCache() true
#endif

#define DEFINE_RAGDOLL_ELEMENT( i ) \
	DEFINE_FIELD( m_ragdoll.list[i].originParentSpace, FIELD_VECTOR ), \
	DEFINE_PHYSPTR( m_ragdoll.list[i].pObject ), \
	DEFINE_PHYSPTR( m_ragdoll.list[i].pConstraint ), \
	DEFINE_FIELD( m_ragdoll.list[i].parentIndex, FIELD_INTEGER )

BEGIN_DATADESC( CBaseCombatCharacter )

#ifdef INVASION_DLL
	DEFINE_FIELD( m_iPowerups, FIELD_INTEGER ),
	DEFINE_ARRAY( m_flPowerupAttemptTimes, FIELD_TIME, MAX_POWERUPS ),
	DEFINE_ARRAY( m_flPowerupEndTimes, FIELD_TIME, MAX_POWERUPS ),
	DEFINE_FIELD( m_flFractionalBoost, FIELD_FLOAT ),
#endif

	DEFINE_FIELD( m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( m_eHull, FIELD_INTEGER ),
	DEFINE_FIELD( m_bloodColor, FIELD_INTEGER ),
	DEFINE_FIELD( m_iDamageCount, FIELD_INTEGER ),
	
	DEFINE_FIELD( m_flFieldOfView, FIELD_FLOAT ),
	DEFINE_FIELD( m_HackedGunPos, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_RelationshipString, FIELD_STRING, "Relationship" ),

	DEFINE_FIELD( m_LastHitGroup, FIELD_INTEGER ),
	DEFINE_FIELD( m_flDamageAccumulator, FIELD_FLOAT ),
	DEFINE_INPUT( m_impactEnergyScale, FIELD_FLOAT, "physdamagescale" ),
	DEFINE_FIELD( m_CurrentWeaponProficiency, FIELD_INTEGER),

	DEFINE_UTLVECTOR( m_Relationship,	FIELD_EMBEDDED),

	DEFINE_AUTO_ARRAY( m_iAmmo, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_hMyWeapons, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hActiveWeapon, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bForceServerRagdoll, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPreventWeaponPickup, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "KilledNPC", InputKilledNPC ),

	DEFINE_AUTO_ARRAY(m_ragdoll.boneIndex, FIELD_INTEGER),
	DEFINE_AUTO_ARRAY(m_ragPos, FIELD_POSITION_VECTOR),
	DEFINE_AUTO_ARRAY(m_ragAngles, FIELD_VECTOR),

	DEFINE_FIELD(m_lastRagdollUpdateTickCount, FIELD_TICK),
	DEFINE_FIELD(m_ragAllAsleep, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bRagdollEnabled, FIELD_BOOLEAN),

	DEFINE_AUTO_ARRAY(m_ragdollMins, FIELD_VECTOR),
	DEFINE_AUTO_ARRAY(m_ragdollMaxs, FIELD_VECTOR),

	DEFINE_FIELD(m_PreRagdollMoveType, FIELD_CHARACTER),
	DEFINE_FIELD(m_PreRagdollMoveCollide, FIELD_CHARACTER),
	DEFINE_FIELD(m_PreRagdollSolidFlags, FIELD_CHARACTER),
	DEFINE_FIELD(m_PreRagdollSolidType, FIELD_CHARACTER),

	DEFINE_FIELD(m_ragdoll.listCount, FIELD_INTEGER),
	DEFINE_FIELD(m_ragdoll.allowStretch, FIELD_BOOLEAN),
	DEFINE_PHYSPTR(m_ragdoll.pGroup),

	//DEFINE_RAGDOLL_ELEMENT( 0 ),
	DEFINE_RAGDOLL_ELEMENT(1),
	DEFINE_RAGDOLL_ELEMENT(2),
	DEFINE_RAGDOLL_ELEMENT(3),
	DEFINE_RAGDOLL_ELEMENT(4),
	DEFINE_RAGDOLL_ELEMENT(5),
	DEFINE_RAGDOLL_ELEMENT(6),
	DEFINE_RAGDOLL_ELEMENT(7),
	DEFINE_RAGDOLL_ELEMENT(8),
	DEFINE_RAGDOLL_ELEMENT(9),
	DEFINE_RAGDOLL_ELEMENT(10),
	DEFINE_RAGDOLL_ELEMENT(11),
	DEFINE_RAGDOLL_ELEMENT(12),
	DEFINE_RAGDOLL_ELEMENT(13),
	DEFINE_RAGDOLL_ELEMENT(14),
	DEFINE_RAGDOLL_ELEMENT(15),
	DEFINE_RAGDOLL_ELEMENT(16),
	DEFINE_RAGDOLL_ELEMENT(17),
	DEFINE_RAGDOLL_ELEMENT(18),
	DEFINE_RAGDOLL_ELEMENT(19),
	DEFINE_RAGDOLL_ELEMENT(20),
	DEFINE_RAGDOLL_ELEMENT(21),
	DEFINE_RAGDOLL_ELEMENT(22),
	DEFINE_RAGDOLL_ELEMENT(23),

END_DATADESC()


BEGIN_SIMPLE_DATADESC( Relationship_t )
	DEFINE_FIELD( entity,			FIELD_EHANDLE ),
	DEFINE_FIELD( classType,		FIELD_INTEGER ),
	DEFINE_FIELD( disposition,	FIELD_INTEGER ),
	DEFINE_FIELD( priority,	FIELD_INTEGER ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
int					CBaseCombatCharacter::m_lastInteraction   = 0;
Relationship_t**	CBaseCombatCharacter::m_DefaultRelationship	= NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCleanupDefaultRelationShips : public CAutoGameSystem
{
public:
	CCleanupDefaultRelationShips( char const *name ) : CAutoGameSystem( name )
	{
	}

	virtual void Shutdown()
	{
		if ( !CBaseCombatCharacter::m_DefaultRelationship )
			return;

		for ( int i=0; i<NUM_AI_CLASSES; ++i )
		{
			delete[] CBaseCombatCharacter::m_DefaultRelationship[ i ];
		}

		delete[] CBaseCombatCharacter::m_DefaultRelationship;
		CBaseCombatCharacter::m_DefaultRelationship = NULL;
	}
};

static CCleanupDefaultRelationShips g_CleanupDefaultRelationships( "CCleanupDefaultRelationShips" );

void *SendProxy_SendBaseCombatCharacterLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Only send to local player if this is a player
	pRecipients->ClearAllRecipients();
	
	CBaseCombatCharacter *pBCC = ( CBaseCombatCharacter * )pStruct;
	if ( pBCC != NULL)
	{
		if ( pBCC->IsPlayer() )
		{
			pRecipients->SetOnly( pBCC->entindex() - 1 );
		}
		else
		{
			// If it's a vehicle, send to "driver" (e.g., operator of tf2 manned guns)
			IServerVehicle *pVehicle = pBCC->GetServerVehicle();
			if ( pVehicle != NULL )
			{
				CBaseCombatCharacter *pDriver = pVehicle->GetPassenger();
				if ( pDriver != NULL )
				{
					pRecipients->SetOnly( pDriver->entindex() - 1 );
				}
			}
		}
	}
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendBaseCombatCharacterLocalDataTable );

// Only send active weapon index to local player
BEGIN_SEND_TABLE_NOBASE( CBaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	SendPropTime( SENDINFO( m_flNextAttack ) ),
END_SEND_TABLE();

//-----------------------------------------------------------------------------
// This table encodes the CBaseCombatCharacter
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(CBaseCombatCharacter, DT_BaseCombatCharacter)
	// Data that only gets sent to the local player.
	SendPropDataTable( "bcc_localdata", 0, &REFERENCE_SEND_TABLE(DT_BCCLocalPlayerExclusive), SendProxy_SendBaseCombatCharacterLocalDataTable ),

	SendPropEHandle( SENDINFO( m_hActiveWeapon ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_hMyWeapons), SendPropEHandle( SENDINFO_ARRAY(m_hMyWeapons) ) ),

#ifdef INVASION_DLL
	SendPropInt( SENDINFO(m_iPowerups), MAX_POWERUPS, SPROP_UNSIGNED ), 
#endif

	SendPropArray(SendPropQAngles(SENDINFO_ARRAY(m_ragAngles), 13, SPROP_CHANGES_OFTEN), m_ragAngles),
	SendPropArray(SendPropVector(SENDINFO_ARRAY(m_ragPos), -1, SPROP_COORD | SPROP_CHANGES_OFTEN), m_ragPos),
	SendPropBool(SENDINFO(m_bRagdollEnabled)),

END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::InitInteractionSystem()
{
	// interaction ids continue to go up with every map load, otherwise you get
	// collisions if a future map has a different set of NPCs from a current map
}


//-----------------------------------------------------------------------------
// Purpose: Return an interaction ID (so we have no collisions)
//-----------------------------------------------------------------------------
int	CBaseCombatCharacter::GetInteractionID(void)
{
	m_lastInteraction++;
	return (m_lastInteraction);
}

// ============================================================================
bool CBaseCombatCharacter::HasHumanGibs( void )
{
#if defined( HL2_DLL )
	Class_T myClass = Classify();
	if ( myClass == CLASS_CITIZEN_PASSIVE   ||
		 myClass == CLASS_CITIZEN_REBEL		||
		 myClass == CLASS_COMBINE			||
		 myClass == CLASS_CONSCRIPT			||
		 myClass == CLASS_METROPOLICE		||
		 myClass == CLASS_PLAYER )	
		 return true;

#elif defined( HL1_DLL )
	Class_T myClass = Classify();
	if (	myClass == CLASS_HUMAN_MILITARY		||
			myClass == CLASS_PLAYER_ALLY		||
			myClass == CLASS_HUMAN_PASSIVE		||
			myClass == CLASS_PLAYER )
	{
		return true;
	}

#elif defined( CSPORT_DLL )
	Class_T myClass = Classify();
	if (	 myClass == CLASS_PLAYER )	
	{
		return true;
	}

#endif

	return false;
}


bool CBaseCombatCharacter::HasAlienGibs( void )
{
#if defined( HL2_DLL )
	Class_T myClass = Classify();
	if ( myClass == CLASS_BARNACLE		 || 
		 myClass == CLASS_STALKER		 ||
		 myClass == CLASS_ZOMBIE		 ||
		 myClass == CLASS_VORTIGAUNT	 ||
		 myClass == CLASS_HEADCRAB )
	{
		 return true;
	}

#elif defined( HL1_DLL )
	Class_T myClass = Classify();
	if ( myClass == CLASS_ALIEN_MILITARY ||
		 myClass == CLASS_ALIEN_MONSTER	||
		 myClass == CLASS_INSECT  ||
		 myClass == CLASS_ALIEN_PREDATOR  ||
		 myClass == CLASS_ALIEN_PREY )
	{
		return true;
	}
#endif

	return false;
}


void CBaseCombatCharacter::CorpseFade( void )
{
	StopAnimation();
	SetAbsVelocity( vec3_origin );
	SetMoveType( MOVETYPE_NONE );
	SetLocalAngularVelocity( vec3_angle );
	m_flAnimTime = gpGlobals->curtime;
	AddEffects( EF_NOINTERP );
	SUB_StartFadeOut();
}

//-----------------------------------------------------------------------------
// Visibility caching
//-----------------------------------------------------------------------------

struct VisibilityCacheEntry_t
{
	CBaseEntity *pEntity1;
	CBaseEntity *pEntity2;
	EHANDLE		pBlocker;
	float		time;
};

class CVisibilityCacheEntryLess
{
public:
	CVisibilityCacheEntryLess( int ) {}
	bool operator!() const { return false; }
	bool operator()( const VisibilityCacheEntry_t &lhs, const VisibilityCacheEntry_t &rhs ) const
	{
		return ( memcmp( &lhs, &rhs, offsetof( VisibilityCacheEntry_t, pBlocker ) ) < 0 );
	}
};

static CUtlRBTree<VisibilityCacheEntry_t, unsigned short, CVisibilityCacheEntryLess> g_VisibilityCache;
const float VIS_CACHE_ENTRY_LIFE = ( !IsXbox() ) ? .090 : .500;

bool CBaseCombatCharacter::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	VPROF( "CBaseCombatCharacter::FVisible" );

	if ( traceMask != MASK_BLOCKLOS || !ShouldUseVisibilityCache() || pEntity == this
#if defined(HL2_DLL)
		 || Classify() == CLASS_BULLSEYE || pEntity->Classify() == CLASS_BULLSEYE 
#endif
		 )
	{
		return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
	}

	VisibilityCacheEntry_t cacheEntry;

	if ( this < pEntity )
	{
		cacheEntry.pEntity1 = this;
		cacheEntry.pEntity2 = pEntity;
	}
	else
	{
		cacheEntry.pEntity1 = pEntity;
		cacheEntry.pEntity2 = this;
	}

	int iCache = g_VisibilityCache.Find( cacheEntry );

	if ( iCache != g_VisibilityCache.InvalidIndex() )
	{
		if ( gpGlobals->curtime - g_VisibilityCache[iCache].time < VIS_CACHE_ENTRY_LIFE )
		{
			bool bCachedResult = !g_VisibilityCache[iCache].pBlocker.IsValid();
			if ( bCachedResult )
			{
				if ( ppBlocker )
				{
					*ppBlocker = g_VisibilityCache[iCache].pBlocker;
					if ( !*ppBlocker )
					{
						*ppBlocker = GetWorldEntity();
					}
				}
			}
			else
			{
				if ( ppBlocker )
				{
					*ppBlocker = NULL;
				}
			}
			return bCachedResult;
		}
	}
	else
	{
		if ( g_VisibilityCache.Count() != g_VisibilityCache.InvalidIndex() )
		{
			iCache = g_VisibilityCache.Insert( cacheEntry );
		}
		else
		{
			return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
		}
	}

	CBaseEntity *pBlocker = NULL;
	if ( ppBlocker == NULL )
	{
		ppBlocker = &pBlocker;
	}

	bool bResult = BaseClass::FVisible( pEntity, traceMask, ppBlocker );

	if ( !bResult )
	{
		g_VisibilityCache[iCache].pBlocker = *ppBlocker;
	}
	else
	{
		g_VisibilityCache[iCache].pBlocker = NULL;
	}

	g_VisibilityCache[iCache].time = gpGlobals->curtime;

	return bResult;
}

void CBaseCombatCharacter::ResetVisibilityCache( CBaseCombatCharacter *pBCC )
{
	VPROF( "CBaseCombatCharacter::ResetVisibilityCache" );
	if ( !pBCC )
	{
		g_VisibilityCache.RemoveAll();
		return;
	}

	int i = g_VisibilityCache.FirstInorder();
	CUtlVector<unsigned short> removals;
	while ( i != g_VisibilityCache.InvalidIndex() )
	{
		if ( g_VisibilityCache[i].pEntity1 == pBCC || g_VisibilityCache[i].pEntity2 == pBCC )
		{
			removals.AddToTail( i );
		}
		i = g_VisibilityCache.NextInorder( i );
	}

	for ( i = 0; i < removals.Count(); i++ )
	{
		g_VisibilityCache.RemoveAt( removals[i] );
	}
}

#ifdef PORTAL
bool CBaseCombatCharacter::FVisibleThroughPortal( const CProp_Portal *pPortal, CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	VPROF( "CBaseCombatCharacter::FVisible" );

	if ( pEntity->GetFlags() & FL_NOTARGET )
		return false;

#if HL1_DLL
	// FIXME: only block LOS through opaque water
	// don't look through water
	if ((m_nWaterLevel != 3 && pEntity->m_nWaterLevel == 3) 
		|| (m_nWaterLevel == 3 && pEntity->m_nWaterLevel == 0))
		return false;
#endif

	Vector vecLookerOrigin = EyePosition();//look through the caller's 'eyes'
	Vector vecTargetOrigin = pEntity->EyePosition();

	// Use the custom LOS trace filter
	CTraceFilterLOS traceFilter( this, COLLISION_GROUP_NONE, pEntity );

	Vector vecTranslatedTargetOrigin;
	UTIL_Portal_PointTransform( pPortal->m_hLinkedPortal->MatrixThisToLinked(), vecTargetOrigin, vecTranslatedTargetOrigin );
	Ray_t ray;
	ray.Init( vecLookerOrigin, vecTranslatedTargetOrigin );

	trace_t tr;

	// If we're doing an opaque search, include NPCs.
	if ( traceMask == MASK_BLOCKLOS )
	{
		traceMask = MASK_BLOCKLOS_AND_NPCS;
	}

	UTIL_Portal_TraceRay_Bullets( pPortal, ray, traceMask, &traceFilter, &tr );

	if (tr.fraction != 1.0 || tr.startsolid )
	{
		// If we hit the entity we're looking for, it's visible
		if ( tr.m_pEnt == pEntity )
			return true;

		// Got line of sight on the vehicle the player is driving!
		if ( pEntity && pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = assert_cast<CBasePlayer*>( pEntity );
			if ( tr.m_pEnt == pPlayer->GetVehicleEntity() )
				return true;
		}

		if (ppBlocker)
		{
			*ppBlocker = tr.m_pEnt;
		}

		return false;// Line of sight is not established
	}

	return true;// line of sight is valid.
}
#endif

//-----------------------------------------------------------------------------

//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInViewCone( CBaseEntity *pEntity )
{
	return FInViewCone( pEntity->WorldSpaceCenter() );
}

//=========================================================
// FInViewCone - returns true is the passed Vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInViewCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - EyePosition() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = EyeDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > m_flFieldOfView )
		return true;

	return false;
}

#ifdef PORTAL
//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
CProp_Portal* CBaseCombatCharacter::FInViewConeThroughPortal( CBaseEntity *pEntity )
{
	return FInViewConeThroughPortal( pEntity->WorldSpaceCenter() );
}

//=========================================================
// FInViewCone - returns true is the passed Vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
CProp_Portal* CBaseCombatCharacter::FInViewConeThroughPortal( const Vector &vecSpot )
{
	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if( iPortalCount == 0 )
		return NULL;

	const Vector ptEyePosition = EyePosition();

	float fDistToBeat = 1e20; //arbitrarily high number
	CProp_Portal *pBestPortal = NULL;

	CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();

	// Check through both portals
	for ( int iPortal = 0; iPortal < iPortalCount; ++iPortal )
	{
		CProp_Portal *pPortal = pPortals[iPortal];

		// Check if this portal is active, linked, and in the view cone
		if( pPortal->IsActivedAndLinked() && FInViewCone( pPortal ) )
		{
			// The facing direction is the eye to the portal to set up a proper FOV through the relatively small portal hole
			Vector facingDir = pPortal->GetAbsOrigin() - ptEyePosition;

			// If the portal isn't facing the eye, bail
			if ( facingDir.Dot( pPortal->m_plane_Origin.normal ) > 0.0f )
				continue;

			// If the point is behind the linked portal, bail
			if ( ( vecSpot - pPortal->m_hLinkedPortal->GetAbsOrigin() ).Dot( pPortal->m_hLinkedPortal->m_plane_Origin.normal ) < 0.0f )
				continue;

			// Remove height from the equation
			facingDir.z = 0.0f;
			float fPortalDist = VectorNormalize( facingDir );

			// Translate the target spot across the portal
			Vector vTranslatedVecSpot;
			UTIL_Portal_PointTransform( pPortal->m_hLinkedPortal->MatrixThisToLinked(), vecSpot, vTranslatedVecSpot );

			// do this in 2D
			Vector los = ( vTranslatedVecSpot - ptEyePosition );
			los.z = 0.0f;
			float fSpotDist = VectorNormalize( los );

			if( fSpotDist > fDistToBeat )
				continue; //no point in going further, we already have a better portal

			// If the target point is closer than the portal (banana juice), bail
			// HACK: Extra 32 is a fix for the player who's origin can be on one side of a portal while his center mirrored across is closer than the portal.
			if ( fPortalDist > fSpotDist + 32.0f )
				continue;

			// Get the worst case FOV from the portal's corners
			float fFOVThroughPortal = 1.0f;

			for ( int i = 0; i < 4; ++i )
			{
				//Vector vPortalCorner = pPortal->GetAbsOrigin() + vPortalRight * PORTAL_HALF_WIDTH * ( ( i / 2 == 0 ) ? ( 1.0f ) : ( -1.0f ) ) + 
				//												 vPortalUp * PORTAL_HALF_HEIGHT * ( ( i % 2 == 0 ) ? ( 1.0f ) : ( -1.0f ) );

				Vector vEyeToCorner = pPortal->m_vPortalCorners[i] - ptEyePosition;
				vEyeToCorner.z = 0.0f;
				VectorNormalize( vEyeToCorner );

				float flCornerDot = DotProduct( vEyeToCorner, facingDir );

				if ( flCornerDot < fFOVThroughPortal )
					fFOVThroughPortal = flCornerDot;
			}

			float flDot = DotProduct( los, facingDir );

			// Use the tougher FOV of either the standard FOV or FOV clipped to the portal hole
			if ( flDot > max( fFOVThroughPortal, m_flFieldOfView ) )
			{
				float fActualDist = ptEyePosition.DistToSqr( vTranslatedVecSpot );
				if( fActualDist < fDistToBeat )
				{
					fDistToBeat = fActualDist;
					pBestPortal = pPortal;
				}
			}
		}
	}

	return pBestPortal;
}
#endif


//=========================================================
// FInAimCone - returns true is the passed ent is in
// the caller's forward aim cone. The dot product is performed
// in 2d, making the aim cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInAimCone( CBaseEntity *pEntity )
{
	return FInAimCone( pEntity->BodyTarget( EyePosition() ) );
}


//=========================================================
// FInAimCone - returns true is the passed Vector is in
// the caller's forward aim cone. The dot product is performed
// in 2d, making the view cone infinitely tall. By default, the
// callers aim cone is assumed to be very narrow
//=========================================================
bool CBaseCombatCharacter::FInAimCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > 0.994 )//!!!BUGBUG - magic number same as FacingIdeal(), what is this?
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  The type of interaction, extra info pointer, and who started it
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt )
{
#ifdef HL2_DLL
	if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		// For now, throw away the NPC and leave the ragdoll.
		UTIL_Remove( this );
		return true;
	}
#endif // HL2_DLL
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor : Initialize some fields
//-----------------------------------------------------------------------------
CBaseCombatCharacter::CBaseCombatCharacter( void )
{
#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_HackedGunPos.Init();
#endif

	// Zero the damage accumulator.
	m_flDamageAccumulator = 0.0f;

	// Init weapon and Ammo data
	m_hActiveWeapon			= NULL;

	// reset all ammo values to 0
	RemoveAllAmmo();

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		m_hMyWeapons.Set( i, NULL );
	}

	// Default so that spawned entities have this set
	m_impactEnergyScale = 1.0f;

	m_bForceServerRagdoll = ai_force_serverside_ragdoll.GetBool();
}

//------------------------------------------------------------------------------
// Purpose : Destructor
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseCombatCharacter::~CBaseCombatCharacter( void )
{
	ResetVisibilityCache( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "BaseCombatCharacter.CorpseGib" );
	PrecacheScriptSound( "BaseCombatCharacter.StopWeaponSounds" );
	PrecacheScriptSound( "BaseCombatCharacter.AmmoPickup" );

	for ( int i = m_Relationship.Count() - 1; i >= 0 ; i--) 
	{
		if ( !m_Relationship[i].entity && m_Relationship[i].classType == CLASS_NONE ) 
		{
			DevMsg( 2, "Removing relationship for lost entity\n" );
			m_Relationship.FastRemove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::Restore( IRestore &restore )
{
	int status = BaseClass::Restore(restore);
	if ( !status )
		return 0;

	if ( gpGlobals->eLoadType == MapLoad_Transition )
	{
		DevMsg( 2, "%s (%s) removing class relationships due to level transition\n", STRING( GetEntityName() ), GetClassname() );

		for ( int i = m_Relationship.Count() - 1; i >= 0; --i )
		{
			if ( !m_Relationship[i].entity && m_Relationship[i].classType != CLASS_NONE ) 
			{
				m_Relationship.FastRemove( i );
			}
		}
	}
	return status;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::UpdateOnRemove( void )
{
	int i;
	// Make sure any weapons I didn't drop get removed.
	for (i=0;i<MAX_WEAPONS;i++) 
	{
		if (m_hMyWeapons[i]) 
		{
			UTIL_Remove( m_hMyWeapons[i] );
		}
	}

	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
		SetOwnerEntity( NULL );
	}

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}


//=========================================================
// CorpseGib - create some gore and get rid of a character's
// model.
//=========================================================
bool CBaseCombatCharacter::CorpseGib( const CTakeDamageInfo &info )
{
	trace_t		tr;
	bool		gibbed = false;

	EmitSound( "BaseCombatCharacter.CorpseGib" );

	// only humans throw skulls !!!UNDONE - eventually NPCs will have their own sets of gibs
	if ( HasHumanGibs() )
	{
		CGib::SpawnHeadGib( this );
		CGib::SpawnRandomGibs( this, 4, GIB_HUMAN );	// throw some human gibs.
		gibbed = true;
	}
	else if ( HasAlienGibs() )
	{
		CGib::SpawnRandomGibs( this, 4, GIB_ALIEN );	// Throw alien gibs
		gibbed = true;
	}

	return gibbed;
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBaseCombatCharacter::GetDeathActivity ( void )
{
	Activity	deathActivity;
	bool		fTriedDirection;
	float		flDot;
	trace_t		tr;
	Vector		vecSrc;

	if (IsPlayer())
	{
		// die in an interesting way
		switch( random->RandomInt(0,7) )
		{
		case 0:	return ACT_DIESIMPLE;
		case 1: return ACT_DIEBACKWARD;
		case 2: return ACT_DIEFORWARD;
		case 3: return ACT_DIEVIOLENT;
		case 4: return ACT_DIE_HEADSHOT;
		case 5: return ACT_DIE_CHESTSHOT;
		case 6: return ACT_DIE_GUTSHOT;
		case 7: return ACT_DIE_BACKSHOT;
		}
	}

	vecSrc = WorldSpaceCenter();

	fTriedDirection = false;
	deathActivity = ACT_DIESIMPLE;// in case we can't find any special deaths to do.

	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	flDot = -DotProduct( forward, g_vecAttackDir );

	switch ( m_LastHitGroup )
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;

	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if ( flDot > 0.3 )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if ( flDot <= -0.3 )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;

	default:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if ( flDot > 0.3 )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if ( flDot <= -0.3 )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}


	// can we perform the prescribed death?
	if ( SelectWeightedSequence ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// no! did we fail to perform a directional death? 
		if ( fTriedDirection )
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if ( flDot > 0.3 )
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if ( flDot <= -0.3 )
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if ( SelectWeightedSequence ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;

		if ( SelectWeightedSequence ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
		{
			Msg( "ERROR! %s missing ACT_DIESIMPLE\n", STRING(GetModelName()) );
		}
	}

	if ( deathActivity == ACT_DIEFORWARD )
	{
			// make sure there's room to fall forward
			UTIL_TraceHull ( vecSrc, vecSrc + forward * 64, Vector(-16,-16,-18), 
				Vector(16,16,18), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0 )
			{
				deathActivity = ACT_DIESIMPLE;
			}
	}

	if ( deathActivity == ACT_DIEBACKWARD )
	{
			// make sure there's room to fall backward
			UTIL_TraceHull ( vecSrc, vecSrc - forward * 64, Vector(-16,-16,-18), 
				Vector(16,16,18), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0 )
			{
				deathActivity = ACT_DIESIMPLE;
			}
	}

	return deathActivity;
}


// UNDONE: Should these operate on a list of weapon/items
Activity CBaseCombatCharacter::Weapon_TranslateActivity( Activity baseAct, bool *pRequired )
{
	Activity translated = baseAct;

	if ( m_hActiveWeapon )
	{
		translated = m_hActiveWeapon->ActivityOverride( baseAct, pRequired );
	}
	else if (pRequired)
	{
		*pRequired = false;
	}

	return translated;
}

//-----------------------------------------------------------------------------
// Purpose: NPCs should override this function to translate activities
//			such as ACT_WALK, etc.
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CBaseCombatCharacter::NPC_TranslateActivity( Activity baseAct )
{
	return baseAct;
}


void CBaseCombatCharacter::Weapon_SetActivity( Activity newActivity, float duration )
{
	if ( m_hActiveWeapon )
	{
		m_hActiveWeapon->SetActivity( newActivity, duration );
	}
}

void CBaseCombatCharacter::Weapon_FrameUpdate( void )
{
	if ( m_hActiveWeapon )
	{
		m_hActiveWeapon->Operator_FrameUpdate( this );
	}
}


//------------------------------------------------------------------------------
// Purpose :	expects a length to trace, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::CheckTraceHullAttack( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale, bool bDamageAnyNPC )
{
	// If only a length is given assume we want to trace in our facing direction
	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector vStart = GetAbsOrigin();

	// The ideal place to start the trace is in the center of the attacker's bounding box.
	// however, we need to make sure there's enough clearance. Some of the smaller monsters aren't 
	// as big as the hull we try to trace with. (SJB)
	float flVerticalOffset = WorldAlignSize().z * 0.5;

	if( flVerticalOffset < maxs.z )
	{
		// There isn't enough room to trace this hull, it's going to drag the ground.
		// so make the vertical offset just enough to clear the ground.
		flVerticalOffset = maxs.z + 1.0;
	}

	vStart.z += flVerticalOffset;
	Vector vEnd = vStart + (forward * flDist );
	return CheckTraceHullAttack( vStart, vEnd, mins, maxs, iDamage, iDmgType, forceScale, bDamageAnyNPC );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pHandleEntity - 
//			contentsMask - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTraceFilterMelee::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
		return false;

	if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		return false;

	// Don't test if the game code tells us we should ignore this collision...
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	
	if ( pEntity )
	{
		if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
			return false;
		
		if ( !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
			return false;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			return false;

		// FIXME: Do not translate this to the driver because the driver only accepts damage from the vehicle
		// Translate the vehicle into its driver for damage
		/*
		if ( pEntity->GetServerVehicle() != NULL )
		{
			CBaseEntity *pDriver = pEntity->GetServerVehicle()->GetPassenger();

			if ( pDriver != NULL )
			{
				pEntity = pDriver;
			}
		}
		*/

		Vector	attackDir = pEntity->WorldSpaceCenter() - m_dmgInfo->GetAttacker()->WorldSpaceCenter();
		VectorNormalize( attackDir );

		CTakeDamageInfo info = (*m_dmgInfo);				
		CalculateMeleeDamageForce( &info, attackDir, info.GetAttacker()->WorldSpaceCenter(), m_flForceScale );

		CBaseCombatCharacter *pBCC = info.GetAttacker()->MyCombatCharacterPointer();
		CBaseCombatCharacter *pVictimBCC = pEntity->MyCombatCharacterPointer();

		// Only do these comparisons between NPCs
		if ( pBCC && pVictimBCC )
		{
			// Can only damage other NPCs that we hate
			if ( m_bDamageAnyNPC || pBCC->IRelationType( pEntity ) == D_HT )
			{
				if ( info.GetDamage() )
				{
					pEntity->TakeDamage( info );
				}
				
				// Put a combat sound in
				CSoundEnt::InsertSound( SOUND_COMBAT, info.GetDamagePosition(), 200, 0.2f, info.GetAttacker() );

				m_pHit = pEntity;
				return true;
			}
		}
		else
		{
			m_pHit = pEntity;

			// Make sure if the player is holding this, he drops it
			Pickup_ForcePlayerToDropThisObject( pEntity );

			// Otherwise just damage passive objects in our way
			if ( info.GetDamage() )
			{
				pEntity->TakeDamage( info );
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// Purpose :	start and end trace position, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale, bool bDamageAnyNPC )
{
	// Handy debuging tool to visualize HullAttack trace
	if ( ai_show_hull_attacks.GetBool() )
	{
		float length	 = (vEnd - vStart ).Length();
		Vector direction = (vEnd - vStart );
		VectorNormalize( direction );
		Vector hullMaxs = maxs;
		hullMaxs.x = length + hullMaxs.x;
		NDebugOverlay::BoxDirection(vStart, mins, hullMaxs, direction, 100,255,255,20,1.0);
		NDebugOverlay::BoxDirection(vStart, mins, maxs, direction, 255,0,0,20,1.0);
	}

#if 1

	CTakeDamageInfo	dmgInfo( this, this, iDamage, DMG_SLASH );
	
	// COLLISION_GROUP_PROJECTILE does some handy filtering that's very appropriate for this type of attack, as well. (sjb) 7/25/2007
	CTraceFilterMelee traceFilter( this, COLLISION_GROUP_PROJECTILE, &dmgInfo, flForceScale, bDamageAnyNPC );

	Ray_t ray;
	ray.Init( vStart, vEnd, mins, maxs );

	trace_t tr;
	enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );

	CBaseEntity *pEntity = traceFilter.m_pHit;
	
	if ( pEntity == NULL )
	{
		// See if perhaps I'm trying to claw/bash someone who is standing on my head.
		Vector vecTopCenter;
		Vector vecEnd;
		Vector vecMins, vecMaxs;

		// Do a tracehull from the top center of my bounding box.
		vecTopCenter = GetAbsOrigin();
		CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );
		vecTopCenter.z = vecMaxs.z + 1.0f;
		vecEnd = vecTopCenter;
		vecEnd.z += 2.0f;
		
		ray.Init( vecTopCenter, vEnd, mins, maxs );
		enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );

		pEntity = traceFilter.m_pHit;
	}

	if( pEntity && !pEntity->CanBeHitByMeleeAttack(this) )
	{
		// If we touched something, but it shouldn't be hit, return nothing.
		pEntity = NULL;
	}

	return pEntity;

#else

	trace_t tr;
	UTIL_TraceHull( vStart, vEnd, mins, maxs, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	CBaseEntity *pEntity = tr.m_pEnt;

	if ( !pEntity )
	{
		// See if perhaps I'm trying to claw/bash someone who is standing on my head.
		Vector vecTopCenter;
		Vector vecEnd;
		Vector vecMins, vecMaxs;

		// Do a tracehull from the top center of my bounding box.
		vecTopCenter = GetAbsOrigin();
		CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );
		vecTopCenter.z = vecMaxs.z + 1.0f;
		vecEnd = vecTopCenter;
		vecEnd.z += 2.0f;
		UTIL_TraceHull( vecTopCenter, vecEnd, mins, maxs, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
		pEntity = tr.m_pEnt;
	}

	if ( !pEntity || !pEntity->m_takedamage || !pEntity->IsAlive() )
		return NULL;

	// Translate the vehicle into its driver for damage
	if ( pEntity->GetServerVehicle() != NULL )
	{
		CBaseEntity *pDriver = pEntity->GetServerVehicle()->GetPassenger();

		if ( pDriver != NULL )
		{
			pEntity = pDriver;
			//FIXME: Hook for damage scale in car here
		}
	}

	// Must hate the hit entity
	if ( IRelationType( pEntity ) == D_HT )
	{
		if ( iDamage > 0 )
		{
			CTakeDamageInfo info( this, this, iDamage, iDmgType );
			CalculateMeleeDamageForce( &info, (vEnd - vStart), vStart, forceScale );
			pEntity->TakeDamage( info );
		}
	}
	return pEntity;

#endif

}


bool  CBaseCombatCharacter::Event_Gibbed( const CTakeDamageInfo &info )
{
	bool fade = false;

	if ( HasHumanGibs() )
	{
		ConVarRef violence_hgibs( "violence_hgibs" );
		if ( violence_hgibs.IsValid() && violence_hgibs.GetInt() == 0 )
		{
			fade = true;
		}
	}
	else if ( HasAlienGibs() )
	{
		ConVarRef violence_agibs( "violence_agibs" );
		if ( violence_agibs.IsValid() && violence_agibs.GetInt() == 0 )
		{
			fade = true;
		}
	}

	m_takedamage	= DAMAGE_NO;
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_lifeState		= LIFE_DEAD;

	if ( fade )
	{
		CorpseFade();
		return false;
	}
	else
	{
		AddEffects( EF_NODRAW ); // make the model invisible.
		return CorpseGib( info );
	}
}


Vector CBaseCombatCharacter::CalcDamageForceVector( const CTakeDamageInfo &info )
{
	// Already have a damage force in the data, use that.
	bool bNoPhysicsForceDamage = g_pGameRules->Damage_NoPhysicsForce( info.GetDamageType() );
	if ( info.GetDamageForce() != vec3_origin || bNoPhysicsForceDamage )
	{
		if( info.GetDamageType() & DMG_BLAST )
		{
			// Fudge blast forces a little bit, so that each
			// victim gets a slightly different trajectory. 
			// This simulates features that usually vary from
			// person-to-person variables such as bodyweight,
			// which are all indentical for characters using the same model.
			float scale = random->RandomFloat( 0.85, 1.15 );
			Vector force = info.GetDamageForce();
			force.x *= scale;
			force.y *= scale;
			// Try to always exaggerate the upward force because we've got pretty harsh gravity
			force.z *= (force.z > 0) ? 1.15 : scale;
			return force;
		}

		return info.GetDamageForce();
	}

	CBaseEntity *pForce = info.GetInflictor();
	if ( !pForce )
	{
		pForce = info.GetAttacker();
	}

	if ( pForce )
	{
		// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
		float forceScale = info.GetDamage() * 75 * 4;

		Vector forceVector;
		// If the damage is a blast, point the force vector higher than usual, this gives 
		// the ragdolls a bodacious "really got blowed up" look.
		if( info.GetDamageType() & DMG_BLAST )
		{
			// exaggerate the force from explosions a little (37.5%)
			forceVector = (GetLocalOrigin() + Vector(0, 0, WorldAlignSize().z) ) - pForce->GetLocalOrigin();
			VectorNormalize(forceVector);
			forceVector *= 1.375f;
		}
		else
		{
			// taking damage from self?  Take a little random force, but still try to collapse on the spot.
			if ( this == pForce )
			{
				forceVector.x = random->RandomFloat( -1.0f, 1.0f );
				forceVector.y = random->RandomFloat( -1.0f, 1.0f );
				forceVector.z = 0.0;
				forceScale = random->RandomFloat( 1000.0f, 2000.0f );
			}
			else
			{
				// UNDONE: Collision forces are baked in to CTakeDamageInfo now
				// UNDONE: Is this MOVETYPE_VPHYSICS code still necessary?
				if ( pForce->GetMoveType() == MOVETYPE_VPHYSICS )
				{
					// killed by a physics object
					IPhysicsObject *pPhysics = VPhysicsGetObject();
					if ( !pPhysics )
					{
						pPhysics = pForce->VPhysicsGetObject();
					}
					pPhysics->GetVelocity( &forceVector, NULL );
					forceScale = pPhysics->GetMass();
				}
				else
				{
					forceVector = GetLocalOrigin() - pForce->GetLocalOrigin();
					VectorNormalize(forceVector);
				}
			}
		}
		return forceVector * forceScale;
	}
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::FixupBurningServerRagdoll( CBaseEntity *pRagdoll )
{
	if ( !IsOnFire() )
		return;

	// Move the fire effects entity to the ragdoll
	CEntityFlame *pFireChild = dynamic_cast<CEntityFlame *>( GetEffectEntity() );
	if ( pFireChild )
	{
		SetEffectEntity( NULL );
		pRagdoll->AddFlag( FL_ONFIRE );
		pFireChild->SetAbsOrigin( pRagdoll->GetAbsOrigin() );
		pFireChild->AttachToEntity( pRagdoll );
		pFireChild->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
 		pRagdoll->SetEffectEntity( pFireChild );

		color32 color = GetRenderColor();
		pRagdoll->SetRenderColor( color.r, color.g, color.b );
	}
}

bool CBaseCombatCharacter::BecomeRagdollBoogie( CBaseEntity *pKiller, const Vector &forceVector, float duration, int flags )
{
	Assert( CanBecomeRagdoll() );

	CTakeDamageInfo info( pKiller, pKiller, 1.0f, DMG_GENERIC );

	info.SetDamageForce( forceVector );

	CBaseEntity *pRagdoll = CreateServerRagdoll( this, 0, info, COLLISION_GROUP_INTERACTIVE_DEBRIS, true );

	pRagdoll->SetCollisionBounds( CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs() );

	CRagdollBoogie::Create( pRagdoll, 200, gpGlobals->curtime, duration, flags );

	CTakeDamageInfo ragdollInfo( pKiller, pKiller, 10000.0, DMG_GENERIC | DMG_REMOVENORAGDOLL );
	ragdollInfo.SetDamagePosition(WorldSpaceCenter());
	ragdollInfo.SetDamageForce( Vector( 0, 0, 1) );
	TakeDamage( ragdollInfo );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	if ( (info.GetDamageType() & DMG_VEHICLE) && !g_pGameRules->IsMultiplayer() )
	{
		CTakeDamageInfo info2 = info;
		info2.SetDamageForce( forceVector );
		Vector pos = info2.GetDamagePosition();
		float flAbsMinsZ = GetAbsOrigin().z + WorldAlignMins().z;
		if ( (pos.z - flAbsMinsZ) < 24 )
		{
			// HACKHACK: Make sure the vehicle impact is at least 2ft off the ground
			pos.z = flAbsMinsZ + 24;
			info2.SetDamagePosition( pos );
		}

// UNDONE: Put in a real sound cue here, don't do this bogus hack anymore
#if 0
		Vector soundOrigin = info.GetDamagePosition();
		CPASAttenuationFilter filter( soundOrigin );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;
		ep.m_pSoundName = "NPC_MetroPolice.HitByVehicle";
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NORM;
		ep.m_pOrigin = &soundOrigin;

		EmitSound( filter, SOUND_FROM_WORLD, ep );
#endif
		// in single player create ragdolls on the server when the player hits someone
		// with their vehicle - for more dramatic death/collisions
		CBaseEntity *pRagdoll = CreateServerRagdoll( this, m_nForceBone, info2, COLLISION_GROUP_INTERACTIVE_DEBRIS, true );
		FixupBurningServerRagdoll( pRagdoll );
		RemoveDeferred();
		return true;
	}

	//Fix up the force applied to server side ragdolls. This fixes magnets not affecting them.
	CTakeDamageInfo newinfo = info;
	newinfo.SetDamageForce( forceVector );

#ifdef HL2_EPISODIC
	// Burning corpses are server-side in episodic, if we're in darkness mode
	if ( IsOnFire() && HL2GameRules()->IsAlyxInDarknessMode() )
	{
		CBaseEntity *pRagdoll = CreateServerRagdoll( this, m_nForceBone, newinfo, COLLISION_GROUP_DEBRIS );
		FixupBurningServerRagdoll( pRagdoll );
		RemoveDeferred();
		return true;
	}
#endif

#ifdef HL2_DLL	
	// Mega physgun requires everything to be a server-side ragdoll
	if ( m_bForceServerRagdoll == true || ( HL2GameRules()->MegaPhyscannonActive() == true ) && !IsPlayer() && Classify() != CLASS_PLAYER_ALLY_VITAL && Classify() != CLASS_PLAYER_ALLY )
	{
		if ( CanBecomeServerRagdoll() == false )
			return false;

		//FIXME: This is fairly leafy to be here, but time is short!
		CBaseEntity *pRagdoll = CreateServerRagdoll( this, m_nForceBone, newinfo, COLLISION_GROUP_INTERACTIVE_DEBRIS, true );
		FixupBurningServerRagdoll( pRagdoll );
		PhysSetEntityGameFlags( pRagdoll, FVPHYSICS_NO_SELF_COLLISIONS );
		RemoveDeferred();

		return true;
	}

	if( hl2_episodic.GetBool() && Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		CreateServerRagdoll( this, m_nForceBone, newinfo, COLLISION_GROUP_INTERACTIVE_DEBRIS, true );
		RemoveDeferred();
		return true;
	}
#endif //HL2_DLL

	return BecomeRagdollOnClient( forceVector );
}


/*
============
Killed
============
*/
void CBaseCombatCharacter::Event_Killed( const CTakeDamageInfo &info )
{
	extern ConVar npc_vphysics;

	// Advance life state to dying
	m_lifeState = LIFE_DYING;

	// Calculate death force
	Vector forceVector = CalcDamageForceVector( info );

	// See if there's a ragdoll magnet that should influence our force.
	CRagdollMagnet *pMagnet = CRagdollMagnet::FindBestMagnet( this );
	if( pMagnet )
	{
		forceVector += pMagnet->GetForceVector( this );
	}

	CBaseCombatWeapon *pDroppedWeapon = m_hActiveWeapon.Get();

	// Drop any weapon that I own
	if ( VPhysicsGetObject() )
	{
		Vector weaponForce = forceVector * VPhysicsGetObject()->GetInvMass();
		Weapon_Drop( m_hActiveWeapon, NULL, &weaponForce );
	}
	else
	{
		Weapon_Drop( m_hActiveWeapon );
	}
	
	// if flagged to drop a health kit
	if (HasSpawnFlags(SF_NPC_DROP_HEALTHKIT))
	{
		CBaseEntity::Create( "item_healthvial", GetAbsOrigin(), GetAbsAngles() );
	}
	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EmitSound( "BaseCombatCharacter.StopWeaponSounds" );

	// Tell my killer that he got me!
	if( info.GetAttacker() )
	{
		info.GetAttacker()->Event_KilledOther(this, info);
		g_EventQueue.AddEvent( info.GetAttacker(), "KilledNPC", 0.3, this, this );
	}
	SendOnKilledGameEvent( info );

	// Ragdoll unless we've gibbed
	if ( ShouldGib( info ) == false )
	{
		bool bRagdollCreated = false;
		if ( (info.GetDamageType() & DMG_DISSOLVE) && CanBecomeRagdoll() )
		{
			int nDissolveType = ENTITY_DISSOLVE_NORMAL;
			if ( info.GetDamageType() & DMG_SHOCK )
			{
				nDissolveType = ENTITY_DISSOLVE_ELECTRICAL;
			}

			bRagdollCreated = Dissolve( NULL, gpGlobals->curtime, false, nDissolveType );

			// Also dissolve any weapons we dropped
			if ( pDroppedWeapon )
			{
				pDroppedWeapon->Dissolve( NULL, gpGlobals->curtime, false, nDissolveType );
			}
		}
#ifdef HL2_DLL
		else if ( PlayerHasMegaPhysCannon() )
		{
			if ( pDroppedWeapon )
			{
				pDroppedWeapon->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
			}
		}
#endif

		if ( !bRagdollCreated && ( info.GetDamageType() & DMG_REMOVENORAGDOLL ) == 0 )
		{
			BecomeRagdoll( info, forceVector );
		}
	}
}

void CBaseCombatCharacter::Event_Dying( void )
{
}


// ===========================================================================
//  > Weapons
// ===========================================================================
bool CBaseCombatCharacter::Weapon_Detach( CBaseCombatWeapon *pWeapon )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( pWeapon == m_hMyWeapons[i] )
		{
			m_hMyWeapons.Set( i, NULL );
			pWeapon->SetOwner( NULL );

			if ( pWeapon == m_hActiveWeapon )
				ClearActiveWeapon();
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// For weapon strip
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::ThrowDirForWeaponStrip( CBaseCombatWeapon *pWeapon, const Vector &vecForward, Vector *pVecThrowDir )
{
	// HACK! Always throw the physcannon directly in front of the player
	// This is necessary for the physgun upgrade scene.
	if ( FClassnameIs( pWeapon, "weapon_physcannon" ) )
	{
		if( hl2_episodic.GetBool() )
		{
			// It has been discovered that it's possible to throw the physcannon out of the world this way.
			// So try to find a direction to throw the physcannon that's legal.
			Vector vecOrigin = EyePosition();
			Vector vecRight;

			CrossProduct( vecForward, Vector( 0, 0, 1), vecRight );

			Vector vecTest[ 4 ];
			vecTest[0] = vecForward;
			vecTest[1] = -vecForward;
			vecTest[2] = vecRight;
			vecTest[3] = -vecRight;

			trace_t tr;
			int i;
			for( i = 0 ; i < 4 ; i++ )
			{
				UTIL_TraceLine( vecOrigin, vecOrigin + vecTest[ i ] * 48.0f, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

				if ( !tr.startsolid && tr.fraction == 1.0f )
				{
					*pVecThrowDir = vecTest[ i ];
					return;
				}
			}
		}

		// Well, fall through to what we did before we tried to make this a bit more robust.
		*pVecThrowDir = vecForward;
	}
	else
	{
		// Nowhere in particular; just drop it.
		VMatrix zRot;
		MatrixBuildRotateZ( zRot, random->RandomFloat( -60.0f, 60.0f ) );

		Vector vecThrow;
		Vector3DMultiply( zRot, vecForward, *pVecThrowDir );

		pVecThrowDir->z = random->RandomFloat( -0.5f, 0.5f );
		VectorNormalize( *pVecThrowDir );
	}
}


//-----------------------------------------------------------------------------
// For weapon strip
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::DropWeaponForWeaponStrip( CBaseCombatWeapon *pWeapon, 
	const Vector &vecForward, const QAngle &vecAngles, float flDiameter )
{
	Vector vecOrigin;
	CollisionProp()->RandomPointInBounds( Vector( 0.5f, 0.5f, 0.5f ), Vector( 0.5f, 0.5f, 1.0f ), &vecOrigin );

	// Nowhere in particular; just drop it.
	Vector vecThrow;
	ThrowDirForWeaponStrip( pWeapon, vecForward, &vecThrow );

	Vector vecOffsetOrigin;
	VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );

	trace_t	tr;
	UTIL_TraceLine( vecOrigin, vecOffsetOrigin, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		
	if ( tr.startsolid || tr.allsolid || ( tr.fraction < 1.0f && tr.m_pEnt != pWeapon ) )
	{
		//FIXME: Throw towards a known safe spot?
		vecThrow.Negate();
		VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );
	}

	vecThrow *= random->RandomFloat( 400.0f, 600.0f );

	pWeapon->SetAbsOrigin( vecOrigin );
	pWeapon->SetAbsAngles( vecAngles );
	pWeapon->Drop( vecThrow );
	pWeapon->SetRemoveable( false );
	Weapon_Detach( pWeapon );
}



//-----------------------------------------------------------------------------
// For weapon strip
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_DropAll( bool bDisallowWeaponPickup )
{
	if ( GetFlags() & FL_NPC )
	{
		for (int i=0; i<MAX_WEAPONS; ++i) 
		{
			CBaseCombatWeapon *pWeapon = m_hMyWeapons[i];
			if (!pWeapon)
				continue;

 			Weapon_Drop( pWeapon );
		}
		return;
	}

	QAngle gunAngles;
	VectorAngles( BodyDirection2D(), gunAngles );

	Vector vecForward;
	AngleVectors( gunAngles, &vecForward, NULL, NULL );

	float flDiameter = sqrt( CollisionProp()->OBBSize().x * CollisionProp()->OBBSize().x +
		CollisionProp()->OBBSize().y * CollisionProp()->OBBSize().y );

	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	for (int i=0; i<MAX_WEAPONS; ++i) 
	{
		CBaseCombatWeapon *pWeapon = m_hMyWeapons[i];
		if (!pWeapon)
			continue;

		// Have to drop this after we've dropped everything else, so autoswitch doesn't happen
		if ( pWeapon == pActiveWeapon )
			continue;

		DropWeaponForWeaponStrip( pWeapon, vecForward, gunAngles, flDiameter );

		// HACK: This hack is required to allow weapons to be disintegrated
		// in the citadel weapon-strip scene
		// Make them not pick-uppable again. This also has the effect of allowing weapons
		// to collide with triggers. 
		if ( bDisallowWeaponPickup )
		{
			pWeapon->RemoveSolidFlags( FSOLID_TRIGGER );
			
			IPhysicsObject *pObj = pWeapon->VPhysicsGetObject();
			
			if ( pObj != NULL )
			{	
				pObj->SetGameFlags( FVPHYSICS_NO_PLAYER_PICKUP );
			}
		}
	}

	// Drop the active weapon normally...
	if ( pActiveWeapon )
	{
		// Nowhere in particular; just drop it.
		Vector vecThrow;
		ThrowDirForWeaponStrip( pActiveWeapon, vecForward, &vecThrow );

		// Throw a little more vigorously; it starts closer to the player
		vecThrow *= random->RandomFloat( 800.0f, 1000.0f );

		Weapon_Drop( pActiveWeapon, NULL, &vecThrow );
		pActiveWeapon->SetRemoveable( false );

		// HACK: This hack is required to allow weapons to be disintegrated
		// in the citadel weapon-strip scene
		// Make them not pick-uppable again. This also has the effect of allowing weapons
		// to collide with triggers. 
		if ( bDisallowWeaponPickup )
		{
			pActiveWeapon->RemoveSolidFlags( FSOLID_TRIGGER );
		}
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: Drop the active weapon, optionally throwing it at the given target position.
// Input  : pWeapon - Weapon to drop/throw.
//			pvecTarget - Position to throw it at, NULL for none.
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	if ( !pWeapon )
		return;

	// If I'm an NPC, fill the weapon with ammo before I drop it.
	if ( GetFlags() & FL_NPC )
	{
		if ( pWeapon->UsesClipsForAmmo1() )
		{
			pWeapon->m_iClip1 = pWeapon->GetDefaultClip1();

			if( FClassnameIs( pWeapon, "weapon_smg1" ) )
			{
				// Drop enough ammo to kill 2 of me.
				// Figure out how much damage one piece of this type of ammo does to this type of enemy.
				float flAmmoDamage = g_pGameRules->GetAmmoDamage( UTIL_GetNearestPlayer( GetAbsOrigin() ), this, pWeapon->GetPrimaryAmmoType() );
				pWeapon->m_iClip1 = (GetMaxHealth() / flAmmoDamage) * 2;
			}
		}
		if ( pWeapon->UsesClipsForAmmo2() )
		{
			pWeapon->m_iClip2 = pWeapon->GetDefaultClip2();
		}

		if ( IsXbox() )
		{
			pWeapon->AddEffects( EF_ITEM_BLINK );
		}
	}

	if ( IsPlayer() )
	{
		Vector vThrowPos = Weapon_ShootPosition() - Vector(0,0,12);

		if( UTIL_PointContents(vThrowPos) & CONTENTS_SOLID )
		{
			Msg("Weapon spawning in solid!\n");
		}

		pWeapon->SetAbsOrigin( vThrowPos );

		QAngle gunAngles;
		VectorAngles( BodyDirection2D(), gunAngles );
		pWeapon->SetAbsAngles( gunAngles );
	}
	else
	{
		int iBIndex = -1;
		int iWeaponBoneIndex = -1;

		CStudioHdr *hdr = pWeapon->GetModelPtr();
		// If I have a hand, set the weapon position to my hand bone position.
		if ( hdr && hdr->numbones() > 0 )
		{
			// Assume bone zero is the root
			for ( iWeaponBoneIndex = 0; iWeaponBoneIndex < hdr->numbones(); ++iWeaponBoneIndex )
			{
				iBIndex = LookupBone( hdr->pBone( iWeaponBoneIndex )->pszName() );
				// Found one!
				if ( iBIndex != -1 )
				{
					break;
				}
			}

			if ( iBIndex == -1 )
			{
				iBIndex = LookupBone( "ValveBiped.Weapon_bone" );
			}
		}
		else
		{
			iBIndex = LookupBone( "ValveBiped.Weapon_bone" );
		}

		if ( iBIndex != -1)  
		{
			Vector origin;
			QAngle angles;
			matrix3x4_t transform;

			// Get the transform for the weapon bonetoworldspace in the NPC
			GetBoneTransform( iBIndex, transform );

			// find offset of root bone from origin in local space
			// Make sure we're detached from hierarchy before doing this!!!
			pWeapon->StopFollowingEntity();
			pWeapon->SetAbsOrigin( Vector( 0, 0, 0 ) );
			pWeapon->SetAbsAngles( QAngle( 0, 0, 0 ) );
			pWeapon->InvalidateBoneCache();
			matrix3x4_t rootLocal;
			pWeapon->GetBoneTransform( iWeaponBoneIndex, rootLocal );

			// invert it
			matrix3x4_t rootInvLocal;
			MatrixInvert( rootLocal, rootInvLocal );

			matrix3x4_t weaponMatrix;
			ConcatTransforms( transform, rootInvLocal, weaponMatrix );
			MatrixAngles( weaponMatrix, angles, origin );
			
			pWeapon->Teleport( &origin, &angles, NULL );
		}
		// Otherwise just set in front of me.
		else 
		{
			Vector vFacingDir = BodyDirection2D();
			vFacingDir = vFacingDir * 10.0; 
			pWeapon->SetAbsOrigin( Weapon_ShootPosition() + vFacingDir );
		}
	}

	Vector vecThrow;
	if (pvecTarget)
	{
		// I've been told to throw it somewhere specific.
		vecThrow = VecCheckToss( this, pWeapon->GetAbsOrigin(), *pvecTarget, 0.2, 1.0, false );
	}
	else
	{
		if ( pVelocity )
		{
			vecThrow = *pVelocity;
			float flLen = vecThrow.Length();
			if (flLen > 400)
			{
				VectorNormalize(vecThrow);
				vecThrow *= 400;
			}
		}
		else
		{
			// Nowhere in particular; just drop it.
			float throwForce = ( IsPlayer() ) ? 400.0f : random->RandomInt( 64, 128 );
			vecThrow = BodyDirection3D() * throwForce;
		}
	}

	pWeapon->Drop( vecThrow );
	Weapon_Detach( pWeapon );

	if ( HasSpawnFlags( SF_NPC_NO_WEAPON_DROP ) )
	{
		// Don't drop weapons when the super physgun is happening.
		UTIL_Remove( pWeapon );
	}

}


//-----------------------------------------------------------------------------
// Lighting origin
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetLightingOriginRelative( CBaseEntity *pLightingOrigin )
{
	BaseClass::SetLightingOriginRelative( pLightingOrigin );
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->SetLightingOriginRelative( pLightingOrigin );
	}
}


//-----------------------------------------------------------------------------
// Purpose:	Add new weapon to the character
// Input  : New weapon
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	// Add the weapon to my weapon inventory
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if (!m_hMyWeapons[i]) 
		{
			m_hMyWeapons.Set( i, pWeapon );
			break;
		}
	}

	// Weapon is now on my team
	pWeapon->ChangeTeam( GetTeamNumber() );

	// ----------------------
	//  Give Primary Ammo
	// ----------------------
	// If gun doesn't use clips, just give ammo
	if (pWeapon->GetMaxClip1() == -1)
	{
#ifdef HL2_DLL
		if( FStrEq(STRING(gpGlobals->mapname), "d3_c17_09") && FClassnameIs(pWeapon, "weapon_rpg") && pWeapon->NameMatches("player_spawn_items") )
		{
			// !!!HACK - Don't give any ammo with the spawn equipment RPG in d3_c17_09. This is a chapter
			// start and the map is way to easy if you start with 3 RPG rounds. It's fine if a player conserves
			// them and uses them here, but it's not OK to start with enough ammo to bypass the snipers completely.
			GiveAmmo( 0, pWeapon->m_iPrimaryAmmoType); 
		}
		else
#endif // HL2_DLL
		GiveAmmo(pWeapon->GetDefaultClip1(), pWeapon->m_iPrimaryAmmoType); 
	}
	// If default ammo given is greater than clip
	// size, fill clips and give extra ammo
	else if (pWeapon->GetDefaultClip1() >  pWeapon->GetMaxClip1() )
	{
		pWeapon->m_iClip1 = pWeapon->GetMaxClip1();
		GiveAmmo( (pWeapon->GetDefaultClip1() - pWeapon->GetMaxClip1()), pWeapon->m_iPrimaryAmmoType); 
	}

	// ----------------------
	//  Give Secondary Ammo
	// ----------------------
	// If gun doesn't use clips, just give ammo
	if (pWeapon->GetMaxClip2() == -1)
	{
		GiveAmmo(pWeapon->GetDefaultClip2(), pWeapon->m_iSecondaryAmmoType); 
	}
	// If default ammo given is greater than clip
	// size, fill clips and give extra ammo
	else if ( pWeapon->GetDefaultClip2() > pWeapon->GetMaxClip2() )
	{
		pWeapon->m_iClip2 = pWeapon->GetMaxClip2();
		GiveAmmo( (pWeapon->GetDefaultClip2() - pWeapon->GetMaxClip2()), pWeapon->m_iSecondaryAmmoType); 
	}

	pWeapon->Equip( this );

	// Players don't automatically holster their current weapon
	if ( IsPlayer() == false )
	{
		if ( m_hActiveWeapon )
		{
			m_hActiveWeapon->Holster();
			// FIXME: isn't this handeled by the weapon?
			m_hActiveWeapon->AddEffects( EF_NODRAW );
		}
		SetActiveWeapon( pWeapon );
		m_hActiveWeapon->RemoveEffects( EF_NODRAW );

	}
	
	// Gotta do this *after* Equip because it may whack maxRange
	if ( IsPlayer() == false )
	{
		// If SF_NPC_LONG_RANGE spawn flags is set let weapon work from any distance
		if ( HasSpawnFlags(SF_NPC_LONG_RANGE) )
		{
			m_hActiveWeapon->m_fMaxRange1 = 999999999;
			m_hActiveWeapon->m_fMaxRange2 = 999999999;
		}
	}

	WeaponProficiency_t proficiency;
	proficiency = CalcWeaponProficiency( pWeapon );
	
	if( weapon_showproficiency.GetBool() != 0 )
	{
		Msg("%s equipped with %s, proficiency is %s\n", GetClassname(), pWeapon->GetClassname(), GetWeaponProficiencyName( proficiency ) );
	}

	SetCurrentWeaponProficiency( proficiency );

	// Pass the lighting origin over to the weapon if we have one
	pWeapon->SetLightingOriginRelative( GetLightingOriginRelative() );
}

//-----------------------------------------------------------------------------
// Purpose:	Leaves weapon, giving only ammo to the character
// Input  : Weapon
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_EquipAmmoOnly( CBaseCombatWeapon *pWeapon )
{
	// Check for duplicates
	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if ( m_hMyWeapons[i].Get() && FClassnameIs(m_hMyWeapons[i], pWeapon->GetClassname()) )
		{
			// Just give the ammo from the clip
			int	primaryGiven	= (pWeapon->UsesClipsForAmmo1()) ? pWeapon->m_iClip1 : pWeapon->GetPrimaryAmmoCount();
			int secondaryGiven	= (pWeapon->UsesClipsForAmmo2()) ? pWeapon->m_iClip2 : pWeapon->GetSecondaryAmmoCount();

			int takenPrimary   = GiveAmmo( primaryGiven, pWeapon->m_iPrimaryAmmoType); 
			int takenSecondary = GiveAmmo( secondaryGiven, pWeapon->m_iSecondaryAmmoType); 
			
			if( pWeapon->UsesClipsForAmmo1() )
			{
				pWeapon->m_iClip1 -= takenPrimary;
			}
			else
			{
				pWeapon->SetPrimaryAmmoCount( pWeapon->GetPrimaryAmmoCount() - takenPrimary );
			}

			if( pWeapon->UsesClipsForAmmo2() )
			{
				pWeapon->m_iClip2 -= takenSecondary;
			}
			else
			{
				pWeapon->SetSecondaryAmmoCount( pWeapon->GetSecondaryAmmoCount() - takenSecondary );
			}
			
			//Only succeed if we've taken ammo from the weapon
			if ( takenPrimary > 0 || takenSecondary > 0 )
				return true;
			
			return false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the weapon passed in would occupy a slot already occupied by the carrier
// Input  : *pWeapon - weapon to test for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon == NULL )
		return false;

	//Check to see if there's a resident weapon already in this slot
	if ( Weapon_GetSlot( pWeapon->GetSlot() ) == NULL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon (if any) in the requested slot
// Input  : slot - which slot to poll
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetSlot( int slot ) const
{
	int	targetSlot = slot;

	// Check for that slot being occupied already
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		if ( m_hMyWeapons[i].Get() != NULL )
		{
			// If the slots match, it's already occupied
			if ( m_hMyWeapons[i]->GetSlot() == targetSlot )
				return m_hMyWeapons[i];
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to a weapon this character has that uses the specified ammo
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetWpnForAmmo( int iAmmoIndex )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CBaseCombatWeapon *weapon = GetWeapon( i );
		if ( !weapon )
			continue;

		if ( weapon->GetPrimaryAmmoType() == iAmmoIndex )
			return weapon;
		if ( weapon->GetSecondaryAmmoType() == iAmmoIndex )
			return weapon;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Can this character operate this weapon?
// Input  : A weapon
// Output :	true or false
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	acttable_t *pTable		= pWeapon->ActivityList();
	int			actCount	= pWeapon->ActivityListCount();

	if( actCount < 1 )
	{
		// If the weapon has no activity table, it definitely cannot be used.
		return false;
	}

	for ( int i = 0; i < actCount; i++, pTable++ )
	{
		if ( pTable->required )
		{
			// The NPC might translate the weapon activity into another activity
			Activity translatedActivity = NPC_TranslateActivity( (Activity)(pTable->weaponAct) );

			if ( SelectWeightedSequence(translatedActivity) == ACTIVITY_NOT_AVAILABLE )
			{
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_Create( const char *pWeaponName )
{
	CBaseCombatWeapon *pWeapon = static_cast<CBaseCombatWeapon *>( Create( pWeaponName, GetLocalOrigin(), GetLocalAngles(), this ) );

	return pWeapon;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	// UNDONE: Some check to make sure that pEvent->pSource is a weapon I'm holding?
	if ( m_hActiveWeapon )
	{
		// UNDONE: Pass to pEvent->pSource instead?
		m_hActiveWeapon->Operator_HandleAnimEvent( pEvent, this );
	}
}

void CBaseCombatCharacter::RemoveAllWeapons()
{
	ClearActiveWeapon();
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( m_hMyWeapons[i] )
		{
			m_hMyWeapons[i]->Delete( );
			m_hMyWeapons.Set( i, NULL );
		}
	}
}


// take health
int CBaseCombatCharacter::TakeHealth (float flHealth, int bitsDamageType)
{
	if (!m_takedamage)
		return 0;
	
	return BaseClass::TakeHealth(flHealth, bitsDamageType);
}


/*
============
OnTakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the NPC is within the trigger_hurt.
When a NPC is poisoned via an arrow etc it takes all the poison damage at once.



GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
int CBaseCombatCharacter::OnTakeDamage( const CTakeDamageInfo &info )
{
	int retVal = 0;

	if (!m_takedamage)
		return 0;

	m_iDamageCount++;

	if ( info.GetDamageType() & DMG_SHOCK )
	{
		g_pEffects->Sparks( info.GetDamagePosition(), 2, 2 );
		UTIL_Smoke( info.GetDamagePosition(), random->RandomInt( 10, 15 ), 10 );
	}

	switch( m_lifeState )
	{
	case LIFE_ALIVE:
		retVal = OnTakeDamage_Alive( info );
		if ( m_iHealth <= 0 )
		{
			IPhysicsObject *pPhysics = VPhysicsGetObject();
			if ( pPhysics )
			{
				pPhysics->EnableCollisions( false );
			}
			
			bool bGibbed = false;

			Event_Killed( info );

			// Only classes that specifically request it are gibbed
			if ( ShouldGib( info ) )
			{
				bGibbed = Event_Gibbed( info );
			}
			
			if ( bGibbed == false )
			{
				Event_Dying();
			}
		}
		return retVal;
		break;

	case LIFE_DYING:
		return OnTakeDamage_Dying( info );
	
	default:
	case LIFE_DEAD:
		retVal = OnTakeDamage_Dead( info );
		if ( m_iHealth <= 0 && g_pGameRules->Damage_ShouldGibCorpse( info.GetDamageType() ) && ShouldGib( info ) )
		{
			Event_Gibbed( info );
			retVal = 0;
		}
		return retVal;
	}
}


int CBaseCombatCharacter::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	if (info.GetInflictor())
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize(vecDir);
	}
	g_vecAttackDir = vecDir;

	//!!!LATER - make armor consideration here!
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		// Separate the fractional amount of damage from the whole
		float flFractionalDamage = info.GetDamage() - floor( info.GetDamage() );
		float flIntegerDamage = info.GetDamage() - flFractionalDamage;

		// Add fractional damage to the accumulator
		m_flDamageAccumulator += flFractionalDamage;

		// If the accumulator is holding a full point of damage, move that point
		// of damage into the damage we're about to inflict.
		if( m_flDamageAccumulator >= 1.0 )
		{
			flIntegerDamage += 1.0;
			m_flDamageAccumulator -= 1.0;
		}

		if ( flIntegerDamage <= 0 )
			return 0;

		m_iHealth -= flIntegerDamage;
	}

	return 1;
}


int CBaseCombatCharacter::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	return 1;
}

int CBaseCombatCharacter::OnTakeDamage_Dead( const CTakeDamageInfo &info )
{
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		m_iHealth -= info.GetDamage();
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Sets vBodyDir to the body direction (2D) of the combat character.  
//			Used as NPC's and players extract facing direction differently
// Input  :
// Output :
//-----------------------------------------------------------------------------
QAngle CBaseCombatCharacter::BodyAngles()
{
	return GetAbsAngles();
}


Vector CBaseCombatCharacter::BodyDirection2D( void )
{
	Vector vBodyDir = BodyDirection3D( );
	vBodyDir.z = 0;
	vBodyDir.AsVector2D().NormalizeInPlace();
	return vBodyDir;
}


Vector CBaseCombatCharacter::BodyDirection3D( void )
{
	QAngle angles = BodyAngles();

	// FIXME: cache this
	Vector vBodyDir;
	AngleVectors( angles, &vBodyDir );
	return vBodyDir;
}


void CBaseCombatCharacter::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Skip this work if we're already marked for transmission.
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );

	bool bLocalPlayer = ( pInfo->m_pClientEnt == edict() );

	if ( bLocalPlayer )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CBaseCombatWeapon *pWeapon = m_hMyWeapons[i];
			if ( !pWeapon )
				continue;

			// The local player is sent all of his weapons.
			pWeapon->SetTransmit( pInfo, bAlways );
		}
	}
	else
	{
		// The check for EF_NODRAW is useless because the weapon will be networked anyway. In CBaseCombatWeapon::
		// UpdateTransmitState all weapons with owners will transmit to clients in the PVS.
		if ( m_hActiveWeapon && !m_hActiveWeapon->IsEffectActive( EF_NODRAW ) )
			m_hActiveWeapon->SetTransmit( pInfo, bAlways );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Add or Change a class relationship for this entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AddClassRelationship ( Class_T class_type, Disposition_t disposition, int priority )
{
	// First check to see if a relationship has already been declared for this class
	// If so, update it with the new relationship
	for (int i=m_Relationship.Count()-1;i >= 0;i--) 
	{
		if (m_Relationship[i].classType == class_type) 
		{
			m_Relationship[i].disposition = disposition;
			if ( priority != DEF_RELATIONSHIP_PRIORITY )
				m_Relationship[i].priority	  = priority;
			return;
		}
	}

	int index = m_Relationship.AddToTail();
	// Add the new class relationship to our relationship table
	m_Relationship[index].classType		= class_type;
	m_Relationship[index].entity		= NULL;
	m_Relationship[index].disposition	= disposition;
	m_Relationship[index].priority		= ( priority != DEF_RELATIONSHIP_PRIORITY ) ? priority : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Add or Change a entity relationship for this entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AddEntityRelationship ( CBaseEntity* pEntity, Disposition_t disposition, int priority )
{
	// First check to see if a relationship has already been declared for this entity
	// If so, update it with the new relationship
	for (int i=m_Relationship.Count()-1;i >= 0;i--) 
	{
		if (m_Relationship[i].entity == pEntity) 
		{
			m_Relationship[i].disposition	= disposition;
			if ( priority != DEF_RELATIONSHIP_PRIORITY )
				m_Relationship[i].priority	= priority;
			return;
		}
	}

	int index = m_Relationship.AddToTail();
	// Add the new class relationship to our relationship table
	m_Relationship[index].classType		= CLASS_NONE;
	m_Relationship[index].entity		= pEntity;
	m_Relationship[index].disposition	= disposition;
	m_Relationship[index].priority		= ( priority != DEF_RELATIONSHIP_PRIORITY ) ? priority : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Removes an entity relationship from our list
// Input  : *pEntity - Entity with whom the relationship should be ended
// Output : True is entity was removed, false if it was not found
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::RemoveEntityRelationship( CBaseEntity *pEntity )
{
	// Find the entity in our list, if it exists
	for ( int i = m_Relationship.Count()-1; i >= 0; i-- ) 
	{
		if ( m_Relationship[i].entity == pEntity )
		{
			// Done, remove it
			m_Relationship.Remove( i );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Allocates default relationships
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AllocateDefaultRelationships( )
{
	if (!m_DefaultRelationship)
	{
		m_DefaultRelationship = new Relationship_t*[NUM_AI_CLASSES];

		for (int i=0; i<NUM_AI_CLASSES; ++i)
		{
			// Be default all relationships are neutral of priority zero
			m_DefaultRelationship[i] = new Relationship_t[NUM_AI_CLASSES];
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return an interaction ID (so we have no collisions)
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetDefaultRelationship(Class_T nClass, Class_T nClassTarget, Disposition_t nDisposition, int nPriority)
{
	if (m_DefaultRelationship)
	{
		m_DefaultRelationship[nClass][nClassTarget].disposition	= nDisposition;
		m_DefaultRelationship[nClass][nClassTarget].priority	= nPriority;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fetch the default (ignore ai_relationship changes) relationship
// Input  :
// Output :
//-----------------------------------------------------------------------------
Disposition_t CBaseCombatCharacter::GetDefaultRelationshipDisposition( Class_T nClassTarget )
{
	Assert( m_DefaultRelationship != NULL );

	return m_DefaultRelationship[Classify()][nClassTarget].disposition;
}


//-----------------------------------------------------------------------------
// Purpose: describes the relationship between two types of NPC.
// Input  :
// Output :
//-----------------------------------------------------------------------------
Relationship_t *CBaseCombatCharacter::FindEntityRelationship( CBaseEntity *pTarget )
{
	if ( !pTarget )
	{
		static Relationship_t dummy; 
		return &dummy;
	}

	// First check for specific relationship with this edict
	int i;
	for (i=0;i<m_Relationship.Count();i++) 
	{
		if (pTarget == (CBaseEntity *)m_Relationship[i].entity) 
		{
			return &m_Relationship[i];
		}
	}

	if (pTarget->Classify() != CLASS_NONE)
	{
		// Then check for relationship with this edict's class
		for (i=0;i<m_Relationship.Count();i++) 
		{
			if (pTarget->Classify() == m_Relationship[i].classType) 
			{
				return &m_Relationship[i];
			}
		}
	}
	AllocateDefaultRelationships();
	// If none found return the default
	return &m_DefaultRelationship[ Classify() ][ pTarget->Classify() ];
}

Disposition_t CBaseCombatCharacter::IRelationType ( CBaseEntity *pTarget )
{
	if ( pTarget )
		return FindEntityRelationship( pTarget )->disposition;
	return D_NU;
}

//-----------------------------------------------------------------------------
// Purpose: describes the relationship between two types of NPC.
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::IRelationPriority( CBaseEntity *pTarget )
{
	if ( pTarget )
		return FindEntityRelationship( pTarget )->priority;
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at current position/orientation
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CBaseCombatCharacter::Weapon_ShootPosition( )
{
	Vector forward, right, up;

	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	Vector vecSrc = GetAbsOrigin() 
					+ forward * m_HackedGunPos.y 
					+ right * m_HackedGunPos.x 
					+ up * m_HackedGunPos.z;

	return vecSrc;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::FindHealthItem( const Vector &vecPosition, const Vector &range )
{
	CBaseEntity *list[1024];
	int count = UTIL_EntitiesInBox( list, 1024, vecPosition - range, vecPosition + range, 0 );

	for ( int i = 0; i < count; i++ )
	{
		CItem *pItem = dynamic_cast<CItem *>(list[ i ]);

		if( pItem )
		{
			// Healthkits and healthvials
			if( pItem->ClassMatches( "item_health*" ) && FVisible( pItem ) )
			{
				return pItem;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Compares the weapon's center with this character's current origin, so it
// will not give reliable results for weapons that are visible to the NPC
// but are upstairs/downstairs, etc.
//
// A weapon is said to be on the ground if it is no more than 12 inches above
// or below the caller's feet.
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_IsOnGround( CBaseCombatWeapon *pWeapon )
{
	if( pWeapon->IsConstrained() )
	{
		// Constrained to a rack.
		return false;
	}

	if( fabs(pWeapon->WorldSpaceCenter().z - GetAbsOrigin().z) >= 12.0f )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &range - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::Weapon_FindUsable( const Vector &range )
{
	bool bConservative = false;

#ifdef HL2_DLL
	if( hl2_episodic.GetBool() && !GetActiveWeapon() )
	{
		// Unarmed citizens are conservative in their weapon finding
		if ( Classify() != CLASS_PLAYER_ALLY_VITAL )
		{
			bConservative = true;
		}
	}
#endif

	CBaseCombatWeapon *weaponList[64];
	CBaseCombatWeapon *pBestWeapon = NULL;

	Vector mins = GetAbsOrigin() - range;
	Vector maxs = GetAbsOrigin() + range;
	int listCount = CBaseCombatWeapon::GetAvailableWeaponsInBox( weaponList, ARRAYSIZE(weaponList), mins, maxs );

	float fBestDist = 1e6;

	for ( int i = 0; i < listCount; i++ )
	{
		// Make sure not moving (ie flying through the air)
		Vector velocity;

		CBaseCombatWeapon *pWeapon = weaponList[i];
		Assert(pWeapon);
		pWeapon->GetVelocity( &velocity, NULL );

		if ( pWeapon->CanBePickedUpByNPCs() == false )
			continue;

		if ( velocity.LengthSqr() > 1 || !Weapon_CanUse(pWeapon) )
			continue;

		if ( pWeapon->IsLocked(this) )
			continue;

		if ( GetActiveWeapon() )
		{
			// Already armed. Would picking up this weapon improve my situation?
			if( GetActiveWeapon()->m_iClassname == pWeapon->m_iClassname )
			{
				// No, I'm already using this type of weapon.
				continue;
			}

			if( FClassnameIs( pWeapon, "weapon_pistol" ) )
			{
				// No, it's a pistol.
				continue;
			}
		}

		float fCurDist = (pWeapon->GetLocalOrigin() - GetLocalOrigin()).Length();

		// Give any reserved weapon a bonus
		if( pWeapon->HasSpawnFlags( SF_WEAPON_NO_PLAYER_PICKUP ) )
		{
			fCurDist *= 0.5f;
		}

		if ( pBestWeapon )
		{
			// UNDONE: Better heuristic needed here
			//			Need to pick by power of weapons
			//			Don't want to pick a weapon right next to a NPC!

			// Give the AR2 a bonus to be selected by making it seem closer.
			if( FClassnameIs( pWeapon, "weapon_ar2" ) )
			{
				fCurDist *= 0.5;
			}

			// choose the last range attack weapon you find or the first available other weapon
			if ( ! (pWeapon->CapabilitiesGet() & bits_CAP_RANGE_ATTACK_GROUP) )
			{
				continue;
			}
			else if (fCurDist > fBestDist ) 
			{
				continue;
			}
		}

		if( Weapon_IsOnGround(pWeapon) )
		{
			// Weapon appears to be lying on the ground. Make sure this weapon is reachable
			// by tracing out a human sized hull just above the weapon.  If not, reject
			trace_t tr;

			Vector	vAboveWeapon = pWeapon->GetAbsOrigin();
			UTIL_TraceEntity( this, vAboveWeapon, vAboveWeapon + Vector( 0, 0, 1 ), MASK_SOLID, pWeapon, COLLISION_GROUP_NONE, &tr );

			if ( tr.startsolid || (tr.fraction < 1.0) )
				continue;
		}
		else if( bConservative )
		{
			// Skip it.
			continue;
		}

		if( FVisible(pWeapon) )
		{
			fBestDist   = fCurDist;
			pBestWeapon = pWeapon;
		}
	}

	if( pBestWeapon )
	{
		// Lock this weapon for my exclusive use. Lock it for just a couple of seconds because my AI 
		// might not actually be able to go pick it up right now.
		pBestWeapon->Lock( 2.0, this );
	}


	return pBestWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound)
{
	if (iCount <= 0)
		return 0;

	if ( !g_pGameRules->CanHaveAmmo( this, iAmmoIndex ) )
	{
		// game rules say I can't have any more of this ammo type.
		return 0;
	}

	if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
		return 0;

	int iMax = GetAmmoDef()->MaxCarry(iAmmoIndex);
	int iAdd = min( iCount, iMax - m_iAmmo[iAmmoIndex] );
	if ( iAdd < 1 )
		return 0;

	// Ammo pickup sound
	if ( !bSuppressSound )
	{
		EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	m_iAmmo.Set( iAmmoIndex, m_iAmmo[iAmmoIndex] + iAdd );

	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::GiveAmmo( int iCount, const char *szName, bool bSuppressSound )
{
	int iAmmoType = GetAmmoDef()->Index(szName);
	if (iAmmoType == -1)
	{
		Msg("ERROR: Attempting to give unknown ammo type (%s)\n",szName);
		return 0;
	}
	return GiveAmmo( iCount, iAmmoType, bSuppressSound );
}


ConVar	phys_stressbodyweights( "phys_stressbodyweights", "5.0" );
void CBaseCombatCharacter::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if (!InRagdollMode())
	{
		ApplyStressDamage(pPhysics, false);
		BaseClass::VPhysicsUpdate(pPhysics);
	}
	else
	{
		ApplyStressDamage(pPhysics, true);

		if (m_lastRagdollUpdateTickCount == (unsigned int)gpGlobals->tickcount)
			return;

		m_lastRagdollUpdateTickCount = gpGlobals->tickcount;
		//NetworkStateChanged();

		matrix3x4_t boneToWorld[MAXSTUDIOBONES];
		QAngle angles;
		Vector surroundingMins, surroundingMaxs;

		int i;
		for (i = 0; i < m_ragdoll.listCount; i++)
		{
			CBoneAccessor boneaccessor(boneToWorld);
			if (RagdollGetBoneMatrix(m_ragdoll, boneaccessor, i))
			{
				Vector vNewPos;
				MatrixAngles(boneToWorld[m_ragdoll.boneIndex[i]], angles, vNewPos);
				m_ragPos.Set(i, vNewPos);
				m_ragAngles.Set(i, angles);
			}
			else
			{
				m_ragPos.GetForModify(i).Init();
				m_ragAngles.GetForModify(i).Init();
			}
		}

		// BUGBUG: Use the ragdollmins/maxs to do this instead of the collides
		m_ragAllAsleep = RagdollIsAsleep(m_ragdoll);

		// Don't scream after you've come to rest
		if (!m_ragAllAsleep)
		{
			if (m_ragdoll.pGroup->IsInErrorState())
			{
				RagdollSolveSeparation(m_ragdoll, this);
			}
		}

		// Interactive debris converts back to debris when it comes to rest
		if (m_ragAllAsleep && GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS)
		{
			SetCollisionGroup(COLLISION_GROUP_DEBRIS);
			RecheckRagdollCollisionFilter();
			//SetContextThink(NULL, gpGlobals->curtime, s_pDebrisContext);
		}

		Vector vecFullMins, vecFullMaxs;
		vecFullMins = m_ragPos[0];
		vecFullMaxs = m_ragPos[0];
		for (i = 0; i < m_ragdoll.listCount; i++)
		{
			Vector mins, maxs;
			matrix3x4_t update;
			if (!m_ragdoll.list[i].pObject)
			{
				m_ragdollMins[i].Init();
				m_ragdollMaxs[i].Init();
				continue;
			}
			m_ragdoll.list[i].pObject->GetPositionMatrix(&update);
			TransformAABB(update, m_ragdollMins[i], m_ragdollMaxs[i], mins, maxs);
			for (int j = 0; j < 3; j++)
			{
				if (mins[j] < vecFullMins[j])
				{
					vecFullMins[j] = mins[j];
				}
				if (maxs[j] > vecFullMaxs[j])
				{
					vecFullMaxs[j] = maxs[j];
				}
			}
		}

		SetAbsOrigin(m_ragPos[0]);
		SetAbsAngles(vec3_angle);
		const Vector& vecOrigin = CollisionProp()->GetCollisionOrigin();
		CollisionProp()->AddSolidFlags(FSOLID_FORCE_WORLD_ALIGNED);
		CollisionProp()->SetSurroundingBoundsType(USE_COLLISION_BOUNDS_NEVER_VPHYSICS);
		SetCollisionBounds(vecFullMins - vecOrigin, vecFullMaxs - vecOrigin);
		CollisionProp()->MarkSurroundingBoundsDirty();

		PhysicsTouchTriggers();
	}
}

float CBaseCombatCharacter::CalculatePhysicsStressDamage( vphysics_objectstress_t *pStressOut, IPhysicsObject *pPhysics )
{
	// stress damage hack.
	float mass = pPhysics->GetMass();
	CalculateObjectStress( pPhysics, this, pStressOut );
	float stress = (pStressOut->receivedStress * m_impactEnergyScale) / mass;

	// Make sure the stress isn't from being stuck inside some static object.
	// how many times your own weight can you hold up?
	if ( pStressOut->hasNonStaticStress && stress > phys_stressbodyweights.GetFloat() )
	{
		// if stuck, don't do this!
		if ( !(pPhysics->GetGameFlags() & FVPHYSICS_PENETRATING) )
			return 200;
	}

	return 0;
}

void CBaseCombatCharacter::ApplyStressDamage( IPhysicsObject *pPhysics, bool bRequireLargeObject )
{
#ifdef HL2_DLL
	if( Classify() == CLASS_PLAYER_ALLY || Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		// Bypass stress completely for allies and vitals.
		if( hl2_episodic.GetBool() )
			return;
	}
#endif//HL2_DLL

	vphysics_objectstress_t stressOut;
	float damage = CalculatePhysicsStressDamage( &stressOut, pPhysics );
	if ( damage > 0 )
	{
		if ( bRequireLargeObject && !stressOut.hasLargeObjectContact )
			return;

		//Msg("Stress! %.2f / %.2f\n", stressOut.exertedStress, stressOut.receivedStress );
		CTakeDamageInfo dmgInfo( GetWorldEntity(), GetWorldEntity(), vec3_origin, vec3_origin, damage, DMG_CRUSH );
		dmgInfo.SetDamageForce( Vector( 0, 0, -stressOut.receivedStress * sv_gravity.GetFloat() * gpGlobals->frametime ) );
		dmgInfo.SetDamagePosition( GetAbsOrigin() );
		TakeDamage( dmgInfo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const impactdamagetable_t
//-----------------------------------------------------------------------------
const impactdamagetable_t &CBaseCombatCharacter::GetPhysicsImpactDamageTable( void )
{
	return gDefaultNPCImpactDamageTable;
}

// how much to amplify impact forces
// This is to account for the ragdolls responding differently than
// the shadow objects.  Also this makes the impacts more dramatic.
ConVar	phys_impactforcescale( "phys_impactforcescale", "1.0" ); 
ConVar	phys_upimpactforcescale( "phys_upimpactforcescale", "0.375" ); 

void CBaseCombatCharacter::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	if (InRagdollMode())
		return;

	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];
	IPhysicsObject *pOtherPhysics = pEvent->pObjects[otherIndex];
	if ( !pOther )
		return;

	// Ragdolls are marked as dying.
	if ( pOther->m_lifeState == LIFE_DYING )
		return;

	if ( pOther->GetMoveType() != MOVETYPE_VPHYSICS )
		return;
	
	if ( !pOtherPhysics->IsMoveable() )
		return;
	
	if ( pOther == GetGroundEntity() )
		return;

	// Player can't damage himself if he's was physics attacker *on this frame*
	// which can occur owing to ordering issues it appears.
	float flOtherAttackerTime = 0.0f;

#ifdef HL2_DLL
	if ( HL2GameRules()->MegaPhyscannonActive() == true )
	{
		flOtherAttackerTime = 1.0f;
	}
#endif

	if ( this == pOther->HasPhysicsAttacker( flOtherAttackerTime ) )
		return;

	int damageType = 0;
	float damage = 0;

	damage = CalculatePhysicsImpactDamage( index, pEvent, GetPhysicsImpactDamageTable(), m_impactEnergyScale, false, damageType );
	
	if ( damage <= 0 )
		return;
	
	// NOTE: We really need some rotational motion for some of these collisions.
	// REVISIT: Maybe resolve this collision on death with a different (not approximately infinite like AABB tensor)
	// inertia tensor to get torque?
	Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass() * phys_impactforcescale.GetFloat();
	
	IServerVehicle *vehicleOther = pOther->GetServerVehicle();
	if ( vehicleOther )
	{
		CBaseCombatCharacter *pPassenger = vehicleOther->GetPassenger();
		if ( pPassenger != NULL )
		{
			// flag as vehicle damage
			damageType |= DMG_VEHICLE;
			// if hit by vehicle driven by player, add some upward velocity to force
			float len = damageForce.Length();
			damageForce.z += len*phys_upimpactforcescale.GetFloat();
			//Msg("Force %.1f / %.1f\n", damageForce.Length(), damageForce.z );

			if ( pPassenger->IsPlayer() )
			{
				CBasePlayer *pPlayer = assert_cast<CBasePlayer *>(pPassenger);
				if( damage >= GetMaxHealth() )
				{
					pPlayer->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAG_RESTART );
				}
				else
				{
					pPlayer->RumbleEffect( RUMBLE_PISTOL, 0, RUMBLE_FLAG_RESTART );
				}
			}
		}
	}

	Vector damagePos;
	pEvent->pInternalData->GetContactPoint( damagePos );
	CTakeDamageInfo dmgInfo( pOther, pOther, damageForce, damagePos, damage, damageType );

	// FIXME: is there a better way for physics objects to keep track of what root entity responsible for them moving?
	CBasePlayer *pPlayer = pOther->HasPhysicsAttacker( 1.0 );
	if (pPlayer)
	{
		dmgInfo.SetAttacker( pPlayer );
	}

	// UNDONE: Find one near damagePos?
	m_nForceBone = 0;
	PhysCallbackDamage( this, dmgInfo, *pEvent, index );
}


//-----------------------------------------------------------------------------
// Purpose: this entity is exploding, or otherwise needs to inflict damage upon 
//			entities within a certain range.  only damage ents that can clearly 
//			be seen by the explosion!
// Input  :
// Output :
//-----------------------------------------------------------------------------	
void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	// NOTE: I did this this way so I wouldn't have to change a whole bunch of
	// code unnecessarily. We need TF2 specific rules for RadiusDamage, so I moved
	// the implementation of radius damage into gamerules. All existing code calls
	// this method, which calls the game rules method
	g_pGameRules->RadiusDamage( info, vecSrc, flRadius, iClassIgnore, pEntityIgnore );

	// Let the world know if this was an explosion.
	if( info.GetDamageType() & DMG_BLAST )
	{
		// Even the tiniest explosion gets attention. Don't let the radius
		// be less than 128 units.
		float soundRadius = max( 128.0f, flRadius * 1.5 );

		CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_EXPLOSION, vecSrc, soundRadius, 0.25, info.GetInflictor() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Change active weapon and notify derived classes
//			
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetActiveWeapon( CBaseCombatWeapon *pNewWeapon )
{
	CBaseCombatWeapon *pOldWeapon = m_hActiveWeapon;
	if ( pNewWeapon != pOldWeapon )
	{
		m_hActiveWeapon = pNewWeapon;
		OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
	}
}

//-----------------------------------------------------------------------------
// Consider the weapon's built-in accuracy, this character's proficiency with
// the weapon, and the status of the target. Use this information to determine
// how accurately to shoot at the target.
//-----------------------------------------------------------------------------
Vector CBaseCombatCharacter::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	if ( pWeapon )
		return pWeapon->GetBulletSpread(GetCurrentWeaponProficiency());
	return VECTOR_CONE_15DEGREES;
}

//-----------------------------------------------------------------------------
float CBaseCombatCharacter::GetSpreadBias( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	if ( pWeapon )
		return pWeapon->GetSpreadBias(GetCurrentWeaponProficiency());
	return 1.0;
}

//-----------------------------------------------------------------------------
// Assume everyone is average with every weapon. Override this to make exceptions.
//-----------------------------------------------------------------------------
WeaponProficiency_t CBaseCombatCharacter::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	return WEAPON_PROFICIENCY_AVERAGE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define MAX_MISS_CANDIDATES 16
CBaseEntity *CBaseCombatCharacter::FindMissTarget( void )
{
	CBaseEntity *pMissCandidates[ MAX_MISS_CANDIDATES ];
	int numMissCandidates = 0;

	// CBasePlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this);
	CBasePlayer *pPlayer = UTIL_GetNearestPlayerPreferVisible(this);
	CBaseEntity *pEnts[256];
	Vector		radius( 100, 100, 100);
	Vector		vecSource = GetAbsOrigin();

	int numEnts = UTIL_EntitiesInBox( pEnts, 256, vecSource-radius, vecSource+radius, 0 );

	for ( int i = 0; i < numEnts; i++ )
	{
		if ( pEnts[i] == NULL )
			continue;

		// New rule for this system. Don't shoot what the player won't see.
		if ( pPlayer && !pPlayer->FInViewCone( pEnts[ i ] ) )
			continue;

		if ( numMissCandidates >= MAX_MISS_CANDIDATES )
			break;

		//See if it's a good target candidate
		if ( FClassnameIs( pEnts[i], "prop_dynamic" ) || 
			 FClassnameIs( pEnts[i], "prop_physics" ) || 
			 FClassnameIs( pEnts[i], "physics_prop" ) )
		{
			pMissCandidates[numMissCandidates++] = pEnts[i];
			continue;
		}
	}

	if( numMissCandidates == 0 )
		return NULL;

	return pMissCandidates[ random->RandomInt( 0, numMissCandidates - 1 ) ];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::ShouldShootMissTarget( CBaseCombatCharacter *pAttacker )
{
	// Don't shoot at NPC's right now.
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::InputKilledNPC( inputdata_t &inputdata )
{
	OnKilledNPC( inputdata.pActivator ? inputdata.pActivator->MyCombatCharacterPointer() : NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
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

void CBaseCombatCharacter::UpdateRagdollNetworkDataFromVPhysics(IPhysicsObject* pPhysics, int index)
{
	Assert(index < m_ragdoll.listCount);

	QAngle angles;
	Vector vPos;
	m_ragdoll.list[index].pObject->GetPosition(&vPos, &angles);
	m_ragPos.Set(index, vPos);
	m_ragAngles.Set(index, angles);

	// move/relink if root moved
	if (index == 0)
	{
		SetAbsOrigin(m_ragPos[0]);
		PhysicsTouchTriggers();
	}
}

void CBaseCombatCharacter::RecheckRagdollCollisionFilter(void)
{
	for (int i = 0; i < m_ragdoll.listCount; i++)
	{
		m_ragdoll.list[i].pObject->RecheckCollisionFilter();
	}
}

void CBaseCombatCharacter::RagdollMode_UnragdollCleanup()
{
	CreateVPhysics();
}

void CBaseCombatCharacter::RagdollMode_Enable(const CTakeDamageInfo& info, int forceBone)
{
	if (InRagdollMode())
		return;

	if (VPhysicsGetObject())
	{
		if (info.GetDamageType() & (DMG_VEHICLE | DMG_CRUSH))
		{
			// if the entity was killed by physics or a vehicle, move to the vphysics shadow position before creating the ragdoll.
			SyncAnimatingWithPhysics(this);
		}

		VPhysicsDestroyObject();
	}

	m_bRagdollEnabled = true;
	matrix3x4_t pBoneToWorld[MAXSTUDIOBONES], pBoneToWorldNext[MAXSTUDIOBONES];
	
	const float dt = TICK_INTERVAL;

	// NPC_STATE_DEAD npc's will have their COND_IN_PVS cleared, so this needs to force SetupBones to happen
	unsigned short fPrevFlags = GetBoneCacheFlags();
	SetBoneCacheFlags(BCF_NO_ANIMATION_SKIP);

	// UNDONE: Extract velocity from bones via animation (like we do on the client)
	// UNDONE: For now, just move each bone by the total entity velocity if set.
	// Get Bones positions before
	// Store current cycle
	float fSequenceDuration = SequenceDuration();
	float fSequenceTime = GetCycle() * fSequenceDuration;

	float fCurCycle = GetCycle();
	float fPreviousCycle = fmodf(1.f + fmodf((GetCycle() - (dt * GetPlaybackRate() * (1 / fSequenceDuration))), 1.f), 1.f);

	// Get current bones positions
	SetupBones(pBoneToWorldNext, BONE_USED_BY_ANYTHING);
	// Get previous bones positions
	SetCycle(fPreviousCycle);
	SetupBones(pBoneToWorld, BONE_USED_BY_ANYTHING);
	// Restore current cycle
	SetCycle(fCurCycle);

	// Reset previous bone flags
	ClearBoneCacheFlags(BCF_NO_ANIMATION_SKIP);
	SetBoneCacheFlags(fPrevFlags);

	Vector vel = GetAbsVelocity();
	if ((vel.IsZero()) && (dt > 0))
	{
		// Compute animation velocity
		CStudioHdr* pstudiohdr = GetModelPtr();
		if (pstudiohdr)
		{
			Vector deltaPos;
			QAngle deltaAngles;
			if (Studio_SeqMovement(pstudiohdr,
				GetSequence(),
				fPreviousCycle,
				fCurCycle,
				GetPoseParameterArray(),
				deltaPos,
				deltaAngles))
			{
				VectorRotate(deltaPos, EntityToWorldTransform(), vel);
				vel /= dt;
			}
		}
	}

	if (vel.LengthSqr() > 0)
	{
		int numbones = GetModelPtr()->numbones();
		vel *= dt;
		for (int i = 0; i < numbones; i++)
		{
			Vector pos;
			MatrixGetColumn(pBoneToWorld[i], 3, pos);
			pos -= vel;
			MatrixSetColumn(pos, 3, pBoneToWorld[i]);
		}
	}

	m_PreRagdollMoveType = GetMoveType();
	m_PreRagdollMoveCollide = GetMoveCollide();
	m_PreRagdollSolidFlags = GetSolidFlags();
	m_PreRagdollSolidType = GetSolid();

	SetMoveType(MOVETYPE_VPHYSICS);
	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST);

	// this is useless info after the initial conditions are set
	SetAbsAngles(vec3_angle);

	ragdollparams_t params;
	params.pGameData = static_cast<void*>(static_cast<CBaseEntity*>(this));
	params.modelIndex = GetModelIndex();
	params.pCollide = modelinfo->GetVCollide(params.modelIndex);
	params.pStudioHdr = GetModelPtr();
	params.forceVector = info.GetDamageForce();
	params.forceBoneIndex = forceBone;
	params.forcePosition = info.GetDamagePosition();
	params.pCurrentBones = pBoneToWorldNext;
	params.jointFrictionScale = 1.0f;
	params.allowStretch = false;
	RagdollCreate(m_ragdoll, params, physenv);
	RagdollApplyAnimationAsVelocity(m_ragdoll, pBoneToWorld, pBoneToWorldNext, dt);

	MEM_ALLOC_CREDIT();
	RagdollActivate(m_ragdoll, params.pCollide, GetModelIndex(), true);

	for (int i = 0; i < m_ragdoll.listCount; i++)
	{
		UpdateRagdollNetworkDataFromVPhysics(m_ragdoll.list[i].pObject, i);
		g_pPhysSaveRestoreManager->AssociateModel(m_ragdoll.list[i].pObject, GetModelIndex());
		physcollision->CollideGetAABB(&m_ragdollMins[i], &m_ragdollMaxs[i], m_ragdoll.list[i].pObject->GetCollide(), vec3_origin, vec3_angle);
	}
	VPhysicsSetObject(m_ragdoll.list[0].pObject);
}

void CBaseCombatCharacter::RagdollMode_Disable()
{
	if (!InRagdollMode())
		return;

	VPhysicsDestroyObject();
	m_bRagdollEnabled = false;

	SetSolid((SolidType_t)m_PreRagdollSolidType);
	SetSolidFlags(m_PreRagdollSolidFlags);
	SetMoveType((MoveType_t)m_PreRagdollMoveType, (MoveCollide_t)m_PreRagdollMoveCollide);

	RagdollMode_UnragdollCleanup();
}

void CBaseCombatCharacter::VPhysicsDestroyObject(void)
{
	if (m_ragdoll.listCount <= 0)
	{
		BaseClass::VPhysicsDestroyObject();
	}
	else
	{
		for (int i = 0; i < m_ragdoll.listCount; i++)
		{
			if (m_ragdoll.list[i].pObject)
			{
				g_pPhysSaveRestoreManager->ForgetModel(m_ragdoll.list[i].pObject);
			}
		}

		// Set to null so that the destructor's call to DestroyObject won't destroy
		//  m_pObjects[ 0 ] twice since that's the physics object for the prop
		VPhysicsSetObject(NULL);

		RagdollDestroy(m_ragdoll);
	}
}

int CBaseCombatCharacter::VPhysicsGetObjectList(IPhysicsObject** pList, int listMax)
{
	if (!InRagdollMode())
		return BaseClass::VPhysicsGetObjectList(pList, listMax);

	for (int i = 0; i < m_ragdoll.listCount; i++)
	{
		if (i < listMax)
		{
			pList[i] = m_ragdoll.list[i].pObject;
		}
	}

	return m_ragdoll.listCount;
}

void CBaseCombatCharacter::VPhysicsCollision(int index, gamevcollisionevent_t* pEvent)
{
	BaseClass::VPhysicsCollision(index, pEvent);

	if (InRagdollMode())
	{
		CBaseEntity* pHitEntity = pEvent->pEntities[!index];
		if (pHitEntity == this)
			return;

		// Don't take physics damage from whoever's holding him with the physcannon.
		if (VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD))
		{
			if (pHitEntity && (pHitEntity == HasPhysicsAttacker(FLT_MAX)))
				return;
		}

		// Don't bother taking damage from the physics attacker
		if (pHitEntity && HasPhysicsAttacker(0.5f) == pHitEntity)
			return;

		if (m_takedamage != DAMAGE_NO)
		{
			int damageType = 0;
			float damage = CalculateDefaultPhysicsDamage(index, pEvent, 1.0f, true, damageType);
			if (damage > 0)
			{
				CBaseEntity* pHitEntity = pEvent->pEntities[!index];
				if (!pHitEntity)
				{
					// hit world
					pHitEntity = GetContainingEntity(INDEXENT(0));
				}
				Vector damagePos;
				pEvent->pInternalData->GetContactPoint(damagePos);
				Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
				if (damageForce == vec3_origin)
				{
					// This can happen if this entity is motion disabled, and can't move.
					// Use the velocity of the entity that hit us instead.
					damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
				}

				if (BloodColor() != BLOOD_COLOR_MECH)
				{
					IPhysicsObject* pObj = pEvent->pObjects[index];

					Vector vecPos;
					pObj->GetPosition(&vecPos, NULL);

					trace_t tr;
					UTIL_TraceLine(vecPos, vecPos + pEvent->preVelocity[0] * 1.5, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

					UTIL_BloodDecalTrace(&tr, BloodColor());
				}

				CTakeDamageInfo dmgInfo(pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType);
				CBasePlayer* pPlayer = HasPhysicsAttacker(5.0f);
				if (!pPlayer)
					pPlayer = pHitEntity->HasPhysicsAttacker(1.0f);

				if (pPlayer)
				{
					dmgInfo.SetAttacker(pPlayer);
				}

				PhysCallbackDamage(this, dmgInfo, *pEvent, index);
			}
		}
	}
}

void CBaseCombatCharacter::TraceAttack(const CTakeDamageInfo& info, const Vector& dir, trace_t* ptr)
{
	if (InRagdollMode() && ptr->physicsbone >= 0 && ptr->physicsbone < m_ragdoll.listCount)
	{
		VPhysicsSwapObject(m_ragdoll.list[ptr->physicsbone].pObject);
	}

	BaseClass::TraceAttack(info, dir, ptr);
}

bool CBaseCombatCharacter::TestCollision(const Ray_t& ray, unsigned int mask, trace_t& trace)
{
	if (!InRagdollMode())
		return BaseClass::TestCollision(ray, mask, trace);

	CStudioHdr* pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	// Just iterate all of the elements and trace the box against each one.
	// NOTE: This is pretty expensive for small/dense characters
	trace_t tr;
	for (int i = 0; i < m_ragdoll.listCount; i++)
	{
		Vector position;
		QAngle angles;

		if (m_ragdoll.list[i].pObject)
		{
			m_ragdoll.list[i].pObject->GetPosition(&position, &angles);
			physcollision->TraceBox(ray, m_ragdoll.list[i].pObject->GetCollide(), position, angles, &tr);

			if (tr.fraction < trace.fraction)
			{
				tr.physicsbone = i;
				tr.surface.surfaceProps = m_ragdoll.list[i].pObject->GetMaterialIndex();
				trace = tr;
			}
		}
		else
		{
			DevWarning("Bogus object in Ragdoll Prop's ragdoll list!\n");
		}
	}

	if (trace.fraction >= 1)
	{
		return false;
	}

	return true;
}

void CBaseCombatCharacter::Teleport(const Vector* newPosition, const QAngle* newAngles, const Vector* newVelocity)
{
	if (!InRagdollMode())
	{
		BaseClass::Teleport(newPosition, newAngles, newVelocity);
		return;
	}

	// newAngles is a relative transform for the entity
	// But a ragdoll entity has identity orientation by design
	// so we compute a relative transform here based on the previous transform
	matrix3x4_t startMatrixInv;
	MatrixInvert(EntityToWorldTransform(), startMatrixInv);
	matrix3x4_t endMatrix;
	MatrixCopy(EntityToWorldTransform(), endMatrix);
	if (newAngles)
	{
		AngleMatrix(*newAngles, endMatrix);
	}
	if (newPosition)
	{
		PositionMatrix(*newPosition, endMatrix);
	}
	// now endMatrix is the refernce matrix for the entity at the target position
	matrix3x4_t xform;
	ConcatTransforms(endMatrix, startMatrixInv, xform);
	// now xform is the relative transform the entity must undergo

	// we need to call the base class and it will teleport our vphysics object, 
	// so set object 0 up and compute the origin/angles for its new position (base implementation has side effects)
	VPhysicsSwapObject(m_ragdoll.list[0].pObject);
	matrix3x4_t obj0source, obj0Target;
	m_ragdoll.list[0].pObject->GetPositionMatrix(&obj0source);
	ConcatTransforms(xform, obj0source, obj0Target);
	Vector obj0Pos;
	QAngle obj0Angles;
	MatrixAngles(obj0Target, obj0Angles, obj0Pos);
	BaseClass::Teleport(&obj0Pos, &obj0Angles, newVelocity);

	for (int i = 1; i < m_ragdoll.listCount; i++)
	{
		matrix3x4_t matrix, newMatrix;
		m_ragdoll.list[i].pObject->GetPositionMatrix(&matrix);
		ConcatTransforms(xform, matrix, newMatrix);
		m_ragdoll.list[i].pObject->SetPositionMatrix(newMatrix, true);
		UpdateRagdollNetworkDataFromVPhysics(m_ragdoll.list[i].pObject, i);
	}
	// fixup/relink object 0
	UpdateRagdollNetworkDataFromVPhysics(m_ragdoll.list[0].pObject, 0);
}

void CBaseCombatCharacter::SetupBones(matrix3x4_t* pBoneToWorld, int boneMask)
{
	// no ragdoll, fall through to base class 
	if (!InRagdollMode())
	{
		BaseClass::SetupBones(pBoneToWorld, boneMask);
		return;
	}

	// Not really ideal, but it'll work for now
	UpdateModelWidthScale();

	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr* pStudioHdr = GetModelPtr();
	bool sim[MAXSTUDIOBONES];
	memset(sim, 0, pStudioHdr->numbones());

	int i;

	CBoneAccessor boneaccessor(pBoneToWorld);
	for (i = 0; i < m_ragdoll.listCount; i++)
	{
		// during restore this may be NULL
		if (!m_ragdoll.list[i].pObject)
			continue;

		if (RagdollGetBoneMatrix(m_ragdoll, boneaccessor, i))
		{
			sim[m_ragdoll.boneIndex[i]] = true;
		}
	}

	mstudiobone_t* pbones = pStudioHdr->pBone(0);
	for (i = 0; i < pStudioHdr->numbones(); i++)
	{
		if (sim[i])
			continue;

		if (!(pStudioHdr->boneFlags(i) & boneMask))
			continue;

		matrix3x4_t matBoneLocal;
		AngleMatrix(pbones[i].rot, pbones[i].pos, matBoneLocal);
		ConcatTransforms(pBoneToWorld[pbones[i].parent], matBoneLocal, pBoneToWorld[i]);
	}
}

void CBaseCombatCharacter::OnSave(IEntitySaveUtils* pUtils)
{
	if (m_ragdoll.listCount)
	{
		// Don't save ragdoll element 0, base class saves the pointer in 
		// m_pPhysicsObject
		Assert(m_ragdoll.list[0].parentIndex == -1);
		Assert(m_ragdoll.list[0].pConstraint == NULL);
		Assert(m_ragdoll.list[0].originParentSpace == vec3_origin);
		Assert(m_ragdoll.list[0].pObject != NULL);
		VPhysicsSetObject(NULL);	// squelch a warning message
		VPhysicsSetObject(m_ragdoll.list[0].pObject);	// make sure object zero is saved by CBaseEntity
	}

	BaseClass::OnSave(pUtils);
}

void CBaseCombatCharacter::OnRestore()
{
	// rebuild element 0 since it isn't saved
	// NOTE: This breaks the rules - the pointer needs to get fixed in Restore()
	m_ragdoll.list[0].pObject = VPhysicsGetObject();
	m_ragdoll.list[0].parentIndex = -1;
	m_ragdoll.list[0].originParentSpace.Init();

	BaseClass::OnRestore();
	if (!m_ragdoll.listCount)
		return;

	// JAY: Reset collision relationships
	RagdollSetupCollisions(m_ragdoll, modelinfo->GetVCollide(GetModelIndex()), GetModelIndex());
	VPhysicsUpdate(VPhysicsGetObject());
}
