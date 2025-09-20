#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"
#include "nanomod.h"


//
// Main stuff
//

void NanoMod_Init()
{
	RegisterCommands();
	ResetNanoMod();
	Init_Menus();
	RPGStats_Setup();
	SetDefaultSkills();
}


void RegisterCommands()
{
	g_engfuncs.pfnAddServerCommand("skill_menu", &ToggleSkillMenu);
	g_engfuncs.pfnAddServerCommand("beast", &AdrenalineRush);
	g_engfuncs.pfnAddServerCommand("bandage", &ApplyBandage);
	g_engfuncs.pfnAddServerCommand("showinventory", &ShowInventory);
}


void WorldInit()
{
	InitHordeMode();
}


// Struct used for skill info and game flags
NanoMod_Data nMod;

void ResetNanoMod()
{
	memset(nMod.pSkills, 0, sizeof(nMod.pSkills));
	memset(nMod.pAccuracy, 0, sizeof(nMod.pAccuracy));

	nMod.pSkillPoints = DEFAULT_SKILL_POINTS;  // Temporarily gifting skill points for testing

	RPGStats_Setup();
}


// Any constant/looped code goes here; called by PreThink()
bool NanoThink(entvars_t* pev)
{
	if (!NanoThinkDelay())
		return false;

	CheckBleed(pev);

	SpawnRandomMonster();

	return true;
}


bool NanoThinkDelay()
{
	static float timedelay;

	if (timedelay == NULL)
	{
		timedelay = gpGlobals->time + NANO_THINKDELAY;
		return false;
	}

	if (timedelay > gpGlobals->time)
		return false;

	if (timedelay <= gpGlobals->time)
	{
		timedelay = NULL;
		return true;
	}

	return false;
}


// 
// New
//

char* FloatToString(float num)
{
	//int numitems = nMod.Inventory[ITEM_MEDKIT];

	char* string = NULL;

	itoa(num, string, 10);

	return string;
}

void ShowInventory()
{
	static float delay;

	// Still delaying
	if (delay > gpGlobals->time)
		return;

	// Start timer for the first time
	if (delay == NULL)
	{
		delay = gpGlobals->time + 5;
		//CVAR_SET_FLOAT("inventory_visible", 1);
		//nMod.ShowingInventory = true;
	}

	// Timer expired so reset it
	if (delay <= gpGlobals->time)
	{
		delay = gpGlobals->time + 5;
		CVAR_SET_FLOAT("inventory_visible", 0);
		//nMod.ShowingInventory = true;
	}


	int medkits = nMod.Inventory[ITEM_MEDKIT];
	int batteries = nMod.Inventory[ITEM_BATTERY];

	if (medkits + batteries == 0)
	{
		Print("NOT CARRYING ANY ITEMS!\n", print_chat);
		Print("\n", print_chat);
		return;
	}

	CVAR_SET_FLOAT("inventory_visible", 1);
	//nMod.ShowingInventory = true;

	Print("== INVENTORY ITEMS ==\n", print_chat);
	Print("\n", print_chat);


	if (medkits > 0)
	{
		char numitems[8] = "";
		char suffix[16];
		char fullstr[64];

		itoa(medkits, numitems, 10);

		strcpy(fullstr, "      Medkits: ");
		strcpy(suffix, "\n");

		strcat(fullstr, numitems);
		strcat(fullstr, suffix);

		Print(fullstr, print_chat);
	}

	if (batteries > 0)
	{
		char numitems[8] = "";
		char suffix[16];
		char fullstr[64];

		itoa(batteries, numitems, 10);

		strcpy(fullstr, "      Batteries: ");
		strcpy(suffix, "\n");

		strcat(fullstr, numitems);
		strcat(fullstr, suffix);

		Print(fullstr, print_chat);
	}

	Print("\n", print_chat);
}


//
// Menu stuff
//

DefaultWeaponMenu default_WeaponMenu;
AlternateWeaponMenu alt_WeaponMenu;

int GetWeaponMenuColumn(int weapon)
{
	bool NewWeaponMenu = CVAR_GET_FLOAT("nano_weaponmenu");

	if (NewWeaponMenu == true)
		return alt_WeaponMenu.Column[weapon];
	else
		return default_WeaponMenu.Column[weapon];
}

int GetWeaponMenuRow(int weapon)
{
	bool NewWeaponMenu = CVAR_GET_FLOAT("nano_weaponmenu");

	if (NewWeaponMenu == true)
		return alt_WeaponMenu.Row[weapon];
	else
		return default_WeaponMenu.Row[weapon];
}

void ToggleSkillMenu()
{
	nMod.ShowUpgradeMenu = !nMod.ShowUpgradeMenu;
}


//
// Points stuff
//

void ScorePoints(entvars_t* pevAttacker, entvars_t* pev)
{
	float bonus;
	
	if (!FClassnameIs(pevAttacker, "player"))
		return;

	if (!FClassnameIs(pevAttacker, "monster_snark"))
		return;

	if (FStrEq(STRING(pev->globalname), "GMAN"))
		bonus = KillGMan(pev);
	else if (FClassnameIs(pev, "monster_scientist") || FClassnameIs(pev, "monster_barney"))
		bonus = NANO_PENALTY_XPBONUS;
	else if (FClassnameIs(pev, "monster_cockroach"))
		return;
	else if (FClassnameIs(pev, "monster_gman"))
		return;
	else
		bonus = pev->max_health * KILL_REWARD_MULTIPLIER;

	nMod.pSkillPoints += bonus;

	DisplayBonus(pevAttacker, bonus);
}


void HeadshotBonus()
{
	nMod.pSkillPoints += 25;

	Print("HEADSHOT!\n", print_center); // nMod
	Print("Headshot bonus! +25 BIO-ENERGY\n", print_console);
}


void DisplayBonus(entvars_t* pevPlayer, int points)
{
	if (CVAR_GET_FLOAT("showxp") == SHOW_NO_XPMSGS)
		return;

	int bonus_xp = points;
	int total_xp = nMod.pSkillPoints;

	char suffix[16];
	char fullstr[64];
	char bonus[16];

	itoa(bonus_xp, bonus, 10);

	strcpy(fullstr, "+BIO-ENERGY: ");
	strcpy(suffix, "\n");

	strcat(fullstr, bonus);
	strcat(fullstr, suffix);

	Print(fullstr, print_chat);

	// TODO: how can we show every time we pass a milestone multiple of 100, not just when it's exactly divisible by 100

	int remainder = total_xp % NANO_SCOREREPORT_INTERVAL; // Only showing total when we hit exact multiples of scorereport interval

	if (!remainder && total_xp > 0 && bonus_xp > 0)
		DisplayScore();
}


void DisplayScore()
{
	if (CVAR_GET_FLOAT("showxp") == SHOW_NO_XPMSGS)
		return;

	int total_xp = nMod.pSkillPoints;

	char score[16];
	char suffix[16];
	char fullstr[64];

	itoa(total_xp, score, 10);

	strcpy(fullstr, "TOTAL BIO-ENERGY: ");
	strcpy(suffix, "\n");

	strcat(fullstr, score);
	strcat(fullstr, suffix);

	Print(fullstr, print_chat);
}


//
// Skill stuff
//

void SetDefaultSkills()
{
	// We will only need this pSkills array once we stop using the skill_ CVARs

	nMod.pSkills[SKILL_WEAPONS_BULLET] = CVAR_GET_FLOAT("skill_bullet");
	nMod.pSkills[SKILL_WEAPONS_SPECIAL] = CVAR_GET_FLOAT("skill_special");
	nMod.pSkills[SKILL_LEGS] = CVAR_GET_FLOAT("skill_legs");
	nMod.pSkills[SKILL_ARMS] = CVAR_GET_FLOAT("skill_arms");
	nMod.pSkills[SKILL_TORSO] = CVAR_GET_FLOAT("skill_torso");
	nMod.pSkills[SKILL_HEAD] = CVAR_GET_FLOAT("skill_head");
	nMod.pSkills[SKILL_STEALTH] = CVAR_GET_FLOAT("skill_stealth");
	nMod.pSkills[SKILL_FIRMWARE] = CVAR_GET_FLOAT("skill_firmware");
	nMod.pSkills[SKILL_TIME] = CVAR_GET_FLOAT("skill_time");
	nMod.pSkills[SKILL_HAZARD] = CVAR_GET_FLOAT("skill_hazard");
}


int GetSkillLevel(int skilltype)
{
	CheckSkillChange(skilltype);

	return nMod.pSkills[skilltype];
}


// Temporary method while we use skill_ CVARS for testing
void CheckSkillChange(int skilltype)
{
	switch (skilltype)
	{
	case SKILL_WEAPONS_BULLET:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_weapons_bullet");
		break;

	case SKILL_WEAPONS_SPECIAL:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_weapons_special");
		break;

	case SKILL_LEGS:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_legs");
		break;

	case SKILL_TORSO:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_torso");
		break;

	case SKILL_ARMS:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_arms");
		break;

	case SKILL_HEAD:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_head");
		break;

	case SKILL_FIRMWARE:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_firmware");
		break;

	case SKILL_STEALTH:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_stealth");
		break;

	case SKILL_TIME:
		nMod.pSkills[skilltype] = CVAR_GET_FLOAT("skill_time");
		break;

	default:
		break;
	}
}


