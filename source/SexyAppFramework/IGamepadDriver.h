#ifndef __IGAMEPADDRIVER_H__
#define __IGAMEPADDRIVER_H__

//#include "SexyAppBase.h" //We don't need SAB

namespace Sexy
{
	class SexyAppBase;
	class IGamepad
	{
	public:
		virtual						~IGamepad() {} //83
		virtual bool				IsConnected() = 0;
		virtual int					GetGamepadIndex() = 0;
		virtual bool				IsButtonDown(int button) = 0;
		virtual float				GetButtonPressure(int button) = 0;
		virtual float				GetAxisXPosition() = 0;
		virtual float				GetAxisYPosition() = 0;
		virtual float				GetAxisXPositionRamped() = 0;
		virtual float				GetAxisYPositionRamped() = 0;
		virtual float				GetRightAxisXPosition() = 0;
		virtual float				GetRightAxisYPosition() = 0;
		virtual void				Update() = 0;
		virtual void				AddRumbleEffect(float theLeft, float theRight, float theFadeTime) = 0;
	};
	class IGamepadDriver
	{
	public:
		static IGamepadDriver*		CreateGamepadDriver();
		virtual						~IGamepadDriver() {} //112
		virtual bool				InitGamepadDriver(SexyAppBase* theApp) = 0;
		virtual IGamepad*			GetGamepad(int thePlayer) = 0;
		virtual void				Update() = 0;
	};
}
#endif //__IGAMEPADDRIVER_H__