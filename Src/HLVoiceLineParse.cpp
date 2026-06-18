#include "HLExporter.h"

static UObject* SoundParentPackage = NULL;

void UHLVoiceLineParser::StaticConstructor()
{
	guard(UHLVoiceLineParser::StaticConstructor);
	HelpCmd = TEXT("hlVoiceLineParser");
	HelpOneLiner = TEXT("Load HL's sentences.txt file and generate UC defaultproperties into UCC.log file.");
	HelpUsage = TEXT("HLVoiceLineParser sentences.txt");
	HelpParm[0] = TEXT("Sentences file");
	HelpDesc[0] = TEXT("Input sentences file");
	IsServer = FALSE;
	IsClient = FALSE;
	IsEditor = TRUE;
	LazyLoad = FALSE;
	LogToStdout = FALSE;
	unguard;
}

FVoicePart::FVoicePart(const TCHAR* GroupName, const TCHAR* SndName, FLOAT inDur, FLOAT inPit, FLOAT inVol)
	: SoundName(FString::Printf(TEXT("%ls.%ls"), GroupName, SndName)), Duration(inDur), Pitch(inPit), Volume(inVol)
{
	if (SoundParentPackage)
	{
		UObject* found = UObject::StaticFindObject(USound::StaticClass(), SoundParentPackage, *SoundName);
		if (found)
			SoundName = found->GetPathName();
		else SoundName = FString(SoundParentPackage->GetName()) + TEXT(".") + SoundName;
	}
}

FVoiceLine::FVoiceLine(const TCHAR* inLine)
	: LineName(FString::Printf(TEXT("VX_%ls"), inLine))
{}
FVoiceLine::~FVoiceLine()
{
	for (INT i = 0; i < Parts.Num(); ++i)
		delete Parts(i);
}

inline UBOOL IsWhiteSpace(TCHAR S)
{
	return (S == ' ' || S == '\t');
}

struct FLineModifier
{
	FLOAT Duration, Pitch, Volume;