float GetDamageReduction(int bodypart)
{
	int skill;
	
	switch (bodypart)
	{
		case LEGS_DMG:
			skill = GetSkillLevel(SKILL_LEGS);
			break;
		case ARMS_DMG:
			skill = GetSkillLevel(SKILL_ARMS);
			break;
		case TORSO_DMG:
			skill = GetSkillLevel(SKILL_TORSO);
			break;
		case HEAD_DMG:
			skill = GetSkillLevel(SKILL_HEAD);
			break;
	}

	switch (skill)
	{
		case SKILL_BEGINNER:
			return DAMAGE_REDUCTION_BEGINNER;

		case SKILL_INTERMEDIATE:
			return DAMAGE_REDUCTION_INTERMEDIATE;

		case SKILL_ADVANCED:
			return DAMAGE_REDUCTION_ADVANCED;

		default:
			return DAMAGE_REDUCTION_BEGINNER;
	}
}


void ApplyBandage()
{
	// TODO: Holster weapon temporarily

	if (!nMod.PlayerIsBleeding)
		return;

	if (nMod.PlayerIsBandaging == false)
	{
		nMod.PlayerIsBandaging = true;
		nMod.bleed_timer = gpGlobals->time + GetBandageTime();
		Print("APPLYING BANDAGE!\n", print_center);
		return;
	}
}


void CheckBleed(entvars_t* pev)
{
	if (pev->health <= 0)
		return;

	if (!nMod.PlayerIsBleeding)
		return;

	if (nMod.PlayerIsBandaging == false)
	{
		Print("PRESS B TO BANDAGE!\n", print_center);
		pev->health--;
		return;
	}

	if (nMod.bleed_timer <= gpGlobals->time)
	{
		Print("HEALED!\n", print_center);
		nMod.PlayerIsBleeding = false;
		nMod.PlayerIsBandaging = false;
		return;
	}
}


float GetBandageTime()
{
	int skill = GetSkillLevel(SKILL_TORSO);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return BANDAGE_TIME_BEGINNER;

		case SKILL_INTERMEDIATE:
			return BANDAGE_TIME_INTERMEDIATE;

		case SKILL_ADVANCED:
			return BANDAGE_TIME_ADVANCED;

		default:
			return BANDAGE_TIME_BEGINNER;
	}
}


float GetMaxCharge()
{
	int skill = GetSkillLevel(SKILL_FIRMWARE);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return HEV_MAX_CHARGE_BEGINNER;

		case SKILL_INTERMEDIATE:
			return HEV_MAX_CHARGE_INTERMEDIATE;

		case SKILL_ADVANCED:
			return HEV_MAX_CHARGE_ADVANCED;

		default:
			return HEV_MAX_CHARGE_BEGINNER;
	}
}


float ThrowStrength(int weapon)
{
	int skill = GetSkillLevel(SKILL_ARMS);

	if (weapon == WEAPON_HANDGRENADE)
	{
		switch (skill)
		{
			case SKILL_BEGINNER:
				return GRENADE_VELOCITY_BEGINNER;

			case SKILL_INTERMEDIATE:
				return GRENADE_VELOCITY_INTERMEDIATE;

			case SKILL_ADVANCED:
				return GRENADE_VELOCITY_ADVANCED;
		}
	}

	if (weapon == WEAPON_SATCHEL)
	{
		switch (skill)
		{
			case SKILL_BEGINNER:
				return SATCHEL_VELOCITY_BEGINNER;

			case SKILL_INTERMEDIATE:
				return SATCHEL_VELOCITY_INTERMEDIATE;

			case SKILL_ADVANCED:
				return SATCHEL_VELOCITY_ADVANCED;

			default:
				return SATCHEL_VELOCITY_BEGINNER;
		}
	}
}


float GetHitPunch(entvars_t* pev)
{
	int skill = GetSkillLevel(SKILL_HEAD);
	int punchangle;

	switch (skill)
	{
		case SKILL_BEGINNER:
			punchangle = HITPUNCH_BEGINNER;
			break;
		case SKILL_INTERMEDIATE:
			punchangle = HITPUNCH_INTERMEDIATE;
			break;
		case SKILL_ADVANCED:
			punchangle = HITPUNCH_ADVANCED;
			break;
		default:
			punchangle = HITPUNCH_BEGINNER;
	}

	return punchangle;
}


float GetLongJumpSpeed()
{
	int skill = GetSkillLevel(SKILL_FIRMWARE);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return LONGJUMP_SPEED_BEGINNER; 

		case SKILL_INTERMEDIATE:
			return LONGJUMP_SPEED_INTERMEDIATE; 

		case SKILL_ADVANCED:
			return LONGJUMP_SPEED_ADVANCED; 

		default:
			return LONGJUMP_SPEED_BEGINNER;
	}

}


float GetFallDamage(float falldamage)
{
	int skill = GetSkillLevel(SKILL_LEGS);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return falldamage;	

		case SKILL_INTERMEDIATE:
			return falldamage * FALLDAMAGE_INTERMEDIATE; 

		case SKILL_ADVANCED:
			return falldamage * FALLDAMAGE_ADVANCED; 

		default:
			return falldamage;
	}
}


float GetGaussRefire()
{
	int skill = GetSkillLevel(SKILL_WEAPONS_SPECIAL);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return GAUSS_REFIRE_BEGINNER;

		case SKILL_INTERMEDIATE:
			return GAUSS_REFIRE_INTERMEDIATE;

		case SKILL_ADVANCED:
			return GAUSS_REFIRE_ADVANCED;

		default:
			return GAUSS_REFIRE_BEGINNER;
	}
}


float GetJumpBoost()
{
	int skill = GetSkillLevel(SKILL_TORSO);
	bool beastmode = IsBeastMode();

	switch (skill)
	{
	case SKILL_BEGINNER:
		if (beastmode)
			return BEGINNER_JUMP_BEAST;
		else
			return BEGINNER_JUMP_NORMAL;

	case SKILL_INTERMEDIATE:
		if (beastmode)
			return INTERMEDIATE_JUMP_BEAST;
		else
			return INTERMEDIATE_JUMP_NORMAL;

	case SKILL_ADVANCED:
		if (beastmode)
			return ADVANCED_JUMP_BEAST;
		else
			return ADVANCED_JUMP_NORMAL;

	default:
		return DEFAULT_JUMP;
	}
}


float GetSpeedBoost()
{
#ifdef CLIENT_DLL
	return NORMAL_SPEED;

	// Client is suddenly crashing in here when accessing CVAR_GET_FLOAT
	// but only on c2a4en???
#endif 

#ifndef CLIENT_DLL

	// Find player to check if their health is low
	CBaseEntity* playerent = UTIL_PlayerByIndex(1);
	edict_t* playeredict = playerent->edict();

	// If bleeding, walk slow
	if (nMod.PlayerIsBleeding)
		return 0.5;

	// If low health, walk slightly slower
	if (playeredict->v.health < 25)
		return 0.75;

	int skill = GetSkillLevel(SKILL_LEGS);
	bool beastmode = IsBeastMode();
	
	switch (skill)
	{
		case SKILL_BEGINNER:
			return NORMAL_SPEED;

		case SKILL_INTERMEDIATE:
			if (beastmode)
				return INTERMEDIATE_RUN_SPEED;
			else
				return INTERMEDIATE_WALK_SPEED;

		case SKILL_ADVANCED:
			if (beastmode)
				return ADVANCED_RUN_SPEED;
			else
				return ADVANCED_WALK_SPEED;

		default:
			return NORMAL_SPEED;
	}

#endif
}


int GaspForAir()
{
	int skill = GetSkillLevel(SKILL_HEAD);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return AIRGASP_BEGINNER;

		case SKILL_INTERMEDIATE:
			return AIRGASP_INTERMEDIATE;

		case SKILL_ADVANCED:
			return AIRGASP_ADVANCED;

		default:
			return AIRGASP_BEGINNER;
	}
}


float GetDrownDamage()
{
	int skill = GetSkillLevel(SKILL_HAZARD);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return BEGINNER_DROWN_DMG;

		case SKILL_INTERMEDIATE:
			return INTERMEDIATE_DROWN_DMG;

		case SKILL_ADVANCED:
			return ADVANCED_DROWN_DMG;

		default:
			return BEGINNER_DROWN_DMG;
	}
}


float GetSwimSpeed()
{
	int skill = GetSkillLevel(SKILL_ARMS);

	switch (skill)
	{
		case SKILL_BEGINNER:
			return BEGINNER_SWIM_SPEED;

		case SKILL_INTERMEDIATE:
			return INTERMEDIATE_SWIM_SPEED;

		case SKILL_ADVANCED:
			return ADVANCED_SWIM_SPEED;

		default:
			return BEGINNER_SWIM_SPEED;
	}
}


void AdrenalineRush()
{
	int skill = GetSkillLevel(SKILL_TORSO);

	if (skill == SKILL_BEGINNER)
	{
		Print("ADRENALINE DEPLETED!", print_chat);
		return;
	}

	if (!IsBeastMode())
	{
		nMod.BeastMode = true;
		Print("ADRENALINE ADMINISTERED!", print_chat);
	}
	else
	{
		nMod.BeastMode = false;
		Print("ADRENALINE DEPLETED!", print_chat);
	}
}


