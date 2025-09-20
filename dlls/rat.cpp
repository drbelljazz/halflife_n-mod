/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// rat - environmental monster
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "decals.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CRat : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;

	//void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void Killed(entvars_t* pevAttacker, int iGib) override;

	//bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	//void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	//void StartTask(Task_t* pTask) override;
	//void RunTask(Task_t* pTask) override;
	//void EXPORT Touch(CBaseEntity* pOther) override;
	
	//EHANDLE m_hPlayer;
};
LINK_ENTITY_TO_CLASS(monster_rat, CRat);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CRat::Classify()
{
	return CLASS_INSECT;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRat::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
	default:
		ys = 45;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CRat::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/nanomod/bigrat.mdl");
	UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 2));  // roach size
	//UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	pev->view_ofs = Vector(0, 0, 6); // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;			 // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	pev->takedamage = DAMAGE_YES; // nMod

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRat::Precache()
{
	PRECACHE_MODEL("models/nanomod/bigrat.mdl");

	PRECACHE_SOUND("headcrab/hc_die1.wav");
	PRECACHE_SOUND("headcrab/hc_die2.wav");
}

//#if 0
void CRat::Killed(entvars_t* pevAttacker, int iGib)
{
	pev->solid = SOLID_NOT;

	//random sound
	if (RANDOM_LONG(0, 4) == 1)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "headcrab/hc_die1.wav", 0.8, ATTN_NORM, 0, 80 + RANDOM_LONG(0, 39));
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "headcrab/hc_die2.wav", 0.7, ATTN_NORM, 0, 80 + RANDOM_LONG(0, 39));
	}

	//CSoundEnt::InsertSound(bits_SOUND_WORLD, pev->origin, 128, 1);
	/*
	CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
	if (pOwner)
	{
		pOwner->DeathNotice(pev);
	}
	UTIL_Remove(this);*/

	CBaseMonster::Killed(pevAttacker, iGib);

}
//#endif

#if 0
bool CRat::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	return true;
}
#endif

#if 0
void CRat::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	//UTIL_Ricochet(ptr->vecEndPos, 1.0);
	//AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
	
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST)) != 0)
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)) != 0)
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CBaseEntity::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}
#endif

#if 0
void CRat::Touch(CBaseEntity* pOther)
{
	Vector vecSpot;
	TraceResult tr;

	int blood_decal;

	if (pOther->pev->velocity == g_vecZero || !pOther->IsPlayer())
	{
		return;
	}

	if (RANDOM_LONG(0, 4) == 1)
		blood_decal = DECAL_BLOOD1;
	else
		blood_decal = DECAL_BLOOD2;
		
		/*	DECAL_BLOOD3,
			DECAL_BLOOD4,
			DECAL_BLOOD5,
			DECAL_BLOOD6,*/

	vecSpot = pev->origin + Vector(0, 0, 8); //move up a bit, and trace down.
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -24), ignore_monsters, ENT(pev), &tr);

	// This isn't really blood.  So you don't have to screen it out based on violence levels (UTIL_ShouldShowBlood())
	UTIL_DecalTrace(&tr, blood_decal + RANDOM_LONG(0, 5));

	TakeDamage(pOther->pev, pOther->pev, pev->health, DMG_CRUSH);
}
#endif
#if 0
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRat::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}
#endif

#if 0
//=========================================================
// AI Schedules Specific to this monster
//=========================================================

#if 0
void CRat::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_WAIT:
		/*if (m_hPlayer == NULL)
		{
			m_hPlayer = UTIL_FindEntityByClassname(NULL, "player");
		}*/
		break;
	}
	CBaseMonster::StartTask(pTask);
}
#endif

void CRat::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_WAIT:
		// look at who I'm talking to
		/*if (m_flTalkTime > gpGlobals->time && m_hTalkTarget != NULL)
		{
			float yaw = VecToYaw(m_hTalkTarget->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180)
				yaw -= 360;
			if (yaw < -180)
				yaw += 360;

			// turn towards vector
			SetBoneController(0, yaw);
		}
		// look at player, but only if playing a "safe" idle animation
		else if (m_hPlayer != NULL && pev->sequence == 0)
		{
			float yaw = VecToYaw(m_hPlayer->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180)
				yaw -= 360;
			if (yaw < -180)
				yaw += 360;

			// turn towards vector
			SetBoneController(0, yaw);
		}
		else
		{*/
			SetBoneController(0, 0);
		//}
		CBaseMonster::RunTask(pTask);
		break;

	// nMod
	case TASK_DIE:
		
		//pev->deadflag = DEAD_DEAD;
		TaskComplete();
		break;
		

	default:
		SetBoneController(0, 0);
		CBaseMonster::RunTask(pTask);
		break;
	}
}
#endif