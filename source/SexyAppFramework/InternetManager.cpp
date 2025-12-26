#include "InternetManager.h"
#include "SexyAppBase.h"
#include <direct.h>
#include <ShlObj.h>

using namespace Sexy;

InternetManager::InternetManager() //30-35
{
	mParsed = false;
	mIsUpToDate = true;
	mDoingTransfers = false;
	mNumMoreGamesAds = 4;
}

InternetManager::~InternetManager() //38-41
{
	StopTransfers();
	SaveAdList();
}

void InternetManager::Init() //44-46
{
	LoadAdList();
}

void InternetManager::StartVersionCheck(const std::string& theUrl) //49-57
{
	mParsed = false;
	mVersionCheckTransfer.Get(theUrl);
}

int InternetManager::GetVersionCheckResult() //60-79
{
	switch (mVersionCheckTransfer.GetResultCode())
	{
	case HTTPTransfer::RESULT_DONE: return VCRESULT_DONE;
	case HTTPTransfer::RESULT_NOT_STARTED: return VCRESULT_NONE;
	case HTTPTransfer::RESULT_NOT_COMPLETED: return VCRESULT_CHECKING;
	default: return VCRESULT_FAILED;
	}
}

AdInstance* InternetManager::FindAdInstanceInList(AdInstanceList& theList, const AdInstance& theInstance) //82-94
{
	if (theInstance.mResourceList.empty())
		return NULL;
	AdInstanceList::iterator anItr = theList.begin();
	if (anItr != theList.end())
	{
		AdInstance& anInstance = (*anItr);
		if (anInstance.mResourceList.empty() && anInstance.mResourceList.front() == theInstance.mResourceList.front())
			return &anInstance;
		anItr++;
	}
	return NULL;
}

bool InternetManager::ConfirmParsed() //97-153 (Correct?)
{
	if (mParsed)
		return true;

	if (mVersionCheckTransfer.GetResultCode())
		return false;

	mDoingTransfers = true;
	mOldAdInstanceList = mAdInstanceList;
	mAdInstanceList.clear();
	mPendingResourceList.clear();
	int aLineNum = 0;
	std::string aString = mVersionCheckTransfer.GetContent();
	while (aString.length())
	{
		int aCRPos = aString.find('\n');
		if (aCRPos == -1)
			aCRPos = aString.length();
		std::string aLine = aString.substr(0, aCRPos);
		while (aLine.length())
		{
			if (aLine[aLine.length() - 1] != '\r' && aLine[aLine.length() - 1] != ' ')
				break;
			aLine.resize(aLine.length() - 1);
		}
		if (aLine.length() > 0 && aLineNum)
		{
			if (aLineNum == 1)
				mUpdateURL = aLine;
			else
				ParseAdLine(aLine);
		}
		else
			mIsUpToDate = atoi(aLine.c_str());
		aLineNum++;
		if (aCRPos + 1 >= aString.length())
			break;
		aString = aString.substr(aCRPos + 1);
	}
	EnsureSufficientMoreGamesAds();
	return true;
}