float RecoilExceptions(CBasePlayer* player)
{
	// If jumping, maximum recoil
	if ((player->pev->flags & FL_ONGROUND) == 0)
	{
		//Print("JUMPING_RECOIL!", print_center);
		return JUMPING_RECOIL;
	}

	// If crouching, no recoil
	if (IsPlayerCrouching(player))
	{
		//Print("NO_RECOIL!", print_center);
		return NO_RECOIL;
	}

	return NO_RECOIL;
}


float GetShotgunDistance()
{
	int skill = GetSkillLevel(SKILL_WEAPONS_BULLET);

	switch (skill)
	{
		case SKILL_BEGINNER:
		return SHOTGUN_DISTANCE_BEGINNER;

		case SKILL_INTERMEDIATE:
			return SHOTGUN_DISTANCE_INTERMEDIATE;

		case SKILL_ADVANCED:
			return SHOTGUN_DISTANCE_ADVANCED;

		default:
			return SHOTGUN_DISTANCE_BEGINNER;
	}
}


// Currently only used by crowbar
float TweakWeaponDamage(int weapon, float damage)
{
	float newDamage = 0;

	newDamage = (damage *= CROWBAR_SUPERSWING_DMG);

	return newDamage;
}


float ExplosionScale()
{
	int skill = GetSkillLevel(SKILL_HAZARD);

	switch (skill)
	{
	case SKILL_BEGINNER:
		return EXPLOSIONSCALE_BEGINNER;

	case SKILL_INTERMEDIATE:
		return EXPLOSIONSCALE_INTERMEDIATE;

	case SKILL_ADVANCED:
		return EXPLOSIONSCALE_ADVANCED;

	default:
		return EXPLOSIONSCALE_BEGINNER;
	}
}


Vector GetAccuracy(CBasePlayer* player, int weapon)
{
	int bullet_skill = GetSkillLevel(SKILL_WEAPONS_BULLET);
	int specialized_skill = GetSkillLevel(SKILL_WEAPONS_SPECIAL);

	bool moving = IsPlayerMoving(player);
	bool crouching = IsPlayerCrouching(player);

	Vector cone;


	switch (weapon)
	{

	default:
	case WEAPON_MP5:
	case WEAPON_AK47:

		if (moving && !crouching || player->pev->movetype == MOVETYPE_FLY || (player->pev->flags & FL_ONGROUND) == 0) //TODO: how to detect a jump while crouched?
		{
			cone = nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS]
								 [bullet_skill]
								 [ACCURACY_POOR];


			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "POOR ACCURACY!\n");
			break;
		}

		if (!moving && IsPlayerZoomed() && crouching)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS]
								 [bullet_skill]
								 [ACCURACY_ULTRA];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "ULTRA ACCURACY!\n");
			break;
		}

		if (!moving && IsPlayerZoomed() && !crouching)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS]
								 [bullet_skill]
								 [ACCURACY_EXCELLENT];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "EXCELLENT ACCURACY!\n");
			break;
		}

		if (moving && IsPlayerZoomed())
		{
			cone = nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS]
								 [bullet_skill]
								 [ACCURACY_NORMAL];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "NORMAL ACCURACY!\n");
			break;
		}

		if (!moving && crouching)
		{
			cone = cone = nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS]
									    [bullet_skill]
									    [ACCURACY_BETTER];
			;
			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "BETTER ACCURACY!\n");
			break;
		}

		if ((moving && crouching) || (!moving && !crouching))
		{
			cone = nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS]
								 [bullet_skill]
								 [ACCURACY_NORMAL];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "NORMAL ACCURACY!\n");
			break;
		}

		break;


	case WEAPON_SHOTGUN:

		if (moving && !crouching || player->pev->movetype == MOVETYPE_FLY)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_SHOTGUN]
								 [bullet_skill]
								 [ACCURACY_POOR];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "POOR ACCURACY\n");
			break;
		}

		if ((moving && crouching) || (!moving && !crouching))
		{
			cone = nMod.pAccuracy[BULLETWEAPON_SHOTGUN]
								 [bullet_skill]
								 [ACCURACY_NORMAL];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "NORMAL ACCURACY\n");
			break;
		}

		if (crouching && !moving)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_SHOTGUN]
								 [bullet_skill]
								 [ACCURACY_BETTER];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "BETTER ACCURACY\n");
			break;
		}

		break;


	case WEAPON_GLOCK:

		if (moving && !crouching || player->pev->movetype == MOVETYPE_FLY)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL]
								 [bullet_skill]
								 [ACCURACY_POOR];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "POOR ACCURACY\n");
			break;
		}

		if ((moving && crouching) || (!moving && !crouching))
		{
			cone = nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL]
								 [bullet_skill]
								 [ACCURACY_NORMAL];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "NORMAL ACCURACY\n");
			break;
		}

		if (!moving && crouching)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL]
								 [bullet_skill]
								 [ACCURACY_BETTER];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "BETTER ACCURACY\n");
			break;
		}

		break;


	case WEAPON_DESERT_EAGLE:
	case WEAPON_PYTHON:

		if (moving && !crouching || player->pev->movetype == MOVETYPE_FLY)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL]
								 [bullet_skill]
								 [ACCURACY_POOR];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "POOR ACCURACY\n");
			break;
		}

		if (!moving && IsPlayerZoomed() && crouching)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL]
								 [bullet_skill]
								 [ACCURACY_ULTRA];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "ULTRA ACCURACY\n");
			break;
		}

		if (!moving && IsPlayerZoomed() && !crouching)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL]
								 [bullet_skill]
								 [ACCURACY_EXCELLENT];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "EXCELLENT ACCURACY\n");
			break;
		}

		if (moving && IsPlayerZoomed())
		{
			cone = nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL]
								 [bullet_skill]
								 [ACCURACY_NORMAL];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "NORMAL ACCURACY\n");
			break;
		}

		if (!moving && crouching)
		{
			cone = nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL]
								 [bullet_skill]
								 [ACCURACY_BETTER];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "BETTER ACCURACY\n");
			break;
		}

		if ((moving && crouching) || (!moving && !crouching))
		{
			cone = nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL]
								 [bullet_skill]
								 [ACCURACY_NORMAL];

			if (CVAR_GET_FLOAT("showaccuracy"))
				ClientPrint(player->pev, HUD_PRINTTALK, "NORMAL ACCURACY\n");
			break;
		}

		break;
	}

	float penalty = GetAccuracyPenalty();

	return cone * penalty;
}


float GetAccuracyPenalty()
{
	static float FiringDelay;

	// Start the timer if not initialized
	if (FiringDelay == 0)
	{
		FiringDelay = gpGlobals->time + ACCURACY_FIRING_DELAY;
		return ACCURACY_PENALTY_NONE;
	}

	// Timer expired so restart it
	if (FiringDelay < gpGlobals->time)
	{
		FiringDelay = gpGlobals->time + ACCURACY_FIRING_DELAY;
		return ACCURACY_PENALTY_NONE;
	}

	// Time delay interval expired, return normal accuracy
	if (FiringDelay <= gpGlobals->time)
		return ACCURACY_PENALTY_NONE;


	// Apply an accuracy penalty if firing rapidly
	int bullet_skill = GetSkillLevel(SKILL_WEAPONS_BULLET);

	switch (bullet_skill)
	{
		case SKILL_BEGINNER:
			return ACCURACY_PENALTY_BEGINNER;

		case SKILL_INTERMEDIATE:
			return ACCURACY_PENALTY_INTERMEDIATE;

		case SKILL_ADVANCED:
			return ACCURACY_PENALTY_NONE;

		default:
			return ACCURACY_PENALTY_NONE;
	}
}


//
// Helper functions
//

void Print(const char* msg, PRINT_TYPE where)
{
	#ifndef CLIENT_DLL

	CBaseEntity* playerent = UTIL_PlayerByIndex(1);
	edict_t* player = FIND_ENTITY_BY_CLASSNAME(NULL, "player");

	switch (where)
	{
	case print_center:
		ClientPrint(playerent->pev, HUD_PRINTCENTER, msg);
		break;

	case print_chat:
		ClientPrint(playerent->pev, HUD_PRINTTALK, msg);
		break;

	default:
	case print_console:
		ClientPrint(playerent->pev, HUD_PRINTCONSOLE, msg);
		break;

	}

	#endif

	//g_engfuncs.pfnClientPrintf(player, where, msg);
}


bool EntityIsPlayer(entvars_t* pev)
{
// Hack: how does client detect player?
#ifdef CLIENT_DLL
	return false;
#endif

#ifndef CLIENT_DLL
	CBaseEntity* player = UTIL_PlayerByIndex(1);
	bool is_player;

	is_player = FStrEq("player", STRING(pev->classname));

	return is_player;
#endif
}


bool KillSequence()
{
	if (CVAR_GET_FLOAT("killsequence"))
		return true;
	else
		return false;
}

bool IsFadeoutEnabled()
{
	if (CVAR_GET_FLOAT("mmfadeout"))
		return true;
	else
		return false;
}

bool IsPlayerZoomed()
{
	if (nMod.PlayerZoomed)
		return true;
	else
		return false;
}

bool IsBeastMode()
{
	//if ((CVAR_GET_FLOAT("skill_agility") == SKILL_BEGINNER) && IsPlayerMoving())
	//	Print("BEAST MODE REQUIRES INTERMEDIATE!", print_center);

	if (nMod.BeastMode)
		return true;
	else
		return false;
}

