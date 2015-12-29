/**
* =============================================================================
* CEntity Entity Handling Framework
* Copyright (C) 2011 Matt Woodrow.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CEntity.h"
#include "shareddefs.h"
#include "CEntityManager.h"
#include "CPlayer.h"
#include "CTakeDamageInfo.h"

extern bool ParseKeyvalue(void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue);
extern bool ExtractKeyvalue(void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen);
IHookTracker *IHookTracker::m_Head = NULL;
IPropTracker *IPropTracker::m_Head = NULL;
IDetourTracker *IDetourTracker::m_Head = NULL;
ISigOffsetTracker *ISigOffsetTracker::m_Head = NULL;

ISaveRestoreOps *eventFuncs = NULL;

SH_DECL_MANUALHOOK3_void(Teleport, 0, 0, 0, const Vector *, const QAngle *, const Vector *);
SH_DECL_MANUALHOOK0_void(UpdateOnRemove, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(Spawn, 0, 0, 0);
SH_DECL_MANUALHOOK1(OnTakeDamage, 0, 0, 0, int, CEntityTakeDamageInfo &);
SH_DECL_MANUALHOOK0_void(Think, 0, 0, 0);
SH_DECL_MANUALHOOK5(AcceptInput, 0, 0, 0, bool, const char *, CBaseEntity *, CBaseEntity *, variant_t, int);
SH_DECL_MANUALHOOK0(GetDataDescMap, 0, 0, 0, datamap_t *);
SH_DECL_MANUALHOOK1_void(StartTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(Touch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(EndTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK0(GetSoundEmissionOrigin, 0, 0, 0, Vector);
SH_DECL_MANUALHOOK0(GetServerVehicle, 0, 0, 0, IServerVehicle *);
SH_DECL_MANUALHOOK1(VPhysicsTakeDamage, 0, 0, 0, int, const CEntityTakeDamageInfo &);
SH_DECL_MANUALHOOK2(VPhysicsGetObjectList, 0, 0, 0, int, IPhysicsObject **, int);
SH_DECL_MANUALHOOK0(GetServerClass, 0, 0, 0, ServerClass *);
SH_DECL_MANUALHOOK2(KeyValue, 0, 0, 0, bool, const char *, const char *);
SH_DECL_MANUALHOOK3(GetKeyValue, 0, 0, 0, bool, const char *, char*, int);


DECLARE_HOOK(Teleport, CEntity);
DECLARE_HOOK(UpdateOnRemove, CEntity);
DECLARE_HOOK(Spawn, CEntity);
DECLARE_HOOK(OnTakeDamage, CEntity);
DECLARE_HOOK(Think, CEntity);
DECLARE_HOOK(AcceptInput, CEntity);
DECLARE_HOOK(GetDataDescMap, CEntity);
DECLARE_HOOK(StartTouch, CEntity);
DECLARE_HOOK(Touch, CEntity);
DECLARE_HOOK(EndTouch, CEntity);
DECLARE_HOOK(GetSoundEmissionOrigin, CEntity);
DECLARE_HOOK(GetServerVehicle, CEntity);
DECLARE_HOOK(VPhysicsTakeDamage, CEntity);
DECLARE_HOOK(VPhysicsGetObjectList, CEntity);
DECLARE_HOOK(GetServerClass, CEntity);
DECLARE_HOOK(GetKeyValue, CEntity);
DECLARE_HOOK(KeyValue, CEntity);

//Sendprops
DEFINE_PROP(m_iTeamNum, CEntity);
DEFINE_PROP(m_vecOrigin, CEntity);
DEFINE_PROP(m_CollisionGroup, CEntity);
DEFINE_PROP(m_hOwnerEntity, CEntity);
DEFINE_PROP(m_fEffects, CEntity);
DEFINE_PROP(m_vecVelocity, CEntity);

//Datamaps
DEFINE_PROP(m_vecAbsOrigin, CEntity);
DEFINE_PROP(m_vecAbsVelocity, CEntity);
DEFINE_PROP(m_nNextThinkTick, CEntity);
DEFINE_PROP(m_iClassname, CEntity);
DEFINE_PROP(m_rgflCoordinateFrame, CEntity);
DEFINE_PROP(m_vecAngVelocity, CEntity);
DEFINE_PROP(m_vecBaseVelocity, CEntity);
DEFINE_PROP(m_hMoveParent, CEntity);
DEFINE_PROP(m_iEFlags, CEntity);
DEFINE_PROP(m_pPhysicsObject, CEntity);
DEFINE_PROP(m_pParent, CEntity);
DEFINE_PROP(m_MoveType, CEntity);
DEFINE_PROP(m_MoveCollide, CEntity);
DEFINE_PROP(m_iName, CEntity);

/* Hacked Datamap declaration to fallback to the corresponding real entities one */
datamap_t CEntity::m_DataMap = { 0, 0, "CEntity", NULL };
//DECLARE_DEFAULTHANDLER(CEntity, GetDataDescMap, datamap_t *, (), ());
datamap_t *CEntity::GetBaseMap() { return NULL; }
BEGIN_DATADESC_GUTS(CEntity)
END_DATADESC()

