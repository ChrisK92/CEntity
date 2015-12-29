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

#ifndef _INCLUDE_ENTITYOUTPUT_H_
#define _INCLUDE_ENTITYOUTPUT_H_

#include "CEntity.h"

#define EVENT_FIRE_ALWAYS	-1

class CEventAction
{
public:
	CEventAction( const char *ActionData = NULL );

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none
	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS.

	int m_iIDStamp;	// unique identifier stamp

	static int s_iNextIDStamp;

	CEventAction *m_pNext; 
/*
	// allocates memory from engine.MPool/g_EntityListPool
	static void *operator new( size_t stAllocateBlock );
	static void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	static void operator delete( void *pMem );
	static void operator delete( void *pMem , int nBlockUse, const char *pFileName, int nLine ) { operator delete(pMem); }
*/
	DECLARE_SIMPLE_DATADESC();

};

class CBaseEntityOutput
{
public:

	~CBaseEntityOutput();

	void ParseEventAction( const char *EventData );
	void AddEventAction( CEventAction *pEventAction );

	int Save( ISave &save );
	int Restore( IRestore &restore, int elementCount );

	int NumberOfElements( void );

	float GetMaxDelay( void );

	fieldtype_t ValueFieldType() { return m_Value.FieldType(); }

	void FireOutput( variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 );
/*
	/// Delete every single action in the action list. 
	void DeleteAllElements( void ) ;
*/
protected:
	variant_t m_Value;
	CEventAction *m_ActionList;
	DECLARE_SIMPLE_DATADESC();
	
	CBaseEntityOutput() {} // this class cannot be created, only it's children

private:
	CBaseEntityOutput( CBaseEntityOutput& ); // protect from accidental copying
};

class COutputEvent : public CBaseEntityOutput
{
public:
	// void Firing, no parameter
	void FireOutput(CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0);
	void FireOutput(CEntity *pActivator, CEntity *pCaller, float fDelay = 0);
	void FireOutput(CBaseEntity *pActivator, CEntity *pCaller, float fDelay = 0);
	void FireOutput(CEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0);
};


#endif // _INCLUDE_ENTITYOUTPUT_H_
