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

#ifndef CScriptCreatedItem_h__
#define CScriptCreatedItem_h__

// Taken from the TF2Items extension by Asher "asherkin" Baker

template< class T, class I = int >
class CUtlMemoryTF2Items : public CUtlMemory< T, I >
{
public:
	CUtlMemoryTF2Items( int nGrowSize = 0, int nInitSize = 0 ) { CUtlMemory< T, I >( nGrowSize, nInitSize ); }
    CUtlMemoryTF2Items( T* pMemory, int numElements ) { CUtlMemory< T, I >( pMemory, numElements ); }
    CUtlMemoryTF2Items( const T* pMemory, int numElements ) { CUtlMemory< T, I >( pMemory, numElements ); }
    
	void Purge()
	{
		if ( !CUtlMemory< T, I >::IsExternallyAllocated() )
		{
			if (CUtlMemory< T, I >::m_pMemory)
			{
				UTLMEMORY_TRACK_FREE();
				//free( (void*)m_pMemory );
				CUtlMemory< T, I >::m_pMemory = 0;
			}
			CUtlMemory< T, I >::m_nAllocationCount = 0;
		}
	}
};

class CScriptCreatedAttribute							// Win Length = 204 / Lin Length = 396
{
public:
	CScriptCreatedAttribute() {};

	CScriptCreatedAttribute(int iAttributeDefinitionIndex, float flValue)
	{
		this->m_iAttributeDefinitionIndex = iAttributeDefinitionIndex;
		this->m_flValue = flValue;
	}

public:
	void * m_pVTable;									// Length = 4 / Win = 0 / Lin = 0

	uint32 m_iAttributeDefinitionIndex;					// Length = 4 / Win = 4 / Lin = 4
	float m_flValue;									// Length = 4 / Win = 8 / Lin = 8
	wchar_t m_szDescription[96];						// Win Length = 192 / Lin Length = 384 / Win = 12 / Lin = 12
};

class CEconItemView								// Win Length = 3552 / Lin Length = 6872
{
public:
	void * m_pVTable;									// Length = 4 / Win = 0 / Lin = 0

#ifdef _WIN32
	char m_Padding[4];									// Length = 4 / Win = 4 / Lin = N/A
#endif

	uint32 m_iItemDefinitionIndex;						// Length = 4 / Win = 8 / Lin = 4
	uint32 m_iEntityQuality;							// Length = 4 / Win = 12 / Lin = 8
	uint32 m_iEntityLevel;								// Length = 4 / Win = 16 / Lin = 12

#ifdef _WIN32
	char m_Padding2[4];									// Length = 4 / Win = 20 / Lin = N/A
#endif

	uint64 m_iGlobalIndex;								// Length = 8 / Win = 24 / Lin = 16
	uint32 m_iGlobalIndexHigh;							// Length = 4 / Win = 32 / Lin = 24
	uint32 m_iGlobalIndexLow;							// Length = 4 / Win = 36 / Lin = 28
	uint32 m_iAccountID;								// Length = 4 / Win = 40 / Lin = 32
	uint32 m_iPosition;									// Length = 4 / Win = 44 / Lin = 36
	wchar_t m_szWideName[128];							// Win Length = 256 / Lin Length = 512 / Win = 48 / Lin = 40
	char m_szName[128];									// Length = 128 / Win = 304 / Lin = 552

	char m_szBlob[20];									// Length = 20 / Win = 432 / Lin = 680
	wchar_t m_szBlob2[1536];							// Win Length = 3072 / Lin Length = 6144 / Win = 452 / Lin = 700

	char m_Unknown[4];									// Length = 4 / Win = 3524 / Lin = 6844
	char m_Unknown2[4];									// Length = 4 / Win = 3528 / Lin = 6848

	CUtlVector<CScriptCreatedAttribute, CUtlMemoryTF2Items<CScriptCreatedAttribute> > m_Attributes;	// Length = 20 / Win = 3532 / Lin = 6852

	bool m_bInitialized;								// Length = 4 / Win = 3552 / Lin = 6872
};
#endif // CScriptCreatedItem_h__
