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

#include "CEntityManager.h"
#include "sourcehook.h"
#include "IEntityFactory.h"
#include "ehandle.h"
class CBaseEntity;
typedef CHandle<CBaseEntity> EHANDLE;
#include "takedamageinfo.h"
#include "server_class.h"
#include "CEntity.h"
#include "shareddefs.h"
#include "usercmd.h"
//#ifdef WIN32
#include "rtti.h"
//#endif
#include "../CDetour/detours.h"

SH_DECL_HOOK2_void(CEntityHackedList, OnRemoveBaseEntity, SH_NOATTRIB, 0, CBaseEntity *, CBaseHandle);
SH_DECL_HOOK1(IEntityFactoryDictionary, Create, SH_NOATTRIB, 0, IServerNetworkable *, const char *);

IEntityFactoryReal *IEntityFactoryReal::m_Head;


CEntityManager *GetEntityManager()
{
	static CEntityManager *entityManager = new CEntityManager();
	return entityManager;
}

CEntityManager::CEntityManager()
{
	m_bEnabled = false;
}

bool CEntityManager::Init(IGameConfig *pConfig)
{
	/* Get the IEntityFactoryDictionary* */
	pDict = srvtools->GetEntityFactoryDictionary();
	if (!pDict)
	{
		g_pSM->LogError(myself, "[CENTITY] Unable to retrieve pointer to IEntityFactoryDictionary");
		return false;
	}
		
	/* Get the CGlobalEntityList* */
	pEntList = srvtools->GetEntityList();
	if (!pEntList)
	{
		g_pSM->LogError(myself, "[CENTITY]  Unable to retrieve pointer to CGlobalEntityList");
		return false;
	}
		

	IEntityFactoryReal *pList = IEntityFactoryReal::m_Head;
	while (pList)
	{
		pList->AddToList();
		pList = pList->m_Next;
	}
	
	void *addr;
	if (!pConfig->GetMemSig("FireOutput", &addr) || addr == NULL)
	{
		g_pSM->LogError(myself, "[CENTITY] Couldn't find sig: %s.", "FireOutput");
		return false;
	}

	FireOutputFunc= (FireOutputFuncType)addr;

	if (!pConfig->GetMemSig("PhysIsInCallback", &addr) || addr == NULL)
	{
		g_pSM->LogError(myself, "[CENTITY] Couldn't find sig: %s.", "PhysIsInCallback");
		return false;
	}

	PhysIsInCallback = (PhysIsInCallbackFuncType)addr;
	
	/* Reconfigure all the hooks */
	IHookTracker *pTracker = IHookTracker::m_Head;
	while (pTracker)
	{
		pTracker->ReconfigureHook(pConfig);
		pTracker = pTracker->m_Next;
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), pConfig);

	IDetourTracker *pDetourTracker = IDetourTracker::m_Head;
	while (pDetourTracker)
	{
		pDetourTracker->AddHook();
		pDetourTracker = pDetourTracker->m_Next;
	}

	ISigOffsetTracker *pSigOffsetTracker = ISigOffsetTracker::m_Head;
	while (pSigOffsetTracker)
	{
		pSigOffsetTracker->FindSig(pConfig);
		pSigOffsetTracker = pSigOffsetTracker->m_Next;
	}
	
	/* Start the creation hooks! */
	SH_ADD_HOOK(IEntityFactoryDictionary, Create, pDict, SH_MEMBER(this, &CEntityManager::Create), true);
	SH_ADD_HOOK(CEntityHackedList, OnRemoveBaseEntity, (CEntityHackedList *)pEntList, SH_MEMBER(this, &CEntityManager::OnRemoveEntity), false);

	srand(time(NULL));

	m_bEnabled = true;
	return true;
}

