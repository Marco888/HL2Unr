
#include "HLExporter.h"

IMPLEMENT_PACKAGE(HLWadExp);
IMPLEMENT_CLASS(UHLWadExp);
IMPLEMENT_CLASS(UHLTexImporter);
IMPLEMENT_CLASS(UHLTexConverter);
IMPLEMENT_CLASS(UHLVoiceLineParser);

class FT3DOutputDevice : public FString, public FOutputDevice
{
	TCHAR Intendent[16];
	INT iIntend;
public:
	FT3DOutputDevice()
		: iIntend(0)
	{
		Intendent[0] = 0;
	}
	void Serialize(const TCHAR* Data, EName Event)
	{
		if (Event == NAME_Remove)
			Intendent[--iIntend] = 0;
		*this += Intendent;
		*this += (TCHAR*)Data;
		*this += TEXT("\r\n");
		if (Event == NAME_Add)
		{
			Intendent[iIntend++] = '\t';
			Intendent[iIntend] = 0;
		}
	}
};

void UHLWadExp::StaticConstructor()
{
	guard(UHLWadExp::StaticConstructor);
	HelpCmd = TEXT("hlWADexp");
	HelpOneLiner = TEXT("Convert Half-Life maps into Unreal packages");
	HelpUsage = TEXT("hlWADexp map.bsp -nomergesurfs -dumpents");
	HelpParm[0] = TEXT("BSP file");
	HelpDesc[0] = TEXT("Filename of the map file");
	IsServer = FALSE;
	IsClient = FALSE;
	IsEditor = TRUE;
	LazyLoad = FALSE;
	LogToStdout = FALSE;
	unguard;
}
INT UHLWadExp::Main(const TCHAR* Parms)
{
	guard(UHLWadExp::Main);

	if (*Parms == ' ')
		++Parms;
	TCHAR delimiter = ' ';
	if (*Parms == '\"' || *Parms == '\'')
	{
		delimiter = *Parms;
		++Parms;
	}
	const TCHAR* Start = Parms;
	while (*Parms && *Parms != delimiter)
		++Parms;
	FString FileURL(Start, Parms);

	FArchive* Ar = GFileManager->CreateFileReader(*FileURL);
	if (!Ar)
	{
		GWarn->Logf(TEXT("Failed to locate BSP: %ls"), *FileURL);
		return 1;
	}

	GWarn->Logf(TEXT("Open file: %ls (Size %i)"), *FileURL, Ar->TotalSize());

	CMapLoadHelper Loader(*Ar, *FileURL);
	if (Loader.Load())
	{
		FT3DOutputDevice Out;
		Loader.ExportMap(Out);

		FString OutFile = FString::Printf(TEXT("../%ls.t3d"), Loader.GetMapName());
		appSaveStringToFile(Out, *OutFile);
		GWarn->Logf(TEXT("Wrote output map: %ls"), *OutFile);
	}
	delete Ar;
	return 0;
	unguard;
}

const TCHAR* GetCCString(const char* InChr, const INT Len)
{
	TCHAR* Result = appStaticString1024();
	for (INT i = 0; i < Len; ++i)
		Result[i] = InChr[i];
	Result[Len] = 0;
	return Result;
}