bool IsPlayerCrouching(CBasePlayer* player) // No distinction between crouching while still vs. moving
{

	// TODO: not all crouch anims are here

	if (player->pev->sequence == 7 ||  // crouch_aim_idle
		player->pev->sequence == 35 || // crouch_aim_onehanded (for shooting glock)
		player->pev->sequence == 36 || // crouch_shoot_onehanded
		player->pev->sequence == 40 || // crouch_shoot_python
		player->pev->sequence == 43 || // crouch_aim_shotgun
		player->pev->sequence == 44 || // crouch_shoot_shotgun
		player->pev->sequence == 51 || // crouch_aim_shotgun
		player->pev->sequence == 52)   // crouch_shoot_mp5
	{
		//Print("Crouching!", print_center);
		return true;
	}
	else
	{
		//Print("Not Crouching!", print_center);
		return false;
	}
}

bool IsPlayerMoving(CBasePlayer* player)
{
	if (player->pev->velocity.x == 0 && player->pev->velocity.y == 0 && player->pev->velocity.z == 0)  // Do we need to check z?
	{
		//ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "STILL!");
		return false;
	}
	else
	{
		//ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "MOVING!");
		return true;
	}
}

void Startup_Message()
{
	//Print(STARTUP_MESSAGE, print_console);
}


//
// Horde mode
//

void InitHordeMode()
{
	int hordemode;
	hordemode = CVAR_GET_FLOAT("hordemode");

	if (!hordemode)
		return;

	UTIL_PrecacheOther("monster_human_grunt");
	UTIL_PrecacheOther("monster_human_assassin");
	UTIL_PrecacheOther("monster_alien_slave");
	UTIL_PrecacheOther("monster_bullchicken");
	UTIL_PrecacheOther("monster_alien_controller");
}


void SpawnRandomMonster()
{
	int hordemode;
	hordemode = CVAR_GET_FLOAT("hordemode");

	if (!hordemode)
		return;

#ifndef CLIENT_DLL
	//CBaseEntity* spawnpoint = UTIL_FindEntityByClassname(NULL, "info_player_deathmatch");
	//CBaseEntity* pEnt = UTIL_FindEntityInMap("info_player_start", m_cameraOrigin, m_cameraAngles))

	edict_t* pent;
	edict_t* spawnpoint = NanoFindSpawnPoint();
	entvars_t* pevCreate;

	if (spawnpoint == NULL)
		return /*true*/;

	const char* hgrunt = "monster_human_grunt";
	const char* agrunt = "monster_bullchicken";
	const char* slave = "monster_alien_slave";
	const char* bull = "monster_alien_controller";
	const char* ass = "monster_human_assassin";

	switch (RANDOM_LONG(0, 4))
	{
	case 0:
		pent = CREATE_NAMED_ENTITY(MAKE_STRING(hgrunt));
		break;
	case 1:
		pent = CREATE_NAMED_ENTITY(MAKE_STRING(agrunt));
		break;
	case 2:
		pent = CREATE_NAMED_ENTITY(MAKE_STRING(slave));
		break;
	case 3:
		pent = CREATE_NAMED_ENTITY(MAKE_STRING(bull));
		break;
	case 4:
		pent = CREATE_NAMED_ENTITY(MAKE_STRING(ass));
		break;
		//case 5:
		//	break;
	}


	//pent = CREATE_NAMED_ENTITY(MAKE_STRING("monstermaker"));

	pevCreate = VARS(pent);

	if (pevCreate == NULL)
		return /* 0*/;

	pevCreate->origin = spawnpoint->v.origin;
	pevCreate->angles = spawnpoint->v.angles;
	SetBits(pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND);

	DispatchSpawn(pent);

	//Print("SPAWNING!", print_center);
#endif
}


edict_t* NanoFindSpawnPoint(/*CBaseEntity* pPlayer*/)
{
#ifndef CLIENT_DLL
	CBaseEntity* pSpot;
	//edict_t* player;

	int nNumRandomSpawnsToTry = 5; // was 10

	//player = pPlayer->edict();

	// choose a info_player_deathmatch point
	/*if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_coop");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else if (g_pGameRules->IsDeathmatch())
	{*/
	if (NULL == g_pLastSpawn)
	{
		int nNumSpawnPoints = 0;
		CBaseEntity* pEnt = UTIL_FindEntityByClassname(NULL, "info_player_deathmatch");
		while (NULL != pEnt)
		{
			nNumSpawnPoints++;
			pEnt = UTIL_FindEntityByClassname(pEnt, "info_player_deathmatch");
		}
		nNumRandomSpawnsToTry = nNumSpawnPoints;
	}

	pSpot = g_pLastSpawn;
	// Randomize the start spot
	for (int i = RANDOM_LONG(1, nNumRandomSpawnsToTry - 1); i > 0; i--)
		pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
	if (FNullEnt(pSpot)) // skip over the null point
		pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");

	CBaseEntity* pFirstSpot = pSpot;

	do
	{
		if (pSpot)
		{
			CBaseEntity* ent = NULL;

			// check if pSpot is valid
			//while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
			{
				//Print("IN HERE", print_center);
				if (pSpot->pev->origin == Vector(0, 0, 0))
				{
					pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
					continue;
				}

				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
	} while (pSpot != pFirstSpot); // loop if we're not back to the start

	//if (IsVisible(pSpot))
	//	return 0;

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if (!FNullEnt(pSpot))
	{
		//return 0;  // nMod experiment


		CBaseEntity* ent = NULL;
		while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
		{
			Print("HI", print_center);
			// if ent is a client, kill em (unless they are ourselves)
			//if (ent->IsPlayer() && !(ent->edict() == player))
			ent->TakeDamage(CWorld::World->pev, CWorld::World->pev, 300, DMG_GENERIC);
		}
		goto ReturnSpot;
	}
	//}

	// If startspot is set, (re)spawn there.
	if (FStringNull(gpGlobals->startspot) || 0 == strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(NULL, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname(NULL, STRING(gpGlobals->startspot));
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}

ReturnSpot:
	if (FNullEnt(pSpot))
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return CWorld::World->edict();
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
#endif

#ifdef CLIENT_DLL
	return 0;
#endif
}


//
// Stats data
//


// Extern so we can access it in the client code
extern void RPGStats_Setup()
{
	Init_AccuracyStats();
	Init_RecoilStats();
	//Init_MaxCarryStats();
}

void Init_AccuracyStats()
{
	// Machinegun accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_POOR] = ACCURACY_MACHINEGUN_BEGINNER_POOR;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_NORMAL] = ACCURACY_MACHINEGUN_BEGINNER_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_BETTER] = ACCURACY_MACHINEGUN_BEGINNER_BETTER;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_EXCELLENT] = ACCURACY_MACHINEGUN_BEGINNER_EXCELLENT;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_ULTRA] = ACCURACY_MACHINEGUN_BEGINNER_ULTRA;

	// Machinegun accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_POOR] = ACCURACY_MACHINEGUN_INTERMEDIATE_POOR;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = ACCURACY_MACHINEGUN_INTERMEDIATE_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_BETTER] = ACCURACY_MACHINEGUN_INTERMEDIATE_BETTER;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = ACCURACY_MACHINEGUN_INTERMEDIATE_EXCELLENT;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = ACCURACY_MACHINEGUN_INTERMEDIATE_ULTRA;

	// Machinegun accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_POOR] = ACCURACY_MACHINEGUN_ADVANCED_POOR;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_NORMAL] = ACCURACY_MACHINEGUN_ADVANCED_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_BETTER] = ACCURACY_MACHINEGUN_ADVANCED_BETTER;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_EXCELLENT] = ACCURACY_MACHINEGUN_ADVANCED_EXCELLENT;
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_ULTRA] = ACCURACY_MACHINEGUN_ADVANCED_ULTRA;


	// Shotgun accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_POOR] = ACCURACY_SHOTGUN_BEGINNER_POOR;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_NORMAL] = ACCURACY_SHOTGUN_BEGINNER_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_BETTER] = ACCURACY_SHOTGUN_BEGINNER_BETTER;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_EXCELLENT] = NOT_USED;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_ULTRA] = NOT_USED;

	// Shotgun accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_POOR] = ACCURACY_SHOTGUN_INTERMEDIATE_POOR;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = ACCURACY_SHOTGUN_INTERMEDIATE_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_BETTER] = ACCURACY_SHOTGUN_INTERMEDIATE_BETTER;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = NOT_USED;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = NOT_USED;

	// Shotgun accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_POOR] = ACCURACY_SHOTGUN_ADVANCED_POOR;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_NORMAL] = ACCURACY_SHOTGUN_ADVANCED_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_BETTER] = ACCURACY_SHOTGUN_ADVANCED_BETTER;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_EXCELLENT] = NOT_USED;
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_ULTRA] = NOT_USED;


	// Light pistol accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_POOR] = ACCURACY_LIGHTPISTOL_BEGINNER_POOR;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL] = ACCURACY_LIGHTPISTOL_BEGINNER_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_BETTER] = ACCURACY_LIGHTPISTOL_BEGINNER_BETTER;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT] = NOT_USED;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA] = NOT_USED;

	// Light pistol accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR] = ACCURACY_LIGHTPISTOL_INTERMEDIATE_POOR;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = ACCURACY_LIGHTPISTOL_INTERMEDIATE_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER] = ACCURACY_LIGHTPISTOL_INTERMEDIATE_BETTER;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = NOT_USED;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = NOT_USED;

	// Light pistol accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_POOR] = ACCURACY_LIGHTPISTOL_ADVANCED_POOR;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL] = ACCURACY_LIGHTPISTOL_ADVANCED_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_BETTER] = ACCURACY_LIGHTPISTOL_ADVANCED_BETTER;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT] = NOT_USED;
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA] = NOT_USED;


	// Heavy pistol accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_POOR] = ACCURACY_HEAVYPISTOL_BEGINNER_POOR;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL] = ACCURACY_HEAVYPISTOL_BEGINNER_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_BETTER] = ACCURACY_HEAVYPISTOL_BEGINNER_BETTER;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT] = ACCURACY_HEAVYPISTOL_BEGINNER_EXCELLENT;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA] = ACCURACY_HEAVYPISTOL_BEGINNER_ULTRA;

	// Heavy pistol accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR] = ACCURACY_HEAVYPISTOL_INTERMEDIATE_POOR;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = ACCURACY_HEAVYPISTOL_INTERMEDIATE_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER] = ACCURACY_HEAVYPISTOL_INTERMEDIATE_BETTER;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = ACCURACY_HEAVYPISTOL_INTERMEDIATE_EXCELLENT;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = ACCURACY_HEAVYPISTOL_INTERMEDIATE_ULTRA;

	// Heavy pistol accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_POOR] = ACCURACY_HEAVYPISTOL_ADVANCED_POOR;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL] = ACCURACY_HEAVYPISTOL_ADVANCED_NORMAL;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_BETTER] = ACCURACY_HEAVYPISTOL_ADVANCED_BETTER;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT] = ACCURACY_HEAVYPISTOL_ADVANCED_EXCELLENT;
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA] = ACCURACY_HEAVYPISTOL_ADVANCED_ULTRA;
}