void CEntityManager::Shutdown()
{
	SH_REMOVE_HOOK(IEntityFactoryDictionary, Create, pDict, SH_MEMBER(this, &CEntityManager::Create), true);
	SH_REMOVE_HOOK(CEntityHackedList, OnRemoveBaseEntity, (CEntityHackedList *)pEntList, SH_MEMBER(this, &CEntityManager::OnRemoveEntity), false);

	IDetourTracker *pDetourTracker = IDetourTracker::m_Head;
	while (pDetourTracker)
	{
		pDetourTracker->RemoveHook();
		pDetourTracker = pDetourTracker->m_Next;
	}
}

void CEntityManager::LinkEntityToClass(IEntityFactory *pFactory, const char *className, bool internalClass)
{
	assert(pFactory);
	if (internalClass)
	{
		pInternalFactoryTrie.insert(className, pFactory);
	} else {
		pFactoryTrie.insert(className, pFactory);
	}
}

void CEntityManager::LinkEntityToClass(IEntityFactory *pFactory, const char *className, const char *replaceName)
{
	assert(pFactory);
	LinkEntityToClass(pFactory, className);
	pSwapTrie.insert(className, replaceName);
}

IServerNetworkable *CEntityManager::Create(const char *pClassName)
{
	IServerNetworkable *pNetworkable = META_RESULT_ORIG_RET(IServerNetworkable *);

	if (!pNetworkable)
	{
		return NULL;
	}

	CBaseEntity *pEnt = pNetworkable->GetBaseEntity();
	edict_t *pEdict = pNetworkable->GetEdict();

	if (!pEnt)
	{
		return NULL;
	}

	IEntityFactory **value = NULL;
	value = FindFactoryInTrie(&pFactoryTrie, pEnt, pClassName);

	if (!value)
	{
		/* Look for CEntity handlers */
		value = FindFactoryInTrie(&pInternalFactoryTrie, pEnt, pClassName);
	}

	if (!value)
	{
		/* No handler for this entity (not an entity?) */
		//value = pFactoryTrie.retrieve("CBaseEntity");
		g_pSM->LogError(myself, "No handler found for %d/%s", gamehelpers->EntityToBCompatRef(pEnt), pClassName);
		//PrintTypeTree(pEnt);
		//_asm int 3;
	}

	IEntityFactory *pFactory = *value;
	assert(pFactory);

	char vtable[20];
	_snprintf(vtable, sizeof(vtable), "%x", (unsigned int) *(void **)pEnt);

	CEntity *pEntity = pFactory->Create(pEdict, pEnt);

	pEntity->ClearFlags();
	pEntity->InitProps();

	if (!pHookedTrie.retrieve(vtable))
	{
		pEntity->InitHooks();
		pEntity->InitDataMap();
		pHookedTrie.insert(vtable, true);
	}

	pEntity->SetClassname(pClassName);

	return NULL;
}

void CEntityManager::OnRemoveEntity(CBaseEntity *pEnt, CBaseHandle handle)
{
	CEntity *pCEnt = CEntity::Instance(handle.GetEntryIndex());
	if (pCEnt)
	{
		g_pSM->LogMessage(myself, "Entity Removed, removing CEntity");
		pCEnt->Destroy();
	}

	RETURN_META(MRES_IGNORED);
}

IEntityFactory **CEntityManager::FindFactoryInTrie(KTrie<IEntityFactory *> *pTrie, CBaseEntity *pEntity, const char *pClassName)
{
	IEntityFactory **value = NULL;
	value = pTrie->retrieve(pClassName); // Old style match to entity classname (also used for custom classes).

	if (!value)
	{
		/* Attempt to do an RTTI lookup for C++ class links */
		IType *pType = GetType(pEntity);
		IBaseType *pBase = pType->GetBaseType();

		do 
		{
			const char *classname = GetTypeName(pBase->GetTypeInfo());
			value = pTrie->retrieve(classname);

			if (value)
			{
				break;
			}

		} while (pBase->GetNumBaseClasses() && (pBase = pBase->GetBaseClass(0)));

		pType->Destroy();
	}

	return value;
}
