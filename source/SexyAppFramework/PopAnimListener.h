#ifndef __POPANIMLISTENER_H__
#define __POPANIMLISTENER_H__

#include "PIEffect.h"

namespace Sexy
{
    class PopAnimListener
    {
    public:
        enum ImagePredrawResult
        {
            ImagePredraw_DontAsk,
            ImagePredraw_Normal,
            ImagePredraw_Skip,
            ImagePredraw_Repeat
        };
        virtual void PopAnimPlaySample(const std::string& theSampleName,int thePan, double theVolume, double theNumSteps) = 0;
        virtual PIEffect* PopAnimLoadParticleEffect(std::string& theEffectName) = 0;
        virtual bool PopAnimObjectPredraw(int theId, Graphics* g, PASpriteInst* theSpriteInst, PAObjectInst* theObjectInst, PATransform* , Color& theColor) = 0;
        virtual bool PopAnimObjectPostdraw(int theId, Graphics* g, PASpriteInst* theSpriteInst, PAObjectInst* theObjectInst, PATransform* , Color& theColor) = 0;
        virtual ImagePredrawResult PopAnimImagePredraw(PASpriteInst* theSpriteInst, PAObjectInst* theObjectInst, PATransform* theTransform, Image* theImage, Graphics* g, int theDrawCount) = 0;
        virtual void PopAnimStopped(int theId) = 0;
        virtual bool PopAnimCommand(int theId, PASpriteInst* ,std::string& theCommand, std::string& theParam) = 0;
        virtual void PopAnimCommand(int theId, std::string& theCommand,std::string& theParam) = 0;
    };
}
#endif