void Init_RecoilStats()
{
	nMod.pRecoil[WEAPON_AK47][SKILL_BEGINNER] = AK47_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_AK47][SKILL_INTERMEDIATE] = AK47_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_AK47][SKILL_ADVANCED] = AK47_RECOIL_ADVANCED;

	nMod.pRecoil[WEAPON_MP5][SKILL_BEGINNER] = GENERIC_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_MP5][SKILL_INTERMEDIATE] = GENERIC_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_MP5][SKILL_ADVANCED] = GENERIC_RECOIL_ADVANCED;

	nMod.pRecoil[WEAPON_DESERT_EAGLE][SKILL_BEGINNER] = GENERIC_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_DESERT_EAGLE][SKILL_INTERMEDIATE] = GENERIC_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_DESERT_EAGLE][SKILL_ADVANCED] = GENERIC_RECOIL_INTERMEDIATE;

	nMod.pRecoil[WEAPON_GLOCK][SKILL_BEGINNER] = GENERIC_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_GLOCK][SKILL_INTERMEDIATE] = GENERIC_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_GLOCK][SKILL_ADVANCED] = GENERIC_RECOIL_INTERMEDIATE;

	nMod.pRecoil[WEAPON_PYTHON][SKILL_BEGINNER] = GENERIC_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_PYTHON][SKILL_INTERMEDIATE] = GENERIC_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_PYTHON][SKILL_ADVANCED] = GENERIC_RECOIL_INTERMEDIATE;

	nMod.pRecoil[WEAPON_SHOTGUN][SKILL_BEGINNER] = GENERIC_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_SHOTGUN][SKILL_INTERMEDIATE] = GENERIC_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_SHOTGUN][SKILL_ADVANCED] = GENERIC_RECOIL_INTERMEDIATE;

	nMod.pRecoil[WEAPON_EGON][SKILL_BEGINNER] = EGON_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_EGON][SKILL_INTERMEDIATE] = EGON_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_EGON][SKILL_ADVANCED] = EGON_RECOIL_INTERMEDIATE;

	nMod.pRecoil[WEAPON_GAUSS][SKILL_BEGINNER] = GAUSS_RECOIL_BEGINNER;
	nMod.pRecoil[WEAPON_GAUSS][SKILL_INTERMEDIATE] = GAUSS_RECOIL_INTERMEDIATE;
	nMod.pRecoil[WEAPON_GAUSS][SKILL_ADVANCED] = GAUSS_RECOIL_INTERMEDIATE;
}

#if 0
void Init_MaxCarryStats()
{
	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_GLOCK] = _9MM_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_GLOCK] = _9MM_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_GLOCK] = _9MM_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_PYTHON] = _357_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_PYTHON] = _357_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_PYTHON] = _357_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_MP5] = _9MM_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_MP5] = _9MM_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_MP5] = _9MM_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_SHOTGUN] = BUCKSHOT_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_SHOTGUN] = BUCKSHOT_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_SHOTGUN] = BUCKSHOT_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_RPG] = ROCKET_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_RPG] = ROCKET_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_RPG] = ROCKET_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_RPG] = ROCKET_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_RPG] = ROCKET_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_RPG] = ROCKET_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_CROSSBOW] = BOLT_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_CROSSBOW] = BOLT_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_CROSSBOW] = BOLT_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_GAUSS] = URANIUM_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_GAUSS] = URANIUM_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_GAUSS] = URANIUM_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_EGON] = URANIUM_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_EGON] = URANIUM_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_EGON] = URANIUM_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_HANDGRENADE] = HANDGRENADE_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_HANDGRENADE] = HANDGRENADE_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_HANDGRENADE] = HANDGRENADE_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_SATCHEL] = SATCHEL_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_SATCHEL] = SATCHEL_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_SATCHEL] = SATCHEL_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_TRIPMINE] = TRIPMINE_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_TRIPMINE] = TRIPMINE_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_TRIPMINE] = TRIPMINE_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_SNARK] = SNARK_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_SNARK] = SNARK_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_SNARK] = SNARK_MAX_CARRY_MAXED;

	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_HORNETGUN] = HORNET_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_HORNETGUN] = HORNET_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_HORNETGUN] = HORNET_MAX_CARRY_MAXED;

	/*
	// eagle
	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_DESERT_EAGLE] = _357_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_DESERT_EAGLE] = _357_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_DESERT_EAGLE] = _357_MAX_CARRY_MAXED;
	*/

	/*
	// tmp
	nMod.MaxCarry[SKILL_BEGINNER][WEAPON_TMP] = _357_MAX_CARRY;
	nMod.MaxCarry[SKILL_INTERMEDIATE][WEAPON_TMP] = _357_MAX_CARRY_INCREASED;
	nMod.MaxCarry[SKILL_ADVANCED][WEAPON_TMP] = _357_MAX_CARRY_MAXED;
	*/
}
#endif