PhysIsInCallbackFuncType PhysIsInCallback;

datamap_t* CEntity::GetDataDescMap()
{
	datamap_t* base = NULL;
	if (!m_bInGetDataDescMap)
	{
		base = SH_MCALL(BaseEntity(), GetDataDescMap)();
		m_DataMap.baseMap = base;
		return &m_DataMap;
	}


	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	base = (thisptr->*(__SoureceHook_FHM_GetRecallMFPGetDataDescMap(thisptr)))();
	m_DataMap.baseMap = base;
	RETURN_META_VALUE(MRES_SUPERCEDE, &m_DataMap);
}

datamap_t* CEntity::InternalGetDataDescMap()
{
	SET_META_RESULT(MRES_SUPERCEDE);
	CEntity *pEnt = (CEntity *)CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
		RETURN_META_VALUE(MRES_IGNORED, (datamap_t *)0);
	int index = pEnt->entindex();
	pEnt->m_bInGetDataDescMap = true;
	datamap_t* retvalue = pEnt->GetDataDescMap();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInGetDataDescMap = false;
	return retvalue;
}

LINK_ENTITY_TO_INTERNAL_CLASS(CBaseEntity, CEntity);

variant_t g_Variant;

void CEntity::Init(edict_t *pEdict, CBaseEntity *pBaseEntity)
{
	m_pEntity = pBaseEntity;
	m_pEdict = pEdict;

	assert(!pEntityData[entindex()]);

	pEntityData[entindex()] = this;

	if(!m_pEntity || !m_pEdict)
		return;

	m_pfnThink = NULL;
	m_pfnTouch = NULL;
}

void CEntity::Destroy()
{
	pEntityData[entindex()] = NULL;
	delete this;
}

CBaseEntity * CEntity::BaseEntity()
{
	return m_pEntity;
}

/* Expanded handler for readability and since this one actually does something */
void CEntity::UpdateOnRemove()
{
	if (!m_bInUpdateOnRemove)
	{
		SH_MCALL(BaseEntity(), UpdateOnRemove);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);

	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPUpdateOnRemove(thisptr)))();

	SET_META_RESULT(MRES_SUPERCEDE);
}

void CEntity::InternalUpdateOnRemove()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInUpdateOnRemove = true;
	pEnt->UpdateOnRemove();
	if (pEnt == CEntity::Instance(index))
	{
		pEnt->m_bInUpdateOnRemove = false;
		pEnt->Destroy();
	}
}

DECLARE_DEFAULTHANDLER_void(CEntity, Teleport, (const Vector *origin, const QAngle* angles, const Vector *velocity), (origin, angles, velocity));
DECLARE_DEFAULTHANDLER_void(CEntity, Spawn, (), ());
DECLARE_DEFAULTHANDLER(CEntity, OnTakeDamage, int, (CEntityTakeDamageInfo &info), (info));
//DECLARE_DEFAULTHANDLER(CEntity, GetServerVehicle, IServerVehicle *, (), ());
DECLARE_DEFAULTHANDLER(CEntity, VPhysicsTakeDamage, int, (const CEntityTakeDamageInfo &inputInfo), (inputInfo));
DECLARE_DEFAULTHANDLER(CEntity, VPhysicsGetObjectList, int, (IPhysicsObject **pList, int listMax), (pList, listMax));
DECLARE_DEFAULTHANDLER(CEntity, GetServerClass, ServerClass *, (), ());

