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

#include "CPlayer.h"
#include "CHelpers.h"
#include "shareddefs.h"
#include "in_buttons.h"
#include "vphysics/vehicles.h"

SH_DECL_MANUALHOOK3(FVisible, 0, 0, 0, bool, CBaseEntity *, int, CBaseEntity **);
SH_DECL_MANUALHOOK2_void(PlayerRunCmd, 0, 0, 0, CUserCmd *, IMoveHelper *);
SH_DECL_MANUALHOOK2_void(LeaveVehicle, 0, 0, 0, const Vector &, const QAngle &);
SH_DECL_MANUALHOOK5_void(ProcessUserCmds, 0, 0, 0, CUserCmd *, int, int, int, bool);
SH_DECL_MANUALHOOK0_void(PreThink, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(PostThink, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(Jump, 0, 0, 0);
SH_DECL_MANUALHOOK1(OnTakeDamage_Alive, 0, 0, 0, int, CEntityTakeDamageInfo &);
SH_DECL_MANUALHOOK2(WeaponSwitch, 0, 0, 0, bool, CBaseEntity */*CBaseCombatWeapon*/, int);
SH_DECL_MANUALHOOK0(IsReadyToSpawn, 0, 0, 0, bool);
SH_DECL_MANUALHOOK0(CanSpeakVoiceCommand, 0, 0, 0, bool);
SH_DECL_MANUALHOOK4(GiveNamedItem, 0, 0, 0, CBaseEntity *, char const *, int, CEconItemView *, bool);
SH_DECL_MANUALHOOK1(RemovePlayerItem, 0, 0, 0, bool, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(Weapon_Equip, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1(Weapon_GetSlot, 0, 0, 0, CBaseEntity *, int);
SH_DECL_MANUALHOOK0(GetClientEyeAngles, 0, 0, 0, QAngle *);
SH_DECL_MANUALHOOK2(ShouldGib, 0, 0, 0, bool, const CEntityTakeDamageInfo &, bool);

DECLARE_HOOK(FVisible, CPlayer);
DECLARE_HOOK(PlayerRunCmd, CPlayer);
DECLARE_HOOK(LeaveVehicle, CPlayer);
DECLARE_HOOK(ProcessUserCmds, CPlayer);
DECLARE_HOOK(PreThink, CPlayer);
DECLARE_HOOK(PostThink, CPlayer);
DECLARE_HOOK(Jump, CPlayer);
DECLARE_HOOK(OnTakeDamage_Alive, CPlayer);
DECLARE_HOOK(WeaponSwitch, CPlayer);
DECLARE_HOOK(IsReadyToSpawn, CPlayer);
DECLARE_HOOK(CanSpeakVoiceCommand, CPlayer);
DECLARE_HOOK(GiveNamedItem, CPlayer);
DECLARE_HOOK(RemovePlayerItem, CPlayer);
DECLARE_HOOK(Weapon_Equip, CPlayer);
DECLARE_HOOK(Weapon_GetSlot, CPlayer);
DECLARE_HOOK(GetClientEyeAngles, CPlayer);
DECLARE_HOOK(ShouldGib, CPlayer);

DECLARE_DETOUR(HandleCommand_JoinClass, CPlayer);

LINK_ENTITY_TO_INTERNAL_CLASS(CTFPlayer, CPlayer);

//Sendprops
DEFINE_PROP(m_flNextAttack, CPlayer);
DEFINE_PROP(m_hActiveWeapon, CPlayer);
DEFINE_PROP(m_hMyWeapons, CPlayer);
DEFINE_PROP(m_hVehicle, CPlayer);
DEFINE_PROP(m_iHealth, CPlayer);
//DEFINE_PROP(m_iMaxHealth, CPlayer);
DEFINE_PROP(m_lifeState, CPlayer);
DEFINE_PROP(m_iClass, CPlayer);
DEFINE_PROP(m_iDesiredPlayerClass, CPlayer);
DEFINE_PROP(m_nPlayerCond, CPlayer);
DEFINE_PROP(m_bJumping, CPlayer);
DEFINE_PROP(m_nPlayerState, CPlayer);
DEFINE_PROP(m_nDisguiseTeam, CPlayer);
DEFINE_PROP(m_nDisguiseClass, CPlayer);
DEFINE_PROP(m_iDisguiseTargetIndex, CPlayer);
DEFINE_PROP(m_iDisguiseHealth, CPlayer);
DEFINE_PROP(m_flMaxspeed, CPlayer);
DEFINE_PROP(m_iObserverMode, CPlayer);
DEFINE_PROP(m_hObserverTarget, CPlayer);
DEFINE_PROP(m_hRagdoll, CPlayer);

//Datamaps
DEFINE_PROP(m_nButtons, CPlayer);

//IMPLEMENT_NULL_DATADESC(CPlayer);

DECLARE_DEFAULTHANDLER_void(CPlayer, LeaveVehicle, (const Vector &vecExitPoint, const QAngle &vecExitAngles), (vecExitPoint, vecExitAngles));
DECLARE_DEFAULTHANDLER(CPlayer, GiveNamedItem, CBaseEntity *, (char const *szName, int iSubType, CEconItemView *pScriptItem, bool bForce), (szName, iSubType, pScriptItem, bForce));
DECLARE_DEFAULTHANDLER(CPlayer, RemovePlayerItem, bool, (CBaseEntity *pItem), (pItem));
DECLARE_DEFAULTHANDLER_void(CPlayer, Weapon_Equip, (CBaseEntity *pWeapon), (pWeapon));
DECLARE_DEFAULTHANDLER(CPlayer, Weapon_GetSlot, CBaseEntity *, (int slot), (slot));
DECLARE_DEFAULTHANDLER(CPlayer, GetClientEyeAngles, QAngle *, (), ());
DECLARE_DEFAULTHANDLER(CPlayer, ShouldGib, bool, (const CEntityTakeDamageInfo &info, bool bFeignDeath), (info, bFeignDeath));

DECLARE_DEFAULTHANDLER_DETOUR_void(CPlayer, HandleCommand_JoinClass, (const char *pClass, bool bAllowSpawn), (pClass, bAllowSpawn));

void CPlayer::PlayerRunCmd(CUserCmd *pCmd, IMoveHelper *pHelper)
{
	IServerVehicle *pVehicle = GetVehicle();
	if (pVehicle)
	{
		/**
		 * We don't call ProcessMovement or FinishMove because we know CPropVehicleDriveable ignores them.  
		 * This may not work in other cases. Same with passing NULL as a CMoveData *
		 */
		pVehicle->SetupMove(*this, pCmd, pHelper, NULL);

		if (pCmd->buttons & IN_USE)
		{
			if (!pVehicle->IsPassengerEntering() && !pVehicle->IsPassengerExiting())
			{
				pVehicle->HandlePassengerExit(*this);

				/* HACKHACK: Implement FindEntityByClassname or sig it */
				/* Unsure why we need to do this, SetPassenger should have been called and removed it - Manually calling this == client crash */
				CEntity *pEnt;
				for (int i=0; i<= MAX_EDICTS; i++)
				{
					pEnt = CEntity::Instance(i);
					if (!pEnt)
					{
						continue;
					}

					if (pEnt->GetOwner() != this)
					{
						continue;
					}

					if (strcmp("entity_blocker", pEnt->GetClassname()) == 0)
					{
						pEnt->AcceptInput("Kill", NULL, NULL, g_Variant, 0);
					}
				}
			}
		}
	}

	if (!m_bInPlayerRunCmd)
	{
		SH_MCALL(BaseEntity(), PlayerRunCmd)(pCmd, pHelper);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPPlayerRunCmd(thisptr)))(pCmd, pHelper);
	SET_META_RESULT(MRES_SUPERCEDE);

	return;
}

void CPlayer::InternalPlayerRunCmd(CUserCmd *pCmd, IMoveHelper *pHelper)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInPlayerRunCmd = true;
	pEnt->PlayerRunCmd(pCmd, pHelper);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInPlayerRunCmd = false;

	return;
}

bool CPlayer::IsPlayer()
{
	return true;
}

bool CPlayer::IsAlive()
{
	return m_lifeState == LIFE_ALIVE;
}

void CPlayer::SetPlayerClass(int playerclass, bool persistant)
{
	*m_iClass = playerclass;
	if (persistant)
		*m_iDesiredPlayerClass = playerclass;
}

int CPlayer::GetPlayerClass()
{
	return *m_iClass;
}

int CPlayer::GetPlayerCond()
{
	return m_nPlayerCond;
}

bool CPlayer::IsDisguised()
{
	return (m_nPlayerCond & PLAYERCOND_DISGUISED) == PLAYERCOND_DISGUISED;
}

int CPlayer::GetDisguisedTeam()
{
	return m_nDisguiseTeam;
}

int CPlayer::GetButtons()
{
	return m_nButtons;
}

float CPlayer::GetMovementSpeed()
{
	return m_flMaxspeed;
}

void CPlayer::SetMovementSpeed(float speed)
{
	m_flMaxspeed = speed;
}

int CPlayer::GetHealth()
{
	return m_iHealth;
}

void CPlayer::SetHealth(int health)
{
	m_iHealth = health;
	//m_iMaxHealth = health;
}

void CPlayer::GetClientEyePosition(Vector &pos)
{
	gameclients->ClientEarPosition(this->m_pEdict, &pos);
}

int CPlayer::GetObserverMode()
{
	return m_iObserverMode;
}

CEntity *CPlayer::GetObserverTarget()
{
	return Instance(m_hObserverTarget);
}

CEntity *CPlayer::GetAimTarget(bool playersOnly=false)
{
	Vector eye_position;
	QAngle eye_angles;

	GetClientEyePosition(eye_position);

	QAngle *angles = GetClientEyeAngles();
	eye_angles.Init(angles->x, angles->y, angles->z);

	trcontents_t *tr;
	tr = pHelpers->TR_TraceRayFilter(eye_position, eye_angles, MASK_SHOT, RayType_Infinite, this);

	if(tr->entity != NULL)
	{
		CEntity *Ent = CEntity::Instance(tr->entity);
		int index = Ent->entindex();

		if(playersOnly && (index < 1 || index > 33))
			return NULL;
		else
			return Ent;
	}

	return NULL;
}

CEntity *CPlayer::GetRagdoll()
{
	return Instance(m_hRagdoll);
}

bool CPlayer::FVisible(CEntity *pEntity, int traceMask, CEntity **ppBlocker)
{
	if (!m_bInFVisible)
	{
		CBaseEntity *pCopyBack;
		bool ret = SH_MCALL(BaseEntity(), FVisible)(*pEntity, traceMask, &pCopyBack);
		if (ppBlocker)
			*ppBlocker = *pCopyBack;

		return ret;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	CBaseEntity *pCopyBack = NULL;
	bool ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPFVisible(thisptr)))(*pEntity, traceMask, &pCopyBack);
	SET_META_RESULT(MRES_SUPERCEDE);

	if (ppBlocker && pCopyBack)
		*ppBlocker = *pCopyBack;

	return ret;
}

bool CPlayer::InternalFVisible(CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	// HACK: Fix this properly
	CEntity *pFirstParam = *pEntity;
	if (!pFirstParam)
	{
		//g_pSM->LogError(myself, "No matching CEntity found for *pEntity in CPlayer::InternalFVisible, aborting call.");
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int index = pEnt->entindex();
	pEnt->m_bInFVisible = true;
	CEntity *pCopyBack;

	bool ret = pEnt->FVisible(/* *pEntity */ pFirstParam, traceMask, &pCopyBack);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInFVisible = false;

	if (ppBlocker)
		*ppBlocker = *pCopyBack;

	return ret;
}

IServerVehicle *CPlayer::GetVehicle()
{
	CEntity *pVehicle = CEntity::Instance(m_hVehicle);
	if (pVehicle)
	{
		return pVehicle->GetServerVehicle();
	}

	return NULL;
}

void CPlayer::ProcessUserCmds(CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused)
{
	if (!m_bInProcessUserCmds)
	{
		SH_MCALL(BaseEntity(), ProcessUserCmds)(cmds, numcmds, totalcmds, dropped_packets, paused);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPProcessUserCmds(thisptr)))(cmds, numcmds, totalcmds, dropped_packets, paused);
	SET_META_RESULT(MRES_SUPERCEDE);

	return;
}

void CPlayer::InternalProcessUserCmds(CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInProcessUserCmds = true;
	pEnt->ProcessUserCmds(cmds, numcmds, totalcmds, dropped_packets, paused);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInProcessUserCmds = false;

	return;
}

void CPlayer::PreThink()
{
	if (!m_bInPreThink)
	{
		SH_MCALL(BaseEntity(), PreThink)();
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPPreThink(thisptr)))();
	SET_META_RESULT(MRES_SUPERCEDE);

	return;
}

