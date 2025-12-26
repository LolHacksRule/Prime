#ifndef __COMPILEDMAP_H__
#define __COMPILEDMAP_H__

#include "Common.h"

namespace Sexy
{
	typedef uchar char_t;
	typedef ulong index_t;
	class CompiledMap //C++ only
	{
	public:
		CompiledMap();
		CompiledMap(char_t* theBuffer, index_t theSize);
		void Init(char_t* theBuffer, index_t theSize);
		bool Initialized();
		char_t* Find(const char* theString);
		char_t* Find(const wchar_t* theString);
	private:
		struct Node
		{
			index_t v;
			char_t ch() { return v; } //40
			index_t alt() { return (v >> 8) & 0xFFFFFF; } //41
		};
		Node* Find(Node* node, const char_t* word, char_t* foundWord);
		Node* mNodes;
		index_t mNumNodes;
		char_t* FindFast(const char_t* word);
	};
}
#endif