void Init_Menus()
{
	// First column
	default_WeaponMenu.Column[WEAPON_CROWBAR] = COLUMN_1;
	default_WeaponMenu.Row[WEAPON_CROWBAR] = ROW_1;

	default_WeaponMenu.Column[WEAPON_PIPEWRENCH] = COLUMN_1;
	default_WeaponMenu.Row[WEAPON_PIPEWRENCH] = ROW_2;

	// Second column
	default_WeaponMenu.Column[WEAPON_GLOCK] = COLUMN_2;
	default_WeaponMenu.Row[WEAPON_GLOCK] = ROW_1;

	default_WeaponMenu.Column[WEAPON_PYTHON] = COLUMN_2;
	default_WeaponMenu.Row[WEAPON_PYTHON] = ROW_2;

	default_WeaponMenu.Column[WEAPON_DESERT_EAGLE] = COLUMN_2;
	default_WeaponMenu.Row[WEAPON_DESERT_EAGLE] = ROW_3;

	default_WeaponMenu.Column[WEAPON_TMP] = COLUMN_2;
	default_WeaponMenu.Row[WEAPON_TMP] = ROW_4;


	// Third column
	default_WeaponMenu.Column[WEAPON_MP5] = COLUMN_3;
	default_WeaponMenu.Row[WEAPON_MP5] = ROW_1;

	default_WeaponMenu.Column[WEAPON_SHOTGUN] = COLUMN_3;
	default_WeaponMenu.Row[WEAPON_SHOTGUN] = ROW_2;

	default_WeaponMenu.Column[WEAPON_CROSSBOW] = COLUMN_3;
	default_WeaponMenu.Row[WEAPON_CROSSBOW] = ROW_3;

	default_WeaponMenu.Column[WEAPON_AK47] = COLUMN_3;
	default_WeaponMenu.Row[WEAPON_AK47] = ROW_4;


	// Fourth column
	default_WeaponMenu.Column[WEAPON_RPG] = COLUMN_4;
	default_WeaponMenu.Row[WEAPON_RPG] = ROW_1;

	default_WeaponMenu.Column[WEAPON_GAUSS] = COLUMN_4;
	default_WeaponMenu.Row[WEAPON_GAUSS] = ROW_2;

	default_WeaponMenu.Column[WEAPON_EGON] = COLUMN_4;
	default_WeaponMenu.Row[WEAPON_EGON] = ROW_3;

	default_WeaponMenu.Column[WEAPON_HORNETGUN] = COLUMN_4;
	default_WeaponMenu.Row[WEAPON_HORNETGUN] = ROW_4;


	// Fifth column
	default_WeaponMenu.Column[WEAPON_HANDGRENADE] = COLUMN_5;
	default_WeaponMenu.Row[WEAPON_HANDGRENADE] = ROW_1;

	default_WeaponMenu.Column[WEAPON_SATCHEL] = COLUMN_5;
	default_WeaponMenu.Row[WEAPON_SATCHEL] = ROW_2;

	default_WeaponMenu.Column[WEAPON_TRIPMINE] = COLUMN_5;
	default_WeaponMenu.Row[WEAPON_TRIPMINE] = ROW_3;

	default_WeaponMenu.Column[WEAPON_SNARK] = COLUMN_5;
	default_WeaponMenu.Row[WEAPON_SNARK] = ROW_4;


	// New redesigned weapon menu layout:

	// Melee weapons
	alt_WeaponMenu.Column[WEAPON_CROWBAR] = COLUMN_1;
	alt_WeaponMenu.Row[WEAPON_CROWBAR] = ROW_1;

	alt_WeaponMenu.Column[WEAPON_PIPEWRENCH] = COLUMN_1;
	alt_WeaponMenu.Row[WEAPON_PIPEWRENCH] = ROW_2;


	// Pistols
	alt_WeaponMenu.Column[WEAPON_GLOCK] = COLUMN_2;
	alt_WeaponMenu.Row[WEAPON_GLOCK] = ROW_1;

	alt_WeaponMenu.Column[WEAPON_PYTHON] = COLUMN_2;
	alt_WeaponMenu.Row[WEAPON_PYTHON] = ROW_2;

	alt_WeaponMenu.Column[WEAPON_DESERT_EAGLE] = COLUMN_2;
	alt_WeaponMenu.Row[WEAPON_DESERT_EAGLE] = ROW_3;

	alt_WeaponMenu.Column[WEAPON_TMP] = COLUMN_2;
	alt_WeaponMenu.Row[WEAPON_TMP] = ROW_4;


	// Big guns
	alt_WeaponMenu.Column[WEAPON_MP5] = COLUMN_3;
	alt_WeaponMenu.Row[WEAPON_MP5] = ROW_1;

	alt_WeaponMenu.Column[WEAPON_SHOTGUN] = COLUMN_3;
	alt_WeaponMenu.Row[WEAPON_SHOTGUN] = ROW_2;

	alt_WeaponMenu.Column[WEAPON_CROSSBOW] = COLUMN_3;
	alt_WeaponMenu.Row[WEAPON_CROSSBOW] = ROW_3;

	alt_WeaponMenu.Column[WEAPON_AK47] = COLUMN_3;
	alt_WeaponMenu.Row[WEAPON_AK47] = ROW_4;


	// Explosive weapons
	alt_WeaponMenu.Column[WEAPON_RPG] = COLUMN_4;
	alt_WeaponMenu.Row[WEAPON_RPG] = ROW_1;

	alt_WeaponMenu.Column[WEAPON_HANDGRENADE] = COLUMN_4;
	alt_WeaponMenu.Row[WEAPON_HANDGRENADE] = ROW_2;

	alt_WeaponMenu.Column[WEAPON_SATCHEL] = COLUMN_4;
	alt_WeaponMenu.Row[WEAPON_SATCHEL] = ROW_3;

	alt_WeaponMenu.Column[WEAPON_TRIPMINE] = COLUMN_4;
	alt_WeaponMenu.Row[WEAPON_TRIPMINE] = ROW_4;


	// Energy weapons

	alt_WeaponMenu.Column[WEAPON_EGON] = COLUMN_5;
	alt_WeaponMenu.Row[WEAPON_EGON] = ROW_1;

	alt_WeaponMenu.Column[WEAPON_GAUSS] = COLUMN_5;
	alt_WeaponMenu.Row[WEAPON_GAUSS] = ROW_2;

	// Alien weapons (TODO: its own column)
	alt_WeaponMenu.Column[WEAPON_HORNETGUN] = COLUMN_5;
	alt_WeaponMenu.Row[WEAPON_HORNETGUN] = ROW_3;

	alt_WeaponMenu.Column[WEAPON_SNARK] = COLUMN_5;
	alt_WeaponMenu.Row[WEAPON_SNARK] = ROW_4;
}


//
// Gman stuff
//


bool IsGManDead()
{
	if (nMod.gManDead)
		return true;
	else
		return false;
}


entvars_t* SpawnGManDeathMDL(entvars_t* pev) // TODO: create our own SpawnEntity()
{
#ifndef CLIENT_DLL

	edict_t* pent;
	entvars_t* pevCreate;

	pent = CREATE_NAMED_ENTITY(MAKE_STRING("monster_generic"));

	pevCreate = VARS(pent);
	pevCreate->origin = pev->origin;
	pevCreate->angles = pev->angles;
	SetBits(pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND);

	SET_MODEL(ENT(pevCreate), "models/nanomod/gmandm.mdl");

	//ClientPrint(pev, HUD_PRINTCENTER, "Is there a problem, Mr. Freeman?");

	//void* pmodel = GET_MODEL_PTR(ENT(pevCreate));
	//pevCreate->sequence = LookupSequence(pmodel, "die_backwards");

	DispatchSpawn(ENT(pevCreate));

	return pevCreate;

#endif

#ifdef CLIENT_DLL
	return 0;
#endif
}


void CheckGManDeath(entvars_t* pev /*, edict_t* pentPlayer*/)
{
	if (FClassnameIs(pev, "monster_gman") && pev->health <= 0 && !pev->deadflag) // TODO: check for negative health to set gibthreshold
	{
		pev->effects = EF_NODRAW; // seems necessary here

		//g_pevLastInflictor = pevInflictor;

		entvars_t* dummy = SpawnGManDeathMDL(pev);
		dummy->globalname = MAKE_STRING("GMAN");

		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NONE, 0, 100);

		/*if (!FClassnameIs(pentPlayer, "player"))
			return;*/

		//Killed(pevAttacker, GIB_ALWAYS); // GMan just EXPLODES
		//Killed(pevAttacker, GIB_NORMAL);

		//g_pevLastInflictor = NULL;
	}
}


int KillGMan(entvars_t* pev)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NONE, 0, 100);

	return NANO_SECRET_XPBONUS;
}




//
// Old stuff
//


extern float GetWeaponRecoil(int weapon)
{
	// Disabled for now since it seems that punchangle does not affect accuracy
	return NO_RECOIL;

#if 0
	int bullet_skill = GetSkillLevel(SKILL_WEAPONS_BULLET);
	int special_skill = GetSkillLevel(SKILL_WEAPONS_SPECIAL);

	switch (weapon)
	{
	case WEAPON_AK47:
		return nMod.pRecoil[WEAPON_AK47][bullet_skill];

	case WEAPON_MP5:
		return nMod.pRecoil[WEAPON_MP5][bullet_skill];

	case WEAPON_SHOTGUN:
		return nMod.pRecoil[WEAPON_SHOTGUN][bullet_skill];

	case WEAPON_GLOCK:
		return nMod.pRecoil[WEAPON_GLOCK][bullet_skill];

	case WEAPON_PYTHON:
		return nMod.pRecoil[WEAPON_PYTHON][bullet_skill];

	case WEAPON_DESERT_EAGLE:
		return nMod.pRecoil[WEAPON_DESERT_EAGLE][bullet_skill];

	case WEAPON_GAUSS:
		return nMod.pRecoil[WEAPON_GAUSS][special_skill];

	case WEAPON_EGON:
		return nMod.pRecoil[WEAPON_GAUSS][special_skill];

	case WEAPON_RPG:
		return nMod.pRecoil[WEAPON_GAUSS][special_skill];

	default:
		return NO_RECOIL;
	}
#endif
}


#if 0
void UpgradeEnergySkill()
{
	int skill = CVAR_GET_FLOAT("skill_energy");

	switch (skill)
	{
	case SKILL_BEGINNER:

		if (nMod.pSkillPoints < SKILLCOST_INTERMEDIATE) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_WEAPONS_ENERGY] += 1;
		CVAR_SET_FLOAT("skill_energy", skill += 1);

		Print("Energy Weapons: Upgraded to Intermediate!", print_center);
		Print("Energy Weapons: Upgraded to Intermediate!", print_console);
		break;

	case SKILL_INTERMEDIATE:

		if (nMod.pSkillPoints < SKILLCOST_ADVANCED) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_WEAPONS_ENERGY] += 1;
		CVAR_SET_FLOAT("skill_energy", skill += 1);

		Print("Energy Weapons: Upgraded to Advanced!", print_center);
		Print("Energy Weapons: Upgraded to Advanced!", print_console);
		break;

	case SKILL_ADVANCED:
		Print("Skill already maxed out!\n", print_center);
		Print("Skill already maxed out!\n", print_console);
		break;

	default:
		Print("No skills upgraded!\n", print_center);
		Print("No skills upgraded!\n", print_console);
		break;
	}
}
#endif

#if 0
void UpgradeSwimmingSkill()
{
	int skill = CVAR_GET_FLOAT("skill_swimming");

	switch (skill)
	{
	case SKILL_BEGINNER:

		if (nMod.pSkillPoints < SKILLCOST_INTERMEDIATE) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_SWIMMING] += 1;
		CVAR_SET_FLOAT("skill_swimming", skill += 1);

		Print("Swimming Skill: Upgraded to Intermediate!", print_console);
		Print("Swimming Skill: Upgraded to Intermediate!", print_center);
		break;

	case SKILL_INTERMEDIATE:
		if (nMod.pSkillPoints < SKILLCOST_ADVANCED) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_SWIMMING] += 1;
		CVAR_SET_FLOAT("skill_swimming", skill += 1);

		Print("Swimming Skill: Upgraded to Advanced!", print_console);
		Print("Swimming Skill: Upgraded to Advanced!", print_center);
		break;

	case SKILL_ADVANCED:
		Print("Skill already maxed out!\n", print_center);
		Print("Skill already maxed out!\n", print_console);
		break;

	default:
		Print("No skills upgraded!\n", print_center);
		Print("No skills upgraded!\n", print_console);
		break;
	}
}
#endif

