#include "RSPlayerState.h"

ARSPlayerState::ARSPlayerState()
{
	Kills = 0;
	Deaths = 0;
}

void ARSPlayerState::AddKill()
{
	++Kills;
}

void ARSPlayerState::AddDeath()
{
	++Deaths;
}