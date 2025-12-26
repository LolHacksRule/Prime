#include "NullGamepadDriver.h"

using namespace Sexy;

IGamepadDriver* IGamepadDriver::CreateGamepadDriver() //Only used by consoles and XNA | Lines 7-9
{
	return new NullGamepadDriver();
}

NullGamepadDriver::NullGamepadDriver() //Lines 12-17
{
	for (int i = 0; i < 4; ++i)
	{
		mGamepads[i] = 0;
	}
}

NullGamepadDriver::~NullGamepadDriver() //Correct? | Lines 20-25
{
	for (int i = 0; i < 4; i++) //4 players
	{
		delete mGamepads[i];
	}
}

bool NullGamepadDriver::InitGamepadDriver(SexyAppBase* theApp) //Correct? | Lines 28-34
{
	for (int i = 0; i < 4; i++) //4 players
	{
		mGamepads[i] = new NullGamepad(theApp, i);
	}
	return true;
}

IGamepad* NullGamepadDriver::GetGamepad(int theIndex) //Lines 37-39
{
	return mGamepads[theIndex];
}

void NullGamepadDriver::Update() //Lines 42-43
{
}