void UHLTexImporter::StaticConstructor()
{
	guard(UHLTexImporter::StaticConstructor);
	HelpCmd = TEXT("hlTexImporter");
	HelpOneLiner = TEXT("Import extracted Half-Life texture bmp's into Unreal package.");
	HelpUsage = TEXT("HLTexFolder OutputName.utx");
	HelpParm[0] = TEXT("Texture folder");
	HelpDesc[0] = TEXT("Filename of the map file");
	IsServer = FALSE;
	IsClient = FALSE;
	IsEditor = TRUE;
	LazyLoad = FALSE;
	LogToStdout = FALSE;
	unguard;
}
INT UHLTexImporter::Main(const TCHAR* Parms)
{
	guard(UHLTexImporter::Main);
	FString BasePath = ParseToken(Parms, FALSE);
	while (*Parms == ' ')
		++Parms;
	FString OutFile = FString(Parms);

	if (!BasePath.Len() || !OutFile.Len())
	{
		GWarn->Logf(TEXT("Missing input path or output filename parm!"));
		return 1;
	}

	StaticLoadObject(UClass::StaticClass(), NULL, TEXT("Engine.Texture"), NULL, LOAD_NoFail, NULL);
	UPackage* BasePack = CreatePackage(NULL, *OutFile);
	{
		TArray<FString> Files = GFileManager->FindFiles(*(BasePath * TEXT("*.bmp")), TRUE, FALSE);
		for (INT i = 0; i < Files.Num(); ++i)
		{
			GWarn->Logf(NAME_Title, TEXT("Import main path %i/%i"), i, Files.Num());
			FString File = BasePath * Files(i);
			ImportTexture(BasePack, *File);
		}
	}
	{
		TArray<FString> Paths = GFileManager->FindFiles(*(BasePath * TEXT("*")), FALSE, TRUE);
		UPackage* GroupPck = NULL;
		INT j;
		TCHAR TempStr[NAME_SIZE + 1];
		for (INT i = 0; i < Paths.Num(); ++i)
		{
			GWarn->Logf(NAME_Progress, TEXT("Import from path %ls (%i/%i)"), *Paths(i), i, Paths.Num());
			GroupPck = NULL;
			FString LookPath = BasePath * Paths(i) + PATH_SEPARATOR;
			TArray<FString> Files = GFileManager->FindFiles(*(LookPath + TEXT("*.bmp")), TRUE, FALSE);
			TArray<UTexture*> ImportedTex;
			TArray<FName> OrgNames;
			TMap<FName, UTexture*> TexMap;
			for (j = 0; j < Files.Num(); ++j)
			{
				GWarn->Logf(NAME_Title, TEXT("Importing path %i/%i"), j, Files.Num());
				if (!GroupPck)
					GroupPck = CreatePackage(BasePack, *Paths(i));
				const TCHAR* orgName = *Files(j);
				FString File = LookPath + Files(j);
				UTexture* T = ImportTexture(GroupPck, *File);
				if (T && orgName[0] == '+')
				{
					FName OrgFName(orgName);
					TexMap.Set(OrgFName, T);
					if (orgName[1] == '0' || (appToUpper(orgName[1]) == 'A' && orgName[2] == '0'))
					{
						OrgNames.AddItem(OrgFName);
						ImportedTex.AddItem(T);
					}
				}
			}

			// Check for animations.
			GWarn->Logf(NAME_Title, TEXT("Parsing animations..."));
			for (j = 0; j < ImportedTex.Num(); ++j)
			{
				const TCHAR* TN = *OrgNames(j);
				appStrcpy(TempStr, TN);
				UTexture* PrevAnim = ImportedTex(j);
				INT numOffset = 1;
				if (appToUpper(TempStr[1]) == 'A')
					++numOffset;

				for (INT z = 1; z < 10; ++z)
				{
					TempStr[numOffset] = '0' + z;
					FName FindTex(TempStr, FNAME_Find);
					if (FindTex == NAME_None)
						break;
					UTexture* AnimNext = TexMap.FindRef(FindTex);
					if (!AnimNext)
						break;
					PrevAnim->AnimNext = AnimNext;
					PrevAnim->MaxFrameRate = 10.f;
					PrevAnim = AnimNext;
				}
			}
		}
	}
	FString SaveFile = FString::Printf(TEXT("../Textures/%ls.utx"), *OutFile);
	UObject::SavePackage(BasePack, NULL, RF_Standalone, *SaveFile);
	return 0;
	unguard;
}

void UHLTexConverter::StaticConstructor()
{
	guard(UHLTexConverter::StaticConstructor);
	HelpCmd = TEXT("hlTexConverter");
	HelpOneLiner = TEXT("Convert skin bmp's with odd resolutions and spews out pcx with Unreal resolution.");
	HelpUsage = TEXT("HLTexFolder");
	HelpParm[0] = TEXT("Texture folder");
	HelpDesc[0] = TEXT("The folder with all of the bmp files");
	IsServer = FALSE;
	IsClient = FALSE;
	IsEditor = TRUE;
	LazyLoad = FALSE;
	LogToStdout = FALSE;
	unguard;
}
INT UHLTexConverter::Main(const TCHAR* Parms)
{
	guard(UHLTexConverter::Main);
	FString BasePath = ParseToken(Parms, FALSE);
	if (!BasePath.Len())
	{
		GWarn->Logf(TEXT("Missing input path!"));
		return 1;
	}
	BasePath *= TEXT("");
	UBOOL bCenter = ParseParam(Parms, TEXT("C"));
	FString SeekWildcard = BasePath + TEXT("*.bmp");
	TArray<FString> Files = GFileManager->FindFiles(*SeekWildcard, TRUE, FALSE);
	warnf(TEXT("Seeking files %ls: %i files found (center %ls)"), *SeekWildcard, Files.Num(), bCenter ? GTrue : GFalse);

	for (INT i = 0; i < Files.Num(); ++i)
	{
		debugf(TEXT("Processing %ls..."), *Files(i));
		warnf(NAME_Progress, TEXT("Processing %ls..."), *Files(i));
		FString FullFilepath = BasePath + Files(i);

		FMipmap TempMip;
		TArray<FColor> TempPalette;
		if (!UHLTexImporter::LoadTexture(TempMip, TempPalette, FALSE, *FullFilepath))
			continue;

		FString Outfile = BasePath + Files(i).GetFilenameOnly() + TEXT(".pcx");
		if (bCenter)
			UHLTexImporter::CenterToPowTwo(TempMip, FALSE);
		else UHLTexImporter::ConvertToPowTwo(TempMip, TempPalette, FALSE, TRUE);
		UHLTexImporter::ExportPCX(TempMip, &TempPalette(0), *Outfile);
		warnf(TEXT("Write output: %ls"), *Outfile);
	}
	return 0;
	unguard;
}