#if 0
void UpgradeSpeedSkill()
{
	int skill = CVAR_GET_FLOAT("skill_agility");

	switch (skill)
	{
	case SKILL_BEGINNER:

		if (nMod.pSkillPoints < SKILLCOST_INTERMEDIATE) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_AGILITY] += 1;
		CVAR_SET_FLOAT("skill_agility", skill += 1);

		Print("Agility Skill: Upgraded to Intermediate!", print_console);
		Print("Agility Skill: Upgraded to Intermediate!", print_center);
		break;

	case SKILL_INTERMEDIATE:
		if (nMod.pSkillPoints < SKILLCOST_ADVANCED) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_AGILITY] += 1;
		CVAR_SET_FLOAT("skill_agility", skill += 1);

		Print("Agility Skill: Upgraded to Advanced!", print_console);
		Print("Agility Skill: Upgraded to Advanced!", print_center);
		break;

	case SKILL_ADVANCED:
		Print("Skill already maxed out!\n", print_center);
		Print("Skill already maxed out!\n", print_console);
		break;

	default:
		Print("No skills upgraded!\n", print_center);
		Print("No skills upgraded!\n", print_console);
		break;
	}
}
#endif

#if 0
void UpgradeBulletSkill()
{
	int skill = CVAR_GET_FLOAT("skill_bullet");

	//CBaseEntity* player = UTIL_PlayerByIndex(1);

	//player->pev->imax

	switch (skill)
	{
	case SKILL_BEGINNER:

		if (nMod.pSkillPoints < SKILLCOST_INTERMEDIATE) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_WEAPONS_BULLET] += 1;
		CVAR_SET_FLOAT("skill_bullet", skill += 1);

		Print("Bullet Weapons: Upgraded to Intermediate!", print_center);
		Print("Bullet Weapons: Upgraded to Intermediate!", print_console);



		break;

	case SKILL_INTERMEDIATE:

		if (nMod.pSkillPoints < SKILLCOST_ADVANCED) // TODO: OR skill not equal to current skill +1
		{
			Print("Not enough skill points!\n", print_center);
			Print("Not enough skill points!\n", print_console);
			return;
		}

		nMod.pSkills[SKILL_WEAPONS_BULLET] += 1;
		CVAR_SET_FLOAT("skill_bullet", skill += 1);

		Print("Bullet Weapons: Upgraded to Advanced!", print_center);
		Print("Bullet Weapons: Upgraded to Advanced!", print_console);
		break;

	case SKILL_ADVANCED:
		Print("Skill already maxed out!\n", print_center);
		Print("Skill already maxed out!\n", print_console);
		break;

	default:
		Print("No skills upgraded!\n", print_center);
		Print("No skills upgraded!\n", print_console);
		break;
	}
}
#endif


#if 0
// Flawed: max carry is only called when weapons are initialized; would not change until next map
int GetMaxCarry(const char* pszAmmoName)
{
	int skill_level;
	int max_carry;
	//int skill_type;
	//int ammo_type;
	
	//if (FClassnameIs(pWeapon->pev, "weapon_9mmhandgun"))
	if (FStrEq(pszAmmoName, "9mm"))
	{
		Print("FOUND 9mm!\n", print_center);
		//ammo_type = AMMO_9MM;
		skill_level = GetSkillLevel(SKILL_WEAPONS_BULLET);
		max_carry = nMod.MaxCarry[skill_level][WEAPON_GLOCK];
		return max_carry;
	}

	//pWeapon->ItemInfoArray[WEAPON_GLOCK];
	
	/*if (FClassnameIs(pWeapon->pev, "9mm_clip"))
		skill_level = */
#if 0
	switch (skill_type)
	{
	case AMMO_9MM:

	/*case WEAPON_GLOCK:
	case WEAPON_AK47:
	case WEAPON_MP5:
	case WEAPON_TMP:
	case WEAPON_CROSSBOW:
	case WEAPON_DESERT_EAGLE:
		skill_level = GetSkillLevel(SKILL_WEAPONS_BULLET);
		return nMod.MaxCarry[skill_level][weapon];*/

	case WEAPON_RPG:
	case WEAPON_HANDGRENADE:
	case WEAPON_SATCHEL:
	case WEAPON_TRIPMINE:
		skill_level = GetSkillLevel(SKILL_WEAPONS_EXPLOSIVE);
		return nMod.MaxCarry[skill_level][weapon];

	case WEAPON_EGON:
	case WEAPON_GAUSS:
		skill_level = GetSkillLevel(SKILL_WEAPONS_ENERGY);
		return nMod.MaxCarry[skill_level][weapon];

	case WEAPON_HORNETGUN:
	case WEAPON_SNARK:
		skill_level = GetSkillLevel(SKILL_WEAPONS_ALIEN);
		return nMod.MaxCarry[skill_level][weapon];

	default:
		return 90;  // Randomly chosen on a whim
	}

	
	int maxcarry = nMod.MaxCarry[skill_level][weapon];

	return maxcarry;
#endif
}
#endif


#if 0
bool IsVisible(CBaseEntity* pEntity)
{
#ifndef CLIENT_DLL
	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecTargetOrigin;

	/*if (FBitSet(pEntity->pev->flags, FL_NOTARGET))
		return false;*/

	CBaseEntity* player = UTIL_GetLocalPlayer();

	// don't look through water
	/*if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) || (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
		return false;*/

	vecLookerOrigin = player->pev->origin + player->pev->view_ofs; //look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();

	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(player->pev) /*pentIgnore*/, &tr);

	if (tr.flFraction != 1.0)
	{
		Print("NOT VISIBLE!", print_center);
		return false; // Line of sight is not established
	}
	else
	{
		Print("VISIBLE!", print_center);
		return true; // line of sight is valid.
	}
#endif

#ifdef CLIENT_DLL
	return 0;
#endif
}
#endif

#if 0
void DoNanoSpark(entvars_t* pev, const Vector& location)
{
	Vector tmp = location/* + pev->size * 0.5*/;
	UTIL_Sparks(tmp);

	float flVolume = RANDOM_FLOAT(0.25, 0.75) * 0.4; //random volume range
	switch ((int)(RANDOM_FLOAT(0, 1) * 6))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark1.wav", flVolume, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark2.wav", flVolume, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark3.wav", flVolume, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark4.wav", flVolume, ATTN_NORM);
		break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark5.wav", flVolume, ATTN_NORM);
		break;
	case 5:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "buttons/spark6.wav", flVolume, ATTN_NORM);
		break;
	}
}
#endif


#if 0

#if 0
void Print(const char* msg, PRINT_TYPE where)
{
#ifndef CLIENT_DLL
		edict_t* player = FIND_ENTITY_BY_CLASSNAME(NULL, "player");
#else
		CBaseEntity* playerent = UTIL_PlayerByIndex(1);
#endif
	
	switch (where)
	{
	case print_chat:
#ifndef CLIENT_DLL
			ClientPrint(player, HUD_PRINTTALK, msg);
#else
		g_engfuncs.pfnClientPrintf(playerent, where, msg);
#endif

		return;

	case print_center:
#ifndef CLIENT_DLL
			ClientPrint(player, HUD_PRINTCENTER, msg);
#else
		g_engfuncs.pfnClientPrintf(playerent, where, msg);
#endif

		return;

	case print_console:
#ifndef CLIENT_DLL
		ClientPrint(player, HUD_PRINTCONSOLE, msg);
#else
		g_engfuncs.pfnClientPrintf(playerent, where, msg);
#endif

		return;
	}
}
#endif

const char* PickRandomMonster(int monster)
{
	switch (monster)
	{
	case MONSTER_HEADCRAB:
		return "monster_headcrab";
		//UTIL_PrecacheOther("monster_headcrab");
		break;

	case MONSTER_ZOMBIE:
		return "monster_zombie";
		//UTIL_PrecacheOther("monster_zombie");
		break;

	case MONSTER_HGRUNT:
		return "monster_human_grunt";
		//UTIL_PrecacheOther("monster_human_grunt");
		break;

	case MONSTER_APACHE:
		return "monster_apache";
		//UTIL_PrecacheOther("monster_apache");

	default:
		break;
	}

	return 0;
}

void SpawnAtDMSpot(Vector origin, Vector angles)
{

	edict_t* pent;
	entvars_t* pevCreate;

	//pent = CREATE_NAMED_ENTITY(MAKE_STRING(PickRandomMonster(MONSTER_HGRUNT)));
	pent = CREATE_NAMED_ENTITY(MAKE_STRING("monstermaker"));

	pevCreate = VARS(pent);
	pevCreate->origin = origin;
	pevCreate->angles = angles;
	//SetBits(pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND);

	//SET_MODEL(ENT(pevCreate), "models/gman.mdl");

	DispatchSpawn(ENT(pevCreate));
}
#endif