IServerVehicle *CEntity::GetServerVehicle()
{
	return SH_MCALL(BaseEntity(), GetServerVehicle)();
}

IServerVehicle *CEntity::InternalGetServerVehicle()
{
	/* Do absolutely nothing since the iface ptr is 0xcccccccc sometimes and we can't handle that yet */
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void CEntity::StartTouch(CEntity *pOther)
{
	if (!m_bInStartTouch)
	{
		SH_MCALL(BaseEntity(), StartTouch)(*pOther);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPStartTouch(thisptr)))(*pOther);
	SET_META_RESULT(MRES_SUPERCEDE);
}

void CEntity::InternalStartTouch(CBaseEntity *pOther)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	CEntity *pEntOther = *pOther;
	if (!pEnt || !pEntOther)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInStartTouch = true;
	pEnt->StartTouch(pEntOther);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInStartTouch = false;
}

void CEntity::EndTouch(CEntity *pOther)
{
	if (!m_bInEndTouch)
	{
		SH_MCALL(BaseEntity(), EndTouch)(*pOther);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPEndTouch(thisptr)))(*pOther);
	SET_META_RESULT(MRES_SUPERCEDE);
}

void CEntity::InternalEndTouch(CBaseEntity *pOther)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	CEntity *pEntOther = *pOther;
	if (!pEnt || !pEntOther)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInEndTouch = true;
	pEnt->EndTouch(pEntOther);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInEndTouch = false;
}

void CEntity::Touch(CEntity *pOther)
{
	if ( m_pfnTouch ) 
		(this->*m_pfnTouch)(pOther);

	//if (m_pParent)
	//	m_pParent->Touch(pOther);

	if (!m_bInTouch)
	{
		SH_MCALL(BaseEntity(), Touch)(*pOther);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPTouch(thisptr)))(*pOther);
	SET_META_RESULT(MRES_SUPERCEDE);
}

void CEntity::InternalTouch(CBaseEntity *pOther)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	CEntity *pEntOther = *pOther;
	if (!pEnt || !pEntOther)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInTouch = true;
	pEnt->Touch(pEntOther);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInTouch = false;
}


bool CEntity::GetKeyValue(const char *szKeyName, char *szValue, int iMaxLen)
{
	for (datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap)
	{
		if (ExtractKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue, iMaxLen))
			return true;

		if (strcmp(dmap->dataClassName, "CEntity") == 0)
			break;
	}

	if (!m_bInGetKeyValue)
		return SH_MCALL(BaseEntity(), GetKeyValue) (szKeyName, szValue, iMaxLen);

	//Msg("end!\n");
	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	RETURN_META_VALUE(MRES_SUPERCEDE, (thisptr->*(__SoureceHook_FHM_GetRecallMFPGetKeyValue(thisptr))) (szKeyName, szValue, iMaxLen));
}

bool CEntity::InternalGetKeyValue(const char *szKeyName, char *szValue, int iMaxLen)
{
	SET_META_RESULT(MRES_SUPERCEDE);
	CEntity *pEnt = (CEntity *)CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
		RETURN_META_VALUE(MRES_IGNORED, (bool)0);

	int index = pEnt->entindex();

	pEnt->m_bInGetKeyValue = true;


	bool retvalue = pEnt->GetKeyValue(szKeyName, szValue, iMaxLen);
	pEnt = (CEntity *)CEntity::Instance(index);

	if (pEnt)
		pEnt->m_bInGetKeyValue = false;
	return retvalue;
}


bool CEntity::KeyValue(const char *szKeyName, const char *szValue)
{
	// loop through the data description, and try and place the keys in
	for (datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap)
	{
		if (ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue))
		{
			return true;
		}
		if (strcmp(dmap->dataClassName, "CEntity") == 0)
			break;
	}
	if (!m_bInKeyValue)
		return SH_MCALL(BaseEntity(), KeyValue) (szKeyName, szValue);

	//Msg("end!\n");
	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	RETURN_META_VALUE(MRES_SUPERCEDE, (thisptr->*(__SoureceHook_FHM_GetRecallMFPKeyValue(thisptr))) (szKeyName, szValue));
}

