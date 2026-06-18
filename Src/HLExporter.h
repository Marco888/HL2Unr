#pragma once

#include "Engine.h"
#include "HLBSP.h"

constexpr INT DefaultLightRadius = 28;

struct FVoicePart
{
	FString SoundName;
	FLOAT Duration, Pitch, Volume;

	FVoicePart(const TCHAR* GroupName, const TCHAR* SndName, FLOAT inDur, FLOAT inPit, FLOAT inVol);
};
struct FVoiceLine
{
	TArray<FVoicePart*> Parts;
	FString LineName;

	FVoiceLine(const TCHAR* inLine);
	~FVoiceLine();
};
struct FVoiceFile
{
	TArray<FVoiceLine*> Lines;
	FString FileName;

	FVoiceFile(const TCHAR* inFile);
	~FVoiceFile() noexcept(false);
	UBOOL HasLine(const TCHAR* LineName) const
	{
		for (INT i = 0; i < Lines.Num(); i++)
			if (Lines(i)->LineName == LineName)
				return TRUE;
		return FALSE;
	}
};

class UHLWadExp : public UCommandlet
{
public:
	DECLARE_CLASS(UHLWadExp, UCommandlet, 0, HLWadExp);

	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};
class UHLTexImporter : public UCommandlet
{
public:
	DECLARE_CLASS(UHLTexImporter, UCommandlet, 0, HLWadExp);

	void StaticConstructor();
	INT Main(const TCHAR* Parms);

	static UTexture* ImportTexture(UPackage* InParent, const TCHAR* FileName);
	static void ConvertToPowTwo(FMipmap& Mip, TArray<FColor>& Palette, UBOOL bIsMasked, UBOOL bUseClosest = FALSE); // Convert texture to power of two.
	static void CenterToPowTwo(FMipmap& Mip, UBOOL bIsMasked); // Center bitmap to closest power of two.
	static UBOOL LoadTexture(FMipmap& OutMip, TArray<FColor>& OutPalette, UBOOL bIsMasked, const TCHAR* FileName); // Load RAW bmp file.
	static UBOOL ExportPCX(const FMipmap& Mip, const FColor* Palette, const TCHAR* FileName);
	static const TCHAR* MakeSafeName(const TCHAR* inName);
};
class UHLTexConverter : public UCommandlet
{
public:
	DECLARE_CLASS(UHLTexConverter, UCommandlet, 0, HLWadExp);

	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};
class UHLVoiceLineParser : public UCommandlet
{
public:
	DECLARE_CLASS(UHLVoiceLineParser, UCommandlet, 0, HLWadExp);

	void StaticConstructor();
	INT Main(const TCHAR* Parms);
};
