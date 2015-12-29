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

#ifndef _INCLUDE_CANIMATING_H_
#define _INCLUDE_CANIMATING_H_

#include "CEntity.h"

class CAnimating : public CEntity
{
public:
	DECLARE_CLASS(CAnimating, CEntity);

	virtual void StudioFrameAdvance();
	virtual bool Dissolve(const char *pMaterialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int iMagnitude);

public:
	DECLARE_DEFAULTHEADER(StudioFrameAdvance, void, ());
	DECLARE_DEFAULTHEADER_DETOUR(Dissolve, bool, (const char *pMaterialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int iMagnitude));
};

#endif // _INCLUDE_CANIMATING_H_