void CPlayer::InternalPreThink()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInPreThink = true;
	pEnt->PreThink();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInPreThink = false;

	return;
}

void CPlayer::PostThink()
{
	if (!m_bInPostThink)
	{
		SH_MCALL(BaseEntity(), PostThink)();
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPPostThink(thisptr)))();
	SET_META_RESULT(MRES_SUPERCEDE);

	return;
}

void CPlayer::InternalPostThink()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInPostThink = true;
	pEnt->PostThink();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInPostThink = false;

	return;
}

void CPlayer::Jump()
{
	if (!m_bInJump)
	{
		SH_MCALL(BaseEntity(), Jump)();
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPJump(thisptr)))();
	SET_META_RESULT(MRES_SUPERCEDE);

	return;
}

void CPlayer::InternalJump()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex();
	pEnt->m_bInJump = true;
	pEnt->Jump();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInJump = false;

	return;
}

int CPlayer::OnTakeDamage_Alive(CEntityTakeDamageInfo &info)
{
	if (!m_bInOnTakeDamage_Alive)
	{
		int ret = SH_MCALL(BaseEntity(), OnTakeDamage_Alive)(info);
		return ret;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	int ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPOnTakeDamage_Alive(thisptr)))(info);
	SET_META_RESULT(MRES_SUPERCEDE);

	return ret;
}

