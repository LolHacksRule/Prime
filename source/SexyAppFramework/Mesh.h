#ifndef __MESH_H__
#define __MESH_H__

#include "SharedImage.h"

namespace Sexy
{
	class MeshPiece
	{
	public:
		std::string mObjectName;
		std::string mSetName;
		SharedImageRef mTexture;
		SharedImageRef mBumpTexture;
		MeshPiece();
		~MeshPiece();
	};
	class Mesh
	{
	public:
		std::string mFileName;
		MeshListener* mListener;
		void* mUserData;
		typedef std::list<MeshPiece*> MeshPieceList; //Official
		MeshPieceList mPieces;
		Mesh();
		~Mesh();
		virtual void Cleanup();
		virtual void SetListener(MeshListener* theListener);
	};
	typedef std::set<Mesh*> MeshSet;
	class MeshListener
	{
	public:
		virtual void MeshPreLoad(Mesh* theMesh) = 0;
		virtual void MeshHandleProperty(Mesh* theMesh, const std::string& theMeshName, const std::string& theSetName, const std::string& thePropertyName, const std::string& thePropertyValue) = 0;
		virtual SharedImageRef MeshLoadTex(Mesh* theMesh, const std::string& theMeshName, const std::string& theSetName, const std::string& thePropertyName, const std::string& theFileName) = 0;
		virtual void MeshPreDraw(Mesh* theMesh) = 0;
		virtual void MeshPostDraw(Mesh* theMesh) = 0;
		virtual void MeshPreDrawSet(Mesh* theMesh, std::string theMeshName, std::string theSetName, bool hasBump) = 0;
		virtual void MeshPostDrawSet(Mesh* theMesh, std::string theMeshName, std::string theSetName) = 0;
		virtual void MeshPreDeleted(Mesh* theMesh) = 0;
	};
}

#endif //__MESH_H__