bool CEntity::InternalKeyValue(const char *szKeyName, const char *szValue)
{
	SET_META_RESULT(MRES_SUPERCEDE);
	CEntity *pEnt = (CEntity *)CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
		RETURN_META_VALUE(MRES_IGNORED, (bool)0);

	int index = pEnt->entindex();

	pEnt->m_bInKeyValue = true;


	bool retvalue = pEnt->KeyValue(szKeyName, szValue);
	pEnt = (CEntity *)CEntity::Instance(index);

	if (pEnt)
		pEnt->m_bInKeyValue = false;
	return retvalue;
}


Vector CEntity::GetSoundEmissionOrigin()
{
	if (!m_bInGetSoundEmissionOrigin)
	{
		Vector ret = SH_MCALL(BaseEntity(), GetSoundEmissionOrigin)();
		return ret;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	Vector ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPGetSoundEmissionOrigin(thisptr)))();
	SET_META_RESULT(MRES_SUPERCEDE);

	return ret;
}

Vector CEntity::InternalGetSoundEmissionOrigin()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, Vector());
	}

	int index = pEnt->entindex();
	pEnt->m_bInGetSoundEmissionOrigin = true;
	Vector ret = pEnt->GetSoundEmissionOrigin();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInGetSoundEmissionOrigin = false;

	return ret;
}

void CEntity::Think()
{
	if (m_pfnThink)
	{
		(this->*m_pfnThink)();
	}

	if (!m_bInThink)
	{
		SH_MCALL(BaseEntity(), Think)();
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPThink(thisptr)))();
	SET_META_RESULT(MRES_SUPERCEDE);
}

void CEntity::InternalThink()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInThink = true;
	pEnt->Think();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInThink = false;
}


BASEPTR	CEntity::ThinkSet(BASEPTR func, float thinkTime, const char *szContext)
{
	if ( !szContext )
	{
		m_pfnThink = func;
		return m_pfnThink;
	}

	return NULL;
}

void CEntity::SetNextThink(float thinkTime, const char *szContext)
{
	int thinkTick = ( thinkTime == TICK_NEVER_THINK ) ? TICK_NEVER_THINK : TIME_TO_TICKS(thinkTime);

	// Are we currently in a think function with a context?
	if ( !szContext )
	{
		// Old system
		m_nNextThinkTick = thinkTick;
		CheckHasThinkFunction( thinkTick == TICK_NEVER_THINK ? false : true );
		return;
	}
}

void CEntity::AddEFlags(int nEFlagMask)
{
	m_iEFlags |= nEFlagMask;
}

void CEntity::RemoveEFlags(int nEFlagMask)
{
	m_iEFlags &= ~nEFlagMask;
}

bool CEntity::IsEFlagSet(int nEFlagMask) const
{
	return (m_iEFlags & nEFlagMask) != 0;
}

void CEntity::CheckHasThinkFunction(bool isThinking)
{
	if ( IsEFlagSet( EFL_NO_THINK_FUNCTION ) && isThinking )
	{
		RemoveEFlags( EFL_NO_THINK_FUNCTION );
	}
	else if ( !isThinking && !IsEFlagSet( EFL_NO_THINK_FUNCTION ) && !WillThink() )
	{
		AddEFlags( EFL_NO_THINK_FUNCTION );
	}
}

bool CEntity::WillThink()
{
	if (m_nNextThinkTick > 0)
		return true;

	return false;
}

const char* CEntity::GetClassname()
{
	return m_iClassname->ToCStr();
}

void CEntity::SetClassname(const char *pClassName)
{
	m_iClassname = MAKE_STRING(pClassName);
}

const char* CEntity::GetTargetName()
{
	return m_iName->ToCStr();
}

void CEntity::SetTargetName(const char *pTargetName)
{
	m_iName = MAKE_STRING(pTargetName);
}

void CEntity::ChangeTeam(int iTeamNum)
{
	m_iTeamNum = iTeamNum;
	edict()->StateChanged(m_iTeamNumPropTrackerObj.GetOffset());
}

int CEntity::GetTeamNumber(void) const
{
	return m_iTeamNum;
}

