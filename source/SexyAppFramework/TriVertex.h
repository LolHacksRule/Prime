#ifndef __TRIVERTEX_H__
#define __TRIVERTEX_H__

#include "SexyVector.h"

namespace Sexy
{

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class SexyVertex
{
	enum
	{
		FVF = 0 //const uint in XNA
	};
};

class SexyVertex2D : public SexyVertex //Struct in XNA, not in H5
{
public:
	float x,y,z,rhw;
	DWORD color; //ARGB (0 = use color specified in function call)
	DWORD specular;
	float u,v; //in XNA u2 and v2 are here

	enum
	{
		FVF = 452
	};

public:
	SexyVertex2D() { z = 0.0; rhw = 1.0; color = 0; specular = 0; } //57
	SexyVertex2D(float theX, float theY) : x(theX), y(theY) { color = 0; }
	SexyVertex2D(float theX, float theY, float theU, float theV) : x(theX), y(theY), u(theU), v(theV), z(0.0), rhw(1.0) { color = 0; specular = 0; } //59
	SexyVertex2D(float theX, float theY, float theU, float theV, DWORD theColor) : x(theX), y(theY), u(theU), v(theV), color(theColor) { }
	SexyVertex2D(float theX, float theY, float theZ, float theU, float theV, DWORD theColor) : x(theX), y(theY), z(theY), u(theU), v(theV), color(theColor) { }
};

class SexyVertex3D : public SexyVertex
{
public:
	float x,y,z;
	DWORD color; //ARGB (0 = use color specified in function call)
	float u,v;

	enum
	{
		FVF = 322
	};

public:
	SexyVertex3D() {}
	SexyVertex3D(float theX, float theY, float theZ) : x(theX), y(theY), z(theZ) { color = 0; }
	SexyVertex3D(float theX, float theY, float theZ, float theU, float theV) : x(theX), y(theY), z(theZ), u(theU), v(theV) { color = 0; }
	SexyVertex3D(float theX, float theY, float theZ, float theU, float theV, DWORD theColor) : x(theX), y(theY), z(theZ), u(theU), v(theV), color(theColor) { }
};

class SexyVertex3DLit : public SexyVertex //Does not exist outside Win or XNA?
{
public:
	float x,y,z,nx,ny,nz;
	DWORD color; //ARGB (0 = use color specified in function call)
	float u,v;

	enum
	{
		FVF = 338
	};

public:
	void MakeDefaultNormal() //Unofficial
	{
		SexyVector3 aVector;
		aVector.Normalize();
		nx = aVector.x;
		ny = aVector.y;
		nz = aVector.z;
	}
	SexyVertex3DLit() {}
	SexyVertex3DLit(const SexyVector3& thePos) : x(thePos.x), y(thePos.y), z(thePos.z) { MakeDefaultNormal(); color = 0; }
	SexyVertex3DLit(const SexyVector3& thePos, const SexyVector3& theNormal) : x(thePos.x), y(thePos.y), z(thePos.z), nx(theNormal.x), ny(theNormal.y),nz(theNormal.z) { color = 0; }
	SexyVertex3DLit(const SexyVector3& thePos, const SexyVector3& theNormal, float theU, float theV) : x(thePos.x), y(thePos.y), z(thePos.z), nx(theNormal.x), ny(theNormal.y), nz(theNormal.z), u(theU), v(theV) { MakeDefaultNormal(); color = 0; }
	SexyVertex3DLit(const SexyVector3& thePos, const SexyVector3& theNormal, float theU, float theV, DWORD theColor) : x(thePos.x), y(thePos.y), z(thePos.z), nx(theNormal.x), ny(theNormal.y),nz(theNormal.z), u(theU), v(theV), color(theColor) { }
	SexyVertex3DLit(const SexyVector3& thePos, float theU, float theV) : x(thePos.x), y(thePos.y), z(thePos.z), u(theU), v(theV) { MakeDefaultNormal(); color = 0; }
	SexyVertex3DLit(const SexyVector3& thePos, float theU, float theV, DWORD theColor) : x(thePos.x), y(thePos.y), z(thePos.z), u(theU), v(theV), color(theColor) { MakeDefaultNormal(); }
};

} // namespace Sexy

#endif