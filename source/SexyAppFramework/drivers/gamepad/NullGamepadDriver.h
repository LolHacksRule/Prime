#ifndef __NULLGAMEPADDRIVER_H_
#define __NULLGAMEPADDRIVER_H_

#include "../../IGamepadDriver.h"

namespace Sexy //SAB would be defined but for line integrity
{
	class NullGamepad : public IGamepad //Recovered pretty much all the lines :)
	{
	public:
		NullGamepad(SexyAppBase* theApp, int theGamepadIndex) { mApp = theApp; mGamepadIndex = theGamepadIndex; } //11
		~NullGamepad() {}; //12

		bool					IsConnected() { return false; }; //14

		int						GetGamepadIndex() { return mGamepadIndex; }; //16

		bool					IsButtonDown(int button) { return false; }; //18
		float					GetButtonPressure(int button) { return 0.0; }; //19

		float					GetAxisXPosition() { return 0.0; }; //21
		float					GetAxisYPosition() { return 0.0; }; //22

		float					GetAxisXPositionRamped() { return 0.0; }; //24
		float					GetAxisYPositionRamped() { return 0.0; }; //25

		float					GetRightAxisXPosition() { return 0.0; }; //27
		float					GetRightAxisYPosition() { return 0.0; }; //28

		void					Update() {} //30


		void					AddRumbleEffect(float theLeft, float theRight, float theFadeTime) {}; //33
	protected:
		int								mGamepadIndex;
		SexyAppBase*					mApp;
	};
	class NullGamepadDriver : public IGamepadDriver
	{
	private:
		NullGamepad*					mGamepads[4];
	public:
		NullGamepadDriver();
		~NullGamepadDriver();
		bool					InitGamepadDriver(SexyAppBase* theApp);
		IGamepad*				GetGamepad(int theIndex);
		void					Update();
	};
}
#endif //__NULLGAMEPADDRIVER_H_