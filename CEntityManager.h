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

#ifndef _INCLUDE_CENTITYMANAGER_H_
#define _INCLUDE_CENTITYMANAGER_H_

#include "sm_trie_tpl.h"
#include "extension.h"

class IEntityFactory;

class CBaseEntityOutput;
#ifndef WIN32
typedef void (* FireOutputFuncType)(CBaseEntityOutput *, variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay);
#else
typedef void (__fastcall * FireOutputFuncType)(CBaseEntityOutput *, void *, variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay);
#endif
extern FireOutputFuncType FireOutputFunc;

typedef bool (* PhysIsInCallbackFuncType)();
extern PhysIsInCallbackFuncType PhysIsInCallback;

class CEntityManager
{
public:
	CEntityManager();
	bool Init(IGameConfig *pConfig);
	void Shutdown();
	void LinkEntityToClass(IEntityFactory *pFactory, const char *className, bool internalClass = false);
	void LinkEntityToClass(IEntityFactory *pFactory, const char *className, const char *replaceName);

	virtual IServerNetworkable *Create(const char *pClassName);
	void OnRemoveEntity(CBaseEntity *pEnt, CBaseHandle handle);
	
private:
	IEntityFactory **FindFactoryInTrie(KTrie<IEntityFactory *> *pTrie, CBaseEntity *pEntity, const char *pClassName);

private:
	KTrie<IEntityFactory *> pFactoryTrie;
	KTrie<IEntityFactory *> pInternalFactoryTrie;
	KTrie<const char *> pSwapTrie;
	KTrie<bool> pHookedTrie;
	IEntityFactoryDictionary *pDict;
	CGlobalEntityList* pEntList;
	bool m_bEnabled;
	
};

// This hack allows us to access the protected virtual functions of CGlobalEntityList 
class CEntityHackedList
{
public:
	virtual void OnAddBaseEntity(CBaseEntity *, CBaseHandle) {}
	virtual void OnRemoveBaseEntity(CBaseEntity *, CBaseHandle) {}
};

CEntityManager *GetEntityManager();

#endif // _INCLUDE_CENTITYMANAGER_H_
