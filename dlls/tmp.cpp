#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"

#include "nanomod.h"
extern struct NanoMod_Data nMod;

LINK_ENTITY_TO_CLASS(weapon_tmp, CPistolTMP);

void CPistolTMP::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_tmp.mdl");
	m_iId = WEAPON_TMP;
	m_iDefaultAmmo = 90; // How much ammo this weapon has on spawn
	FallInit();			 // get ready to fall down.
}

void CPistolTMP::Precache()
{
	PRECACHE_MODEL("models/v_tmp.mdl");
	PRECACHE_MODEL("models/w_tmp.mdl");
	PRECACHE_MODEL("models/p_tmp.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("weapons/tmp-1.wav");
	PRECACHE_SOUND("weapons/tmp-2.wav");

	// Precache fire event
	m_usFireTMP = PRECACHE_EVENT(1, "events/tmp.sc");
}

bool CPistolTMP::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm"; // Which ammo type this weapon use
	p->iMaxAmmo1 = _9MM_MAX_CARRY;	 //GetMaxCarry(WEAPON_MP5, SKILL_WEAPONS_BULLET);	// nMod: TMP using 9mm for now
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = NULL;
	p->iMaxClip = 30; // How many ammo this weapon's clip or magazine has
	p->iSlot = /*1;*/     GetWeaponMenuColumn(WEAPON_TMP);			 // nMod
	p->iPosition = /*3;*/ GetWeaponMenuRow(WEAPON_TMP); // nMod
	p->iFlags = 0;	  // Special flags this weapon has
	p->iId = m_iId = WEAPON_TMP;
	p->iWeight = MP5_WEIGHT; // How much "priority" this weapon has when auto-switch is triggered

	return true;
}

bool CPistolTMP::Deploy()
{
	//  The last parameter is the animation set for the player model in thirdperson to use
	return DefaultDeploy("models/v_tmp.mdl", "models/p_tmp.mdl", TMP_DRAW, "mp5");
}

void CPistolTMP::SecondaryAttack()
{
#ifdef CLIENT_DLL
	if (!bIsMultiplayer())
#else
	if (!g_pGameRules->IsMultiplayer())
	{
		//		return;  // nMod: this is usually multiplayer-only
	}
#endif

		if (m_pPlayer->m_iFOV != 0)
		{
			m_pPlayer->m_iFOV = 0;		  // 0 means reset to default fov
			nMod.PlayerZoomed = false; // nMod
		}
		else if (m_pPlayer->m_iFOV != 40)
		{
			m_pPlayer->m_iFOV = 40;
			nMod.PlayerZoomed = true; // nMod
		}

	m_flNextSecondaryAttack = 0.5;
}

