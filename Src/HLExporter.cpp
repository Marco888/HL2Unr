
#include "HLExporter.h"

IMPLEMENT_PACKAGE(HLWadExp);
IMPLEMENT_CLASS(UHLWadExp);
IMPLEMENT_CLASS(UHLTexImporter);

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
	HelpUsage = TEXT("hlWADexp map.bsp");
	HelpParm[0] = TEXT("BSP file");
	HelpDesc[0] = TEXT("Filename of the map file");
	IsServer = FALSE;
	IsClient = FALSE;
	IsEditor = TRUE;
	LazyLoad = FALSE;
	//LogToStdout = TRUE;
	unguard;
}
INT UHLWadExp::Main(const TCHAR* Parms)
{
	guard(UHLWadExp::Main);
	FArchive* Ar = GFileManager->CreateFileReader(Parms);
	if (!Ar)
	{
		GWarn->Logf(TEXT("Failed to locate BSP: %ls"), Parms);
		return 1;
	}

	GWarn->Logf(TEXT("Open file: %ls (Size %i)"), Parms, Ar->TotalSize());

	CMapLoadHelper Loader(*Ar, Parms);
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
		for (INT i = 0; i < Paths.Num(); ++i)
		{
			GWarn->Logf(NAME_Progress, TEXT("Import from path %ls (%i/%i)"), *Paths(i), i, Paths.Num());
			GroupPck = NULL;
			FString LookPath = BasePath * Paths(i) + PATH_SEPARATOR;
			TArray<FString> Files = GFileManager->FindFiles(*(LookPath + TEXT("*.bmp")), TRUE, FALSE);
			TArray<UTexture*> ImportedTex;
			TMap<FName, UTexture*> TexMap;
			for (j = 0; j < Files.Num(); ++j)
			{
				GWarn->Logf(NAME_Title, TEXT("Importing path %i/%i"), j, Files.Num());
				if (!GroupPck)
					GroupPck = CreatePackage(BasePack, *Paths(i));
				FString File = LookPath + Files(j);
				UTexture* T = ImportTexture(GroupPck, *File);
				if (T)
				{
					TexMap.Set(T->GetFName(), T);
					ImportedTex.AddItem(T);
				}
			}

			// Check for animations.
			GWarn->Logf(NAME_Title, TEXT("Parsing animations..."));
			for (j = 0; j < ImportedTex.Num(); ++j)
			{
				const TCHAR* TN = ImportedTex(j)->GetName();
				const INT ln = appStrlen(TN);
				if (ln <= 4)
					continue;
				if (TN[ln - 3] == '_' && TN[ln - 2] == '0' && TN[ln - 1] == '1')
				{
					FString MainTex(TN, &TN[ln - 3]);
					UTexture* T = TexMap.FindRef(*MainTex);
					if (T)
					{
						//debugf(TEXT("NEXTANIM %ls -> %ls"), T->GetFullName(), ImportedTex(j)->GetFullName());
						T->AnimNext = ImportedTex(j);
						T->MaxFrameRate = 15.f;
						T = ImportedTex(j);

						for (INT z = 2; ; ++z)
						{
							FString NextName = MainTex + FString::Printf(TEXT("_%02i"), z);
							UTexture* NT = TexMap.FindRef(*NextName);
							//debugf(TEXT("+NEXTANIM %ls -> %ls (%ls)"), T->GetFullName(), NT->GetFullName(), *NextName);
							if (NT)
							{
								T->AnimNext = NT;
								T = NT;
							}
							else break;
						}
					}
				}
			}
		}
	}
	FString SaveFile = FString::Printf(TEXT("../Textures/%ls.utx"), *OutFile);
	UObject::SavePackage(BasePack, NULL, RF_Standalone, *SaveFile);
	return 0;
	unguard;
}
