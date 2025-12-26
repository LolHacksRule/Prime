#ifndef __D3DOBJECTLISTENER_H__
#define __D3DOBJECTLISTENER_H__

#include "SharedImage.h"

namespace Sexy
{
	class D3DObject;
	class D3DObjectListener
	{
		public:
			virtual void D3DObjectPreLoad(D3DObject*);
			virtual void D3DObjectHandleProperty(D3DObject*, std::string&, std::string&, std::string&, std::string&);
			virtual SharedImageRef D3DObjectLoadTex(D3DObject*, std::string&, std::string&, std::string&, std::string&);
			virtual void D3DObjectPreDraw(D3DObject*);
			virtual void D3DObjectPostDraw(D3DObject*);
			virtual void D3DObjectPreDrawSet(D3DObject*, std::string&, std::string&, bool);
			virtual void D3DObjectPostDrawSet(D3DObject*, std::string&, std::string&);
			virtual void D3DObjectPreDeleted(D3DObject*);
	};
}
#endif