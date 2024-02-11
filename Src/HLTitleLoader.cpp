
#include "HLExporter.h"

HLTitleManager::HLTitleManager(const TCHAR* MapURL)
	: bInit(FALSE)
{
	TitleFileURL = FString::GetFilePathStr(MapURL);
	TitleFileURL = TitleFileURL.Left(TitleFileURL.Len() - 1);
	INT SepA = TitleFileURL.InStrRight(TEXT("\\"));
	INT SepB = TitleFileURL.InStrRight(TEXT("/"));
	if (SepA == INDEX_NONE || SepB > SepA)
		SepA = SepB;
	TitleFileURL = TitleFileURL.Left(SepA) * TEXT("titles.txt");
}
HLTitleManager::~HLTitleManager()
{
	for (TMap<FString, FTitleInfo*>::TConstIterator It(TitleMap); It; ++It)
		delete It.Value();
}
HLTitleManager::FTitleInfo* HLTitleManager::FindTitle(const TCHAR* Name)
{
	guard(HLTitleManager::FindTitle);
	if (!bInit)
		InitTitles();
	return TitleMap.FindRef(Name);
	unguard;
}

UBOOL HLTitleManager::TextStartsWith(const TCHAR*& Str, const TCHAR* Match)
{
	const TCHAR* S = Str;
	while (*Match)
	{
		if (*S != *Match)
			return FALSE;
		++S;
		++Match;
	}
	Str = S;
	return TRUE;
}
FLOAT HLTitleManager::ParseNextFloat(const TCHAR*& Str, const FLOAT Default)
{
	while (*Str == ' ')
		++Str;
	if (!*Str)
		return Default;
	const FLOAT Result = appAtof(Str);
	while (*Str == '-' || *Str == '+' || *Str == '.' || appIsDigit(*Str))
		++Str;
	while (*Str == ' ')
		++Str;
	return Result;
}
INT HLTitleManager::ParseNextInt(const TCHAR*& Str, const INT Default)
{
	while (*Str == ' ')
		++Str;
	if (!*Str)
		return Default;
	const INT Result = appAtoi(Str);
	while (*Str == '-' || *Str == '+' || appIsDigit(*Str))
		++Str;
	while (*Str == ' ')
		++Str;
	return Result;
}

void HLTitleManager::InitTitles()
{
	guard(HLTitleManager::InitTitles);
	bInit = TRUE;
	FString FileStr;
	if (!appLoadFileToString(FileStr, *TitleFileURL))
	{
		GWarn->Logf(TEXT("Failed to open title file: %ls"), *TitleFileURL);
		return;
	}

	FLOAT CurX = -1.f;
	FLOAT CurY = -1.f;
	FColor Color(255, 255, 255, 255);
	FColor HiColor(255, 255, 255, 255);
	FLOAT FadeIn = 0.1f;
	FLOAT FadeOut = 0.25f;
	FLOAT ScreenTime = 3.f;
	FLOAT HiTime = 0.25f;
	INT MsgType = 0;
	const TCHAR* Str = *FileStr;
	FString Line, MessageID;
	while (ParseLine(&Str, Line))
	{
		const TCHAR* StrL = *Line;
		while (*StrL == ' ')
			++StrL;
		if ((!*StrL) || TextStartsWith(StrL, TEXT("//")))
			continue;

		if (TextStartsWith(StrL, TEXT("$")))
		{
			if (TextStartsWith(StrL, TEXT("position ")))
			{
				CurX = ParseNextFloat(StrL, CurX);
				CurY = ParseNextFloat(StrL, CurY);
			}
			else if (TextStartsWith(StrL, TEXT("color ")))
			{
				Color.R = ParseNextInt(StrL, Color.R);
				Color.G = ParseNextInt(StrL, Color.G);
				Color.B = ParseNextInt(StrL, Color.B);
			}
			else if (TextStartsWith(StrL, TEXT("color2 ")))
			{
				HiColor.R = ParseNextInt(StrL, HiColor.R);
				HiColor.G = ParseNextInt(StrL, HiColor.G);
				HiColor.B = ParseNextInt(StrL, HiColor.B);
			}
			else if (TextStartsWith(StrL, TEXT("fadein ")))
				FadeIn = ParseNextFloat(StrL, FadeIn);
			else if (TextStartsWith(StrL, TEXT("fadeout ")))
				FadeOut = ParseNextFloat(StrL, FadeOut);
			else if (TextStartsWith(StrL, TEXT("holdtime ")))
				ScreenTime = ParseNextFloat(StrL, ScreenTime);
			else if (TextStartsWith(StrL, TEXT("effect ")))
				MsgType = ParseNextInt(StrL, MsgType);
			else if (TextStartsWith(StrL, TEXT("fxtime ")))
				HiTime = ParseNextFloat(StrL, HiTime);
		}
		else if (TextStartsWith(StrL, TEXT("{")))
		{
			FString InLine;
			FString MessageStr;
			while (ParseLine(&Str, InLine))
			{
				const TCHAR* inL = *InLine;
				if (TextStartsWith(inL, TEXT("}")))
					break;
				if (MessageStr.Len())
					MessageStr = MessageStr + TEXT("|") + InLine;
				else MessageStr = InLine;
			}
			if (MessageID.Len() && MessageStr.Len())
			{
				FTitleInfo* NewTitle = new FTitleInfo(MessageStr);
				NewTitle->XPos = CurX;
				NewTitle->YPos = CurY;
				NewTitle->Color = Color;
				NewTitle->HighlightColor = HiColor;
				NewTitle->FadeIn = FadeIn;
				NewTitle->FadeOut = FadeOut;
				NewTitle->DisplayTime = ScreenTime;
				NewTitle->HighlightFadeTime = HiTime;
				NewTitle->MsgType = MsgType;
				TitleMap.Set(*MessageID, NewTitle);
			}
			MessageID.Empty();
		}
		else MessageID = Line;
	}
	unguard;
}
