//****************************************************************************
//**
//**    FEASTLIBMAIN.CPP
//**
//****************************************************************************
#include "FeastInternal.h"

#define VERSION 1.04f //Roughly identical either way

namespace FEAST {

class CLibClient
: public ILibClient
{
public:
	void* LibMalloc(unsigned long inSize) //17-22
	{
		void* ptr = malloc(inSize);
		if (!ptr)
			LibError("Out of memory");
		return(ptr);
	}
	void* LibRealloc(void* inPtr, unsigned long inNewSize) //24-29
	{
		void* ptr = realloc(inPtr, inNewSize);
		if (!ptr)
			LibError("Out of memory");
		return(ptr);
	}
	void LibFree(void* inPtr) //31-34
	{
		if (inPtr)
			free(inPtr);
	}
	void LibError(const char* inErrorStr) //36-39
	{
		printf("FEAST Error: %s", inErrorStr);
		exit(1);
	}
};

ILibClient* LIB_GetDefaultClient() //43-46
{
	static CLibClient sClient;
	return(&sClient);
}

ILibClient*& LIB_GetClientRef() //49-52
{
	static ILibClient* spClient = LIB_GetDefaultClient();
	return(spClient);
}

char* LIB_Va(const char*& inFmt, char* inBuffer=NULL, ...) //55-66
{
	static char buf[4096];
	if (!inFmt)
		return(NULL);
	if (!inBuffer)
		inBuffer = buf;
	va_list args;
	va_start(args, inBuffer);
	vsprintf(inBuffer, inFmt, args);
	va_end(args);
	return(inBuffer);
}

void* LIB_ClientMalloc(NDword inSize) //69-74
{
	// allocate a block of memory, with padding to store a pointer to the client that allocated it
	void* ptr = LIB_GetClientRef()->LibMalloc(inSize + sizeof(ILibClient*));
	*((ILibClient**)ptr) = LIB_GetClientRef();
	return((NByte*)ptr + sizeof(ILibClient*));
}
void* LIB_ClientRealloc(void* inPtr, NDword inSize) //76-83
{
	// realloc a block of memory using the client that allocated it
	NByte* basePtr = (NByte*)inPtr - sizeof(ILibClient*);
	ILibClient* client = *((ILibClient**)basePtr);
	void* ptr = client->LibRealloc(basePtr, inSize + sizeof(ILibClient*));
	*((ILibClient**)ptr) = client;
	return((NByte*)ptr + sizeof(ILibClient*));
}
void LIB_ClientFree(void* inPtr) //85-92
{
	// free a block of memory using the client that allocated it
	if (!inPtr)
		return;
	NByte* basePtr = (NByte*)inPtr - sizeof(ILibClient*);
	ILibClient* client = *((ILibClient**)basePtr);
	client->LibFree(basePtr);
}

void LIB_Errorf(const char* inFmt, ... ) //95-97
{
	LIB_GetClientRef()->LibError(LIB_Va(inFmt));
}

FEAST_API ILibClient* LIB_SetClient(ILibClient* inClient) //100-106
{
	ILibClient* oldClient = LIB_GetClientRef();
	if (!inClient)
		inClient = LIB_GetDefaultClient(); // use default if null is passed in
	LIB_GetClientRef() = inClient;
	return(oldClient);
}
FEAST_API ILibClient* LIB_GetClient() //108-110
{
	return(LIB_GetClientRef());
}

FEAST_API float LIB_GetVersion() //113-115
{
	return(VERSION);
}

} // namespace FEAST

//****************************************************************************
//**
//**    END MODULE FEASTLIBMAIN.CPP
//**
//****************************************************************************

