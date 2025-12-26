#include "Mesh.h"
#include "SexyAppBase.h"
#include "IGraphicsDriver.h"

using namespace Sexy;

MeshPiece::MeshPiece() //9-10
{
}

MeshPiece::~MeshPiece() //13-14
{
}

Mesh::Mesh() //17-23
{
	mListener = NULL;
	mUserData = NULL;

	//
	gSexyAppBase->mGraphicsDriver->AddMesh(this);
}

Mesh::~Mesh() //26-33
{
	if (mListener != NULL)
		mListener->MeshPreDeleted(this);

	Cleanup();

	gSexyAppBase->mGraphicsDriver->RemoveMesh(this); //C++ only
}

void Mesh::Cleanup() //36-45 (Correct?)
{
	MeshPieceList::iterator anItr = mPieces.begin();
	while (anItr != mPieces.end())
	{
		MeshPiece* aPiece = *anItr;
		delete aPiece;
		anItr++;
	}
	mPieces.clear();
}

void Mesh::SetListener(MeshListener* theListener) //48-50
{
	mListener = theListener;
}