#if 0
void RPGStats_Defaults()
{
	// Default recoil values
	gWeaponRecoil[WEAPON_AK47][SKILL_BEGINNER] = RECOIL_BEGINNER;
	gWeaponRecoil[WEAPON_AK47][SKILL_INTERMEDIATE] = RECOIL_INTERMEDIATE;
	gWeaponRecoil[WEAPON_AK47][SKILL_ADVANCED] = RECOIL_ADVANCED;

	gWeaponRecoil[WEAPON_MP5][SKILL_BEGINNER] = RECOIL_BEGINNER;
	gWeaponRecoil[WEAPON_MP5][SKILL_INTERMEDIATE] = RECOIL_INTERMEDIATE;
	gWeaponRecoil[WEAPON_MP5][SKILL_ADVANCED] = RECOIL_ADVANCED;

	gWeaponRecoil[WEAPON_DESERT_EAGLE][SKILL_BEGINNER] = RECOIL_BEGINNER;
	gWeaponRecoil[WEAPON_DESERT_EAGLE][SKILL_INTERMEDIATE] = RECOIL_INTERMEDIATE;
	gWeaponRecoil[WEAPON_DESERT_EAGLE][SKILL_ADVANCED] = RECOIL_ADVANCED;

	gWeaponRecoil[WEAPON_GLOCK][SKILL_BEGINNER] = RECOIL_BEGINNER;
	gWeaponRecoil[WEAPON_GLOCK][SKILL_INTERMEDIATE] = RECOIL_INTERMEDIATE;
	gWeaponRecoil[WEAPON_GLOCK][SKILL_ADVANCED] = RECOIL_ADVANCED;

	gWeaponRecoil[WEAPON_PYTHON][SKILL_BEGINNER] = RECOIL_BEGINNER;
	gWeaponRecoil[WEAPON_PYTHON][SKILL_INTERMEDIATE] = RECOIL_INTERMEDIATE;
	gWeaponRecoil[WEAPON_PYTHON][SKILL_ADVANCED] = RECOIL_ADVANCED;

	gWeaponRecoil[WEAPON_SHOTGUN][SKILL_BEGINNER] = RECOIL_BEGINNER;
	gWeaponRecoil[WEAPON_SHOTGUN][SKILL_INTERMEDIATE] = RECOIL_INTERMEDIATE;
	gWeaponRecoil[WEAPON_SHOTGUN][SKILL_ADVANCED] = RECOIL_ADVANCED;


	// Machinegun accuracy for beginner
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_POOR] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_NORMAL] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_BETTER] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_EXCELLENT] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_ULTRA] = DEFAULT_ACCURACY_BEGINNER;

	// Machinegun accuracy for intermediate
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_POOR] = VECTOR_CONE_17DEGREES;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = VECTOR_CONE_6DEGREES;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_BETTER] = VECTOR_CONE_4DEGREES;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = VECTOR_CONE_2DEGREES;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = VECTOR_CONE_1DEGREES;

	// Machinegun accuracy for advanced
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_POOR] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_NORMAL] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_BETTER] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_EXCELLENT] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_ULTRA] = DEFAULT_ACCURACY_ADVANCED;


	// Shotgun accuracy for beginner
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_POOR] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_NORMAL] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_BETTER] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_EXCELLENT] = NOT_USED;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_ULTRA] = NOT_USED;

	// Shotgun accuracy for intermediate
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_POOR] = VECTOR_CONE_20DEGREES;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = VECTOR_CONE_15DEGREES;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_BETTER] = VECTOR_CONE_10DEGREES;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = NOT_USED;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = NOT_USED;

	// Shotgun accuracy for advanced
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_POOR] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_NORMAL] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_BETTER] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_EXCELLENT] = NOT_USED;
	gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_ULTRA] = NOT_USED;


	// Light pistol accuracy for beginner
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_POOR] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_BETTER] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT] = NOT_USED;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA] = NOT_USED;

	// Light pistol accuracy for intermediate
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR] = VECTOR_CONE_15DEGREES;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = VECTOR_CONE_5DEGREES;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER] = VECTOR_CONE_1DEGREES;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = NOT_USED;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = NOT_USED;

	// Light pistol accuracy for advanced
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_POOR] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_BETTER] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT] = NOT_USED;
	gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA] = NOT_USED;


	// Heavy pistol accuracy for beginner
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_POOR] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_BETTER] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT] = DEFAULT_ACCURACY_BEGINNER;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA] = DEFAULT_ACCURACY_BEGINNER;

	// Heavy pistol accuracy for intermediate
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR] = VECTOR_CONE_17DEGREES;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = VECTOR_CONE_7DEGREES;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER] = VECTOR_CONE_4DEGREES;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = VECTOR_CONE_2DEGREES;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = VECTOR_CONE_1DEGREES;

	// Heavy pistol accuracy for advanced
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_POOR] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_BETTER] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT] = DEFAULT_ACCURACY_ADVANCED;
	gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA] =  DEFAULT_ACCURACY_ADVANCED;
}
#endif

#if 0
void RPGStats_PlayerDefaults()  // TODO: this should be for loops, or one big nested for loop
{
	// Recoil values for player
	nMod.pRecoil[WEAPON_AK47][SKILL_BEGINNER] = gWeaponRecoil[WEAPON_AK47][SKILL_BEGINNER];
	nMod.pRecoil[WEAPON_AK47][SKILL_INTERMEDIATE] = gWeaponRecoil[WEAPON_AK47][SKILL_INTERMEDIATE];
	nMod.pRecoil[WEAPON_AK47][SKILL_ADVANCED] = gWeaponRecoil[WEAPON_AK47][SKILL_ADVANCED];

	nMod.pRecoil[WEAPON_MP5][SKILL_BEGINNER] = gWeaponRecoil[WEAPON_MP5][SKILL_BEGINNER];
	nMod.pRecoil[WEAPON_MP5][SKILL_INTERMEDIATE] = gWeaponRecoil[WEAPON_MP5][SKILL_INTERMEDIATE];
	nMod.pRecoil[WEAPON_MP5][SKILL_ADVANCED] = gWeaponRecoil[WEAPON_MP5][SKILL_ADVANCED];

	nMod.pRecoil[WEAPON_DESERT_EAGLE][SKILL_BEGINNER] = gWeaponRecoil[WEAPON_DESERT_EAGLE][SKILL_BEGINNER];
	nMod.pRecoil[WEAPON_DESERT_EAGLE][SKILL_INTERMEDIATE] = gWeaponRecoil[WEAPON_DESERT_EAGLE][SKILL_INTERMEDIATE];
	nMod.pRecoil[WEAPON_DESERT_EAGLE][SKILL_ADVANCED] = gWeaponRecoil[WEAPON_DESERT_EAGLE][SKILL_ADVANCED];

	nMod.pRecoil[WEAPON_GLOCK][SKILL_BEGINNER] = gWeaponRecoil[WEAPON_GLOCK][SKILL_BEGINNER];
	nMod.pRecoil[WEAPON_GLOCK][SKILL_INTERMEDIATE] = gWeaponRecoil[WEAPON_GLOCK][SKILL_INTERMEDIATE];
	nMod.pRecoil[WEAPON_GLOCK][SKILL_ADVANCED] = gWeaponRecoil[WEAPON_GLOCK][SKILL_ADVANCED];

	nMod.pRecoil[WEAPON_PYTHON][SKILL_BEGINNER] = gWeaponRecoil[WEAPON_PYTHON][SKILL_BEGINNER];
	nMod.pRecoil[WEAPON_PYTHON][SKILL_INTERMEDIATE] = gWeaponRecoil[WEAPON_PYTHON][SKILL_INTERMEDIATE];
	nMod.pRecoil[WEAPON_PYTHON][SKILL_ADVANCED] = gWeaponRecoil[WEAPON_PYTHON][SKILL_ADVANCED];

	nMod.pRecoil[WEAPON_SHOTGUN][SKILL_BEGINNER] = gWeaponRecoil[WEAPON_SHOTGUN][SKILL_BEGINNER];
	nMod.pRecoil[WEAPON_SHOTGUN][SKILL_INTERMEDIATE] = gWeaponRecoil[WEAPON_SHOTGUN][SKILL_INTERMEDIATE];
	nMod.pRecoil[WEAPON_SHOTGUN][SKILL_ADVANCED] = gWeaponRecoil[WEAPON_SHOTGUN][SKILL_ADVANCED];


	// Machinegun accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_BEGINNER][ACCURACY_ULTRA];

	// Machinegun accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_INTERMEDIATE][ACCURACY_ULTRA];

	// Machinegun accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_MACHINEGUNS][SKILL_ADVANCED][ACCURACY_ULTRA];


	// Shotgun accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_BEGINNER][ACCURACY_ULTRA];

	// Shotgun accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_INTERMEDIATE][ACCURACY_ULTRA];

	// Shotgun accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_SHOTGUN][SKILL_ADVANCED][ACCURACY_ULTRA];


	// Light pistol accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA];

	// Light pistol accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA];

	// Light pistol accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_LIGHTPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA];


	// Heavy pistol accuracy for beginner
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_BEGINNER][ACCURACY_ULTRA];

	// Heavy pistol accuracy for intermediate
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_INTERMEDIATE][ACCURACY_ULTRA];

	// Heavy pistol accuracy for advanced
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_POOR] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_POOR];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_NORMAL];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_BETTER] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_BETTER];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_EXCELLENT];
	nMod.pAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA] = gWeaponAccuracy[BULLETWEAPON_HEAVYPISTOL][SKILL_ADVANCED][ACCURACY_ULTRA];
}

#endif