void InternetManager::ParseAdLine(const std::string& theString, bool justDownloaded) //156-270 (Correct?)
{
	if (theString.length() == 0)
		return;
	
	std::string aLine = theString;
	AdInstance anAdInstance;
	anAdInstance.mHasAllResources = false;
	anAdInstance.mIsNew = false;
	int aWordNum = 0;
	int aSectionNum = 0;
	std::string aWord;
	int aNonSpacePos = aLine.find_first_of(" ", 0);
	int aSpacePos = aLine.find_first_of(" ", aNonSpacePos + 1);
	if (aSpacePos == -1 || aNonSpacePos == -1)
		return;
	
	aWord = StringToLower(aLine.substr(aNonSpacePos, aSpacePos - aNonSpacePos));

	if (aWord == "popup" || isdigit(aWord[0]))
		anAdInstance.mType = AdType_Popup;
	else
	{
		if (aWord != "more")
			return;
		anAdInstance.mType = AdType_MoreGames;
	}

	if (!isdigit(aWord[0]))
		aLine = aLine.substr(aSpacePos + 1);


	while (aLine.length())
	{
		int aSpacePos = aLine.find(" ");
		if (aSpacePos == -1)
			aSpacePos = aLine.length();
		aWord = aLine.substr(0, aSpacePos);

		if (aWord.compare("|") == 0)
		{
			aWordNum = 0;
			aSectionNum++;
		}
		else
		{
			if (aSectionNum)
			{
				if (aSectionNum == 1)
				{
					anAdInstance.mResourceList.push_back(aWord);
					StringList::iterator anItr = std::find(mPendingResourceList.begin(), mPendingResourceList.end(), aWord); //Correct?
					
					if (anItr == mPendingResourceList.end())
						mPendingResourceList.push_back(aWord);

					if (anAdInstance.mResourceList.size() == 1)
					{
						anAdInstance.mIsNew = 1;
						AdInstance* anOldInstance = FindAdInstanceInList(mOldAdInstanceList, anAdInstance);
						if (anOldInstance)
						{
							anAdInstance.mIsNew = anOldInstance->mIsNew;
							anAdInstance.mRemove = anOldInstance->mRemove;
							anAdInstance.mJustDownloaded = anOldInstance->mJustDownloaded;
						}
						else
							anAdInstance.mJustDownloaded = justDownloaded;
					}
				}

				else if (aSectionNum == 2)
				{
					if (aWordNum == 1)
						anAdInstance.mRemove = atoi(aWord.c_str());
					else
						anAdInstance.mIsNew = atoi(aWord.c_str());
				}
			}

			else if (anAdInstance.mType == AdType_Popup)
			{
				if (aWordNum == 1)
					anAdInstance.mHeight = atoi(aWord.c_str());
				else
					anAdInstance.mWidth = atoi(aWord.c_str());
			}

			else if (anAdInstance.mType == AdType_MoreGames)
			{
				if (aWordNum == 1)
					anAdInstance.mInstanceName = aWord;
				else
					anAdInstance.mClickURL = aWord;
			}

			aWordNum++;
		}

		if (aSpacePos + 1 >= aLine.length())
			break;

		aLine = aLine.substr(aSpacePos + 1);
	}

	mAdInstanceList.push_back(anAdInstance);
}

void InternetManager::EnsureSufficientMoreGamesAds() //273-344 (Correct?)
{
	typedef std::set<std::string, StringLessNoCase> StringSet; //Officially here
	StringSet aNameSet;
	AdInstanceList aSubstituteMoreGamesList;
	AdInstanceList::iterator anItr = mOldAdInstanceList.begin();
	while (anItr != mOldAdInstanceList.end())
	{
		AdInstance& anInstance = *anItr;
		if (anInstance.mType == AdType_MoreGames && !FindAdInstanceInList(mAdInstanceList, anInstance))
		{
			CheckAdResources(anInstance);
			if (anInstance.mHasAllResources)
				aSubstituteMoreGamesList.push_back(anInstance);
		}
		anItr++;
	}
	int aNumDownloadedMoreGamesAds = 0;
	while (anItr != mAdInstanceList.end())
	{
		AdInstance& anInstance = *anItr;
		if (anInstance.mType == AdType_MoreGames)
		{
			CheckAdResources(anInstance);
			if (anInstance.mHasAllResources && !anInstance.mRemove)
			{
				aNameSet.insert(anInstance.mInstanceName);
				aNumDownloadedMoreGamesAds++;
			}
			else
				aSubstituteMoreGamesList.push_back(*anItr);
		}
		if (anInstance.mRemove)
			mAdInstanceList.erase(anItr);
		else
			anItr++;
	}
	anItr = aSubstituteMoreGamesList.begin();
	while (anItr != mAdInstanceList.end() || aNumDownloadedMoreGamesAds <= mNumMoreGamesAds)
	{
		AdInstance& anInstance = *anItr;
		if (aNameSet.find(anInstance.mInstanceName) == aNameSet.end())
		{
			aNameSet.insert(anInstance.mInstanceName);
			anInstance.mRemove = true;
			mAdInstanceList.push_back(anInstance);
			aNumDownloadedMoreGamesAds++;
		}
		anItr++; //The decompiler puts it in the if too it's prob here
	}

	anItr = aSubstituteMoreGamesList.begin();
	while (anItr != mAdInstanceList.end() || aNumDownloadedMoreGamesAds <= mNumMoreGamesAds)
	{
		AdInstance& anInstance = *anItr;
		anInstance.mRemove = true;
		mAdInstanceList.push_back(anInstance);
		aNumDownloadedMoreGamesAds++;
		anItr++; //The decompiler puts it in the if too it's prob here
	}
}

