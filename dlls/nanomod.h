#pragma once
#include "nanodata.h"


//
// Main data struct
//

#ifndef NANOSTRUCT
#define NANOSTRUCT

struct NanoMod_Data
{
	Vector pAccuracy[BULLETWEAPON_MAX][MAX_SKILLLEVELS][ACCURACY_MAX];
	float pRecoil[MAX_WEAPONS][MAX_SKILLLEVELS];
	//float pSpeeds[MAX_SKILLLEVELS];
	//int MaxCarry[MAX_SKILLLEVELS][MAX_WEAPONS];
	//int BodyGroupHealth[MAX_BODYPARTS];

	int pSkills[SKILL_TYPE_MAX];

	float pSkillPoints;

	//float FiringDelay;

	int Inventory[MAX_ITEMS];
	//bool ShowingInventory;

	float bleed_timer;

	bool BeastMode;
	bool PlayerIsBleeding;
	bool PlayerIsBandaging;
	bool ShowUpgradeMenu;
	bool PlayerZoomed;
	bool gManDead;
};

#endif 


//
// Function prototypes
//

float TweakWeaponDamage(int weapon, float damage);
Vector GetAccuracy(CBasePlayer* player, int weapon);
void ScorePoints(entvars_t* pevAttacker, entvars_t* pev);
void HeadshotBonus();
void DisplayScore(/*entvars_t* pevPlayer, int points*/);
void DisplayBonus(entvars_t* pevPlayer, int points);
float ExplosionScale();
void ResetNanoMod();
bool IsGManDead();
entvars_t* SpawnGManDeathMDL(entvars_t* pev);
void CheckGManDeath(entvars_t* pev);
int KillGMan(entvars_t* pev);
bool KillSequence();
bool IsPlayerMoving(CBasePlayer* player);  // TODO: change these to PlayerIsMoving etc.
bool IsPlayerCrouching(CBasePlayer* player);
bool IsPlayerZoomed();
bool IsFadeoutEnabled();
void SpawnAtDMSpot(Vector origin, Vector angles);
void TestShit();
const char* PickRandomMonster(int monster);
void RPGStats_Defaults();
void RPGStats_Init();
void RPGStats_PlayerDefaults();
void EXPORT NanoMod_Init();
void Startup_Message();
void SetDefaultSkills();
extern float GetWeaponRecoil(/*CBasePlayer* player,*/ int weapon);
void RegisterCommands();
void UpgradeBulletSkill();
extern void RPGStats_Setup();
void Print(const char* msg, PRINT_TYPE where);
float GetSpeedBoost();
void UpgradeSpeedSkill();
float GetJumpBoost();
void AdrenalineRush();
bool IsBeastMode();
void ToggleSkillMenu();
float GetSwimSpeed();
void UpgradeSwimmingSkill();
float GetGaussRefire();
void UpgradeEnergySkill();
float GetDrownDamage();
int GaspForAir();
void CheckSkillChange(int skilltype);
float RecoilExceptions(CBasePlayer* player);
float GetBandageTime();
float GetFallDamage(float falldamage);
int GetSkillLevel(int skilltype);
void Init_Menus();
int GetWeaponMenuColumn(int weapon);
int GetWeaponMenuRow(int weapon);
float GetShotgunDistance();
//void Init_MaxCarryStats();
void Init_AccuracyStats();
void Init_RecoilStats();
int GetMaxCarry(const char* pszAmmoName);
bool NanoThink(entvars_t* pev);
bool NanoThinkDelay();
void CheckBleed(entvars_t* pev);
void ApplyBandage();
bool EntityIsPlayer(entvars_t* pev);
float GetHitPunch(entvars_t* pev);
float GetMaxCharge();
float ThrowStrength(int weapon);
float GetSatchelSpeed();
void InitHordeMode();
void WorldInit();
void DoNanoSpark(entvars_t* pev, const Vector& location);
float GetLongJumpSpeed();
void ShowInventory();
void SpawnRandomMonster();
edict_t* NanoFindSpawnPoint();
float GetDamageReduction(int bodypart);
float GetAccuracyPenalty();


//
// Notes
//

/*
 	// read from the MOTD.txt file
	int length, char_count = 0;
	char* pFileList;
	char* aFileList = pFileList = (char*)LOAD_FILE_FOR_ME((char*)CVAR_GET_STRING("motdfile"), &length);
	*/

// FStrEq(const char* sz1, const char* sz2) for comparing strings!

/*
* class CSkillTemplate
{

};
*/