int CPlayer::InternalOnTakeDamage_Alive(CEntityTakeDamageInfo &info)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, 0);
	}

	int index = pEnt->entindex();
	pEnt->m_bInOnTakeDamage_Alive = true;
	int ret = pEnt->OnTakeDamage_Alive(info);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInOnTakeDamage_Alive = false;

	return ret;
}

bool CPlayer::WeaponSwitch(CBaseEntity *pWeapon, int viewmodelindex)
{
	if (!m_bInWeaponSwitch)
	{
		bool ret = SH_MCALL(BaseEntity(), WeaponSwitch)(pWeapon, viewmodelindex);
		return ret;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	bool ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPWeaponSwitch(thisptr)))(pWeapon, viewmodelindex);
	SET_META_RESULT(MRES_SUPERCEDE);

	return ret;
}

bool CPlayer::InternalWeaponSwitch(CBaseEntity *pWeapon, int viewmodelindex)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int index = pEnt->entindex();
	pEnt->m_bInWeaponSwitch = true;
	bool ret = pEnt->WeaponSwitch(pWeapon, viewmodelindex);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInWeaponSwitch = false;

	return ret;
}

bool CPlayer::IsReadyToSpawn()
{
	if (!m_bInIsReadyToSpawn)
	{
		bool ret = SH_MCALL(BaseEntity(), IsReadyToSpawn)();
		return ret;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	bool ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPIsReadyToSpawn(thisptr)))();
	SET_META_RESULT(MRES_SUPERCEDE);

	return ret;
}

bool CPlayer::InternalIsReadyToSpawn()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int index = pEnt->entindex();
	pEnt->m_bInIsReadyToSpawn = true;
	bool ret = pEnt->IsReadyToSpawn();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInIsReadyToSpawn = false;

	return ret;
}

bool CPlayer::CanSpeakVoiceCommand()
{
	if (!m_bInCanSpeakVoiceCommand)
	{
		bool ret = SH_MCALL(BaseEntity(), CanSpeakVoiceCommand)();
		return ret;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	bool ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPCanSpeakVoiceCommand(thisptr)))();
	SET_META_RESULT(MRES_SUPERCEDE);

	return ret;
}

bool CPlayer::InternalCanSpeakVoiceCommand()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CPlayer *pEnt = dynamic_cast<CPlayer *>(CEntity::Instance(META_IFACEPTR(CBaseEntity)));
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int index = pEnt->entindex();
	pEnt->m_bInCanSpeakVoiceCommand = true;
	bool ret = pEnt->CanSpeakVoiceCommand();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInCanSpeakVoiceCommand = false;

	return ret;
}

void CPlayer::Spawn()
{
	BaseClass::Spawn();
}