void InternetManager::TryLoadMoreGamesAd(AdInstance* theInstance, MoreGamesAd* theAds, int& theIndex) //677-702 (Correct?)
{
	if (theInstance->mResourceList.empty())
		return;
	MoreGamesAd& anAd = theAds[theIndex];
	delete anAd.mImage;
	delete anAd.mOverImage;
	anAd.mOverImage = 0;
	anAd.mImage = gSexyAppBase->GetImage(theInstance->mResourceList.front());

	if (theInstance->mResourceList.size() < 2)
		return;

	StringList::iterator anItr = theInstance->mResourceList.begin()++;
	anAd.mOverImage = gSexyAppBase->GetImage(*anItr);
	if (anAd.mOverImage && anAd.mImage)
	{
		anAd.mClickURL = theInstance->mClickURL;
		theInstance->mIsNew = 0;
		theIndex++;
	}
}

bool InternetManager::LoadAdList() //347-361 (Correct?)
{
	Buffer aBuffer;
	std::string aString = "adlist.txt";
	if (!gSexyAppBase->ReadBufferFromFile(aString, &aBuffer))
		return false;

	while (!aBuffer.AtEnd())
	{
		aString = aBuffer.ReadLine();
		ParseAdLine(aString);
	}
	EnsureSufficientMoreGamesAds();
	return true;
}

bool InternetManager::SaveAdList() //388-410
{
	if (!mAdInstanceList.size())
		return true;

	Buffer aBuffer;
	EnsureSufficientMoreGamesAds();
	AdInstanceList::iterator anItr = mAdInstanceList.begin();
	while (anItr != mAdInstanceList.end())
	{
		AdInstance& anAdInstance = *anItr;
		std::string aLine;
		GenAdLine(anAdInstance, aLine);
		aBuffer.WriteLine(aLine);
		anItr++;
	}
	return gSexyAppBase->WriteBufferToFile("adlist.txt", &aBuffer);
}

void InternetManager::GenAdLine(AdInstance& theInstance, std::string& theLine) //364-385 (Correct?)
{
	AdInstance& anAdInstance = theInstance;
	std::string& aLine = theLine;

	if (anAdInstance.mType == AdType_Popup)
		aLine = StrFormat("popup %d %d |", anAdInstance.mWidth, anAdInstance.mHeight);
	else if (anAdInstance.mType == AdType_MoreGames)
		aLine = StrFormat("more %s %s |", anAdInstance.mInstanceName, anAdInstance.mClickURL);

	StringList::iterator aStringItr = anAdInstance.mResourceList.begin();
	while (aStringItr != anAdInstance.mResourceList.end())
	{
		aLine += *aStringItr;
		aLine += " ";
		aStringItr++;
	}
	aLine += " | ";

	if (anAdInstance.mIsNew)
		aLine += "1";
	else
		aLine += "0";

	if (anAdInstance.mRemove)
		aLine += "1";
	else
		aLine += "0";
}

