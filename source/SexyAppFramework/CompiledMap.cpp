#include "CompiledMap.h"

using namespace Sexy;

CompiledMap::CompiledMap() //9-12
{
	mNodes = 0;
	mNumNodes = 0;
}

CompiledMap::CompiledMap(char_t* theBuffer, index_t theSize) //15-17
{
	Init(theBuffer, theSize);
}

void CompiledMap::Init(char_t* theBuffer, index_t theSize) //20-23
{
	mNodes = (Node*)theBuffer;
	mNumNodes = theSize >> 2;
}

bool CompiledMap::Initialized() //26-28
{
	return mNodes != 0;
}

char_t* CompiledMap::Find(const char* theString) //31-35
{
	char_t* foundFastPtr = (char_t*)theString; //?
	if (mNumNodes != 0)
		return FindFast(foundFastPtr);
	else
		return 0;
}

char_t* CompiledMap::Find(const wchar_t* theString) //38-41
{
	return NULL;
}

CompiledMap::Node* CompiledMap::Find(Node* node, const char_t* word, char_t* foundWord) //Correct? | 44-65
{
	Node* foundNode = NULL;
	if (node->ch() == toupper(*word) || foundWord && (node->ch() == toupper(*word)))
	{
		if (foundWord)
			*foundWord = node->ch();
		if (node->ch() == NULL)
			return node;
		if (foundWord)
			return Find(node, word, foundWord);
		else
			return Find(node, word, NULL);
	}
	else if (node->alt())
		return Find(&mNodes[node->alt()], word, foundWord);
	return foundNode;
}

char_t* CompiledMap::FindFast(const char_t* word) //68-104
{
	Node* searchNode = mNodes;
	const char_t* searchChar; //?
	while (searchNode)
	{
		char_t comp_ch = _toupper(*word);
		if (searchNode->ch() == comp_ch)
		{
			if (searchNode->ch() == NULL)
				return (char_t*)searchNode->ch();
			searchNode++;
			word++;
		}
		else
		{
			if (searchNode->ch() > (int)comp_ch)
				return NULL;
			if (searchNode->alt() == NULL)
				return NULL;
			searchNode = &mNodes[searchNode->alt()];
		}
	}
	return NULL;
}