void CPistolTMP::PrimaryAttack()
{
	// nMod
	//float skill = (CVAR_GET_FLOAT("skill_hazard"));
	int skill = GetSkillLevel(SKILL_HAZARD);

	if (skill == SKILL_BEGINNER) // nMod
	{
		// don't fire underwater
		if (m_pPlayer->pev->waterlevel == 3)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
			return;
		}
	}

	// Don't fire if empty
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	// Size of the muzzle flash and how much volume in the world the firing sound produces
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

	// Bullet firing code. By order:
	// 1. No. of bullet: 1 for other guns, multiple for shotgun)
	// 2. Firing source: its firing from player 3rd person mdl's gun attachment point
	// 3. Firing direction: The autoaim vector is mainly for autoaim feature, however it's still used to pinpoint end position
	// 4. Spread: How big the randomness of the bullet spread. To make 100% accuracy, make it Vector(0, 0, 0). Otherwise, you can use vector cone
	// 5. Trace line end position: 8192 is the max world length in HL
	// 6. Bullet type: Control end point effects and damage. If damage is not specified in slot number 8 below, it will use default value of MP5 damage.
	// 7. Frequency of tracer effect. Has a number between 0-4. 0 means no tracer at all (like in CS 1.6), 1 is the highest frequency.
	// 8. Damage. If left empty, it will use the default of value of bullet type.
	//
	//m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_357, 1, 0);

	vecDir = m_pPlayer->FireBulletsPlayer(
		1,		   // Number of bullets to shoot
		vecSrc,	   // The source of the bullets (i.e. the gun)
		vecAiming, // The direction to fire in (i.e. where the player is pointing)
		GetAccuracy(m_pPlayer, WEAPON_MP5),
		//VECTOR_CONE_10DEGREES, // The accuracy spread of the weapon
		8192,				   // The distance the bullet can go (8192 is the limit for the engine)
		BULLET_PLAYER_357,	   // The type of bullet being fired
		1,					   // Number of tracer bullets to fire (none in this case)
		0,					   // Set to non-zero to override the amount of damage (usually, leave this as 0)
		m_pPlayer->pev,		   // Attacker entity
		m_pPlayer->random_seed // The random seed
	);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireTMP, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip == 0) ? 1 : 0, 0);
	//PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireAK47, 0.0, g_vecZero, g_vecZero, vecSrc.x, vecSrc.y, 0, 0, 0, 0);

	// If the clip is now empty and there's no more ammo available, update the HEV
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	// Play view model animation and firing sound
	SendWeaponAnim(TMP_SHOOT1 + RANDOM_LONG(0, 2));
	// the 3rd entry defines the firing sound path. Make sure to precache first
	EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/tmp-1.wav", 1, ATTN_NORM);

	// Eject the brass
	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT(100, 200) +
							  gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 20 +
				   gpGlobals->v_right * -8,
		vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	// Punch the camera to simulate recoil
	//if (!RecoilExceptions(m_pPlayer))
	m_pPlayer->pev->punchangle.x -= GetWeaponRecoil(/*m_pPlayer, */ WEAPON_AK47); // nMod: default was 2.0

	// Remove a bullet
	m_iClip--;
	// Next time for attack and weapon idling
	m_flNextPrimaryAttack = 0.1;
	m_flTimeWeaponIdle = 0.85;
}


void CPistolTMP::Reload()
{
	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}

	// If ammo is not 30, reload using AK47_RELOAD animation and disable firing and idling for 2.5 seconds
	DefaultReload(30, TMP_RELOAD, 2.5, 0);
}


void CPistolTMP::WeaponIdle()
{
	ResetEmptySound();

	// Not the time to idle
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// Play idle animation
	SendWeaponAnim(TMP_IDLE1);
	// When to call idle animation again in seconds. Change this value if your mdl has a certain value for idle animation for smooth transitioning
	m_flTimeWeaponIdle = 10;
}




#if 0
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "nanomod.h"

LINK_ENTITY_TO_CLASS(weapon_tmp, CPistolTMP)

void CPistolTMP::Spawn()
{
	//pev->classname = MAKE_STRING("weapon_tmp");
	Precache();
	SET_MODEL(ENT(pev), "models/w_tmp.mdl");
	m_iId = WEAPON_TMP;
	m_iDefaultAmmo = 90;
	FallInit(); // get ready to fall down.
}

void CPistolTMP::Precache()
{
	PRECACHE_MODEL("models/v_tmp.mdl");
	PRECACHE_MODEL("models/w_tmp.mdl");
	PRECACHE_MODEL("models/p_tmp.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("weapons/tmp-1.wav");
	PRECACHE_SOUND("weapons/tmp-2.wav");

	m_TMP = PRECACHE_EVENT(1, "events/tmp.sc");
}

bool CPistolTMP::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = NULL;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TMP;
	p->iWeight = MP5_WEIGHT;

	return true;
}


void CPistolTMP::PrimaryAttack()
{
	// nMod
	if (GetSkillLevel(SKILL_SWIMMING) != SKILL_ADVANCED)

		// don't fire underwater
		if (m_pPlayer->pev->waterlevel == 3)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
			return;
		}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	// Punch the camera to simulate recoil
	m_pPlayer->pev->punchangle.x -= GetWeaponRecoil(WEAPON_MP5); // nMod


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;


	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_TMP, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

bool CPistolTMP::Deploy()
{
	return DefaultDeploy("models/v_tmp.mdl", "models/p_tmp.mdl", TMP_DRAW, "mp5");
}

/*
void CPistolTMP::Holster()
{
}
*/

void CPistolTMP::Reload()
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	DefaultReload(MP5_MAX_CLIP, TMP_RELOAD, 1.5);
}


void CPistolTMP::WeaponIdle()
{
	ResetEmptySound();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(TMP_IDLE1);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}
#endif