void InternetManager::Update() //413-472 (Correct?)
{
	if (!mDoingTransfers)
		return;
	for (int aTransferNum = 0; aTransferNum < 4; aTransferNum++)
	{
		int aResultCode = mFileTransfers[aTransferNum].GetResultCode();
		if (aResultCode != HTTPTransfer::RESULT_DONE)
		{
			OutputDebugString(("Error on AdFile: ", mFileTransfers[aTransferNum].mSpecifiedRelURL + "\r\n").c_str());
			mFileTransfers[aTransferNum].Reset();
		}
		else
		{
			std::string aPath = mFileTransfers[aTransferNum].mSpecifiedRelURL;
			int aCurPos = 0;
			for (;;)
			{
				int aSlashPos = aPath.find("/", aCurPos);
				if (aSlashPos == -1)
					break;
				aCurPos = aSlashPos + 1;
				std::string aCurPath = aPath.substr(0, aSlashPos);
				mkdir(aCurPath.c_str());
			}
			OutputDebugString(StrFormat("AdFile: %s\r\n", aPath.c_str()).c_str());
			FILE* aFP = fopen(aPath.c_str(), "wb");
			if (aFP != NULL)
			{
				std::string aData = mFileTransfers[aTransferNum].GetContent();
				fwrite(aData.c_str(), 1, aData.size(), aFP);
				fclose(aFP);
			}
			mFileTransfers[aTransferNum].Reset();
		}
		if (aResultCode == HTTPTransfer::RESULT_NOT_COMPLETED)
		{
			if (mPendingResourceList.size())
			{
				std::string aFileName = mPendingResourceList.front();
				mPendingResourceList.pop_front();
				mFileTransfers[aTransferNum].Get(mVersionCheckTransfer.mURL, aFileName);
			}
		}
	}
}

const bool InternetManager::IsUpToDate() //489-494
{
	return !ConfirmParsed() || mIsUpToDate;
}

const std::string InternetManager::GetUpdateURL() //497-502
{
	if (ConfirmParsed())
		return mUpdateURL;
	else
		return "";
}

void InternetManager::StartTransfers() //505-521 (Correct?)
{
	ConfirmParsed();
	StringList::iterator anItr = mPendingResourceList.begin();
	while (anItr != mPendingResourceList.end())
	{
		std::string aPath = *anItr; //?
		if (gSexyAppBase->FileExists(aPath))
			mPendingResourceList.erase(anItr);
		anItr++;
	}
}

void InternetManager::StopTransfers() //524-525
{
}

void InternetManager::CheckAdResources(AdInstance& theInstance) //528-544
{
	AdInstance* aCurAdInstance = &theInstance;
	aCurAdInstance->mHasAllResources = true;
	StringList::iterator aStringItr = aCurAdInstance->mResourceList.begin();
	while (aStringItr != aCurAdInstance->mResourceList.end())
	{
		if (!gSexyAppBase->FileExists(*aStringItr))
			aCurAdInstance->mHasAllResources = false;
		aStringItr++;
	}
	if (!aCurAdInstance->mResourceList.size())
		aCurAdInstance->mHasAllResources = false;
}

void InternetManager::CheckAdResources() //547-554
{
	AdInstanceList::iterator anItr = mAdInstanceList.begin();
	while (anItr != mAdInstanceList.end())
	{
		CheckAdResources(*anItr);
		anItr++;
	}
}

bool InternetManager::HasNewAds(AdType theType, bool includeJustDownloaded) //557-575 (CORRECT?)
{
	CheckAdResources();
	AdInstanceList::iterator anItr = mAdInstanceList.begin();
	while (anItr != mAdInstanceList.end())
	{
		AdInstance* aCurAdInstance = &(*anItr);
		if (aCurAdInstance->mType == theType && aCurAdInstance->mHasAllResources && aCurAdInstance->mIsNew && (includeJustDownloaded || !aCurAdInstance->mJustDownloaded))
			return true;
		anItr++;
	}
	return false;
}

