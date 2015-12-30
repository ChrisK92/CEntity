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

#include "util.h"
#include "isaverestore.h"
#include "tier0/dbg.h"

//Isn't used but causes linker error when not there!
CBaseEntityList * g_pEntityList;

const char *variant_t::ToString( void ) const
{
	COMPILE_TIME_ASSERT( sizeof(string_t) == sizeof(int) );

	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
		{
			return(STRING(iszVal));
		}

	case FIELD_BOOLEAN:
		{
			if (bVal == 0)
			{
				Q_strncpy(szBuf, "false",sizeof(szBuf));
			}
			else
			{
				Q_strncpy(szBuf, "true",sizeof(szBuf));
			}
			return(szBuf);
		}

	case FIELD_INTEGER:
		{
			Q_snprintf( szBuf, sizeof( szBuf ), "%i", iVal );
			return(szBuf);
		}

	case FIELD_FLOAT:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%g", flVal);
			return(szBuf);
		}

	case FIELD_COLOR32:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
			return(szBuf);
		}

	case FIELD_VECTOR:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
			return(szBuf);
		}

	case FIELD_VOID:
		{
			szBuf[0] = '\0';
			return(szBuf);
		}

	case FIELD_EHANDLE:
		{
			const char *pszName = NULL;
			CEntity *pEnt = CEntity::Instance(eVal);
			if (pEnt)
			{
				pszName = pEnt->GetClassname();
			}
			else
			{
				pszName = "<<null entity>>";
			}

			Q_strncpy( szBuf, pszName, 512 );
			return (szBuf);
		}

	default:
		break;
	}

	return("No conversion to string");
}

