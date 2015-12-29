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

#include "CAnimating.h"

LINK_ENTITY_TO_INTERNAL_CLASS(CBaseAnimating, CAnimating);


SH_DECL_MANUALHOOK0_void(StudioFrameAdvance, 0, 0, 0);

DECLARE_HOOK(StudioFrameAdvance, CAnimating);
DECLARE_DETOUR(Dissolve, CAnimating);

DECLARE_DEFAULTHANDLER_void(CAnimating, StudioFrameAdvance, (), ());
DECLARE_DEFAULTHANDLER_DETOUR(CAnimating, Dissolve, bool, (const char *pMaterialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int iMagnitude), (pMaterialName, flStartTime, bNPCOnly, nDissolveType, vDissolverOrigin, iMagnitude));