	FLineModifier()
		: Duration(1.f), Pitch(1.f), Volume(1.f)
	{}
	void ApplyModifier(FVoicePart& Vox)
	{
		if (Duration > 0.f)
			Vox.Duration = Duration;
		if (Pitch > 0.f)
			Vox.Pitch = Pitch;
		if (Volume > 0.f)
			Vox.Volume = Volume;
	}
	void ApplyModifier(FLineModifier& Mod)
	{
		if (Duration > 0.f)
			Mod.Duration = Duration;
		if (Pitch > 0.f)
			Mod.Pitch = Pitch;
		if (Volume > 0.f)
			Mod.Volume = Volume;
	}
	void Reset()
	{
		Duration = Pitch = Volume = -1.f;
	}
	UBOOL ParseModifier(const TCHAR*& Str)
	{
		if (*Str != '(')
			return FALSE;
		Reset();
		++Str;
		while (*Str)
		{
			TCHAR type = *Str;
			++Str;
			INT iValue = appAtoi(Str);
			if (type == 'p')
				Pitch = static_cast<FLOAT>(iValue) / 100.f;
			else if (type == 'v')
				Volume = static_cast<FLOAT>(iValue) / 100.f;
			else if (type == 'e')
				Duration = static_cast<FLOAT>(iValue) / 100.f;
			while (appIsDigit(*Str))
				++Str;
			while (*Str == ' ')
				++Str;
			if (*Str == ')')
			{
				++Str;
				break;
			}
		}
		return TRUE;
	}
};
FVoiceFile::FVoiceFile(const TCHAR* inFile)
	: FileName(inFile)
{
	guard(FVoiceFile::FVoiceFile);
	FString FileTxt;
	if (!appLoadFileToString(FileTxt, inFile))
	{
		GWarn->Logf(TEXT("Voice file '%ls' not found."), inFile);
		return;
	}

	const TCHAR* current = *FileTxt;
	FString Line;
	while (ParseLine(&current, Line))
	{
		const TCHAR* Str = *Line;
		const TCHAR* Start = Str;
		while (*Str && !IsWhiteSpace(*Str))
			++Str;
		if (!*Str)
			continue;
		FString lineName(Start, Str);
		FString curGroup(TEXT("vox"));
		FLineModifier PendingModifier, CurrentModifier;

		FVoiceLine* newLine = new FVoiceLine(*lineName);
		Lines.AddItem(newLine);

		while (*Str)
		{
			while (IsWhiteSpace(*Str))
				++Str;
			if (PendingModifier.ParseModifier(Str))
			{
				PendingModifier.ApplyModifier(CurrentModifier);
				continue;
			}
			if (*Str == ',' || *Str == '.')
			{
				FString punc((*Str == ',') ? TEXT("_comma") : TEXT("_period"));
				++Str;
				FVoicePart* newPart = new FVoicePart(*curGroup, *punc, CurrentModifier.Duration, CurrentModifier.Pitch, CurrentModifier.Volume);
				newLine->Parts.AddItem(newPart);
				continue;
			}
			Start = Str;
			while (*Str && !IsWhiteSpace(*Str) && *Str != '(' && *Str != ',' && *Str != '.')
				++Str;

			FString section(Start, Str);
			INT iSplit = section.InStr(TEXT("/"));
			if (iSplit != INDEX_NONE)
			{
				curGroup = section.Left(iSplit);
				section = section.Mid(iSplit + 1);
			}
			if (section.Len() == 0)
			{
				if (PendingModifier.ParseModifier(Str))
					PendingModifier.ApplyModifier(CurrentModifier);
				continue;
			}
			FVoicePart* newPart = new FVoicePart(*curGroup, *section, CurrentModifier.Duration, CurrentModifier.Pitch, CurrentModifier.Volume);
			newLine->Parts.AddItem(newPart);
			if (PendingModifier.ParseModifier(Str))
				PendingModifier.ApplyModifier(*newPart);
		}
		if (newLine->Parts.Num() == 0)
		{
			auto* last = Lines.Pop();
			delete last;
		}
	}
	unguard;
}
FVoiceFile::~FVoiceFile() noexcept(false)
{
	guard(FVoiceFile::~FVoiceFile);
	for (INT i = 0; i < Lines.Num(); ++i)
		delete Lines(i);
	unguard;
}

INT UHLVoiceLineParser::Main(const TCHAR* Parms)
{
	guard(UHLVoiceLineParser::Main);
	SoundParentPackage = LoadPackage(NULL, TEXT("HL_Ambience"), LOAD_NoWarn);

	FVoiceFile VoiceFile(Parms);

	FStringOutputDevice Out(TEXT("defaultproperties\r\n{\r\n"));
	for (INT i = 0; i < VoiceFile.Lines.Num(); ++i)
	{
		const auto* VX = VoiceFile.Lines(i);
		Out.Logf(TEXT("\tBEGIN OBJECT CLASS=VoiceLineObject NAME=%ls\r\n"), *VX->LineName);
		for (INT j = 0; j < VX->Parts.Num(); ++j)
		{
			const auto& P = *VX->Parts(j);
			Out.Logf(TEXT("\t\tLine(%i)=(Sound=Sound'%ls',Duration=%.2f,Pitch=%.2f,Volume=%.2f)\r\n"), j, *P.SoundName, P.Duration, P.Pitch, P.Volume);
		}
		Out.Log(TEXT("\tEND OBJECT\r\n"));
	}
	for (INT i = 0; i < VoiceFile.Lines.Num(); ++i)
	{
		const auto* VX = VoiceFile.Lines(i);
		Out.Logf(TEXT("\tVox(%i)=%ls\r\n"), i, *VX->LineName);
	}
	Out.Log(TEXT("}\r\n"));
	GLog->Log(Out);
	GWarn->Logf(TEXT("Exported %i sound lines"), VoiceFile.Lines.Num());
	return 0;
	unguard;
}