bool CEntity::InSameTeam(CEntity *pEntity) const
{
	if (!pEntity)
		return false;

	return (pEntity->GetTeamNumber() == GetTeamNumber());
}

const Vector& CEntity::GetLocalOrigin(void) const
{
	return m_vecOrigin;
}

const Vector& CEntity::GetAbsOrigin(void) const
{
	return m_vecAbsOrigin;
}

const Vector &CEntity::GetAbsVelocity() const
{
	if (IsEFlagSet(EFL_DIRTY_ABSVELOCITY))
	{
		//const_cast<CEntity*>(this)->CalcAbsoluteVelocity();
	}
	return m_vecAbsVelocity;
}

const Vector & CEntity::GetVelocity() const
{
	return m_vecVelocity;
}

CEntity *CEntity::GetMoveParent(void)
{
	return Instance(m_hMoveParent); 
}

edict_t *CEntity::edict()
{
	return m_pEdict;
}

int CEntity::entindex()
{
	return BaseEntity()->GetRefEHandle().GetEntryIndex();
}

bool CEntity::IsPlayer()
{
	return false;
}

int CEntity::GetTeam()
{
	return m_iTeamNum;
}


bool CEntity::AcceptInput(const char *szInputName, CEntity *pActivator, CEntity *pCaller, variant_t Value, int outputID)
{
	bool inBaseEntity = false;
	for (datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap)
	{


		// search through all the actions in the data description, looking for a match
		for (int i = 0; i < dmap->dataNumFields; i++)
		{
			if (dmap->dataDesc[i].flags & FTYPEDESC_INPUT)
			{
				if (!Q_stricmp(dmap->dataDesc[i].externalName, szInputName))
				{
					// found a match
					char szBuffer[256];
					// mapper debug message
					if (pCaller != NULL)
					{
						Q_snprintf(szBuffer, sizeof(szBuffer), "(%0.2f) input %s: %s.%s(%s)\n", gpGlobals->curtime, STRING(*pCaller->m_iName), GetDebugName(), szInputName, Value.String());
					}
					else
					{
						Q_snprintf(szBuffer, sizeof(szBuffer), "(%0.2f) input <NULL>: %s.%s(%s)\n", gpGlobals->curtime, GetDebugName(), szInputName, Value.String());
					}
					DevMsg(2, "%s", szBuffer);

					// convert the value if necessary
					if (Value.FieldType() != dmap->dataDesc[i].fieldType)
					{
						if (!(Value.FieldType() == FIELD_VOID && dmap->dataDesc[i].fieldType == FIELD_STRING)) // allow empty strings
						{
							if (!Value.Convert((fieldtype_t)dmap->dataDesc[i].fieldType))
							{
								//Warning
								// bad conversion
								Warning("!! ERROR: bad input/output link:\n!! %s(%s,%s) doesn't match type from %s(%s)\n",
									GetClassname(), GetClassname(), szInputName,
									(pCaller != NULL) ? pCaller->GetClassname() : "<null>",
									(pCaller != NULL) ? pCaller->GetTargetName() : "<null>");
								return false;
							}
						}
					}
					if (inBaseEntity)
						continue;
					// call the input handler, or if there is none just set the value
					inputfunc_centity_t pfnInput = (inputfunc_centity_t)dmap->dataDesc[i].inputFunc;

					if (pfnInput)
					{
						// Package the data into a struct for passing to the input handler.
						inputdata_t data;
						data.pActivator = pActivator->BaseEntity();
						data.pCaller = pCaller->BaseEntity();
						data.value = Value;
						data.nOutputID = outputID;

						(this->*pfnInput)(data);
					}
					else if (dmap->dataDesc[i].flags & FTYPEDESC_KEY)
					{
						// set the value directly
						Value.SetOther(((char*)this) + dmap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL]);

						// TODO: if this becomes evil and causes too many full entity updates, then we should make
						// a macro like this:
						//
						// define MAKE_INPUTVAR(x) void Note##x##Modified() { x.GetForModify(); }
						//
						// Then the datadesc points at that function and we call it here. The only pain is to add
						// that function for all the DEFINE_INPUT calls.
						//NetworkStateChanged();
					}

					return true;
				}

			}

		}
		if (strcmp(dmap->dataClassName, "CEntity") == 0)
			inBaseEntity = true;// break;
	}

	if (!m_bInAcceptInput)
	{
		return SH_MCALL(BaseEntity(), AcceptInput)(szInputName, *pActivator, *pCaller, Value, outputID);
	}
	/**
	* This gets the award for the worst hack so far. Detects the end of waiting for players and probably lots of other things.
	* Forces players out of vehicles.


	if (strcmp(szInputName, "ShowInHUD") == 0 || strcmp(szInputName, "RoundSpawn") == 0 || strcmp(szInputName, "RoundWin") == 0)
	{
		CEntity *pEnt;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			pEnt = CEntity::Instance(i);
			if (!pEnt)
			{
				continue;
			}

			CPlayer *pPlayer = dynamic_cast<CPlayer *>(pEnt);
			assert(pPlayer);

			IServerVehicle *pVehicle = pPlayer->GetVehicle();
			if (pVehicle && !pVehicle->IsPassengerExiting())
			{
				pPlayer->LeaveVehicle();
			}
		}
	}
	*/
	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	bool ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPAcceptInput(thisptr)))(szInputName, *pActivator, *pCaller, Value, outputID);
	SET_META_RESULT(MRES_SUPERCEDE);
	return ret;
}