void variant_t::Set(fieldtype_t ftype, void *data)
{
	fieldType = ftype;

	switch (ftype)
	{
	case FIELD_BOOLEAN:		bVal = *((bool *)data);				break;
	case FIELD_CHARACTER:	iVal = *((char *)data);				break;
	case FIELD_SHORT:		iVal = *((short *)data);			break;
	case FIELD_INTEGER:		iVal = *((int *)data);				break;
	case FIELD_STRING:		iszVal = *((string_t *)data);		break;
	case FIELD_FLOAT:		flVal = *((float *)data);			break;
	case FIELD_COLOR32:		rgbaVal = *((color32 *)data);		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		vecVal[0] = ((float *)data)[0];
		vecVal[1] = ((float *)data)[1];
		vecVal[2] = ((float *)data)[2];
		break;
	}

	case FIELD_EHANDLE:		eVal = *((EHANDLE *)data);			break;
	case FIELD_CLASSPTR:	eVal = *((CBaseEntity **)data);		break;
	case FIELD_VOID:
	default:
		iVal = 0; fieldType = FIELD_VOID;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Converts the variant to a new type. This function defines which I/O
//          types can be automatically converted between. Connections that require
//          an unsupported conversion will cause an error message at runtime.
// Input  : newType - the type to convert to
// Output : Returns true on success, false if the conversion is not legal
//-----------------------------------------------------------------------------
bool variant_t::Convert(fieldtype_t newType)
{
	if (newType == fieldType)
	{
		return true;
	}

	//
	// Converting to a null value is easy.
	//
	if (newType == FIELD_VOID)
	{
		Set(FIELD_VOID, NULL);
		return true;
	}

	//
	// FIELD_INPUT accepts the variant type directly.
	//
	if (newType == FIELD_INPUT)
	{
		return true;
	}

	switch (fieldType)
	{
	case FIELD_INTEGER:
	{
		switch (newType)
		{
		case FIELD_FLOAT:
		{
			SetFloat((float)iVal);
			return true;
		}

		case FIELD_BOOLEAN:
		{
			SetBool(iVal != 0);
			return true;
		}
		}
		break;
	}

	case FIELD_FLOAT:
	{
		switch (newType)
		{
		case FIELD_INTEGER:
		{
			SetInt((int)flVal);
			return true;
		}

		case FIELD_BOOLEAN:
		{
			SetBool(flVal != 0);
			return true;
		}
		}
		break;
	}

	//
	// Everyone must convert from FIELD_STRING if possible, since
	// parameter overrides are always passed as strings.
	//
	case FIELD_STRING:
	{
		switch (newType)
		{
		case FIELD_INTEGER:
		{
			if (iszVal != NULL_STRING)
			{
				SetInt(atoi(STRING(iszVal)));
			}
			else
			{
				SetInt(0);
			}
			return true;
		}

		case FIELD_FLOAT:
		{
			if (iszVal != NULL_STRING)
			{
				SetFloat(atof(STRING(iszVal)));
			}
			else
			{
				SetFloat(0);
			}
			return true;
		}

		case FIELD_BOOLEAN:
		{
			if (iszVal != NULL_STRING)
			{
				SetBool(atoi(STRING(iszVal)) != 0);
			}
			else
			{
				SetBool(false);
			}
			return true;
		}

		case FIELD_VECTOR:
		{
			Vector tmpVec = vec3_origin;
			if (sscanf(STRING(iszVal), "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
			{
				// Try sucking out 3 floats with no []s
				sscanf(STRING(iszVal), "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);
			}
			SetVector3D(tmpVec);
			return true;
		}

		case FIELD_COLOR32:
		{
			int nRed = 0;
			int nGreen = 0;
			int nBlue = 0;
			int nAlpha = 255;

			sscanf(STRING(iszVal), "%d %d %d %d", &nRed, &nGreen, &nBlue, &nAlpha);
			SetColor32(nRed, nGreen, nBlue, nAlpha);
			return true;
		}

		case FIELD_EHANDLE:
		{
			// convert the string to an entity by locating it by classname
			CEntity *ent = NULL;
			if (iszVal != NULL_STRING)
			{
				// FIXME: do we need to pass an activator in here?
				CEntity *pEnt;
				for (int i = 0; i <= NUM_ENT_ENTRIES; i++)
				{
					pEnt = CEntity::Instance(i);
					if (!pEnt)
					{
						continue;
					}

					char name[128];
					if (strcmp(name, pEnt->GetTargetName()) == 0)
					{
						ent = pEnt;
					}
				}
			}
			SetEntity(ent->BaseEntity());
			return true;
		}
		}

		break;
	}

	case FIELD_EHANDLE:
	{
		switch (newType)
		{
		case FIELD_STRING:
		{
			// take the entities targetname as the string
			string_t iszStr = NULL_STRING;
			if (eVal != NULL)
			{
				CEntity* pEnt = CEntity::Instance(eVal->GetBaseEntity());
				if (pEnt != NULL)
				{
					SetString(MAKE_STRING(pEnt->GetTargetName()));
				}

			}
			return true;
		}
		}
		break;
	}
	}

	// invalid conversion
	return false;
}

void variant_t::SetEntity(CBaseEntity *val)
{
	eVal = val;
	fieldType = FIELD_EHANDLE;
}


//-----------------------------------------------------------------------------
// Purpose: Copies the value in the variant into a block of memory
// Input  : *data - the block to write into
//-----------------------------------------------------------------------------
void variant_t::SetOther(void *data)
{
	switch (fieldType)
	{
	case FIELD_BOOLEAN:     *((bool *)data) = bVal != 0;        break;
	case FIELD_CHARACTER:   *((char *)data) = iVal;             break;
	case FIELD_SHORT:       *((short *)data) = iVal;            break;
	case FIELD_INTEGER:     *((int *)data) = iVal;              break;
	case FIELD_STRING:      *((string_t *)data) = iszVal;       break;
	case FIELD_FLOAT:       *((float *)data) = flVal;           break;
	case FIELD_COLOR32:     *((color32 *)data) = rgbaVal;       break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		((float *)data)[0] = vecVal[0];
		((float *)data)[1] = vecVal[1];
		((float *)data)[2] = vecVal[2];
		break;
	}

	case FIELD_EHANDLE:     *((EHANDLE *)data) = eVal;          break;
	case FIELD_CLASSPTR:    *((CBaseEntity **)data) = eVal;     break;
	}
}

/**
 * This is the worst util ever, incredibly specific usage.
 * Searches a datamap for output types and swaps the SaveRestoreOps pointer for the global eventFuncs one.
 * Reason for this is we didn't have the eventFuncs pointer available statically (when the datamap structure was generated)
 */
void UTIL_PatchOutputRestoreOps(datamap_t *pMap)
{
	for (int i=0; i<pMap->dataNumFields; i++)
	{
		if (pMap->dataDesc[i].flags & FTYPEDESC_OUTPUT)
		{
			pMap->dataDesc[i].pSaveRestoreOps = eventFuncs;
		}

		if (pMap->dataDesc[i].td)
		{
			UTIL_PatchOutputRestoreOps(pMap->dataDesc[i].td);
		}
	}
}

void UTIL_StringToFloatArray(float *pVector, int count, const char *pString)
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy(tempString, pString, sizeof(tempString));
	pstr = pfront = tempString;

	for (j = 0; j < count; j++)			// lifted from pr_edict.c
	{
		pVector[j] = atof(pfront);

		// skip any leading whitespace
		while (*pstr && *pstr <= ' ')
			pstr++;

		// skip to next whitespace
		while (*pstr && *pstr > ' ')
			pstr++;

		if (!*pstr)
			break;

		pstr++;
		pfront = pstr;
	}
	for (j++; j < count; j++)
	{
		pVector[j] = 0;
	}
}


void UTIL_StringToVector(float *pVector, const char *pString)
{
	UTIL_StringToFloatArray(pVector, 3, pString);
}

void UTIL_StringToIntArray(int *pVector, int count, const char *pString)
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy(tempString, pString, sizeof(tempString));
	pstr = pfront = tempString;

	for (j = 0; j < count; j++)			// lifted from pr_edict.c
	{
		pVector[j] = atoi(pfront);

		while (*pstr && *pstr != ' ')
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for (j++; j < count; j++)
	{
		pVector[j] = 0;
	}
}

void UTIL_StringToColor32(color32 *color, const char *pString)
{
	int tmp[4];
	UTIL_StringToIntArray(tmp, 4, pString);
	color->r = tmp[0];
	color->g = tmp[1];
	color->b = tmp[2];
	color->a = tmp[3];
}





//-----------------------------------------------------------------------------
// Purpose: iterates through a typedescript data block, so it can insert key/value data into the block
// Input  : *pObject - pointer to the struct or class the data is to be insterted into
//			*pFields - description of the data
//			iNumFields - number of fields contained in pFields
//			char *szKeyName - name of the variable to look for
//			char *szValue - value to set the variable to
// Output : Returns true if the variable is found and set, false if the key is not found.
//-----------------------------------------------------------------------------
bool ExtractKeyvalue(void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen)
{
	int i;
	typedescription_t 	*pField;

	for (i = 0; i < iNumFields; i++)
	{
		pField = &pFields[i];

		int fieldOffset = pField->fieldOffset[TD_OFFSET_NORMAL];

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			for (datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap)
			{
				void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
				if (ExtractKeyvalue(pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue, iMaxLen))
					return true;
			}
		}

		if ((pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName))
		{
			switch (pField->fieldType)
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				Q_strncpy(szValue, ((char *)pObject + fieldOffset), iMaxLen);
				return true;

			case FIELD_TIME:
			case FIELD_FLOAT:
				Q_snprintf(szValue, iMaxLen, "%f", (*(float *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_BOOLEAN:
				Q_snprintf(szValue, iMaxLen, "%d", (*(bool *)((char *)pObject + fieldOffset)) != 0);
				return true;

			case FIELD_CHARACTER:
				Q_snprintf(szValue, iMaxLen, "%d", (*(char *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_SHORT:
				Q_snprintf(szValue, iMaxLen, "%d", (*(short *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_INTEGER:
			case FIELD_TICK:
				Q_snprintf(szValue, iMaxLen, "%d", (*(int *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				Q_snprintf(szValue, iMaxLen, "%f %f %f",
					((float *)((char *)pObject + fieldOffset))[0],
					((float *)((char *)pObject + fieldOffset))[1],
					((float *)((char *)pObject + fieldOffset))[2]);
				return true;

			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
				return false;

			case FIELD_MATRIX3X4_WORLDSPACE:
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
				return false;

			case FIELD_COLOR32:
				Q_snprintf(szValue, iMaxLen, "%d %d %d %d",
					((int *)((char *)pObject + fieldOffset))[0],
					((int *)((char *)pObject + fieldOffset))[1],
					((int *)((char *)pObject + fieldOffset))[2],
					((int *)((char *)pObject + fieldOffset))[3]);
				return true;

			case FIELD_CUSTOM:
			{
				/*
				SaveRestoreFieldInfo_t fieldInfo =
				{
				(char *)pObject + fieldOffset,
				pObject,
				pField
				};
				pField->pSaveRestoreOps->Parse( fieldInfo, szValue );
				*/
				return false;
			}

			default:
			case FIELD_INTERVAL: // Fixme, could write this if needed
			case FIELD_CLASSPTR:
			case FIELD_MODELINDEX:
			case FIELD_MATERIALINDEX:
			case FIELD_EDICT:
				Warning("Bad field in entity!!\n");
				Assert(0);
				break;
			}
		}
	}

	return false;
}

bool ParseKeyvalue(void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue)
{
	int i;
	typedescription_t   *pField;

	for (i = 0; i < iNumFields; i++)
	{
		pField = &pFields[i];

		int fieldOffset = pField->fieldOffset[TD_OFFSET_NORMAL];

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			for (datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap)
			{
				void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
				if (ParseKeyvalue(pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue))
					return true;
			}
		}

		if ((pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName))
		{
			switch (pField->fieldType)
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				(*(string_t *)((char *)pObject + fieldOffset)) = MAKE_STRING(szValue);//AllocPooledString
				return true;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*(float *)((char *)pObject + fieldOffset)) = atof(szValue);
				return true;

			case FIELD_BOOLEAN:
				(*(bool *)((char *)pObject + fieldOffset)) = (bool)(atoi(szValue) != 0);
				return true;

			case FIELD_CHARACTER:
				(*(char *)((char *)pObject + fieldOffset)) = (char)atoi(szValue);
				return true;

			case FIELD_SHORT:
				(*(short *)((char *)pObject + fieldOffset)) = (short)atoi(szValue);
				return true;

			case FIELD_INTEGER:
			case FIELD_TICK:
				(*(int *)((char *)pObject + fieldOffset)) = atoi(szValue);
				return true;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				UTIL_StringToVector((float *)((char *)pObject + fieldOffset), szValue);
				return true;

			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
				UTIL_StringToFloatArray((float *)((char *)pObject + fieldOffset), 16, szValue);
				return true;

			case FIELD_MATRIX3X4_WORLDSPACE:
				UTIL_StringToFloatArray((float *)((char *)pObject + fieldOffset), 12, szValue);
				return true;

			case FIELD_COLOR32:
				UTIL_StringToColor32((color32 *)((char *)pObject + fieldOffset), szValue);
				return true;

			case FIELD_CUSTOM:
			{
				SaveRestoreFieldInfo_t fieldInfo =
				{
					(char *)pObject + fieldOffset,
					pObject,
					pField
				};
				pField->pSaveRestoreOps->Parse(fieldInfo, szValue);
				return true;
			}

			default:
			case FIELD_INTERVAL: // Fixme, could write this if needed
			case FIELD_CLASSPTR:
			case FIELD_MODELINDEX:
			case FIELD_MATERIALINDEX:
			case FIELD_EDICT:
				Warning("Bad field in entity!!\n");
				Assert(0);
				break;
			}
		}
	}

	return false;
}