void InternetManager::TryShowAd() //Correct? | 578-674
{
#ifdef _WIN32 //This only appears on Windows.
	StopTransfers();

	CheckAdResources();

	int aNumActiveAds = 0;
	AdInstance* anAdInstance;
	AdInstanceList::iterator anItr = mAdInstanceList.begin();
	while (anItr != mAdInstanceList.end())
	{
		AdInstance* aCurAdInstance = &(*anItr);
		if (aCurAdInstance->mHasAllResources && aCurAdInstance->mType == AdType_Popup)
		{
			if (aCurAdInstance->mIsNew && !anAdInstance)
				anAdInstance = aCurAdInstance;

			++aNumActiveAds;
		}
		anItr++;
	}

	if (!anAdInstance && aNumActiveAds > 0)
	{
		int anIdx = Rand() % aNumActiveAds;
		while (anItr != mAdInstanceList.end())
		{
			anAdInstance = &(*anItr);
			if (anAdInstance->mHasAllResources && anAdInstance->mType == AdType_Popup && anIdx--)
				break;
			anItr++;
		}
	}

	if (!anAdInstance)
		return;

	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		IWebBrowser2* pWebBrowser = NULL;
		hr = CoCreateInstance(CLSID_InternetExplorer, NULL, 0x15, IID_IWebBrowser2, (void**)pWebBrowser); //What is 0x15?
		if (SUCCEEDED(hr))
		{
			DWORD aWindowStyle = 0x82CA0000;
			RECT aRect;
			aRect.left = 0;
			aRect.top = 0;
			aRect.right = anAdInstance->mWidth;
			aRect.bottom = anAdInstance->mHeight;
			BOOL worked = AdjustWindowRect(&aRect, aWindowStyle, FALSE);
			pWebBrowser->put_AddressBar(FALSE);
			pWebBrowser->put_MenuBar(FALSE);
			pWebBrowser->put_StatusBar(FALSE);
			pWebBrowser->put_ToolBar(FALSE);
			pWebBrowser->put_Resizable(FALSE);
			pWebBrowser->put_Width(aRect.right - aRect.left);
			pWebBrowser->put_Height(aRect.bottom - aRect.top);
			char aLocalURL[4096];
			//Correct?
			strcpy(aLocalURL, "file://");
			_getcwd(&aLocalURL[strlen(aLocalURL)], sizeof(aLocalURL));
			strcat(aLocalURL, "\\");
			strcat(aLocalURL, anAdInstance->mResourceList.front().c_str());
			wchar_t aStr[4096];
			mbstowcs(aStr, aLocalURL, sizeof(aLocalURL));
			BSTR aBString = SysAllocString(aStr);
			VARIANT vDummy; //?
			pWebBrowser->Navigate(aBString, &vDummy, &vDummy, &vDummy, &vDummy);
			SysFreeString(aBString);
			pWebBrowser->put_Visible(-1);
			pWebBrowser->Release();
		}
	}
	anAdInstance->mIsNew = false;
	
#endif
}

bool InternetManager::GetMoreGamesAds(MoreGamesAdContainer* theContainer, bool includeJustDownloaded) //705-747 (Correct?)
{
	typedef std::vector<AdInstance*> AdPList;
	AdPList aList[4];
	AdInstanceList::iterator anItr = mAdInstanceList.begin();
	theContainer->mAds.resize(mNumMoreGamesAds);
	while (anItr != mAdInstanceList.end())
	{
		AdInstance& anInstance = *anItr;
		CheckAdResources(anInstance);
		if (anInstance.mHasAllResources)
		{
			if (!anInstance.mJustDownloaded || includeJustDownloaded)
			{
				if (anInstance.mIsNew)
					aList->push_back(&anInstance);
				else if (anInstance.mRemove)
					aList[2].push_back(&anInstance);
				else
					aList[1].push_back(&anInstance);
			}
			else
				aList[3].push_back(&anInstance);
			anItr++;
		}
	}
	
	int anIndex = 0;
	for (int i = 0; i < sizeof(aList) / 16 && anIndex < mNumMoreGamesAds; i++)
	{
		if (aList->size() > mNumMoreGamesAds)
			random_shuffle(aList[i].begin(), aList[i].end());
		AdPList::iterator anItr2 = aList[i].begin();
		while (anItr2 != aList[i].end())
		{
			TryLoadMoreGamesAd(*anItr2, &theContainer->mAds[0], anIndex);
			anItr2++;
		}
	}
	return anIndex == mNumMoreGamesAds;
}