bool CEntity::InternalAcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int index = pEnt->entindex();
	pEnt->m_bInAcceptInput = true;
	bool ret = pEnt->AcceptInput(szInputName, *pActivator, *pCaller, Value, outputID);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInAcceptInput = false;

	return ret;
}

void CEntity::InitHooks()
{
	IHookTracker *pTracker = IHookTracker::m_Head;
	while (pTracker)
	{
		pTracker->AddHook(this);
		pTracker = pTracker->m_Next;
	}
}

void CEntity::InitProps()
{
	IPropTracker *pTracker = IPropTracker::m_Head;
	while (pTracker)
	{
		pTracker->InitProp(this);
		pTracker = pTracker->m_Next;
	}
}

void CEntity::ClearFlags()
{
	IHookTracker *pTracker = IHookTracker::m_Head;
	while (pTracker)
	{
		pTracker->ClearFlag(this);
		pTracker = pTracker->m_Next;
	}
}

CEntity *CEntity::GetOwner()
{
	return m_hOwnerEntity;
}

void CEntity::SetOwner(CEntity *pOwnerEntity)
{
	(*m_hOwnerEntity.ptr).Set(pOwnerEntity->edict()->GetIServerEntity());
}

IPhysicsObject *CEntity::VPhysicsGetObject(void) const
{
	return m_pPhysicsObject;
}

void CEntity::SetCollisionGroup(int collisionGroup)
{
	if ((int)m_CollisionGroup != collisionGroup)
	{
		m_CollisionGroup = collisionGroup;
		CollisionRulesChanged();
	}
}

#define VPHYSICS_MAX_OBJECT_LIST_COUNT	1024
void CEntity::CollisionRulesChanged()
{
	// ivp maintains state based on recent return values from the collision filter, so anything
	// that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
	if (VPhysicsGetObject())
	{
		if (PhysIsInCallback())
		{
			Warning("Changing collision rules within a callback is likely to cause crashes!\n");
			Assert(0);
		}
		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = VPhysicsGetObjectList(pList, ARRAYSIZE(pList));
		for (int i = 0; i < count; i++)
		{
			if (pList[i] != NULL) //this really shouldn't happen, but it does >_<
				pList[i]->RecheckCollisionFilter();
		}
	}
}

int CEntity::GetMoveType() const
{
	return *m_MoveType;
}

void CEntity::SetMoveType( int MoveType )
{
	*m_MoveType = MoveType;
}

int CEntity::GetMoveCollide() const
{
	return *m_MoveCollide;
}

void CEntity::SetMoveCollide( int MoveCollide )
{
	*m_MoveCollide = MoveCollide;
}


const char *CEntity::GetDebugName()
{
	if (this == NULL)
		return "<<null>>";

	if (*m_iName != NULL_STRING)
	{
		return STRING(*m_iName);
	}
	else
	{
		return STRING(*m_iClassname);
	}
}