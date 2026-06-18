
#include "HLExporter.h"

struct CModelBuilder;

static INT SwitchMatIndex = 0;
constexpr INT ExpIDBSPHead = IDBSPHEADER;

static FVoiceFile* ActiveVoiceFile = NULL;

enum EExpEntFlags : DWORD
{
	EXP_None = 0,
	EXP_Mover = 0x0001,
	EXP_TriggerLight = 0x0002,
	EXP_Sound = 0x0004,
	EXP_Animator = 0x0008,
	EXP_Ignore = 0x0010,
	EXP_BrushSeperate = 0x0020,
	EXP_SkipEvents = 0x0040,
	EXP_PreCondition = 0x0080,
	EXP_Inventory = 0x0100,
	EXP_PostCondition = 0x0200,
	EXP_CheckTriggerTex = 0x0400,
};

static const TCHAR* GetUCClassName(const TCHAR* testClass);

static FVector GetVectorOf(const TCHAR* Str)
{
	FVector Result(0, 0, 0);
	FLOAT* fRes = &Result.X;
	for (INT i = 0; i < 3; ++i)
	{
		const TCHAR* Start = Str;
		while (appIsDigit(*Str) || *Str == '-' || *Str == '+' || *Str == '.')
			++Str;
		FString Temp(Start, Str);
		fRes[i] = appAtof(*Temp);
		while (*Str == ' ')
			++Str;
		if (!*Str)
			break;
	}
	return Result;
}
inline INT ToUnrealAngles(const INT InAng)
{
	return appRound(DOUBLE(InAng) * (65536.0 / 360.0));
}

CMapLoadHelper::CMapLoadHelper(FArchive& InAr, const TCHAR* inFileURL)
	: Loader(InAr), MapFile(FString::GetFilenameOnlyStr(inFileURL)), MapFileURL(inFileURL), iModelIndex(2), Titles(inFileURL)
{
	bMergeSurfs = (appStrfind(appCmdLine(), TEXT("-NOMERGESURFS")) == NULL);
	bDumpEnts = (appStrfind(appCmdLine(), TEXT("-DUMPENTS")) != NULL);
}

UBOOL CMapLoadHelper::Load()
{
	guard(CMapLoadHelper::Load);
	Loader << bspHeader;
	if (bspHeader.nVersion != 30)
	{
		GWarn->Logf(TEXT("CMapLoadHelper::Init, map has wrong version (got %i expected 30)!"), bspHeader.nVersion);
		return FALSE;
	}

#if 0
	for (INT i = 0; i < ARRAY_COUNT(BSPHEADER::lump); ++i)
	{
		debugf(TEXT("HeaderLump[%i] offset %i size %i"), i, bspHeader.lump[i].nOffset, bspHeader.lump[i].nLength);
	}
#endif

	LoadEntities();
	LoadPlanes();
	LoadTextures();
	LoadTexInfos();
	LoadVertices();
	LoadSurfs();
	LoadEdges();
	LoadSurfEdges();
	LoadModels();

	if (TexLightMap.Num() == 0)
	{
		TexLightMap.Set(FName(TEXT("ps_0_s_TNNL_LGT4")), FColor(239, 228, 176, 168));
		TexLightMap.Set(FName(TEXT("Red")), FColor(255, 0, 0, 180));
		TexLightMap.Set(FName(TEXT("ps_0_s_TNNL_LGT1")), FColor(204, 255, 204, 196));
		TexLightMap.Set(FName(TEXT("ps_0_s_LIGHT2A")), FColor(255, 255, 179, 200));
		TexLightMap.Set(FName(TEXT("ps_0_s_FIFTS_LGHT01")), FColor(230, 255, 254, 194));
		TexLightMap.Set(FName(TEXT("SKKYLITE")), FColor(230, 254, 255, 180));
		TexLightMap.Set(FName(TEXT("ps_0_s_FIFTIES_LGT2")), FColor(230, 255, 253, 196));
		TexLightMap.Set(FName(TEXT("_s_LIGHT3C")), FColor(239, 228, 176, 200));
		TexLightMap.Set(FName(TEXT("_s_SPOTBLUE")), FColor(130, 255, 255, 220));
		TexLightMap.Set(FName(TEXT("ps_0_s_LIGHT6A")), FColor(255, 64, 64, 180));
		TexLightMap.Set(FName(TEXT("ps_0LAB1_W7")), FColor(221, 255, 255, 250));
		TexLightMap.Set(FName(TEXT("ps_0_s_GENERIC86")), FColor(255, 255, 128, 164));
		TexLightMap.Set(FName(TEXT("_s_SPOTYELLOW")), FColor(255, 255, 128, 64));
		TexLightMap.Set(FName(TEXT("ps_0_s_FIFTS_LGHT4")), FColor(255, 255, 255, 64));
		TexLightMap.Set(FName(TEXT("LITEPANEL1")), FColor(128, 255, 255, 64));
		TexLightMap.Set(FName(TEXT("ps_0_s_GENERIC85")), FColor(128, 255, 255, 64));
		TexLightMap.Set(FName(TEXT("ps_0_s_FIFTS_LGHT5")), FColor(255, 255, 255, 80));
		TexLightMap.Set(FName(TEXT("_s_LIGHT5F")), FColor(255, 255, 180, 80));
		TexLightMap.Set(FName(TEXT("ps_0_s_LIGHT1")), FColor(128, 255, 255, 86));
		TexLightMap.Set(FName(TEXT("ps_0LAB1_W6")), FColor(200, 248, 255, 142));
		TexLightMap.Set(FName(TEXT("FLATBED_HLITE2")), FColor(255, 255, 186, 64));
		TexLightMap.Set(FName(TEXT("ps_A_s_FIFTIES_LGT2")), FColor(255, 255, 255, 96));
		TexLightMap.Set(FName(TEXT("ps_0_s_GENERIC86R")), FColor(255, 32, 32, 64));
	}

	if(bDumpEnts)
		EntData.DumpEntities();
	EntData.StripBogusEnts();
	return TRUE;
	unguard;
}

static UBOOL ParseNextValue(const TCHAR*& Str, FString& Value)
{
	while (*Str && *Str != '\"')
		++Str;
	if (*Str != '\"')
		return FALSE;
	++Str;
	const TCHAR* Start = Str;
	while (*Str && *Str != '\"')
		++Str;
	if (*Str != '\"')
		return FALSE;
	Value = FString(Start, Str);
	++Str;
	return TRUE;
}

void FEntListData::Clear()
{
	guard(FEntListData::Clear);
	for (INT i = 0; i < EntList.Num(); ++i)
		delete EntList(i);
	EntList.Empty();
	unguard;
}
void FEntListData::LoadFrom(const TCHAR* String)
{
	guard(FEntListData::LoadFrom);
	FEntity* Entity = NULL;
	FString Line;
	while (ParseLine(&String, Line))
	{
		if (Line == TEXT("{"))
		{
			Entity = new FEntity;
			EntList.AddItem(Entity);
		}
		else if (Line == TEXT("}"))
		{
			if (Entity)
				Entity->Init();
			// End entity.
			Entity = NULL;
		}
		else if (!Entity)
			GWarn->Logf(TEXT("Invalid data when loading entitys (no entity selected): %ls"), *Line);
		else
		{
			const TCHAR* Str = *Line;
			FString Key, Value;
			if (!ParseNextValue(Str, Key) || !ParseNextValue(Str, Value))
			{
				GWarn->Logf(TEXT("Invalid data when loading entitys (invalid key or value): %ls"), *Line);
				continue;
			}
			Entity->KeyMap.Set(FName(*Key), *Value);
		}
	}
	if (Entity)
		Entity->Init();
	unguard;
}
void FEntListData::StripBogusEnts()
{
	guard(FEntListData::StripBogusEnts);
	for (INT i = (EntList.Num() - 1); i >= 0; --i)
	{
		if (EntList(i)->GetClass() == NAME_None)
			EntList.Remove(i);
	}
	unguard;
}
void FEntListData::DumpEntities() const
{
	guard(FEntListData::DumpEntities);
	debugf(TEXT("===== LIST OF ENTITIES ====="));
	FName ClassName(TEXT("classname"));
	for (INT i = 0; i < EntList.Num(); ++i)
	{
		const FEntity& Entity = *EntList(i);
		if (Entity.ClassName != NAME_None)
			debugf(TEXT(" Class: '%ls'"), *Entity.ClassName);
		else debugf(TEXT(" Class: <UNKNOWN CLASS>"));

		for (TMap<FName, FString>::TConstIterator It(Entity.KeyMap); It; ++It)
		{
			if (It.Key() != ClassName)
				debugf(TEXT("   '%ls'='%ls'"), *It.Key(), *It.Value());
		}
	}
	debugf(TEXT("============================"));
	unguard;
}
FEntListData::FEntity* FEntListData::FindEntity(const TCHAR* EntName, const TCHAR* EventName, const TCHAR* EventKeyName) const
{
	guard(FEntListData::FindEntity);
	FName nEntName(EntName, FNAME_Find);
	if (nEntName != NAME_None)
	{
		for (INT i = 0; i < EntList.Num(); ++i)
		{
			if (EntList(i)->ClassName == nEntName)
			{
				if (EventKeyName)
				{
					auto* eventStr = EntList(i)->Value(EventKeyName);
					if (!eventStr || appStricmp(eventStr, EventName))
						continue;
				}
				return EntList(i);
			}
		}
	}
	return nullptr;
	unguard;
}
void FEntListData::FEntity::Init()
{
	guardSlow(FEntity::Init);
	FString* Res = KeyMap.Find(TEXT("spawnflags"));
	if (Res)
		SpawnFlags = Res->Int();
	Res = KeyMap.Find(TEXT("classname"));
	if (Res)
		ClassName = FName(**Res);

	Res = KeyMap.Find(TEXT("origin"));
	if (Res)
		Location = ToUnrealScale(GetVectorOf(**Res));

	Res = KeyMap.Find(TEXT("angles"));
	if (Res)
	{
		const TCHAR* Str = **Res;
		Rotation.Pitch = ToUnrealAngles(HLTitleManager::ParseNextInt(Str, 0));
		Rotation.Yaw = ToUnrealAngles(HLTitleManager::ParseNextInt(Str, 0));
		Rotation.Roll = ToUnrealAngles(HLTitleManager::ParseNextInt(Str, 0));
		INT iPitch = IntValue(TEXT("pitch"));
		if (iPitch != INT_NF)
			Rotation.Pitch = ToUnrealAngles(iPitch);
	}
	else
	{
		INT i = IntValue(TEXT("angle"));
		if (i != INT_NF)
			Rotation.Yaw = -ToUnrealAngles(i);
		i =	IntValue(TEXT("pitch"));
		if (i != INT_NF)
			Rotation.Pitch = ToUnrealAngles(i);
	}
	unguardSlow;
}

void CMapLoadHelper::LoadEntities()
{
	guard(CMapLoadHelper::LoadEntities);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_ENTITIES];
	Loader.Seek(Lump.nOffset);
	TCHAR* WStr;

	{
		FScopedMemMark Mark(GMem);
		ANSICHAR* Str = New<ANSICHAR>(GMem, Lump.nLength + 1);
		Loader.Serialize(Str, Lump.nLength);
		Str[Lump.nLength] = 0;
		WStr = appFromAnsi(Str);
	}

	EntData.LoadFrom(WStr);
	unguard;
}

void CMapLoadHelper::LoadPlanes()
{
	guard(CMapLoadHelper::LoadPlanes);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_PLANES];
	Loader.Seek(Lump.nOffset);

	if (Lump.nLength % dplane_t_size)
	{
		GWarn->Logf(TEXT("CollisionBSPData_LoadPlanes: funny lump size"));
	}

	const INT count = Lump.nLength / dplane_t_size;

	if (count < 1)
		GWarn->Logf(TEXT("Map with no planes"));

	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
	{
		GWarn->Logf(TEXT("Map has too many planes (%i/%i)"), count, MAX_MAP_PLANES);
	}

	map_planes.SetSize(count);
	for (INT i = 0; i < count; i++)
		Loader << map_planes(i);
	unguard;
}

CMapLoadHelper::FMapTex::FMapTex(const TCHAR* InName, INT inX, INT inY)
	: Name(UHLTexImporter::MakeSafeName(InName)), OrgX(FLOAT(inX)), OrgY(FLOAT(inY)), ScaleX(1.f), ScaleY(1.f)
{
	INT rX = FNextPowerOfTwo(inX);
	INT rY = FNextPowerOfTwo(inY);
	X = FLOAT(rX);
	Y = FLOAT(rY);
	if (inX != rX)
		ScaleX = FLOAT(rX) / FLOAT(inX);
	if (inY != rY)
		ScaleY = FLOAT(rY) / FLOAT(inY);
}

void CMapLoadHelper::LoadTextures()
{
	guard(CMapLoadHelper::LoadTextures);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_TEXDATA];
	Loader.Seek(Lump.nOffset);
	
	INT i;
	INT nMipTextures; // Number of BSPMIPTEX structures
	TArray<INT> BSPMIPTEXOFFSET;

	Loader << nMipTextures;

	BSPMIPTEXOFFSET.SetSize(nMipTextures);
	for (i = 0; i < nMipTextures; ++i)
		Loader << BSPMIPTEXOFFSET(i);

	map_tex.Empty(nMipTextures);
	BSPMIPTEX Head;
	for (i = 0; i < nMipTextures; ++i)
	{
		Loader.Seek(Lump.nOffset + BSPMIPTEXOFFSET(i));
		Loader << Head;

		new(map_tex) FMapTex(Head.GetName(), Head.nWidth, Head.nHeight);
	}
	unguard;
}

void CMapLoadHelper::LoadVertices()
{
	guard(CMapLoadHelper::LoadVertices);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_VERTEXES];
	Loader.Seek(Lump.nOffset);
	if (Lump.nLength % vertex_size)
	{
		GWarn->Logf(TEXT("Mod_LoadVertices: funny lump size"));
	}
	const INT count = Lump.nLength / vertex_size;
	map_verts.SetSize(count);

	FVector V;
	for (INT i = 0; i < count; ++i)
	{
		Loader << V;
		map_verts(i) = ToUnrealScale(V);
	}
	unguard;
}

void CMapLoadHelper::LoadSurfs()
{
	guard(CMapLoadHelper::LoadSurfs);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_FACES];
	Loader.Seek(Lump.nOffset);
	if (Lump.nLength % BSPFACE_size)
	{
		GWarn->Logf(TEXT("Mod_LoadSurfs: funny lump size"));
	}
	const INT count = Lump.nLength / BSPFACE_size;
	map_surfs.SetSize(count);

	for (INT i = 0; i < count; ++i)
		Loader << map_surfs(i);
	unguard;
}

void CMapLoadHelper::LoadEdges()
{
	guard(CMapLoadHelper::LoadEdges);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_EDGES];
	Loader.Seek(Lump.nOffset);
	if (Lump.nLength % BSPEDGE_size)
	{
		GWarn->Logf(TEXT("Mod_LoadEdges: funny lump size"));
	}
	const INT count = Lump.nLength / BSPEDGE_size;
	map_edges.SetSize(count);

	for (INT i = 0; i < count; ++i)
		Loader << map_edges(i);
	unguard;
}

void CMapLoadHelper::LoadSurfEdges()
{
	guard(CMapLoadHelper::LoadSurfEdges);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_SURFEDGES];
	Loader.Seek(Lump.nOffset);
	if (Lump.nLength % sizeof(INT))
	{
		GWarn->Logf(TEXT("Mod_LoadSurfEdges: funny lump size"));
	}
	const INT count = Lump.nLength / sizeof(INT);
	map_surfedges.SetSize(count);

	for (INT i = 0; i < count; ++i)
		Loader << map_surfedges(i);
	unguard;
}

void CMapLoadHelper::LoadModels()
{
	guard(CMapLoadHelper::LoadModels);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_MODELS];
	Loader.Seek(Lump.nOffset);
	if (Lump.nLength % BSPMODEL_size)
	{
		GWarn->Logf(TEXT("Mod_LoadModels: funny lump size"));
	}
	const INT count = Lump.nLength / BSPMODEL_size;
	map_models.SetSize(count);

	//debugf(TEXT("BSPmodels %i"), count);
	for (INT i = 0; i < count; ++i)
	{
		Loader << map_models(i);
		//debugf(TEXT("Model(%i) nFaces %i FaceOffset %i/%i"), i, map_models(i).nFaces, map_models(i).iFirstFace, map_surfs.Num());
	}
	unguard;
}

void CMapLoadHelper::LoadTexInfos()
{
	guard(CMapLoadHelper::LoadTexInfos);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_TEXINFO];
	Loader.Seek(Lump.nOffset);
	if (Lump.nLength % BSPTEXTUREINFO_size)
	{
		GWarn->Logf(TEXT("Mod_LoadTexInfos: funny lump size"));
	}
	const INT count = Lump.nLength / BSPTEXTUREINFO_size;
	map_texinfo.SetSize(count);

	for (INT i = 0; i < count; ++i)
		Loader << map_texinfo(i);
	unguard;
}

static void WriteCubeBrush(const FVector& Offset, FOutputDevice& Out)
{
	Out.Log(NAME_Add, TEXT("BEGIN PolyList"));
	constexpr FLOAT brExtent = 128.f;
	constexpr FLOAT Sides[6][3] = { {-1.f,0.f,0.f},{0.f,1.f,0.f},{1.f,0.f,0.f},{0.f,-1.f,0.f},{0.f,0.f,1.f},{0.f,0.f,-1.f} };
	for (INT iSide = 0; iSide < 6; ++iSide)
	{
		Out.Log(NAME_Add, TEXT("BEGIN Polygon"));
		const FVector Normal(Sides[iSide][0], Sides[iSide][1], Sides[iSide][2]);
		const FVector MidPoint = Offset + (Normal * brExtent);
		FVector YAxis, ZAxis;
		if (Abs(Normal.X) > 0.5f)
			YAxis = FVector(0.f, -1.f, 0.f);
		else YAxis = FVector(-1.f, 0.f, 0.f);
		ZAxis = Normal ^ YAxis;

		const FVector Edges[4] = { MidPoint + (YAxis * brExtent) + (ZAxis * brExtent),
									MidPoint - (YAxis * brExtent) + (ZAxis * brExtent),
									MidPoint - (YAxis * brExtent) - (ZAxis * brExtent),
									MidPoint + (YAxis * brExtent) - (ZAxis * brExtent) };

		Out.Logf(TEXT("Origin    %i,%i,%i"), appRound(Edges[0].X), appRound(Edges[0].Y), appRound(Edges[0].Z));
		Out.Logf(TEXT("Normal    %i,%i,%i"), appRound(Normal.X), appRound(Normal.Y), appRound(Normal.Z));
		Out.Logf(TEXT("TextureU  %i,%i,%i"), appRound(YAxis.X), appRound(YAxis.Y), appRound(YAxis.Z));
		Out.Logf(TEXT("TextureV  %i,%i,%i"), appRound(ZAxis.X), appRound(ZAxis.Y), appRound(ZAxis.Z));
		Out.Log(TEXT("Pan       U=0 V=0"));
		for (INT i = 0; i < 4; ++i)
			Out.Logf(TEXT("Vertex    %i,%i,%i"), appRound(Edges[i].X), appRound(Edges[i].Y), appRound(Edges[i].Z));
		Out.Log(NAME_Remove, TEXT("END Polygon"));
	}
	Out.Log(NAME_Remove, TEXT("END PolyList"));
}

static FColor GetColorOf(const TCHAR* Str)
{
	FColor Result(255, 255, 255, 255);
	BYTE* bRes = &Result.R;
	for (INT i = 0; i < 4; ++i)
	{
		const TCHAR* Start = Str;
		while (appIsDigit(*Str) || *Str == '-' || *Str == '+' || *Str == '.')
			++Str;
		FString Temp(Start, Str);
		bRes[i] = Clamp(appAtoi(*Temp), 0, 255);
		while (*Str == ' ')
			++Str;
		if (!*Str)
			break;
	}
	return Result;
}
static const TCHAR* GetRenderFX(INT i)
{
#define FXST(ix,nm) case ix: return TEXT(nm);
	switch (i)
	{
		FXST(0, "RFX_Normal");
		FXST(1, "RFX_SlowPulse");
		FXST(2, "RFX_FastPulse");
		FXST(3, "RFX_SlowPulse");
		FXST(4, "RFX_FastPulse");
		FXST(15, "RFX_Distort");
		FXST(16, "RFX_Hologram");
	default: return nullptr;
	}
#undef FXST
}

static const TCHAR* SongList[] = { TEXT("Half-Life01"),TEXT("Prospero01"),TEXT("Half-Life12"),TEXT("Half-Life07"),TEXT("Half-Life10"),TEXT("Suspense01"),TEXT("Suspense03"),TEXT("Half-Life09"),
	TEXT("Half-Life02"),TEXT("Half-Life13"),TEXT("Half-Life04"),TEXT("Half-Life15"),TEXT("Half-Life14"),TEXT("Half-Life16"),TEXT("Suspense02"),TEXT("Half-Life03"),
	TEXT("Half-Life08"),TEXT("Prospero02"),TEXT("Half-Life05"),TEXT("Prospero04"),TEXT("Half-Life11"),TEXT("Half-Life06"),TEXT("Prospero03"),TEXT("Half-Life17"),TEXT("Prospero05"),
	TEXT("Suspense05") ,TEXT("Suspense07") ,TEXT("gamestartup") };

static const TCHAR* GetMusicName(INT iSong)
{
	if (iSong <= 0 || iSong > ARRAY_COUNT(SongList))
		return TEXT("None");
	TCHAR* Result = appStaticString1024();
	appSprintf(Result, TEXT("Music'%ls'"), SongList[iSong - 1]);
	return Result;
}

static void ExportEntityActor(FEntListData::FEntity& Entity, FOutputDevice& Out, const FLOAT EntHeight = 0.f)
{
	const TCHAR* Res;
	if (!Entity.Rotation.IsZero())
	{
		Out.Logf(TEXT("Rotation=(Yaw=%i,Pitch=%i,Roll=%i)"), Entity.Rotation.Yaw, Entity.Rotation.Pitch, Entity.Rotation.Roll);
		if (Entity.HasExportFlag(EXP_Mover))
			Out.Logf(TEXT("BaseRot=(Yaw=%i,Pitch=%i,Roll=%i)"), Entity.Rotation.Yaw, Entity.Rotation.Pitch, Entity.Rotation.Roll);
	}
	if (!Entity.Location.IsZero())
	{
		const FVector Pos = Entity.Location + FVector(0.f, 0.f, EntHeight);
		Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), Pos.X, Pos.Y, Pos.Z);
		if (Entity.HasExportFlag(EXP_Mover))
			Out.Logf(TEXT("BasePos=(X=%f,Y=%f,Z=%f)"), Pos.X, Pos.Y, Pos.Z);
	}
	if (!Entity.HasExportFlag(EXP_SkipEvents))
	{
		Res = Entity.Value(TEXT("targetname"));
		if (Res)
			Out.Logf(TEXT("Tag=\"%ls\""), Res);
		Res = Entity.Value(TEXT("target"));
		if (Res)
			Out.Logf(TEXT("Event=\"%ls\""), Res);
		Res = Entity.Value(TEXT("master"));
		if (Res)
			Out.Logf(TEXT("MasterTag=\"%ls\""), Res);
	}
	if (Entity.HasExportFlag(EXP_TriggerLight))
	{
		Out.Logf(TEXT("bInitiallyOn=%ls"), Entity.HasSpawnFlag(1) ? GFalse : GTrue);
		Out.Log(TEXT("InitialState=\"TriggerToggle\""));
	}
}

inline UBOOL CanMergeFaces(const FPoly& A, const FPoly& B)
{
	return (A.ItemName == B.ItemName) && (A.PolyFlags == B.PolyFlags) && (A.TextureU.DistSquared(B.TextureU) < 0.1f) && (A.TextureV.DistSquared(B.TextureV) < 0.1f);
}

void CMapLoadHelper::AddStaticLight(const FPoly& Poly, const FVector& Offset, UBOOL bFlippedNormal)
{
	const FColor* C = TexLightMap.Find(Poly.ItemName);
	if (!C)
		return;
	const FVector SurfNormal = bFlippedNormal ? (-Poly.Normal) : Poly.Normal;
	FVector CenterPoint(SurfNormal * 5.f);
	const FLOAT Div = 1.f / FLOAT(Poly.NumVertices);
	for (INT j = 0; j < Poly.NumVertices; ++j)
		CenterPoint += (Poly.Vertex[j] * Div);
	const FVector HSL = C->GetHSL();

	auto* Light = new (BspLights) CMapLoadHelper::FBspLight;
	Light->LightColor = FPlane(HSL.X, HSL.Y, HSL.Z, (C->A / 255.f));
	Light->Location = CenterPoint + Offset;
	Light->Normal = SurfNormal;
}
void CMapLoadHelper::DumpBspLights(FOutputDevice& Out)
{
	for (INT i = 0; i < BspLights.Num(); ++i)
	{
		const auto& Light = BspLights(i);
		const FVector ExportLocation = Light.Location + Light.Normal * 4.f;
		const FRotator LightRotation = Light.Normal.Rotation();

		ExportEntity(TEXT("Light"), Out);
		Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), ExportLocation.X, ExportLocation.Y, ExportLocation.Z);
		Out.Logf(TEXT("Rotation=(Yaw=%i,Pitch=%i,Roll=%i)"), LightRotation.Yaw, LightRotation.Pitch, LightRotation.Roll);
		Out.Logf(TEXT("LightHue=%i"), appRound(Light.LightColor.X * 255.f));
		Out.Logf(TEXT("LightSaturation=%i"), appRound(Light.LightColor.Y * 255.f));
		Out.Logf(TEXT("LightBrightness=%i"), appRound(Light.LightColor.W * 128.f));
		Out.Logf(TEXT("LightRadius=%i"), DefaultLightRadius);
		Out.Log(TEXT("LightCone=132"));
		Out.Log(NAME_Remove, TEXT("END Actor"));
	}
}

struct FTextureSwitch
{
	FName TexA, TexB;
	FName MatSeq;
	INT U, V;

	FTextureSwitch()
		: TexA(NAME_None), TexB(NAME_None), MatSeq(NAME_None)
	{}
	void Export(FOutputDevice& Out) const;
};

static const TCHAR SwitchPrefix[] = TEXT("ps_");
inline UBOOL IsSwitchName(const TCHAR* inName)
{
	const TCHAR* T = SwitchPrefix;
	while (*T)
	{
		if (appToLower(*inName) != *T)
			return FALSE;
		++T;
		++inName;
	}
	return (*inName == '0' || *inName == 'A' || *inName == 'a');
}

struct CModel
{
	friend struct CModelBuilder;
private:
	CMapLoadHelper& Data;
	UPolys* Poly;
	UBOOL bWasInverted;
	UBOOL bHasLinkedSurfs;
	FBox Bounds;
	FVector Centroid;

public:
	FTextureSwitch TexSwitch;

	CModel(CMapLoadHelper& D)
		: Data(D), bWasInverted(FALSE), bHasLinkedSurfs(FALSE)
	{
		Poly = new UPolys;
	}
	~CModel()
	{
		delete Poly;
	}

	FBox GetBounds() const
	{
		return Bounds;
	}
	FVector GetCentroid() const
	{
		return Centroid;
	}
	void MergePolys(UBOOL bKeepTex = TRUE, UBOOL bOnlyUnlit = FALSE)
	{
		guard(CModel::MergePolys);
		FScopedMemMark Mark(GMem);
		INT i, j;
		TArray<INT> PolysToMerge;
		for (i = 0; i < Poly->Element.Num(); ++i)
		{
			const auto& A = Poly->Element(i);
			PolysToMerge.EmptyNoRealloc();
			for (j = (i + 1); j < Poly->Element.Num(); ++j)
			{
				const auto& B = Poly->Element(j);
				if (bOnlyUnlit && !(A.PolyFlags & PF_Unlit))
					continue;
				if (bKeepTex && !CanMergeFaces(A, B))
					continue;
				if (!PolysToMerge.Num())
					PolysToMerge.AddItem(i);
				PolysToMerge.AddItem(j);
			}
			if (PolysToMerge.Num() && Poly->TryToMergePolys(&PolysToMerge(0), PolysToMerge.Num()))
				--i;
		}
		unguard;
	}
	void RotateModel(const FRotator& byAngle)
	{
		FCoords RotCoords = GMath.UnitCoords / byAngle;
		for (INT i = 0; i < Poly->Element.Num(); ++i)
		{
			auto& P = Poly->Element(i);
			P.Base = P.Base.TransformVectorBy(RotCoords);
			P.Normal = P.Normal.TransformVectorBy(RotCoords);
			P.TextureU = P.TextureU.TransformVectorBy(RotCoords);
			P.TextureV = P.TextureV.TransformVectorBy(RotCoords);
			for (INT j = 0; j < P.NumVertices; ++j)
				P.Vertex[j] = P.Vertex[j].TransformVectorBy(RotCoords);
		}
	}
	FPoly* AddPoly()
	{
		FPoly* Result = new(Poly->Element) FPoly;
		Result->iLink = Poly->Element.Num() - 1;
		return Result;
	}
	void RemoveLastPoly()
	{
		Poly->Element.Pop();
	}
	void DumpOutput(FOutputDevice& Out, UBOOL bExportTextures, UBOOL bAllowCenter = TRUE, UBOOL bExtractLights = TRUE, DWORD ForceFlags = 0) const
	{
		guard(CModel::DumpOutput);
		INT j;
		for (INT i = 0; i < Poly->Element.Num(); ++i)
		{
			const FPoly& P = Poly->Element(i);
			FString LinkStr = bHasLinkedSurfs ? FString::Printf(TEXT(" Link=%i"), P.iLink) : FString(TEXT(""));
			if (bExportTextures)
			{
				const DWORD PolyFlags = P.PolyFlags | ForceFlags;
				if (PolyFlags != PF_None)
				{
					Out.Logf(NAME_Add, TEXT("BEGIN Polygon Texture=%ls Flags=%u%ls"), *P.ItemName, PolyFlags, *LinkStr);

					if (bExtractLights && (P.PolyFlags & PF_Unlit))
						Data.AddStaticLight(P, FVector(0.f, 0.f, 0.f), bWasInverted);
				}
				else Out.Logf(NAME_Add, TEXT("BEGIN Polygon Texture=%ls%ls"), *P.ItemName, *LinkStr);
			}
			else Out.Logf(NAME_Add, TEXT("BEGIN Polygon%ls"), *LinkStr);

			if (bAllowCenter)
				Out.Logf(TEXT("Origin    %f,%f,%f"), P.Base.X - Centroid.X, P.Base.Y - Centroid.Y, P.Base.Z - Centroid.Z);
			else Out.Logf(TEXT("Origin    %f,%f,%f"), P.Base.X, P.Base.Y, P.Base.Z);
			Out.Logf(TEXT("Normal    %f,%f,%f"), P.Normal.X, P.Normal.Y, P.Normal.Z);
			Out.Logf(TEXT("TextureU  %f,%f,%f"), P.TextureU.X, P.TextureU.Y, P.TextureU.Z);
			Out.Logf(TEXT("TextureV  %f,%f,%f"), P.TextureV.X, P.TextureV.Y, P.TextureV.Z);
			Out.Logf(TEXT("Pan       U=%i V=%i"), P.PanU, P.PanV);
			if (bAllowCenter)
			{
				for (j = 0; j < P.NumVertices; ++j)
					Out.Logf(TEXT("Vertex    %f,%f,%f"), P.Vertex[j].X - Centroid.X, P.Vertex[j].Y - Centroid.Y, P.Vertex[j].Z - Centroid.Z);
			}
			else
			{
				for (j = 0; j < P.NumVertices; ++j)
					Out.Logf(TEXT("Vertex    %f,%f,%f"), P.Vertex[j].X, P.Vertex[j].Y, P.Vertex[j].Z);
			}
			Out.Log(NAME_Remove, TEXT("END Polygon"));
		}
		unguard;
	}
	void InverseModel()
	{
		guardSlow(CModel::InverseModel);
		for (INT i = 0; i < Poly->Element.Num(); ++i)
			Poly->Element(i).Reverse();
		unguardSlow;
	}

	UBOOL CheckInverted()
	{
		guard(CModel::CheckInverted);
		INT i, j, BestSurf;
		FLOAT Score, BestScore;
		static const FLOAT Axises[3][3] = { {1,0,0},{0,1,0},{0,0,1} };
		FLOAT TotalNormal = 0.f;
		for (INT d = 0; d < 2; ++d)
		{
			const FLOAT Sign = (d * 2.f) - 1.f;
			for (INT v = 0; v < 3; ++v)
			{
				const FVector Axis(Sign * Axises[v][0], Sign * Axises[v][1], Sign * Axises[v][2]);
				BestSurf = INDEX_NONE;
				for (i = 0; i < Poly->Element.Num(); ++i)
				{
					const FPoly& P = Poly->Element(i);
					Score = 0.f;
					for (j = 0; j < P.NumVertices; ++j)
						Score += (P.Vertex[j] | Axis);
					Score /= FLOAT(P.NumVertices);
					if ((BestSurf == INDEX_NONE) || (Score > BestScore))
					{
						BestSurf = i;
						BestScore = Score;
					}
				}
				TotalNormal += (Poly->Element(BestSurf).Normal | Axis);
			}
		}
		bWasInverted = (TotalNormal < 0.f);
		return bWasInverted;
		unguard;
	}
	void CalcBounds()
	{
		guard(CModel::CalcBounds);
		Bounds.IsValid = FALSE;
		INT j;
		for (INT i = 0; i < Poly->Element.Num(); ++i)
		{
			const auto& P = Poly->Element(i);
			for (j = 0; j < P.NumVertices; ++j)
				Bounds += P.Vertex[j];
		}
		Centroid = Bounds.GetCentroid().GridSnap(FVector(8.f, 8.f, 8.f));
		unguard;
	}
	UBOOL FindSwitchTexture()
	{
		guard(CModel::FindSwitchTexture);
		auto& E = Poly->Element;
		TArray<FName> LookedTex;
		UBOOL bFound = FALSE;
		for (INT i = 0; i < E.Num(); ++i)
		{
			auto& P = E(i);
			if (bFound)
			{
				if (P.ItemName == TexSwitch.TexA || P.ItemName == TexSwitch.TexB)
					P.ItemName = TexSwitch.MatSeq;
				continue;
			}
			if (LookedTex.FindItemIndex(P.ItemName) != INDEX_NONE)
				continue;
			LookedTex.AddItem(P.ItemName);
			if (!IsSwitchName(*P.ItemName))
				continue;

			FString otherName(*P.ItemName);
			if (otherName[3] == '0')
				otherName[3] = 'A';
			else otherName[3] = '0';
			FString fullTexName = FString(TEXT("HL_Textures.")) + otherName;

			UTexture* testObj = reinterpret_cast<UTexture*>(UObject::StaticLoadObject(UTexture::StaticClass(), NULL, *fullTexName, NULL, LOAD_NoWarn, NULL));
			if (!testObj)
				continue;

			TexSwitch.TexA = P.ItemName;
			TexSwitch.TexB = FName(*otherName);
			FString SwitchName(FString::Printf(TEXT("SwitchMaterial%i"), SwitchMatIndex++));
			TexSwitch.MatSeq = FName(*SwitchName);
			TexSwitch.U = testObj->USize;
			TexSwitch.V = testObj->VSize;
			P.ItemName = TexSwitch.MatSeq;
			bFound = TRUE;
		}
		return bFound;
		unguard;
	}
	UBOOL ModelContainsTriggerTextures() const
	{
		guardSlow(CModel::ModelContainsTriggerTextures);
		for (INT i = 0; i < Poly->Element.Num(); ++i)
		{
			const auto& P = Poly->Element(i);
			if (!appStricmp(*P.ItemName, TEXT("AAATRIGGER")))
				return TRUE;
		}
		return FALSE;
		unguardSlow;
	}
};

static void ProjectUVToCoords(const FVector& V0, FVector2D UV0, const FVector& V1, FVector2D UV1, const FVector& V2, FVector2D UV2, FVector& BaseResult, FVector& UResult, FVector& VResult)
{
	FVector PN(FVector((V0 - V1) ^ (V2 - V0)).SafeNormalSlow());

	if ((UV0.X == UV1.X) || (UV2.X == UV1.X) || (UV2.X == UV0.X) || (UV0.Y == UV1.Y) || (UV2.Y == UV1.Y) || (UV2.Y == UV0.Y))
	{
		UV1 += FVector2D(0.004173f, 0.004123f);
		UV2 += FVector2D(0.003173f, 0.003123f);
	}

	FCoords Coords(FVector(0, 0, 0), FVector(V1.X - V0.X, V1.Y - V0.Y, V1.Z - V0.Z), FVector(V2.X - V0.X, V2.Y - V0.Y, V2.Z - V0.Z), FVector(PN.X, PN.Y, PN.Z));
	Coords = Coords.Inverse();
	UResult = FVector(UV1.X - UV0.X, UV2.X - UV0.X, 0.0f).TransformVectorBy(Coords);
	VResult = FVector(UV1.Y - UV0.Y, UV2.Y - UV0.Y, 0.0f).TransformVectorBy(Coords);
	Coords = FCoords(FVector(0, 0, 0), UResult, VResult, FVector(PN.X, PN.Y, PN.Z)).Inverse();
	BaseResult = -FVector(UV0.X - (UResult | V0), UV0.Y - (VResult | V0), 0.0f).TransformVectorBy(Coords);
}

static INT iLookTag = 0;
static INT iEdgeSeekTag = 0;

static QSORT_RETURN CDECL CompareiSurfs(const INT A, const INT B)
{
	return B - A;
}

struct CModelBuilder
{
protected:
	CMapLoadHelper& Data;

public:
	TArray<CModel*> Models;

	CModelBuilder(CMapLoadHelper* H)
		: Data(*H)
	{}
	~CModelBuilder()
	{
		for (INT i = 0; i < Models.Num(); ++i)
			delete Models(i);
	}
	CModel* GrabModel()
	{
		CModel* Result = nullptr;
		if (Models.Num())
		{
			Result = Models(0);
			Models.Remove(0);
		}
		return Result;
	}
	void AddSurfs(const BSPMODEL& Model, const UBOOL bAllowSplit = TRUE, const UBOOL bAddTextures = TRUE)
	{
		guard(CModelBuilder::AddSurfs);
		++iLookTag;
		INT i, j;
		BSPEDGE* edges = &Data.map_edges(0);
		const FVector* pts = &Data.map_verts(0);
		TArray<INT> SurfList;
		TArray<FVector> Verts;
		BSPFACE* mSurfs = &Data.map_surfs(Model.iFirstFace);
		FName SkyName(TEXT("Sky"));
		FName TriggerName(TEXT("AAATRIGGER"));

		for (INT k = 0; k < Model.nFaces; ++k)
		{
			const BSPFACE& mainsurf = mSurfs[k];

			if ((mainsurf.iTag == iLookTag) || mainsurf.nEdges < 3)
				continue;

			SurfList.AddItem(k);
			if (bAllowSplit)
			{
				// Find all concurrent surfaces and group them together.
				for (i = 0; i < SurfList.Num(); ++i)
				{
					++iEdgeSeekTag;
					BSPFACE& surf = mSurfs[SurfList(i)];
					surf.iTag = iLookTag;

					{
						const INT* surfedges = &Data.map_surfedges(surf.iFirstEdge);
						for (j = 0; j < surf.nEdges; ++j)
							edges[Abs(surfedges[j])].iTag = iEdgeSeekTag;
					}

					for (INT z = 0; z < Model.nFaces; ++z)
					{
						BSPFACE& osurf = mSurfs[z];
						if (osurf.iTag != iLookTag)
						{
							const INT* surfedges = &Data.map_surfedges(osurf.iFirstEdge);
							for (j = 0; j < osurf.nEdges; ++j)
								if (edges[Abs(surfedges[j])].iTag == iEdgeSeekTag)
								{
									osurf.iTag = iLookTag;
									SurfList.AddItem(z);
									break;
								}
						}
					}
				}
				appQsort(SurfList.GetData(), SurfList.Num(), sizeof(INT), (QSORT_COMPARE)CompareiSurfs);
			}
			else
			{
				// Just add every face.
				for (i = (k + 1); i < Model.nFaces; ++i)
				{
					mSurfs[i].iTag = iLookTag;
					if (mSurfs[i].nEdges >= 3)
						SurfList.AddItem(i);
				}
			}

			CModel* Model = new CModel(Data);
			Models.AddItem(Model);
			FScopedMemMark Mark(GMem);
			INT* SurfRefs = nullptr;
			if (bAddTextures)
				SurfRefs = New<INT>(GMem, SurfList.Num());
			INT iLastSurf = 0;

			// Create a new CModel out of them
			for (i = 0; i < SurfList.Num(); ++i)
			{
				const BSPFACE& surf = mSurfs[SurfList(i)];
				const INT numEdges = surf.nEdges;
				const INT* surfedges = &Data.map_surfedges(surf.iFirstEdge);
				for (j = 0; j < numEdges; ++j)
				{
					const INT iVertex = (surfedges[j] < 0) ? edges[-surfedges[j]].iVertex[1] : edges[surfedges[j]].iVertex[0];
					Verts.AddItem(pts[iVertex]);
				}

				FPoly* P = Model->AddPoly();
				P->Base = P->Vertex[0];
				P->PanU = P->PanV = 0;
				P->NumVertices = Verts.Num();
				appMemcpy(P->Vertex, &Verts(0), sizeof(FVector) * Verts.Num());
				P->CalcNormal();
				P->RemoveColinears();
				if (P->NumVertices < 3)
				{
					Model->RemoveLastPoly();
					continue;
				}

				P->PolyFlags = PF_None;
				if (!bAddTextures)
				{
					P->ItemName = NAME_None;
					P->TextureU = (P->Vertex[1] - P->Vertex[0]).SafeNormal();
					P->TextureV = (P->TextureU ^ P->Normal).SafeNormal();
				}
				else
				{
					const BSPTEXTUREINFO& tex = Data.map_texinfo(surf.iTextureInfo);
					const CMapLoadHelper::FMapTex& mTex = Data.map_tex(tex.iMiptex);
					P->ItemName = FName(*mTex.Name);
					if (Data.TexLightMap.Find(P->ItemName))
						P->PolyFlags = PF_Unlit;
					else if (P->ItemName == NAME_Black) // Black pit, no lighting needed.
						P->PolyFlags = PF_Unlit;
					else if (P->ItemName == SkyName) // Sky texture.
						P->PolyFlags = PF_FakeBackdrop;
					else if (P->ItemName == TriggerName) // Invisible collision.
						P->PolyFlags = PF_Invisible;

					// Check if we find a other co-planar adjacent surface
					UBOOL FoundLink = FALSE;
					for (INT j = 0; j < iLastSurf; ++j)
					{
						const FPoly& otherPoly = Model->Poly->Element(j);
						if (otherPoly.ItemName != P->ItemName || otherPoly.PolyFlags != P->PolyFlags || !FCoplanar(otherPoly.Vertex[0], otherPoly.Normal, P->Vertex[0], P->Normal))
							continue;
						const BSPFACE& cmpSurf = mSurfs[SurfList(SurfRefs[j])];
						const BSPTEXTUREINFO& cmpTex = Data.map_texinfo(cmpSurf.iTextureInfo);
						if (Abs(tex.fSShift - cmpTex.fSShift) > THRESH_POINTS_ARE_NEAR || Abs(tex.fTShift - cmpTex.fTShift) > THRESH_POINTS_ARE_NEAR || !FPointsAreSame(tex.vS, cmpTex.vS) || !FPointsAreSame(tex.vT, cmpTex.vT))
							continue;
						FoundLink = TRUE;
						P->iLink = j;

						P->Base = otherPoly.Base;
						P->TextureU = otherPoly.TextureU;
						P->TextureV = otherPoly.TextureV;
						Model->bHasLinkedSurfs = TRUE;
						//debugf(TEXT("Found Surfs that match [%i -> %i]"), i, j);
						break;
					}

					// Add this to references list.
					SurfRefs[iLastSurf++] = i;

					if (!FoundLink)
					{
						// Bleh complex way of figuring out exact HL texture coordinates...
						FVector2D UVs[3];
						for (j = 0; j < 3; ++j)
						{
							UVs[j].X = ((ToUnrealUVMap(P->Vertex[j]) | tex.vS) + tex.fSShift) * mTex.ScaleX;
							UVs[j].Y = ((ToUnrealUVMap(P->Vertex[j]) | tex.vT) + tex.fTShift) * mTex.ScaleY;
						}
						ProjectUVToCoords(P->Vertex[0], UVs[0], P->Vertex[1], UVs[1], P->Vertex[2], UVs[2], P->Base, P->TextureU, P->TextureV);
					}
				}

				Verts.EmptyNoRealloc();
			}

			SurfList.EmptyNoRealloc();
		}

		for (i = 0; i < Models.Num(); ++i)
			Models(i)->CalcBounds();
		unguard;
	}
};

void CMapLoadHelper::WriteCModel(CModel* CModel, FOutputDevice& Out, const UBOOL bWorld, const UBOOL bAddTextures, const UBOOL bCenterModel, const UBOOL bIsMover, DWORD ForceFlags, INT RotateModel)
{
	guard(CMapLoadHelper::WriteCModel);
	Out.Logf(NAME_Add, TEXT("BEGIN Brush Name=Brush%i"), iModelIndex);
	Out.Log(NAME_Add, TEXT("BEGIN PolyList"));

	if (CModel)
	{
		if (RotateModel)
			CModel->RotateModel(FRotator(0, RotateModel, 0));
		CModel->MergePolys(bAddTextures);
		CModel->DumpOutput(Out, bAddTextures, bCenterModel, (bAddTextures && !bIsMover), ForceFlags);
	}

	Out.Log(NAME_Remove, TEXT("END PolyList"));
	Out.Log(NAME_Remove, TEXT("END Brush"));
	Out.Logf(TEXT("Brush=Model'Brush%i'"), iModelIndex);
	if (bCenterModel)
	{
		const FVector V = CModel ? CModel->GetCentroid() : FVector(0, 0, 0);
		Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), V.X, V.Y, V.Z);
		if (bIsMover)
			Out.Logf(TEXT("BasePos=(X=%f,Y=%f,Z=%f)"), V.X, V.Y, V.Z);
	}
	++iModelIndex;
	unguard;
}
void CMapLoadHelper::WriteModel(const BSPMODEL& Model, FOutputDevice& Out, const UBOOL bWorld, const UBOOL bAddTextures, const UBOOL bCenterModel, const UBOOL bIsMover, DWORD ForceFlags, INT RotateModel)
{
	guard(CMapLoadHelper::WriteModel);
	CModelBuilder Builder(this);
	Builder.AddSurfs(Model, FALSE, bAddTextures);
	WriteCModel(Builder.Models.Num() ? Builder.Models(0) : nullptr, Out, bWorld, bAddTextures, bCenterModel, bIsMover, ForceFlags, RotateModel);
	unguard;
}

CModel* CMapLoadHelper::BuildModel(const BSPMODEL& Model)
{
	guard(CMapLoadHelper::BuildModel);
	CModelBuilder Builder(this);
	Builder.AddSurfs(Model, FALSE, TRUE);
	return Builder.GrabModel();
	unguard;
}

void CMapLoadHelper::WriteModelOrigin(FOutputDevice& Out, FEntListData::FEntity& Entity)
{
	guard(CMapLoadHelper::WriteModelOrigin);
	const INT mdlIndex = Entity.GetModelBrushNum();
	if (mdlIndex != INDEX_NONE)
	{
		BSPMODEL* M = GetModel(mdlIndex);
		if (M)
		{
			CModelBuilder Builder(this);
			Builder.AddSurfs(*M, FALSE, FALSE);
			if (Builder.Models.Num())
			{
				const FVector V = Builder.Models(0)->GetCentroid();
				Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), V.X, V.Y, V.Z);
			}
		}
	}
	unguard;
}
void CMapLoadHelper::WriteModelVolume(FOutputDevice& Out, FEntListData::FEntity& Entity)
{
	guard(CMapLoadHelper::WriteModelVolume);
	const INT mdlIndex = Entity.GetModelBrushNum();
	if (mdlIndex != INDEX_NONE)
	{
		BSPMODEL* M = GetModel(mdlIndex);
		if (M)
		{
			WriteModel(*M, Out, FALSE, FALSE, TRUE, FALSE);
			Entity.Location = FVector(0, 0, 0);
		}
	}
	unguard;
}

static FString TranslateSound(const TCHAR* Input)
{
	if (*Input == '*')
		++Input;
	const TCHAR* Separator = appStrstr(Input, TEXT("/"));
	FString SoundName = FString::GetFilenameOnlyStr(Input);
	FString Result;
	if (Separator)
	{
		FString Group = FString(Input, Separator);
		Result = FString::Printf(TEXT("Sound'HL_Ambience.%ls.%ls'"), *Group, *SoundName);
	}
	else Result = FString::Printf(TEXT("Sound'HL_Ambience.%ls'"), *SoundName);
	return Result;
}
static const TCHAR* TranslateSprite(const TCHAR* Input)
{
	static TMap<FString, FString>* SprMap = NULL;
	if (!SprMap)
	{
		SprMap = new TMap<FString, FString>;
		SprMap->Set(TEXT("blueflare1"), TEXT("GenFX.LensFlar.3"));
		SprMap->Set(TEXT("flare1"), TEXT("GenFX.LensFlar.new2"));
		SprMap->Set(TEXT("glow01"), TEXT("GenFX.LensFlar.Dot_A"));
		SprMap->Set(TEXT("glow04"), TEXT("GenFX.LensFlar.3"));
		SprMap->Set(TEXT("flare3"), TEXT("GenFX.LensFlar.1"));
		SprMap->Set(TEXT("XSmoke4"), TEXT("UnrealShare.SmokeColm.sc_a00"));
		SprMap->Set(TEXT("zerogxplode"), TEXT("UnrealShare.MainEffect.e4_a00"));
		SprMap->Set(TEXT("Fexplo1"), TEXT("FX_fexplo1"));
		SprMap->Set(TEXT("XFlare1"), TEXT("FX_xflare1"));
		SprMap->Set(TEXT("XSsmke1"), TEXT("UnrealShare.EffectASMD.ASMDSmoke"));
	}
	FString* S = SprMap->Find(Input);
	if (!S)
		GWarn->Logf(TEXT("Missing glow sprite '%ls'"), Input);
	return S ? **S : nullptr;
}
static const TCHAR* GetInventoryClass(INT inType, FLOAT* HeightModifier)
{
#define IC(ix,hg,cl) case ix: *HeightModifier = hg; return TEXT(cl)
	switch (inType)
	{
		IC(44, 0.f, "INV_SuitArmor");
		IC(45, 0.f, "INV_HevSuit");
	default:
		GWarn->Logf(TEXT("Found inventory with invalid ID: %i"), inType);
		*HeightModifier = 0.f;
		return nullptr;
	}
#undef IC
}

static void ParseOptNameParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out)
{
	auto* S = Ent.Value(ValveName);
	if (S)
		Out.Logf(TEXT("%ls=\"%ls\""), ScriptName ? ScriptName : ValveName, S);
}
static void ParseOptNumberParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out)
{
	INT i = Ent.IntValue(ValveName);
	if (i != INT_NF)
		Out.Logf(TEXT("%ls=%i"), ScriptName ? ScriptName : ValveName, i);
}
static void ParseOptScaledNumParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out, const FLOAT ScaleValue = ToUnrScale)
{
	FLOAT f = Ent.FloatValue(ValveName);
	if (f != FLOAT_NF)
		Out.Logf(TEXT("%ls=%f"), ScriptName ? ScriptName : ValveName, f * ScaleValue);
}
static void ParseOptScaledIntParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out, const FLOAT ScaleValue)
{
	INT i = Ent.IntValue(ValveName);
	if (i != INT_NF)
		Out.Logf(TEXT("%ls=%i"), ScriptName ? ScriptName : ValveName, appRound(i * ScaleValue));
}
static void ParseOptBoolParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out)
{
	INT i = Ent.IntValue(ValveName);
	if (i != INT_NF)
		Out.Logf(TEXT("%ls=%ls"), ScriptName ? ScriptName : ValveName, (i != 0) ? GTrue : GFalse);
}

#define EXPENTPARMS CMapLoadHelper& Loader, FEntListData::FEntity& Entity, FOutputDevice& Out
#define CALLEXPPARMS Loader, Entity, Out
typedef void (f_ExpEntFunc)(EXPENTPARMS);

void CMapLoadHelper::ExportEntity(const TCHAR* EntName, FOutputDevice& Out)
{
	guard(CMapLoadHelper::ExportEntity);
	INT Index = 0;
	INT* nr = ExportCount.Find(EntName);
	if (nr)
	{
		(*nr)++;
		Index = *nr;
	}
	else ExportCount.Set(EntName, 1);
	Out.Logf(NAME_Add, TEXT("BEGIN Actor Class=%ls Name=%ls%i"), EntName, EntName, Index);
	unguard;
}

static void ExportNothing(EXPENTPARMS)
{
}
static void ExportGeneric(EXPENTPARMS)
{
	auto* Res = Entity.Value(TEXT("model"));
	if (Res)
	{
		FString N = FString::GetFilenameOnlyStr(Res);
		Out.Logf(TEXT("Mesh=%lsM"), *N);
	}
}
static void ExportAutoTrigger(EXPENTPARMS)
{
	ParseOptNumberParm(TEXT("Delay"), TEXT("TriggerDelay"), Entity, Out);
}
static void ExportLights(EXPENTPARMS)
{
	if (Entity.IsClass(TEXT("light_spot")))
	{
		Out.Log(TEXT("LightEffect=LE_Spotlight"));
		Out.Log(TEXT("bDirectional=True"));
	}
	auto* Res = Entity.Value(TEXT("_light"));
	if (Res)
	{
		FColor C = GetColorOf(Res);
		FVector HSL = C.GetHSL();
		Out.Logf(TEXT("LightHue=%i"), appRound(HSL.X * 255.f));
		Out.Logf(TEXT("LightSaturation=%i"), appRound(HSL.Y * 255.f));
		Out.Logf(TEXT("LightBrightness=%i"), INT(C.A));
	}
	FLOAT f = Entity.FloatValue(TEXT("_distance"));
	if (f != FLOAT_NF)
		Out.Logf(TEXT("LightRadius=%i"), Min(appRound(f * 32.f), 255));
	else Out.Logf(TEXT("LightRadius=%i"), DefaultLightRadius);

	f = Entity.FloatValue(TEXT("_cone2"));
	if (f != FLOAT_NF)
		Out.Logf(TEXT("LightCone=%i"), Clamp(appRound(f * (255.f / 90.f)), 10, 255));
}
static void ExportTalker(EXPENTPARMS)
{
	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("bQuiet=True"));
	if (Entity.HasSpawnFlag(256))
		Out.Log(TEXT("bPreDisaster=True"));
}
static void ExportScientist(EXPENTPARMS)
{
	ExportTalker(CALLEXPPARMS);
	ParseOptNumberParm(TEXT("body"), TEXT("BodyType"), Entity, Out);
}
static void ExportBarney(EXPENTPARMS)
{
	ExportTalker(CALLEXPPARMS);
	ParseOptNameParm(TEXT("AnimEvent"), TEXT("SpecialAnimEvent"), Entity, Out);
}
static void ExportScriptedSequence(EXPENTPARMS)
{
	ParseOptNameParm(TEXT("m_iszEntity"), NULL, Entity, Out);
	ParseOptNameParm(TEXT("m_iszIdle"), NULL, Entity, Out);
	ParseOptNameParm(TEXT("m_iszEntry"), NULL, Entity, Out);
	ParseOptNameParm(TEXT("m_iszPlay"), NULL, Entity, Out);
	ParseOptNameParm(TEXT("m_iszPostIdle"), NULL, Entity, Out);
	ParseOptNameParm(TEXT("m_iszCustomMove"), NULL, Entity, Out);
	ParseOptNameParm(TEXT("m_iszNextScript"), NULL, Entity, Out);
	ParseOptScaledNumParm(TEXT("m_flRadius"), NULL, Entity, Out);
	ParseOptBoolParm(TEXT("m_LoopActionAnim"), NULL, Entity, Out);
	ParseOptNumberParm(TEXT("m_flRepeat"), NULL, Entity, Out);
	ParseOptBoolParm(TEXT("m_bIgnoreGravity"), NULL, Entity, Out);
	ParseOptBoolParm(TEXT("m_bDisableNPCCollisions"), NULL, Entity, Out);

	if (Entity.HasSpawnFlag(4))
		Out.Log(TEXT("sf_Repeatable=True"));
	if (Entity.HasSpawnFlag(16))
		Out.Log(TEXT("sf_StartOnSpawn=True"));
	if (Entity.HasSpawnFlag(32))
		Out.Log(TEXT("sf_NoInterruptions=True"));
	if (Entity.HasSpawnFlag(64))
		Out.Log(TEXT("sf_OverrideAI=True"));
	if (Entity.HasSpawnFlag(128))
		Out.Log(TEXT("sf_NoTeleportAtEnd=True"));
	if (Entity.HasSpawnFlag(256))
		Out.Log(TEXT("sf_LoopInPostIdle=True"));
	if (Entity.HasSpawnFlag(512))
		Out.Log(TEXT("sf_PriorityScript=True"));
	if (Entity.HasSpawnFlag(1024))
		Out.Log(TEXT("sf_SearchCyclically=True"));

	INT i = Entity.IntValue(TEXT("m_fMoveTo"), INDEX_NONE);
	if (i >= 0 && i <= 5)
	{
		static const TCHAR* MoveTypes[] = { TEXT("MTT_None"), TEXT("MTT_Walk"), TEXT("MTT_Run") , TEXT("MTT_Custom") , TEXT("MTT_Instant") , TEXT("MTT_TurnTo") };
		Out.Logf(TEXT("m_fMoveTo=%ls"), MoveTypes[i]);
	}
}
static void ExportEnvMessage(EXPENTPARMS)
{
	auto* Res = Entity.Value(TEXT("message"));
	if (Res)
	{
		auto* T = Loader.Titles.FindTitle(Res);
		if (T)
		{
			Out.Logf(TEXT("Message=\"%ls\""), *T->Text);
			Out.Logf(TEXT("MessageColor=(R=%i,G=%i,B=%i)"), INT(T->Color.R), INT(T->Color.G), INT(T->Color.B));
			Out.Logf(TEXT("MessageHiColor=(R=%i,G=%i,B=%i)"), INT(T->HighlightColor.R), INT(T->HighlightColor.G), INT(T->HighlightColor.B));
			Out.Logf(TEXT("MessagePosition=(X=%f,Y=%f)"), T->XPos, T->YPos);
			switch (T->MsgType)
			{
			case 0:
				Out.Logf(TEXT("MessageFadeIn=%f"), T->FadeIn);
				Out.Logf(TEXT("MessageFadeOut=%f"), T->FadeIn);
				Out.Log(TEXT("CharsPerSec=0.0"));
				Out.Logf(TEXT("MessageDisplayTime=%f"), T->DisplayTime);
				break;
			case 1:
				{
					const FLOAT QtTime = T->DisplayTime * 0.15f;
					Out.Logf(TEXT("MessageFadeIn=%f"), QtTime);
					Out.Logf(TEXT("MessageFadeOut=%f"), QtTime);
					Out.Log(TEXT("CharsPerSec=0.0"));
					Out.Logf(TEXT("MessageDisplayTime=%f"), T->DisplayTime - (QtTime * 2));
					break;
				}
			default:
				Out.Logf(TEXT("MessageFadeIn=%f"), T->FadeIn);
				Out.Logf(TEXT("MessageFadeOut=%f"), T->FadeIn);
				Out.Logf(TEXT("MessageFadeOut=%f"), T->FadeIn);
				Out.Logf(TEXT("HighlightFadeTime=%f"), T->HighlightFadeTime);
				Out.Logf(TEXT("MessageDisplayTime=%f"), T->DisplayTime);
			}
		}
	}
	ParseOptNumberParm(TEXT("messagesound"), TEXT("MessageSound"), Entity, Out);
	ParseOptNumberParm(TEXT("messagevolume"), TEXT("MessageVolume"), Entity, Out);
	INT i = Entity.IntValue(TEXT("messageattenuation"), INDEX_NONE);
	if (i >= 0 && i <= 3)
	{
		static const TCHAR* SAttenTypes[] = { TEXT("MSA_Small"), TEXT("MSA_Medium"), TEXT("MSA_Large") , TEXT("MSA_Everywhere") };
		Out.Logf(TEXT("MessageAttenuate=%ls"), SAttenTypes[i]);
	}

	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("sf_PlayOnce=True"));
	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("sf_AllClients=True"));
}
struct FMMKeyPair
{
	FName Key;
	FLOAT Delay;

	FMMKeyPair(const TCHAR* inKey, FLOAT inDelay)
		: Key(inKey), Delay(inDelay)
	{}
};
static INT Compare(const FMMKeyPair& A, const FMMKeyPair& B)
{
	if (A.Delay == B.Delay)
		return 0;
	if (A.Delay > B.Delay)
		return 1;
	return -1;
}
static void ExportMultiManager(EXPENTPARMS)
{
	TArray<FMMKeyPair> MMKeys;
	for (auto It = Entity.MapIterator(); It; ++It)
	{
		if (!appStricmp(*It.Key(), TEXT("origin")) || !appStricmp(*It.Key(), TEXT("targetname")) || !appStricmp(*It.Key(), TEXT("classname")))
			continue;

		// Strip token
		FString keyName = *It.Key();
		INT f = keyName.InStr(TEXT("#"));
		if (f > 0)
			keyName = keyName.Left(f);

		new (MMKeys) FMMKeyPair(*keyName, It.Value().Float());
	}
	Sort(MMKeys);

	FLOAT lastDelay = 0.f;
	for (INT i = 0; i < MMKeys.Num(); ++i)
	{
		const auto& k = MMKeys(i);
		Out.Logf(TEXT("Events(%i)=(Event=\"%ls\",Delay=%f)"), i, *k.Key, k.Delay - lastDelay);
		lastDelay = k.Delay;
	}
	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("bMultiThreaded=True"));
}
static void ExportEnvGlov(EXPENTPARMS)
{
	auto* Res = Entity.Value(TEXT("model"));
	if (Res)
	{
		FString SpriteName = FString::GetFilenameOnlyStr(Res);
		if (SpriteName == TEXT("glow05"))
		{
			// Invis sprite but still uses a tag? must have some special purpose.
			Out.Log(TEXT("bHidden=True"));
		}
		else
		{
			const TCHAR* SPR = TranslateSprite(*SpriteName);
			if (SPR)
				Out.Logf(TEXT("Texture=Texture'%ls'"), SPR);
		}
	}
	Res = Entity.Value(TEXT("rendercolor"));
	if (Res)
	{
		FColor RC = GetColorOf(Res);
		Out.Logf(TEXT("ActorRenderColor=(R=%i,G=%i,B=%i,A=255)"), INT(RC.R), INT(RC.G), INT(RC.B));
	}
	INT i = Entity.IntValue(TEXT("rendermode"), INDEX_NONE);
	if (i >= 1 && i <= 5)
		Out.Log(TEXT("Style=STY_Translucent"));
	ParseOptScaledNumParm(TEXT("renderamt"), TEXT("ScaleGlow"), Entity, Out, (1.f / 255.f));
	ParseOptScaledNumParm(TEXT("scale"), TEXT("DrawScale"), Entity, Out, 1.f);

	if (Entity.IsClass(TEXT("env_sprite")))
	{
		if (!Entity.HasSpawnFlag(1))
			Out.Log(TEXT("bHidden=True"));
		if (Entity.HasSpawnFlag(2))
			Out.Log(TEXT("DrawType=DT_SpriteAnimOnce"));
	}
}
static void ExportEnvSparks(EXPENTPARMS)
{
	if (Entity.HasSpawnFlag(32))
		Out.Log(TEXT("TriggerAction=ETR_ToggleDisabled"));
	if (Entity.HasSpawnFlag(64))
		Out.Log(TEXT("bDisabled=False"));
}
static void ExportDecal(EXPENTPARMS)
{
	const TCHAR* Tex = Entity.Value(TEXT("texture"));
	if (Tex)
		Out.Logf(TEXT("DecalTexture=Texture'%ls'"), UHLTexImporter::MakeSafeName(Tex));
}
static void ExportDispenser(EXPENTPARMS)
{
	INT i = Entity.IntValue(TEXT("health"), 0);
	if (i > 0)
		Out.Logf(TEXT("numCans=%i"), i);
	i = Entity.IntValue(TEXT("skin"), 0);
	if (i > 0)
		Out.Logf(TEXT("canSkinNum=%i"), i);
}
static void ExportEmitter(EXPENTPARMS)
{
	auto* Res = Entity.Value(TEXT("shootmodel"));
	if (Res)
	{
		const TCHAR* SPR = TranslateSprite(*FString::GetFilenameOnlyStr(Res));
		if (SPR)
			Out.Logf(TEXT("ParticleTextures(0)=Texture'%ls'"), SPR);
	}

	INT i = Entity.IntValue(TEXT("angle"));
	if (i != INT_NF)
	{
		FRotator R(0, 0, 0);
		if (i == -1)
			R.Pitch = 16384;
		else if (i == -2)
			R.Pitch = -16384;
		else R.Yaw = -ToUnrealAngles(i);
		Out.Logf(TEXT("Rotation=(Yaw=%i,Pitch=%i,Roll=0)"), R.Yaw, R.Pitch);
	}

	FLOAT f = Entity.FloatValue(TEXT("Scale"));
	if (f != FLOAT_NF)
		Out.Logf(TEXT("StartingScale=(Min=%f,Max=%f)"), f * ToUnrScale * 0.95f, f * ToUnrScale * 1.05f);
	f = Entity.FloatValue(TEXT("renderamt"));
	if (f != FLOAT_NF)
		Out.Logf(TEXT("FadeInMaxAmount=%f"), f / 255.f);

	Res = Entity.Value(TEXT("rendercolor"));
	if (Res)
	{
		FVector C = GetColorOf(Res).Vector();
		Out.Logf(TEXT("ParticleColor=(X=(Min=%f,Max=%f),Y=(Min=%f,Max=%f),Z=(Min=%f,Max=%f))"), C.X, C.X, C.Y, C.Y, C.Z, C.Z);
	}

	i = Entity.IntValue(TEXT("renderfx"));
	if (i != INT_NF)
	{
		switch (i)
		{
		case 5:
			Out.Log(TEXT("FadeOutTime=0.5"));
			break;
		case 7:
			Out.Log(TEXT("FadeInTime=0.5"));
			break;
		}
	}
	FLOAT pTime = Entity.FloatValue(TEXT("m_flGibLife"), 1.f);
	Out.Logf(TEXT("LifetimeRange=(Min=%f,Max=%f)"), pTime * 0.95f, pTime * 1.05f);
	FLOAT angDiff = Entity.FloatValue(TEXT("m_flVariance"), 0.f);
	f = Entity.FloatValue(TEXT("m_flVelocity"));
	if (f != FLOAT_NF)
	{
		FLOAT cosVar = angDiff * f;
		Out.Logf(TEXT("BoxVelocity=(X=(Min=%f,Max=%f),Y=(Min=%f,Max=%f),Z=(Min=%f,Max=%f))"), f * 0.95f, f * 1.05f, -cosVar, cosVar, -cosVar, cosVar);
	}
	f = Entity.FloatValue(TEXT("Delay"));
	if (f != FLOAT_NF)
		Out.Logf(TEXT("ParticlesPerSec=%f"), (1.f / Max<FLOAT>(f, 0.025f)));
	i = Entity.IntValue(TEXT("m_iGibs"));
	if (i != INT_NF)
		Out.Logf(TEXT("MaxParticles=%i"), i);
}
static void ExportTriggerRelay(EXPENTPARMS)
{
	ParseOptNameParm(TEXT("killtarget"), TEXT("KillTarget"), Entity, Out);
	ParseOptNumberParm(TEXT("delay"), TEXT("Delay"), Entity, Out);
	ParseOptNumberParm(TEXT("triggerstate"), TEXT("TriggerState"), Entity, Out);
}
static void ExportPathTrack(EXPENTPARMS)
{
	ParseOptNameParm(TEXT("altpath"), TEXT("AlternativePath"), Entity, Out);
	ParseOptNameParm(TEXT("message"), TEXT("ReachEvent"), Entity, Out);
	ParseOptNameParm(TEXT("netname"), TEXT("ReachEvent"), Entity, Out);
	ParseOptScaledNumParm(TEXT("radius"), TEXT("PathRadius"), Entity, Out);
	ParseOptScaledNumParm(TEXT("speed"), TEXT("TrainSpeed"), Entity, Out);
	ParseOptScaledNumParm(TEXT("wait"), TEXT("PauseTime"), Entity, Out, 1.f);

	if (Entity.IsClass(TEXT("func_trackautochange")) || Entity.IsClass(TEXT("func_trackchange")))
	{
		ParseOptNameParm(TEXT("train"), TEXT("TrainTag"), Entity, Out);
		ParseOptNameParm(TEXT("bottomtrack"), TEXT("BottomTrackTag"), Entity, Out);
		ParseOptNameParm(TEXT("toptrack"), TEXT("TopTrackTag"), Entity, Out);
		ParseOptScaledIntParm(TEXT("Rotation"), TEXT("RotationOffset"), Entity, Out, (65536.f / 360.f));
		ParseOptScaledNumParm(TEXT("Height"), TEXT("MovementHeight"), Entity, Out);

		if (Entity.HasSpawnFlag(8))
			Out.Log(TEXT("bStartBottom=True"));
		if (Entity.HasSpawnFlag(16))
			Out.Log(TEXT("bRotateOnly=True"));
		if (Entity.HasSpawnFlag(64))
			Out.Log(TEXT("bRotateX=True"));
		if (Entity.HasSpawnFlag(128))
			Out.Log(TEXT("bRotateY=True"));
	}
	else if (Entity.IsClass(TEXT("path_corner")))
		Out.Log(TEXT("OrientationType=OT_None"));
	else
	{
		INT i = Entity.IntValue(TEXT("orientationtype"), INDEX_NONE);
		if (i >= 0 && i <= 2)
		{
			static const TCHAR* OrNames[] = { TEXT("OT_None"), TEXT("OT_Velocity"), TEXT("OT_Rotation") };
			Out.Logf(TEXT("OrientationType=%ls"), OrNames[i]);
			if (i == 2)
				Out.Log(TEXT("bDirectional=True"));
		}
	}
	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("bTriggerOnce=True"));
}
static void ExportBrush(EXPENTPARMS)
{
	static const DWORD RenderModeFlags[] = { PF_None, PF_Translucent, PF_None, (PF_Translucent | PF_Unlit), PF_Masked, PF_Translucent, PF_Invisible, PF_None, PF_None, PF_Unlit, PF_Invisible };

	const DWORD Solidity = Entity.IsClass(TEXT("func_illusionary")) ? PF_NotSolid : PF_Semisolid;
	INT i = Clamp<INT>(Entity.IntValue(TEXT("rendermode"), 0), 0, 10);
	Out.Logf(TEXT("PolyFlags=%i"), Solidity | RenderModeFlags[i]);
	Out.Log(TEXT("CsgOper=CSG_Add"));

	const UBOOL bHasNoLocation = Entity.Location.IsZero();
	if (Entity.pendingModel)
	{
		Loader.WriteCModel(Entity.pendingModel, Out, FALSE, TRUE, bHasNoLocation, FALSE);
	}
	else
	{
		const UBOOL CheckToggleSkin = (Entity.HasExportFlag(EXP_CheckTriggerTex) && (Entity.Value(TEXT("targetname")) != NULL));
		i = Entity.GetModelBrushNum();
		if (i != INDEX_NONE)
		{
			BSPMODEL* M = Loader.GetModel(i);
			if (M)
			{
				if (CheckToggleSkin)
				{
					auto* Model = Loader.BuildModel(*M);
					FVector ModelOrigin = Model->GetCentroid();
					if (Model->FindSwitchTexture())
					{
						Entity.pendingModel = Model;
						Entity.SpecialFlags = 2;
					}
					Loader.WriteCModel(Model, Out, FALSE, TRUE, bHasNoLocation, FALSE);
				}
				else Loader.WriteModel(*M, Out, FALSE, TRUE, bHasNoLocation, FALSE);
			}
		}
	}
}

#define DOORSND(idx,name) case idx: return TEXT("Sound'HL_Ambience.Doors." name "'")
static const TCHAR* GetDoorLoopSound(INT SoundIndex)
{
	switch (SoundIndex)
	{
	DOORSND(1, "doormove1");
	DOORSND(2, "doormove2");
	DOORSND(3, "doormove3");
	DOORSND(4, "doormove4");
	DOORSND(5, "doormove5");
	DOORSND(6, "doormove6");
	DOORSND(7, "doormove7");
	DOORSND(8, "doormove8");
	DOORSND(9, "doormove9");
	DOORSND(10, "doormove10");
	default: return TEXT("None");
	}
}
static UBOOL DoorOpenSoundIsLoop(INT SoundIndex)
{
	switch (SoundIndex)
	{
	case 6:
	case 7:
		return TRUE;
	default:
		return FALSE;
	}
}
static const TCHAR* GetDoorStopSound(INT SoundIndex)
{
	switch (SoundIndex)
	{
	DOORSND(1, "doorstop1");
	DOORSND(2, "doorstop2");
	DOORSND(3, "doorstop3");
	DOORSND(4, "doorstop4");
	DOORSND(5, "doorstop5");
	DOORSND(6, "doorstop6");
	DOORSND(7, "doorstop7");
	DOORSND(8, "doorstop8");
	default:	return TEXT("None");
	}
}
#undef DOORSND

#define PLATSND(idx,name) case idx: return TEXT("Sound'HL_Ambience.Plats." name "'")
static const TCHAR* GetPlatLoopSound(INT SoundIndex)
{
	switch (SoundIndex)
	{
	PLATSND(1, "bigmove1");
	PLATSND(2, "bigmove2");
	PLATSND(3, "elevmove1");
	PLATSND(4, "elevmove2");
	PLATSND(5, "elevmove3");
	PLATSND(6, "freightmove1");
	PLATSND(7, "freightmove2");
	PLATSND(8, "heavymove1");
	PLATSND(9, "rackmove1");
	PLATSND(10, "railmove1");
	PLATSND(11, "squeekmove1");
	PLATSND(12, "talkmove1");
	PLATSND(13, "talkmove2");
	default: return TEXT("None");
	}
}
static const TCHAR* GetPlatStopSound(INT SoundIndex)
{
	switch (SoundIndex)
	{
	PLATSND(1, "bigstop1");
	PLATSND(2, "bigstop2");
	PLATSND(3, "freightstop1");
	PLATSND(4, "heavystop2");
	PLATSND(5, "rackstop1");
	PLATSND(6, "railstop1");
	PLATSND(7, "squeekstop1");
	PLATSND(8, "talkstop1");
	default: return TEXT("None");
	}
}
#undef PLATSND

#define BUTTONSND(idx,name) case idx: return TEXT("Sound'HL_Ambience.Buttons." name "'")
static const TCHAR* GetButtonSound(INT SoundIndex)
{
	switch (SoundIndex)
	{
	BUTTONSND(1, "button1");
	BUTTONSND(2, "button2");
	BUTTONSND(3, "button3");
	BUTTONSND(4, "button4");
	BUTTONSND(5, "button5");
	BUTTONSND(6, "button6");
	BUTTONSND(7, "button7");
	BUTTONSND(8, "button8");
	BUTTONSND(9, "button9");
	BUTTONSND(10, "button10");
	BUTTONSND(11, "button11");
	BUTTONSND(12, "latchlocked1");
	BUTTONSND(13, "latchunlocked1");
	BUTTONSND(14, "lightswitch2");
	BUTTONSND(21, "lever1");
	BUTTONSND(22, "lever2");
	BUTTONSND(23, "lever3");
	BUTTONSND(24, "lever4");
	BUTTONSND(25, "lever5");
	default: return TEXT("Sound'HL_Ambience.Buttons.button9'");
	}
}
#undef BUTTONSND

#define VOXPRESET(idx,name) case idx: return TEXT("VX_" name "_")
static const TCHAR* GetVOXPreset(INT PresetIndex)
{
	switch (PresetIndex)
	{
		VOXPRESET(0, "C1A0");
		VOXPRESET(1, "C1A1");
		VOXPRESET(2, "C1A2");
		VOXPRESET(3, "C1A3");
		VOXPRESET(4, "C1A4");
		VOXPRESET(5, "C2A1");
		VOXPRESET(6, "C2A2");
		VOXPRESET(7, "C2A3");
		VOXPRESET(8, "C2A4");
		VOXPRESET(9, "C2A5");
		VOXPRESET(10, "C3A1");
		VOXPRESET(11, "C3A2");
		VOXPRESET(12, "C3A3");
	default: return nullptr;
	}
}
#undef VOXPRESET

static void FillVoxSentences(FOutputDevice& Out, const TCHAR* varName, const TCHAR* baseName)
{
	TCHAR tempFullName[512];
	for (INT i = 0; i < 32; ++i)
	{
		appSnprintf(tempFullName, 512, TEXT("HL_Assets.VX_%ls%i"), baseName, i);
		UObject* Result = UObject::StaticLoadObject(UObject::StaticClass(), NULL, tempFullName, nullptr, LOAD_NoFail | LOAD_Quiet, nullptr);
		if (Result)
			Out.Logf(TEXT("%ls(%i)=%ls'%ls'"), varName, i, Result->GetClass()->GetName(), Result->GetPathName());
		else
			break;
	}
}

static void ExportMover(EXPENTPARMS)
{
	const UBOOL IsTrainMover = (Entity.IsClass(TEXT("func_tracktrain")) || Entity.IsClass(TEXT("func_train")));
	BSPMODEL* M = nullptr;
	{
		const INT mdlIndex = Entity.GetModelBrushNum();
		M = nullptr;
		if (mdlIndex != INDEX_NONE)
			M = Loader.GetModel(mdlIndex);

		INT RAMT = Entity.IntValue(TEXT("renderamt"));
		INT RMODE = Entity.IntValue(TEXT("rendermode"));
		UBOOL bHiddenMover = (RAMT == 0 && (RMODE == 2 || RMODE == 4));

		if (bHiddenMover && Entity.HasSpawnFlag(8))
		{
			Out.Log(TEXT("DrawType=DT_Sprite"));
			Out.Log(TEXT("bHidden=True"));
			Out.Log(TEXT("bCollideActors=False"));
			Out.Log(TEXT("Texture=Texture'Engine.S_Patrol'"));
		}
		else
		{
			DWORD ForcedFlags = PF_None;
			if (RMODE)
			{
				if(RMODE == 2)
					ForcedFlags |= (PF_Translucent | PF_Unlit);
				if(RMODE == 5)
					ForcedFlags |= PF_Translucent;
			}
			if(bHiddenMover)
				ForcedFlags |= PF_Invisible;
			else if (Entity.HasSpawnFlag(8))
				ForcedFlags |= PF_NotSolid;

			if (M)
			{
				auto* Test = Entity.Value(TEXT("origin"));
				Loader.WriteModel(*M, Out, FALSE, TRUE, (Test == nullptr), TRUE, ForcedFlags, (IsTrainMover ? 32768 : 0));
			}
		}
	}

	INT noSolidFlag = 8;

	if (IsTrainMover)
	{
		if (Entity.IsClass(TEXT("func_train")))
		{
			Out.Log(TEXT("sf_FixedOrientation=True"));
			if (!Entity.Value(TEXT("targetname")))
				Out.Log(TEXT("sf_StartActive=True"));
		}
		ParseOptScaledNumParm(TEXT("startspeed"), TEXT("MaxSpeed"), Entity, Out);
		ParseOptScaledNumParm(TEXT("speed"), TEXT("TrainSpeed"), Entity, Out);
		ParseOptScaledNumParm(TEXT("wheels"), TEXT("TurnDistance"), Entity, Out);
		ParseOptScaledNumParm(TEXT("height"), TEXT("TrackHeight"), Entity, Out);
		INT i = Entity.IntValue(TEXT("sounds"));
		if (i != INT_NF)
		{
			auto* Res = Entity.Value(TEXT("StartSound"));
			if (Res)
				Out.Logf(TEXT("OpeningSound=Sound'%ls'"), Res);
			else if (i == 1)
				Out.Log(TEXT("OpeningSound=Sound'HL_Ambience.Plats.ttrain_start1'"));
			Res = Entity.Value(TEXT("StopSound"));
			if (Res)
				Out.Logf(TEXT("OpenedSound=Sound'%ls'"), Res);
			else if (i == 1)
				Out.Log(TEXT("OpenedSound=Sound'HL_Ambience.Plats.ttrain_brake1'"));
			Res = Entity.Value(TEXT("MoveSound"));
			if (Res)
				Out.Logf(TEXT("MoveAmbientSound=Sound'%ls'"), Res);
			else Out.Logf(TEXT("MoveAmbientSound=Sound'HL_Ambience.Plats.ttrain%i'"), i);
		}
		else
		{
			auto* Res = Entity.Value(TEXT("StartSound"));
			Out.Logf(TEXT("OpeningSound=Sound'%ls'"), Res ? Res : TEXT("HL_Ambience.Plats.ttrain_start1"));
			Res = Entity.Value(TEXT("StopSound"));
			Out.Logf(TEXT("OpenedSound=Sound'%ls'"), Res ? Res : TEXT("HL_Ambience.Plats.ttrain_brake1"));
		}

		if (Entity.HasSpawnFlag(1))
			Out.Log(TEXT("sf_NoPitch=True"));
		if (Entity.HasSpawnFlag(2))
			Out.Log(TEXT("sf_NoUserControl=True"));
		if (Entity.HasSpawnFlag(16))
			Out.Log(TEXT("sf_FixedOrientation=True"));
		if (Entity.HasSpawnFlag(256))
			Out.Log(TEXT("sf_SoundPitchBySpeed=True"));
		if (Entity.HasSpawnFlag(512))
			Out.Log(TEXT("MoverEncroachType=ME_IgnoreWhenEncroach"));
		Out.Log(TEXT("Rotation=(Yaw=32768)"));
	}
	else if (Entity.IsClass(TEXT("func_door")) || Entity.IsClass(TEXT("func_button")) || Entity.IsClass(TEXT("func_door_rotating")) || Entity.IsClass(TEXT("func_platrot")))
	{
		const UBOOL IsElevator = Entity.IsClass(TEXT("func_platrot"));

		INT i = Entity.IntValue(TEXT("movesnd"));
		if (i != INT_NF)
		{
			if(IsElevator)
				Out.Logf(TEXT("MoveAmbientSound=%ls"), GetPlatLoopSound(i));
			else if (DoorOpenSoundIsLoop(i))
				Out.Logf(TEXT("MoveAmbientSound=%ls"), GetDoorLoopSound(i));
			else
			{
				const TCHAR* Snd = GetDoorLoopSound(i);
				Out.Logf(TEXT("OpeningSound=%ls"), Snd);
				Out.Logf(TEXT("ClosingSound=%ls"), Snd);
			}
		}
		i = Entity.IntValue(TEXT("stopsnd"));
		if (i != INT_NF)
		{
			const TCHAR* Snd = IsElevator ? GetPlatStopSound(i) : GetDoorStopSound(i);
			Out.Logf(TEXT("OpenedSound=%ls"), Snd);
			Out.Logf(TEXT("ClosedSound=%ls"), Snd);
		}

		FVector SecPos(0.f, 0.f, 0.f);
		FRotator SecRot(0, 0, 0);

		if (Entity.IsClass(TEXT("func_door")) || Entity.IsClass(TEXT("func_button")))
		{
			Entity.Rotation = FRotator(0, 0, 0);

			FLOAT Speed = 100.f;
			FLOAT Distance = 100.f;
			FRotator R(0, 0, 0);
			i = Entity.IntValue(TEXT("angle"));
			if (i != INT_NF)
			{
				if (i == -1)
					R.Pitch = 16384;
				else if (i == -2)
					R.Pitch = -16384;
				else R.Yaw = -ToUnrealAngles(i);
			}
			FLOAT f = Entity.FloatValue(TEXT("speed"));
			if (f != FLOAT_NF)
				Speed = f * ToUnrScale;
			const FVector MoveDir = R.Vector();
			if (M)
			{
				FVector BoxSize(M->nMaxs[0] - M->nMins[0], M->nMaxs[1] - M->nMins[1], M->nMaxs[2] - M->nMins[2]);
				Distance = FBoxPushOut(MoveDir, BoxSize) * ToUnrScale;
			}
			f = Entity.FloatValue(TEXT("lip"));
			if (f != FLOAT_NF)
				Distance -= (f * ToUnrScale);

			SecPos = MoveDir * Distance;
			Out.Logf(TEXT("MoveTime=%f"), Distance / Max(Speed, 0.00001f));
		}
		else if (Entity.IsClass(TEXT("func_platrot")))
		{
			FLOAT Speed = 100.f;
			FLOAT Distance = 100.f;

			FLOAT f = Entity.FloatValue(TEXT("speed"));
			if (f != FLOAT_NF)
				Speed = f * ToUnrScale;
			f = Entity.FloatValue(TEXT("height"));
			if (f != FLOAT_NF)
				Distance = f * ToUnrScale;
			SecPos = FVector(0.f, 0.f, -Distance);
			Out.Logf(TEXT("MoveTime=%f"), Abs(Distance) / Max(Speed, 0.00001f));

			i = Entity.IntValue(TEXT("rotation"));
			if (i != INT_NF)
			{
				FRotator R(0, 0, 0);
				if (Entity.HasSpawnFlag(64))
					R.Roll = ToUnrealAngles(i);
				else if (Entity.HasSpawnFlag(128))
					R.Pitch = ToUnrealAngles(i);
				else R.Yaw = -ToUnrealAngles(i);
				SecRot = R;
			}

			// Some weird quirk, if elevator has tag, start rotation at keyframe 2.
			if (Entity.Value(TEXT("targetname")))
			{
				Entity.Rotation = SecRot;
				SecRot = FRotator(-SecRot.Pitch, -SecRot.Yaw, -SecRot.Roll);
			}
		}
		else
		{
			FRotator R(0, 0, 0);
			i = Entity.IntValue(TEXT("angle"));
			if (i != INT_NF)
			{
				if (i == -1)
					R.Pitch = 16384;
				else if (i == -2)
					R.Pitch = -16384;
				else R.Yaw = -ToUnrealAngles(i);
			}
			Entity.Rotation = R;

			FLOAT totalRot = 0.f;
			INT RotAng = 0;
			INT i = Entity.IntValue(TEXT("distance"));
			if (i != INT_NF)
			{
				totalRot = i;
				RotAng = ToUnrealAngles(i);
				FRotator Rot(0, 0, 0);

				if (Entity.HasSpawnFlag(2))
					RotAng = -RotAng;
				if (Entity.HasSpawnFlag(64))
					Rot.Roll = RotAng;
				if (Entity.HasSpawnFlag(128))
					Rot.Pitch = -RotAng;
				if ((Entity.GetSpawnFlags() & (64 | 128)) == 0)
					Rot.Yaw = -RotAng;
				SecRot = Rot;
				if ((Entity.GetSpawnFlags() & (16 | 64 | 128)) == 0)
				{
					Rot.Yaw = RotAng;
					Out.Logf(TEXT("KeyRot(2)=(Yaw=%i,Pitch=%i,Roll=%i)"), Rot.Yaw, Rot.Pitch, Rot.Roll);
					Out.Log(TEXT("bDirectionalOpen=True"));
					if (Entity.HasSpawnFlag(2))
						Out.Log(TEXT("bReverseRotate=True"));
				}
			}

			FLOAT rotSpeed = 100.f;
			FLOAT f = Entity.FloatValue(TEXT("speed"));
			if (f != FLOAT_NF)
				rotSpeed = f;

			Out.Logf(TEXT("MoveTime=%f"), Abs<FLOAT>(totalRot) / Max(FLOAT(rotSpeed), 1.f));
		}

		if (!SecPos.IsZero())
			Out.Logf(TEXT("KeyPos(1)=(X=%f,Y=%f,Z=%f)"), SecPos.X, SecPos.Y, SecPos.Z);
		if (!SecRot.IsZero())
			Out.Logf(TEXT("KeyRot(1)=(Yaw=%i,Pitch=%i,Roll=%i)"), SecRot.Yaw, SecRot.Pitch, SecRot.Roll);

		if (IsElevator)
		{
			const TCHAR* StateType = TEXT("TriggerOpenTimed");
			if (Entity.HasSpawnFlag(1))
				StateType = TEXT("TriggerToggle");
			Out.Logf(TEXT("InitialState=\"%ls\""), StateType);
		}
		else
		{
			UBOOL bIsTriggeredDoor = Entity.HasSpawnFlag(256);
			if (!bIsTriggeredDoor && Entity.Value(TEXT("targetname"))) // Permanently locked door.
			{
				i = Entity.IntValue(TEXT("locked_sound"), 0);
				if (i)
					Out.Logf(TEXT("LockedSound=%ls"), GetButtonSound(i));
				i = Entity.IntValue(TEXT("locked_sentence"), 0);
				if (i)
				{
					static const TCHAR* VoxNames[] = { TEXT("NA"), TEXT("ND"), TEXT("NF"), TEXT("NFIRE"), TEXT("NCHEM"), TEXT("NRAD"), TEXT("NCON"), TEXT("NH"), TEXT("NG") };
					if (i <= ARRAY_COUNT(VoxNames))
						FillVoxSentences(Out, TEXT("LockedLines"), VoxNames[i - 1]);
				}
			}

			if (Entity.HasSpawnFlag(1))
				Out.Log(TEXT("bStartsOpen=True"));
			if (Entity.HasSpawnFlag(32))
			{
				const TCHAR* StateType = TEXT("TriggerToggle");
				if (!bIsTriggeredDoor)
					StateType = TEXT("BumpToggle");
				Out.Logf(TEXT("InitialState=\"%ls\""), StateType);
			}
			else if (!bIsTriggeredDoor)
				Out.Log(TEXT("InitialState=\"BumpOpenTimed\""));
			if (Entity.HasSpawnFlag(512))
				Out.Log(TEXT("bPlayersOnly=True"));
		}

		FLOAT f = Entity.FloatValue(TEXT("wait"));
		if (f != FLOAT_NF)
		{
			if (f < 0.f)
				Out.Log(TEXT("bTriggerOnceOnly=True"));
			else Out.Logf(TEXT("StayOpenTime=%f"), f);
		}
		else Out.Log(TEXT("StayOpenTime=3.0"));
	}
	else if (Entity.IsClass(TEXT("func_breakable")) || Entity.IsClass(TEXT("func_pushable")))
	{
		UBOOL IsUnbreakable = FALSE;
		ParseOptNumberParm(TEXT("health"), TEXT("Health"), Entity, Out);
		ParseOptNumberParm(TEXT("explodemagnitude"), TEXT("ExplosionMag"), Entity, Out);

		INT i = Entity.IntValue(TEXT("material"), INDEX_NONE);
		if (i >= 0 && i <= 8)
		{
			IsUnbreakable = (i == 7); // Unbreakable glass.
			static const TCHAR* MatNames[] = { TEXT("EST_Glass"), TEXT("EST_Wood") ,TEXT("EST_Metal") ,TEXT("EST_Flesh") ,TEXT("EST_Custom01") ,TEXT("EST_Custom02") ,TEXT("EST_Custom00") ,TEXT("EST_Glass"),TEXT("EST_Rock") };
			Out.Logf(TEXT("MaterialType=%ls"), MatNames[i]);
		}
		i = Entity.IntValue(TEXT("spawnobject"));
		if (i >= 0 && i <= 21)
		{
			static const TCHAR* ItemClasses[] = { TEXT(""),TEXT("I_Battery"),TEXT("I_HealthKit"),TEXT("W_Pistol"),TEXT("A_Pistol"),TEXT("W_SMG"),TEXT("A_SMG"),TEXT("A_SMG_Grenade"),TEXT("W_Shotgun"),TEXT("A_Shotgun"),TEXT("W_Crossbow"),TEXT("A_Crossbow"),
				TEXT("W_Magnum"),TEXT("A_Magnum"),TEXT("W_RPG"),TEXT("A_RPG"),TEXT("A_Gauss"),TEXT("W_Grenade"),TEXT("W_Tripmine"),TEXT("W_RemoteExp"),TEXT("W_Snark"),TEXT("W_HornetGun") };
			Out.Logf(TEXT("Contents=Class'%ls'"), ItemClasses[i]);
		}

		if (Entity.IsClass(TEXT("func_breakable")))
		{
			Out.Log(TEXT("bPushable=False"));
			
			if (!IsUnbreakable)
			{
				Out.Log(TEXT("bTriggerBreakable=True"));
				if (!Entity.HasSpawnFlag(1))
				{
					Out.Log(TEXT("bDamageTriggered=True"));
					if (Entity.HasSpawnFlag(2))
						Out.Log(TEXT("bTouchBreakable=True"));
					if (Entity.HasSpawnFlag(4))
						Out.Log(TEXT("bPressureBreakable=True"));
					if (Entity.HasSpawnFlag(256))
						Out.Log(TEXT("bCrowbarBreakable=True"));
				}
			}
		}
	}
	else if (Entity.IsClass(TEXT("func_pendulum")))
	{
		Entity.Rotation = FRotator(0, 0, 0);

		Out.Logf(TEXT("InitialState=\"%ls\""), Entity.HasSpawnFlag(1) ? TEXT("ConstantLoop") : TEXT("TriggeredLoop"));
		Out.Log(TEXT("MoverGlideType=MV_GlideByTime"));

		INT RotAng = 0;
		INT i = Entity.IntValue(TEXT("distance"));
		if (i != INT_NF)
		{
			RotAng = ToUnrealAngles(i);
			FRotator Rot(0, 0, 0);

			if (Entity.HasSpawnFlag(64))
				Rot.Roll = RotAng;
			if (Entity.HasSpawnFlag(128))
				Rot.Pitch = RotAng;
			if ((Entity.GetSpawnFlags() & (64 | 128)) == 0)
				Rot.Yaw = -RotAng;
			Out.Logf(TEXT("KeyRot(1)=(Yaw=%i,Pitch=%i,Roll=%i)"), Rot.Yaw*2, Rot.Pitch*2, Rot.Roll*2);
			Out.Logf(TEXT("BaseRot=(Yaw=%i,Pitch=%i,Roll=%i)"), -Rot.Yaw, -Rot.Pitch, -Rot.Roll);
			Out.Logf(TEXT("Rotation=(Yaw=%i,Pitch=%i,Roll=%i)"), -Rot.Yaw, -Rot.Pitch, -Rot.Roll);
			Entity.Rotation = FRotator(-Rot.Pitch, -Rot.Yaw, -Rot.Roll);
		}

		INT Speed = 32000;
		i = Entity.IntValue(TEXT("speed"));
		if (i != INT_NF)
			Speed = ToUnrealAngles(i);

		Out.Logf(TEXT("MoveTime=%f"), Abs<FLOAT>(RotAng * 2.71828f) / Max(FLOAT(Speed), 0.1f));
		Out.Logf(TEXT("StayOpenTime=0"));
	}
	else if (Entity.IsClass(TEXT("func_rotating")))
	{
		ParseOptScaledNumParm(TEXT("fanfriction"), TEXT("m_flFanFriction"), Entity, Out, 1.f / 100.f);
		ParseOptScaledNumParm(TEXT("speed"), TEXT("m_flFanSpeed"), Entity, Out, 1.f);
		FLOAT f = Entity.FloatValue(TEXT("Volume"));
		if (f != FLOAT_NF)
			Out.Logf(TEXT("SoundVolume=%i"), Clamp<INT>(appRound(f / 10.f * 128.f), 0, 255));
		const auto* Res = Entity.Value(TEXT("spawnorigin"));
		if (Res)
		{
			FVector SpawnOrigin = GetVectorOf(Res);
			if (!SpawnOrigin.IsZero())
				Entity.Location = ToUnrealScale(SpawnOrigin);
		}

		INT i = Entity.IntValue(TEXT("sounds"), 0);
		if (i > 0 && i <= 5)
			Out.Logf(TEXT("MoveAmbientSound=Sound'HL_Ambience.fans.fan%i'"), i);
		else
		{
			Res = Entity.Value(TEXT("message"));
			if (Res)
				Out.Logf(TEXT("MoveAmbientSound=%ls"), *TranslateSound(Res));
		}

		FVector rotAxis(0.f, 0.f, 0.f);
		if (Entity.HasSpawnFlag(4))
			rotAxis.Z = 1.f;
		else if (Entity.HasSpawnFlag(8))
			rotAxis.X = 1.f;
		else
			rotAxis.Y = 1.f;
		if (Entity.HasSpawnFlag(2))
			rotAxis = -rotAxis;
		Out.Logf(TEXT("RotationAxis=(X=%f,Y=%f,Z=%f)"), rotAxis.X, rotAxis.Y, rotAxis.Z);

		if (Entity.HasSpawnFlag(1))
			Out.Log(TEXT("bInitiallyActive=True"));
		if (Entity.HasSpawnFlag(128))
			Out.Log(TEXT("SoundRadius=32"));
		else if (Entity.HasSpawnFlag(256))
			Out.Log(TEXT("SoundRadius=255"));
		else if (Entity.HasSpawnFlag(512))
			Out.Log(TEXT("SoundRadius=128"));
		if (Entity.HasSpawnFlag(32))
			Out.Log(TEXT("bHurtOnTouch=True"));

		noSolidFlag = 64;
	}
	if (Entity.HasSpawnFlag(noSolidFlag))
	{
		Out.Log(TEXT("bCollideActors=False"));
		Out.Log(TEXT("bBlockActors=False"));
		Out.Log(TEXT("bBlockPlayers=False"));
	}
}
static void ExportTeleporter(EXPENTPARMS)
{
	Loader.WriteModelVolume(Out, Entity);

	auto* LM = Entity.Value(TEXT("landmark"));
	auto* MP = Entity.Value(TEXT("Map"));
	if (MP)
		Out.Logf(TEXT("URL=\"%ls#%ls\""), MP, LM ? LM : TEXT("go"));
}
static void ExportLadder(EXPENTPARMS)
{
	Loader.WriteModelVolume(Out, Entity);
}
static void ExportSoundTrigger(EXPENTPARMS)
{
	auto* Res = Entity.Value(TEXT("Message"));
	if (Res && Res[0] == '!')
		Out.Logf(TEXT("VoxLine=VX_%ls"), (Res + 1));
	else
	{
		if (Entity.SpecialFlags)
		{
			if (Res)
				Out.Logf(TEXT("AmbientSound=%ls"), *TranslateSound(Res));
			FLOAT f = Entity.FloatValue(TEXT("radius"));
			if (f != FLOAT_NF)
				Out.Logf(TEXT("SoundRadius=%i"), Clamp<INT>(appRound(f * ToUnrScale / 25.f) - 1, 0, 255));
			f = Entity.FloatValue(TEXT("health"));
			if (f != FLOAT_NF)
				Out.Logf(TEXT("SoundVolume=%i"), Clamp<INT>(appRound(f / 10.f * 128.f), 0, 255));
			f = Entity.FloatValue(TEXT("pitch"));
			if (f != FLOAT_NF)
				Out.Logf(TEXT("SoundPitch=%i"), Clamp<INT>(appRound(f / 100.f * 64.f), 0, 255));
			return;
		}
		if (Res)
			Out.Logf(TEXT("SoundEffect=%ls"), *TranslateSound(Res));
	}
	ParseOptScaledNumParm(TEXT("radius"), TEXT("EffectRadius"), Entity, Out);
	ParseOptScaledNumParm(TEXT("health"), TEXT("EffectVolume"), Entity, Out, 1.f / 10.f);
	ParseOptScaledNumParm(TEXT("pitch"), TEXT("EffectPitch"), Entity, Out, 1.f / 100.f);
	ParseOptNameParm(TEXT("SourceEntityName"), TEXT("SourceEntityName"), Entity, Out);

	Out.Logf(TEXT("sf_PlayEverywhere=%ls"), Entity.HasSpawnFlag(1) ? GTrue : GFalse);
	Out.Logf(TEXT("sf_StartPlaying=%ls"), Entity.HasSpawnFlag(16) ? GFalse : GTrue);
	Out.Logf(TEXT("sf_LoopSound=%ls"), Entity.HasSpawnFlag(32) ? GFalse : GTrue);
}
static void ExportEnvFade(EXPENTPARMS)
{
	ParseOptNumberParm(TEXT("duration"), TEXT("Duration"), Entity, Out);
	ParseOptNumberParm(TEXT("holdtime"), TEXT("HoldTime"), Entity, Out);
	FLOAT f = Entity.FloatValue(TEXT("renderamt"));
	if (f != FLOAT_NF)
		Out.Logf(TEXT("FadeOpacity=%f"), (f / 255.f));
	auto* Res = Entity.Value(TEXT("rendercolor"));
	if (Res)
	{
		FColor C = GetColorOf(Res);
		Out.Logf(TEXT("FadeColor=(R=%i,G=%i,B=%i)"), INT(C.R), INT(C.G), INT(C.B));
	}
	ParseOptNumberParm(TEXT("ReverseFadeDuration"), TEXT("ReverseFadeDuration"), Entity, Out);
	
	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("sf_FadeFrom=True"));
	if (Entity.HasSpawnFlag(8))
		Out.Log(TEXT("sf_StayOut=True"));
}
static void ExportBeam(EXPENTPARMS)
{
	ParseOptNumberParm(TEXT("StrikeTime"), TEXT("RestrikeTime"), Entity, Out);
	ParseOptNumberParm(TEXT("Radius"), TEXT("BeamRadius"), Entity, Out);
	ParseOptNumberParm(TEXT("life"), TEXT("BeamLifeTime"), Entity, Out);
	ParseOptNumberParm(TEXT("damage"), TEXT("BeamDamage"), Entity, Out);

	FLOAT f = Entity.FloatValue(TEXT("BoltWidth"));
	if (f != FLOAT_NF)
	{
		f /= 10.f;
		Out.Logf(TEXT("StartingScale=(Min=%f,Max=%f)"), (f * ToUnrScale * 0.8), (f * ToUnrScale));
	}
	ParseOptNameParm(TEXT("LightningStart"), TEXT("BeamStartTag"), Entity, Out);
	ParseOptNameParm(TEXT("LightningEnd"), TEXT("BeamEndTag"), Entity, Out);

	INT i = Entity.IntValue(TEXT("TouchType"), INDEX_NONE);
	if (i >= 0 && i <= 4)
	{
		static const TCHAR* TouchTypes[] = { TEXT("BTT_None"), TEXT("BTT_Players") ,TEXT("BTT_NPC") ,TEXT("BTT_PlayersNPC") ,TEXT("BTT_PlayersNPCProps") };
		Out.Logf(TEXT("TouchType=%ls"), TouchTypes[i]);
	}
	i = Entity.IntValue(TEXT("ClipStyle"));
	if (i >= 0 && i <= 2)
	{
		static const TCHAR* ClipStyles[] = { TEXT("CLIP_None"), TEXT("CLIP_BSP") ,TEXT("CLIP_AllColliding") };
		Out.Logf(TEXT("ClipMode=%ls"), ClipStyles[i]);
	}
	auto* Res = Entity.Value(TEXT("targetpoint"));
	if (Res)
	{
		const FVector v = ToUnrealScale(GetVectorOf(Res));
		Out.Logf(TEXT("BeamEndPointVec=(X=%f,Y=%f,Z=%f)"), v.X, v.Y, v.Z);
	}
	Res = Entity.Value(TEXT("Texture"));
	if (Res)
	{
		FString S = FString::GetFilenameOnlyStr(Res);
		Out.Logf(TEXT("ParticleTextures(0)=Texture'%ls'"), *S);
	}
	f = Entity.FloatValue(TEXT("NoiseAmplitude"));
	if (f != FLOAT_NF)
	{
		f *= 0.15f;
		Out.Logf(TEXT("NoiseRange=(X=(Min=%f,Max=%f),Y=(Min=%f,Max=%f),Z=(Min=%f,Max=%f))"), -f, f, -f, f, -f, f);
		Out.Log(TEXT("bDoBeamNoise=True"));
		Out.Log(TEXT("Segments=6"));
	}
	f = Entity.FloatValue(TEXT("renderamt"));
	if (f != FLOAT_NF)
	{
		f /= 255.f;
		Out.Logf(TEXT("FadeInMaxAmount=%f"), f);
	}
	Res = Entity.Value(TEXT("rendercolor"));
	if (Res)
	{
		FColor c = GetColorOf(Res);
		FVector v = c.Vector();
		Out.Logf(TEXT("ParticleColor=(X=(Min=%f,Max=%f),Y=(Min=%f,Max=%f),Z=(Min=%f,Max=%f))"), v.X * 0.9f, v.X, v.Y * 0.9f, v.Y, v.Z * 0.9f, v.Z);
	}

	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("bInitiallyOn=True"));
	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("bToggleBeam=True"));
	if (Entity.HasSpawnFlag(4))
		Out.Log(TEXT("bRandomStrike=True"));
}
static void ExportPusher(EXPENTPARMS)
{
	Loader.WriteModelVolume(Out, Entity);
	FRotator R(0, 0, 0);
	INT i = Entity.IntValue(TEXT("angle"));
	if (i != INT_NF)
	{
		if (i == -1)
			R.Pitch = 16384;
		else if (i == -2)
			R.Pitch = -16384;
		else R.Yaw = -ToUnrealAngles(i);
	}
	Out.Logf(TEXT("PushDirection=(Yaw=%i,Pitch=%i,Roll=0)"), R.Yaw, R.Pitch);

	ParseOptScaledNumParm(TEXT("Speed"), TEXT("PushSpeed"), Entity, Out);

	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("bInitiallyActive=False"));
	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("bPushOnceOnly=True"));
}
static void ExportMusic(EXPENTPARMS)
{
	Loader.WriteModelOrigin(Out, Entity);
	INT i = Entity.IntValue(TEXT("Health"), INDEX_NONE);
	if (i <= 0)
		Out.Log(TEXT("bSilence=True"));
	else Out.Logf(TEXT("Song=%ls"), GetMusicName(i));
	
	Out.Log(TEXT("RemoteRole=ROLE_None"));
	Out.Log(TEXT("bCollideActors=False"));
}
static void ExportTriggerHurt(EXPENTPARMS)
{
	Loader.WriteModelVolume(Out, Entity);

	INT i = Entity.IntValue(TEXT("DamageType"), 0);
	static const TCHAR* HurtDamageType[] = { TEXT("Damage"), TEXT("Crushed"), TEXT("Shot"), TEXT("Hacked"), TEXT("Burned"), TEXT("Frozen"), TEXT("Fell"), TEXT("Exploded"), TEXT("Blunt"), TEXT("Jolted"), TEXT("Zapped"), TEXT("Drowned"),
		TEXT("Paralyzed"), TEXT("NerveGas"), TEXT("Poisoned"), TEXT("Toxin"), TEXT("Drowned"), TEXT("Chemical"), TEXT("Burned"), TEXT("Frozen") };
	if( i==0 )
		Out.Logf(TEXT("DamageType=\"%ls\""), HurtDamageType[0]);
	else
	{
		for (INT j = 1; j < ARRAY_COUNT(HurtDamageType); ++j)
		{
			const INT mask = 1 << (j - 1);
			if (i & mask)
			{
				Out.Logf(TEXT("DamageType=\"%ls\""), HurtDamageType[j]);
				break;
			}
		}
		if (i & DMG_RADIATION)
			Out.Log(TEXT("bRadiationDamage=True"));
	}

	ParseOptNumberParm(TEXT("dmg"), TEXT("Damage"), Entity, Out);

	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("bDamageOnceOnly=True"));
	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("bInitiallyActive=False"));
	if (Entity.HasSpawnFlag(8))
		Out.Log(TEXT("bMonsterOnly=True"));
	if (Entity.HasSpawnFlag(32))
		Out.Log(TEXT("bPlayerOnly=True"));
}
static void ExportAutoSaver(EXPENTPARMS)
{
	Loader.WriteModelOrigin(Out, Entity);
}
static void ExportTriggerOnce(EXPENTPARMS)
{
	Loader.WriteModelVolume(Out, Entity);
	if (Entity.IsClass(TEXT("trigger_once")))
		Out.Log(TEXT("bOnceOnly=True"));
	ParseOptScaledNumParm(TEXT("wait"), TEXT("RepeatTriggerTime"), Entity, Out, 1.f);

	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("bNotPlayers=True"));
	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("bMonsterOnly=True"));
	if (Entity.HasSpawnFlag(4))
		Out.Log(TEXT("bAllowDecorations=True"));
}
static void ExportScriptedSentence(EXPENTPARMS)
{
	const TCHAR* Res = Entity.Value(TEXT("sentence"));
	if (Res && Res[0] == '!')
		Out.Logf(TEXT("VoiceLine=VX_%ls"), (Res + 1));
	ParseOptNameParm(TEXT("entity"), TEXT("m_iszEntity"), Entity, Out);
	ParseOptNameParm(TEXT("listener"), TEXT("Listener"), Entity, Out);
	ParseOptScaledNumParm(TEXT("Radius"), TEXT("m_flRadius"), Entity, Out);
	ParseOptScaledNumParm(TEXT("Delay"), TEXT("StartDelay"), Entity, Out, 1.f);
	ParseOptScaledNumParm(TEXT("duration"), TEXT("SentanceDuration"), Entity, Out, 1.f);
	ParseOptScaledNumParm(TEXT("refire"), TEXT("RefireTime"), Entity, Out, 1.f);

	if (Entity.HasSpawnFlag(1))
		Out.Log(TEXT("bOnceOnly=True"));
	if (Entity.HasSpawnFlag(2))
		Out.Log(TEXT("bFollowersOnly=True"));
	if (Entity.HasSpawnFlag(4))
		Out.Log(TEXT("bAllowInterrupt=True"));
	if (Entity.HasSpawnFlag(8))
		Out.Log(TEXT("bConCurrent=True"));
}
static void ExportEnvRender(EXPENTPARMS)
{
	UBOOL IgnoreScaleGlow = FALSE;
	if (!Entity.HasSpawnFlag(4))
	{
		Out.Log(TEXT("bChangeStyle=True"));
		INT i = Entity.IntValue(TEXT("rendermode"));
		if (i > 1 && i <= 5)
			Out.Log(TEXT("TargetStyle=STY_Translucent"));
		else IgnoreScaleGlow = TRUE;
	}
	if (!Entity.HasSpawnFlag(1))
	{
		Out.Log(TEXT("bChangeRenderFX=True"));
		const TCHAR* n = GetRenderFX(Entity.IntValue(TEXT("renderfx")));
		if (n)
			Out.Logf(TEXT("TargetRenderFX=%ls"), n);
	}
	if (!Entity.HasSpawnFlag(2))
	{
		Out.Log(TEXT("bChangeScaleGlow=True"));
		if (IgnoreScaleGlow)
			Out.Log(TEXT("TargetScaleGlow=1.0"));
		else ParseOptScaledNumParm(TEXT("renderamt"), TEXT("TargetScaleGlow"), Entity, Out, 1.f);
	}
	if (!Entity.HasSpawnFlag(8))
	{
		Out.Log(TEXT("bChangeRenderColor=True"));
		const auto* Res = Entity.Value(TEXT("rendercolor"));
		if (Res)
		{
			FColor c = GetColorOf(Res);
			Out.Logf(TEXT("TargetRenderColor=(R=%i,G=%i,B=%i)"), INT(c.R), INT(c.G), INT(c.B));
		}
	}
}
static void ExportSpeaker(EXPENTPARMS)
{
	INT i = Entity.IntValue(TEXT("preset"));
	if (i != INT_NF)
	{
		if (!ActiveVoiceFile)
		{
			FString CurPath = FString::GetFilePathStr(Loader.GetMapURL()) * TEXT("../sound/sentences.txt");
			debugf(TEXT("Loading sentences from: %ls"), *CurPath);
			ActiveVoiceFile = new FVoiceFile(*CurPath);
		}

		const TCHAR* BaseName = GetVOXPreset(i - 1);
		if (BaseName)
		{
			INT ix = 0;
			for (INT j = 0; j < 32; ++j)
			{
				FString Snd = FString::Printf(TEXT("%ls%i"), BaseName, j);
				if (ActiveVoiceFile->HasLine(*Snd))
					Out.Logf(TEXT("VoxLines(%i)=VoiceLineObject'HL_Assets.%ls'"), ix++, *Snd);
				else break;
			}
		}
	}
	if (Entity.HasSpawnFlag(1)) // wait for trigger 'on' to start announcements
		Out.Log(TEXT("bInitiallyActive=False"));
}
static void ExportExplosion(EXPENTPARMS)
{
	INT i = Entity.IntValue(TEXT("iMagnitude"));
	if (i != INT_NF)
		Out.Logf(TEXT("Size=%f"), FLOAT(i - 50) * 0.06f);
	if (Entity.HasSpawnFlag(1)) // when set, ENV_EXPLOSION will not actually inflict damage
		Out.Log(TEXT("Damage=0.0"));
	else if (i != INT_NF)
		Out.Logf(TEXT("Damage=%f"), FLOAT(i));
	if (Entity.HasSpawnFlag(2)) // can this entity be refired?
		Out.Log(TEXT("bRepeat=True"));
	if (Entity.HasSpawnFlag(4)) // don't draw the fireball
		Out.Log(TEXT("bNoFireball=True"));
	if (Entity.HasSpawnFlag(8)) // don't draw the smoke
		Out.Log(TEXT("bNoSmoke=True"));
	if (Entity.HasSpawnFlag(16)) // don't make a scorch mark
		Out.Log(TEXT("bNoScorch=True"));
	if (Entity.HasSpawnFlag(32)) // don't spawn sparks
		Out.Log(TEXT("bNoSparks=True"));
}
static void ExportEarthquake(EXPENTPARMS)
{
	FLOAT f = Entity.FloatValue(TEXT("amplitude"), 1.f) * Entity.FloatValue(TEXT("frequency"), 1.f);
	Out.Logf(TEXT("Magnitude=%f"), f);
	ParseOptScaledNumParm(TEXT("duration"), TEXT("Duration"), Entity, Out, 1.f);
	f = Entity.HasSpawnFlag(1) ? 0.f : Entity.FloatValue(TEXT("radius"));
	if (f != FLOAT_NF)
	{
		if (f <= 0.f)
			f = 1000000.f;
		Out.Logf(TEXT("Radius=%f"), f);
	}
	Out.Log(TEXT("bThrowPlayer=False"));
	Out.Log(TEXT("RemoteRole=ROLE_None"));
}
static void ExportTTeleport(EXPENTPARMS)
{
	Loader.WriteModelVolume(Out, Entity);

	if (Entity.HasSpawnFlag(1)) // allow monsters
		Out.Log(TEXT("bAllowMonsters=True"));
	if (Entity.HasSpawnFlag(2)) // don't allow players
		Out.Log(TEXT("bAllowPlayers=False"));
}
static void ExportFunnel(EXPENTPARMS)
{
	if (Entity.HasSpawnFlag(1)) // reverse effect
		Out.Log(TEXT("bReverseEffect=True"));
}
static void ExportMonsterMaker(EXPENTPARMS)
{
	ParseOptNumberParm(TEXT("m_imaxlivechildren"), TEXT("MaxLiveChildren"), Entity, Out);
	ParseOptNumberParm(TEXT("monstercount"), TEXT("MonsterCount"), Entity, Out);
	ParseOptScaledNumParm(TEXT("delay"), TEXT("Delay"), Entity, Out, 1.f);
	ParseOptNameParm(TEXT("netname"), TEXT("MonsterTag"), Entity, Out);

	const TCHAR* Res = Entity.Value(TEXT("monstertype"));
	if (Res)
	{
		const TCHAR* UCName = GetUCClassName(Res);
		if (!UCName)
			warnf(TEXT("Failed to find monstermaker class '%ls'"), Res);
		else Out.Logf(TEXT("MonsterType=Class'%ls'"), UCName);
	}
}

static void PositionTriggerAtBrush(EXPENTPARMS)
{
	Entity.Rotation = FRotator(0, 0, 0);
	const INT mdlIndex = Entity.GetModelBrushNum();
	if (mdlIndex != INDEX_NONE)
	{
		BSPMODEL* M = Loader.GetModel(mdlIndex);
		if (M)
		{
			CModel* CM = Loader.BuildModel(*M);
			if (CM)
			{
				Entity.pendingModel = CM;
				FVector Org, Ext;
				CM->GetBounds().GetExtentNCentroid(Org, Ext);
				Entity.Location = Org;
				Out.Logf(TEXT("CollisionHeight=%f"), (Ext.Z + 10.f));
				Out.Logf(TEXT("CollisionRadius=%f"), (Max(Ext.X, Ext.Y) + 10.f));
				return;
			}
		}
	}
}

void FTextureSwitch::Export(FOutputDevice& Out) const
{
	Out.Logf(NAME_Add, TEXT("BEGIN OBJECT CLASS=MaterialSequence NAME=%ls"), *MatSeq);
	Out.Logf(TEXT("SequenceItems(0)=(Material=Texture'%ls',DisplayTime=1.0,FadeOutTime=0.0)"), *TexA);
	Out.Logf(TEXT("SequenceItems(1)=(Material=Texture'%ls',DisplayTime=1.0,FadeOutTime=0.0)"), *TexB);
	Out.Log(TEXT("Loop=False"));
	Out.Log(TEXT("Paused=True"));
	Out.Logf(TEXT("MatUSize=%i"), U);
	Out.Logf(TEXT("MatVSize=%i"), V);
	Out.Log(NAME_Remove, TEXT("END OBJECT"));
}
static void ExportHealthCharger(EXPENTPARMS)
{
	PositionTriggerAtBrush(CALLEXPPARMS);

	if (Entity.pendingModel && Entity.pendingModel->FindSwitchTexture())
	{
		Entity.pendingModel->TexSwitch.Export(Out);
		Out.Logf(TEXT("ButtonMaterial=MaterialSequence'%ls'"), *Entity.pendingModel->TexSwitch.MatSeq);
	}
}
static void ExportButton(EXPENTPARMS)
{
	PositionTriggerAtBrush(CALLEXPPARMS);

	if (Entity.pendingModel && Entity.pendingModel->FindSwitchTexture())
	{
		Out.Logf(NAME_Add, TEXT("BEGIN OBJECT CLASS=MaterialSequence NAME=%ls"), *Entity.pendingModel->TexSwitch.MatSeq);
		Out.Logf(TEXT("SequenceItems(0)=(Material=Texture'%ls',DisplayTime=1.0,FadeOutTime=0.0)"), *Entity.pendingModel->TexSwitch.TexA);
		Out.Logf(TEXT("SequenceItems(1)=(Material=Texture'%ls',DisplayTime=1.0,FadeOutTime=0.0)"), *Entity.pendingModel->TexSwitch.TexB);
		Out.Log(TEXT("Loop=False"));
		Out.Log(TEXT("Paused=True"));
		Out.Logf(TEXT("MatUSize=%i"), Entity.pendingModel->TexSwitch.U);
		Out.Logf(TEXT("MatVSize=%i"), Entity.pendingModel->TexSwitch.V);
		Out.Log(NAME_Remove, TEXT("END OBJECT"));

		Out.Logf(TEXT("ButtonMaterial=MaterialSequence'%ls'"), *Entity.pendingModel->TexSwitch.MatSeq);
	}

	INT i = Entity.IntValue(TEXT("sounds"), 0);
	if(i)
		Out.Logf(TEXT("PressedSound=%ls"), GetButtonSound(i));
	i = Entity.IntValue(TEXT("locked_sound"), 0);
	if (i)
		Out.Logf(TEXT("LockedSound=%ls"), GetButtonSound(i));
	i = Entity.IntValue(TEXT("locked_sentence"), 0);
	if (i)
	{
		static const TCHAR* VoxNames[] = { TEXT("NA"), TEXT("ND"), TEXT("NF"), TEXT("NFIRE"), TEXT("NCHEM"), TEXT("NRAD"), TEXT("NCON"), TEXT("NH"), TEXT("NG") };
		if (i <= ARRAY_COUNT(VoxNames))
			FillVoxSentences(Out, TEXT("LockedLines"), VoxNames[i - 1]);
	}
	FLOAT f = Entity.FloatValue(TEXT("wait"));
	if (f != FLOAT_NF)
	{
		if (f == -1.f)
			Out.Log(TEXT("bOnceOnly=True"));
		else Out.Logf(TEXT("RetriggerTime=%f"), f);
	}

	if (!Entity.HasSpawnFlag(32))
		Out.Log(TEXT("bToggleButton=True"));
	if (!Entity.HasSpawnFlag(64))
		Out.Log(TEXT("bButtonSparks=True"));
	if (!Entity.HasSpawnFlag(256))
		Out.Log(TEXT("bTouchActivate=True"));
}

struct FEntityExport
{
	f_ExpEntFunc* Func;
	const TCHAR* UnrealClass;
	FLOAT EntityHeight;
	const DWORD ExpFlags;

	FEntityExport(f_ExpEntFunc* InFunc, const TCHAR* InClass, const FLOAT inHeight = 0.f, const DWORD inExpFlags = EXP_None)
		: Func(InFunc), UnrealClass(InClass), EntityHeight(inHeight), ExpFlags(inExpFlags)
	{}
	FEntityExport operator=(const FEntityExport& Other)
	{
		appMemcpy(this, &Other, sizeof(FEntityExport));
		return *this;
	}
};

struct FAnimatorEntry
{
	const TCHAR* Class;
	const FLOAT ZHeight, ScaleMod;
	const UBOOL bAttachAnimator;

	FAnimatorEntry(const TCHAR* inClass, FLOAT inZHeight = 0.f, UBOOL bAttach = FALSE, FLOAT ScMod=-1.f)
		: Class(inClass), ZHeight(inZHeight), bAttachAnimator(bAttach), ScaleMod(ScMod)
	{}
	FAnimatorEntry operator=(const FAnimatorEntry& Other)
	{
		appMemcpy(this, &Other, sizeof(FAnimatorEntry));
		return *this;
	}
};

void CMapLoadHelper::ConvertAnimator(FOutputDevice& Out, FEntListData::FEntity& Entity)
{
	guardSlow(CMapLoadHelper::ConvertAnimator);
	static TMap<FString, FAnimatorEntry>* AnimMap = nullptr;
	if (!AnimMap)
	{
		AnimMap = new TMap<FString, FAnimatorEntry>;
		AnimMap->Set(TEXT("forklift"), FAnimatorEntry(TEXT("D_Forklift"), 38.f, TRUE));
		AnimMap->Set(TEXT("apache"), FAnimatorEntry(TEXT("D_Apache"), 0.f));
		AnimMap->Set(TEXT("Loader"), FAnimatorEntry(TEXT("D_MechLoader"), 0.f, TRUE));
		AnimMap->Set(TEXT("scientist"), FAnimatorEntry(TEXT("M_Scientist"), 38.f, TRUE));
		AnimMap->Set(TEXT("filecabinet"), FAnimatorEntry(TEXT("D_FileCabin"), 34.f));
		AnimMap->Set(TEXT("hair"), FAnimatorEntry(TEXT("D_Hair"), 4.f));
		AnimMap->Set(TEXT("fungus(large)"), FAnimatorEntry(TEXT("D_Fungus"), 140.f, FALSE, 1.f));
		AnimMap->Set(TEXT("fungus(small)"), FAnimatorEntry(TEXT("D_Fungus"), 140.f, FALSE, 0.25f));
		AnimMap->Set(TEXT("fungus"), FAnimatorEntry(TEXT("D_Fungus"), 140.f, FALSE, 0.5f));
		AnimMap->Set(TEXT("protozoa"), FAnimatorEntry(TEXT("D_Protozoa"), 34.f));
	}

	auto* Mdl = Entity.Value(TEXT("Model"));
	if (!Mdl)
	{
		GWarn->Logf(TEXT("Found animator '%ls' without model!"), *Entity.GetClass());
		return;
	}

	const FString ModelName = FString::GetFilenameOnlyStr(Mdl);
	auto* AnimClass = AnimMap->Find(*ModelName);
	if (!AnimClass)
	{
		GWarn->Logf(TEXT("Found animator '%ls' with unknown model: %ls"), *Entity.GetClass(), *ModelName);
		return;
	}

	FLOAT ScaleMod = AnimClass->ScaleMod;
	FLOAT zOffset = AnimClass->ZHeight;
	const TCHAR* UseClass = AnimClass->Class;
	FEntListData::FEntity* Animator = nullptr;
	if (AnimClass->bAttachAnimator)
	{
		auto* Tag = Entity.Value(TEXT("targetname"));
		if (Tag)
		{
			Animator = EntData.FindEntity(TEXT("scripted_sequence"), Tag, TEXT("m_iszEntity"));

			// Loader hack.
			if (Animator && ModelName == TEXT("Loader"))
			{
				// Loader bot may actually need AI movement.
				INT move = Animator->IntValue(TEXT("m_fMoveTo"));
				if (move)
				{
					UseClass = TEXT("M_Generic");
					Animator = nullptr;
				}
			}
			if (Animator)
			{
				Animator->ExportFlags = EXP_Ignore;
				auto* Res = Animator->Value(TEXT("Origin"));
				if (Res)
					Entity.SetValue(TEXT("Origin"), Res);
				Res = Animator->Value(TEXT("Angle"));
				if (Res)
					Entity.SetValue(TEXT("Angle"), Res);
				Res = Animator->Value(TEXT("targetname"));
				if (Res)
					Entity.SetValue(TEXT("targetname"), Res);
			}
		}
	}
	if (ScaleMod > 0.f)
	{
		ScaleMod *= (0.9f + appFrand() * 0.2f);
		zOffset *= ScaleMod;
	}
	
	ExportEntity(UseClass, Out);
	ExportEntityActor(Entity, Out, zOffset);
	if (Animator)
	{
		auto* Res = Animator->Value(TEXT("m_iszPlay"));
		if (Res)
			Out.Logf(TEXT("AnimSequence=\"%ls\""), Res);
		Res = Animator->Value(TEXT("m_iszIdle"));
		if (Res)
			Out.Logf(TEXT("AnimFinishAnim=\"%ls\""), Res);
	}
	if (ScaleMod > 0.f)
		Out.Logf(TEXT("DrawScale=%f"), ScaleMod);
	Out.Log(NAME_Remove, TEXT("END Actor"));
	unguardSlow;
}

static TMap<FName, FEntityExport>* ExpMapRef = NULL;

static const TCHAR* GetUCClassName(const TCHAR* testClass)
{
	if (ExpMapRef)
	{
		FName testName(testClass, FNAME_Find);
		if (testName == NAME_None)
			return NULL;

		FEntityExport* Exp = ExpMapRef->Find(FName(testClass));
		if (Exp && Exp->UnrealClass)
			return Exp->UnrealClass;
	}
	return NULL;
}

void CMapLoadHelper::ExportMap(FOutputDevice& Out)
{
	guard(CMapLoadHelper::ExportMap);
	Out.Log(TEXT("BEGIN Map"));
	{
		Out.Log(NAME_Add, TEXT("BEGIN Actor Class=LevelInfo Name=LevelInfo0"));
		FEntListData::FEntity* Ent = EntData.FindEntity(TEXT("worldspawn"));
		if (Ent)
		{
			auto* Title = Ent->Value(TEXT("message"));
			auto* Chapter = Ent->Value(TEXT("chaptertitle"));
			if (Title)
			{
				Out.Logf(TEXT("Title=\"%ls - %ls\""), *MapFile, Title);
				INT DisplayMsg = Ent->IntValue(TEXT("gametitle"), 0);
				if (DisplayMsg == 1)
					Out.Logf(TEXT("LevelEnterText=\"%ls\""), Title);
			}
			else
			{
				if (Chapter)
					Out.Logf(TEXT("LevelEnterText=\"%ls\""), Chapter);
				Out.Logf(TEXT("Title=\"%ls\""), *MapFile);
			}
			
			INT Song = Ent->IntValue(TEXT("Sounds"), 0);
			if (Song != 0)
				Out.Logf(TEXT("Song=%ls"), GetMusicName(Song));
		}

		// Find ambient light
		Ent = EntData.FindEntity(TEXT("light_environment"));
		if (Ent)
		{
			auto* Amb = Ent->Value(TEXT("_ambient"));
			if (Amb)
			{
				FColor C = GetColorOf(Amb);
				FVector HSL = C.GetHSL();
				Out.Logf(TEXT("AmbientHue=%i"), appRound(HSL.X * 255.f));
				Out.Logf(TEXT("AmbientSaturation=%i"), appRound(HSL.Y * 255.f));
				Out.Logf(TEXT("AmbientBrightness=%i"), INT(C.A));
			}
		}

		Out.Log(TEXT("Author=\"Valve Software\""));
		Out.Log(TEXT("bEnhancedIcePhysics=True"));
		Out.Log(TEXT("bSupportsCrouchJump=True"));
		Out.Log(TEXT("bSupportsRealCrouching=True"));
		Out.Log(NAME_Remove, TEXT("END Actor"));
	}

	ExportEntity(TEXT("Brush"), Out);
	Out.Log(NAME_Add, TEXT("BEGIN Brush Name=Brush"));
	WriteCubeBrush(FVector(0, 0, 0), Out);
	Out.Log(NAME_Remove, TEXT("END Brush"));
	Out.Log(TEXT("Brush=Model'Brush'"));
	Out.Log(NAME_Remove, TEXT("END Actor"));

	if (map_models.Num())
	{
		CModelBuilder Builder(this);
		Builder.AddSurfs(map_models(0));

		for (INT i = 0; i < Builder.Models.Num(); ++i)
		{
			const UBOOL bInvert = Builder.Models(i)->CheckInverted();
			ExportEntity(TEXT("Brush"), Out);
			if (bInvert)
			{
				Out.Log(TEXT("CsgOper=CSG_Subtract"));
				Builder.Models(i)->InverseModel();
			}
			else
			{
				Out.Logf(TEXT("PolyFlags=%i"), PF_Semisolid);
				Out.Log(TEXT("CsgOper=CSG_Add"));
			}
			Out.Logf(NAME_Add, TEXT("BEGIN Brush Name=Brush%i"), iModelIndex);
			Out.Log(NAME_Add, TEXT("BEGIN PolyList"));
			if (bMergeSurfs)
				Builder.Models(i)->MergePolys(TRUE);
			else Builder.Models(i)->MergePolys(TRUE, TRUE);
			Builder.Models(i)->DumpOutput(Out, TRUE);
			Out.Log(NAME_Remove, TEXT("END PolyList"));
			Out.Log(NAME_Remove, TEXT("END Brush"));
			Out.Logf(TEXT("Brush=Model'Brush%i'"), iModelIndex);
			const FVector Center = Builder.Models(i)->GetCentroid();
			Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), Center.X, Center.Y, Center.Z);
			++iModelIndex;
			Out.Log(NAME_Remove, TEXT("END Actor"));
		}
	}
	
	{
#define DECL_ABS_CLASS(vent) ExpMap.Set(FName(TEXT(vent)), FEntityExport(nullptr, nullptr, 0.f, EXP_Ignore))
#define DECL_L_CLASS(vent, uscls, func, offs, flgs) ExpMap.Set(FName(TEXT(vent)), FEntityExport(&func, TEXT(uscls), offs, flgs))
#define DECL_S_CLASS(vent, uscls, func) ExpMap.Set(FName(TEXT(vent)), FEntityExport(&func, TEXT(uscls)))

		TMap<FName, FEntityExport> ExpMap;
		ExpMapRef = &ExpMap;
		DECL_ABS_CLASS("worldspawn");
		DECL_ABS_CLASS("info_landmark");
		DECL_ABS_CLASS("env_sound");

		// Lightenv
		DECL_L_CLASS("light_spot", "Light", ExportLights, 0.f, EXP_TriggerLight);
		DECL_L_CLASS("light", "Light", ExportLights, 0.f, EXP_TriggerLight);
		DECL_S_CLASS("light_environment", "Sunlight", ExportLights);

		// NPC
		DECL_L_CLASS("monster_scientist", "M_Scientist", ExportScientist, 39.f, EXP_None);
		DECL_L_CLASS("monster_barney", "M_Barney", ExportBarney, 39.f, EXP_PreCondition);
		DECL_L_CLASS("monster_gman", "M_GMan", ExportNothing, 39.f, EXP_None);
		DECL_L_CLASS("monster_alien_slave", "M_ASlave", ExportNothing, 39.f, EXP_None);
		DECL_L_CLASS("monster_bullchicken", "M_Bullsquid", ExportNothing, 39.f, EXP_None);
		DECL_L_CLASS("monster_headcrab", "M_Headcrab", ExportNothing, 39.f, EXP_None);

		// Deco
		DECL_L_CLASS("monster_generic", "M_Generic", ExportGeneric, 0.f, EXP_Ignore | EXP_PreCondition | EXP_Animator);
		DECL_L_CLASS("monster_furniture", "HL_AnimEntity", ExportNothing, 0.f, EXP_Ignore | EXP_PreCondition | EXP_Animator);
		DECL_L_CLASS("monster_sitting_scientist", "M_Scientist_Sitting", ExportScientist, 10.f, EXP_None);
		DECL_L_CLASS("cycler", "HL_AnimEntity", ExportNothing, 10.f, EXP_Ignore | EXP_PreCondition | EXP_Animator);
		DECL_L_CLASS("env_glow", "ScaledSprite", ExportEnvGlov, 0.f, EXP_PreCondition);
		DECL_S_CLASS("env_sprite", "HL_AnimSprite", ExportEnvGlov);
		DECL_S_CLASS("env_spark", "HL_SparksEffect", ExportEnvSparks);
		DECL_S_CLASS("env_shooter", "HL_Emitter", ExportEmitter);
		DECL_S_CLASS("infodecal", "HL_Decal", ExportDecal);
		DECL_S_CLASS("env_beverage", "HL_VendingDispenser", ExportDispenser);

		// Nodes
		DECL_L_CLASS("info_node", "PathNode", ExportNothing, 16.f, EXP_None);
		DECL_L_CLASS("info_player_start", "PlayerStart", ExportNothing, 1.f, EXP_None);
		DECL_L_CLASS("world_items", "PathNode", ExportNothing, 0.f, EXP_Inventory);

		// Triggers
		DECL_S_CLASS("trigger_auto", "HL_TriggerAuto", ExportAutoTrigger);
		DECL_L_CLASS("scripted_sequence", "HL_ScriptedSequence", ExportScriptedSequence, 39.f, EXP_None);
		DECL_S_CLASS("env_message", "HL_TriggerMessage", ExportEnvMessage);
		DECL_S_CLASS("multi_manager", "HL_MultiManager", ExportMultiManager);
		DECL_S_CLASS("trigger_relay", "HL_RelayTrigger", ExportTriggerRelay);
		DECL_S_CLASS("trigger_changelevel", "Teleporter", ExportTeleporter);
		DECL_L_CLASS("ambient_generic", "HL_SoundTrigger", ExportSoundTrigger, 0.f, EXP_Sound);
		DECL_S_CLASS("env_fade", "HL_EnvFade", ExportEnvFade);
		DECL_S_CLASS("info_target", "HL_TargetPoint", ExportNothing);
		DECL_S_CLASS("env_beam", "HL_BeamEffect", ExportBeam);
		DECL_S_CLASS("trigger_push", "HL_TriggerPush", ExportPusher);
		DECL_S_CLASS("trigger_cdaudio", "MusicEvent", ExportMusic);
		DECL_S_CLASS("trigger_hurt", "HL_TriggerHurt", ExportTriggerHurt);
		DECL_S_CLASS("trigger_autosave", "AutosaveTrigger", ExportAutoSaver);
		DECL_S_CLASS("trigger_once", "HL_TriggerMultiple", ExportTriggerOnce);
		DECL_S_CLASS("trigger_multiple", "HL_TriggerMultiple", ExportTriggerOnce);
		DECL_S_CLASS("scripted_sentence", "HL_ScriptedSentence", ExportScriptedSentence);
		DECL_S_CLASS("env_render", "HL_RenderModeTrigger", ExportEnvRender);
		DECL_S_CLASS("multisource", "HL_MultiSource", ExportNothing);
		DECL_S_CLASS("speaker", "HL_Speaker", ExportSpeaker);
		DECL_S_CLASS("env_explosion", "HL_Explosion", ExportExplosion);
		DECL_S_CLASS("env_shake", "Earthquake", ExportEarthquake);
		DECL_S_CLASS("trigger_teleport", "HL_TriggerTeleport", ExportTTeleport);
		DECL_S_CLASS("env_funnel", "HL_FunnelEffect", ExportFunnel);
		DECL_S_CLASS("monstermaker", "HL_MonsterMaker", ExportMonsterMaker);

		// BSP/Movers
		DECL_L_CLASS("func_wall", "Brush", ExportBrush, 0.f, EXP_CheckTriggerTex | EXP_SkipEvents);
		DECL_L_CLASS("func_illusionary", "Brush", ExportBrush, 0.f, EXP_CheckTriggerTex | EXP_SkipEvents);
		DECL_L_CLASS("func_breakable", "HL_Pushable", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_pushable", "HL_Pushable", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_door", "HL_MoveLinear", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_door_rotating", "HL_MoveLinear", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_pendulum", "HL_MoveLinear", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_rotating", "HL_MoveRotating", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_platrot", "HL_MoveRotating", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_healthcharger", "HL_HealthCharger", ExportHealthCharger, 0.f, EXP_BrushSeperate);
		DECL_L_CLASS("func_button", "HL_TriggerButton", ExportButton, 0.f, EXP_PostCondition | EXP_BrushSeperate);
		DECL_S_CLASS("func_ladder", "HL_Ladder", ExportLadder);

		// Train/tracks
		DECL_L_CLASS("func_tracktrain", "HL_TrainMover", ExportMover, 0.f, EXP_Mover);
		DECL_L_CLASS("func_train", "HL_TrainMover", ExportMover, 0.f, EXP_Mover);
		DECL_S_CLASS("path_track", "HL_TrainTrack", ExportPathTrack);
		DECL_S_CLASS("path_corner", "HL_TrainTrack", ExportPathTrack);
		DECL_S_CLASS("func_trackautochange", "HL_TrainTrackAutoChange", ExportPathTrack);
		DECL_S_CLASS("func_trackchange", "HL_TrainTrackChange", ExportPathTrack);

#undef DECL_ABS_CLASS
#undef DECL_L_CLASS
#undef DECL_S_CLASS

		SwitchMatIndex = 0;

		for (INT i = 0; i < EntData.EntList.Num(); ++i)
		{
			auto& Ent = *EntData.EntList(i);
			auto* f = ExpMap.Find(Ent.GetClass());
			if (f)
			{
				Ent.EntData = f;
				Ent.ExportFlags = f->ExpFlags;
			}
			else
			{
				Ent.ExportFlags = EXP_Ignore;
				warnf(TEXT("Unknown entity: %ls"), *Ent.GetClass());
			}
		}

		// Special handle for animators first because they need to be linked with scripted_sequence
		for (INT i = 0; i < EntData.EntList.Num(); ++i)
		{
			auto& Ent = *EntData.EntList(i);
			if (!Ent.HasExportFlag(EXP_PreCondition))
				continue;
			if (Ent.HasExportFlag(EXP_Animator))
				ConvertAnimator(Out, Ent);
			else if (Ent.IsClass(TEXT("monster_barney")))
			{
				const TCHAR* barneyTag = Ent.Value(TEXT("targetname"));
				if (!barneyTag)
					continue;

				// See if this is a sitting barney, then needs special trigger event.
				for (INT j = 0; j < EntData.EntList.Num(); ++j)
				{
					const auto& EB = EntData.EntList(j);
					if (!EB->IsClass(TEXT("scripted_sequence")))
						continue;
					const TCHAR* targetEnt = EB->Value(TEXT("m_iszEntity"));
					if (!targetEnt || appStricmp(targetEnt, barneyTag))
						continue;
					const TCHAR* animName = EB->Value(TEXT("m_iszIdle"));
					if (!animName || appStricmp(animName, TEXT("sit1")))
						continue;
					Ent.SetValue(TEXT("AnimEvent"), TEXT("introchair"));
				}
			}
			else if (Ent.IsClass(TEXT("env_glow")))
			{
				const TCHAR* sprt = Ent.Value(TEXT("model"));
				if (sprt)
				{
					FString SpriteName = FString::GetFilenameOnlyStr(sprt);
					if (SpriteName == TEXT("glow05") && !Ent.Value(TEXT("targetname")))
					{
						// Remove invisible sprites.
						Ent.ExportFlags = EXP_Ignore;
					}
				}
			}
		}

		for (INT i = 0; i < EntData.EntList.Num(); ++i)
		{
			auto& Ent = *EntData.EntList(i);
			if (Ent.HasExportFlag(EXP_Ignore))
				continue;

			FEntityExport* f = reinterpret_cast<FEntityExport*>(Ent.EntData);
			const TCHAR* USClass = f->UnrealClass;
			if (Ent.HasExportFlag(EXP_TriggerLight))
			{
				Ent.ExportFlags = EXP_None;
				if (Ent.HasSpawnFlag(1))
				{
					Ent.ExportFlags |= EXP_TriggerLight;
					USClass = TEXT("TriggerLight");
				}
				else
				{
					auto* SF = Ent.Value(TEXT("targetname"));
					if (SF)
					{
						Ent.ExportFlags |= EXP_TriggerLight;
						USClass = TEXT("TriggerLight");
					}
				}
			}
			else if (Ent.HasExportFlag(EXP_Sound))
			{
				auto* TagName = Ent.Value(TEXT("targetname"));
				if (!TagName && !Ent.HasSpawnFlag(16) && !Ent.HasSpawnFlag(32))
				{
					USClass = TEXT("AmbientSound");
					Ent.SpecialFlags |= 1;
				}
			}
			else if (Ent.HasExportFlag(EXP_Inventory))
			{
				INT i = Ent.IntValue(TEXT("Type"), 0);
				USClass = GetInventoryClass(i, &f->EntityHeight);
			}
			else if (Ent.HasExportFlag(EXP_PostCondition))
			{
				if (Ent.IsClass(TEXT("func_button")))
				{
					if (!Ent.HasSpawnFlag(1))
					{
						Ent.Rotation = FRotator(0, 0, 0);
						Ent.ExportFlags = EXP_Mover;
						USClass = nullptr;

						ExportEntity(TEXT("HL_MoveButton"), Out);
						ExportMover(*this, Ent, Out);
						ExportEntityActor(Ent, Out, 0.f);
						Out.Log(NAME_Remove, TEXT("END Actor"));
					}
				}
			}
			if (!USClass)
				continue;

			ExportEntity(USClass, Out);
			(*f->Func)(*this, Ent, Out);
			ExportEntityActor(Ent, Out, f->EntityHeight);
			Out.Log(NAME_Remove, TEXT("END Actor"));

			if (Ent.HasExportFlag(EXP_CheckTriggerTex) && Ent.SpecialFlags && Ent.pendingModel)
			{
				Ent.Location = Ent.pendingModel->GetCentroid() + FVector(0, 0, 16.f);
				Ent.Rotation = FRotator(0, 0, 0);
				Ent.ExportFlags = 0;
				ExportEntity(TEXT("MaterialTrigger"), Out);

				Ent.pendingModel->TexSwitch.Export(Out);
				Out.Logf(TEXT("Sequence=MaterialSequence'%ls'"), *Ent.pendingModel->TexSwitch.MatSeq);
				Out.Logf(TEXT("Materials(0)=(Material=Texture'%ls',FadeInTime=0.075)"), *Ent.pendingModel->TexSwitch.TexA);
				Out.Logf(TEXT("Materials(1)=(Material=Texture'%ls',FadeInTime=0.075)"), *Ent.pendingModel->TexSwitch.TexB);
				Out.Log(TEXT("bLoopSequence=True"));

				ExportEntityActor(Ent, Out, 0.f);
				Out.Log(NAME_Remove, TEXT("END Actor"));
			}
			else if (Ent.HasExportFlag(EXP_BrushSeperate) && Ent.pendingModel && !Ent.pendingModel->ModelContainsTriggerTextures())
			{
				Ent.Location = FVector(0, 0, 0);
				Ent.Rotation = FRotator(0, 0, 0);
				Ent.ExportFlags = EXP_SkipEvents;
				ExportEntity(TEXT("Brush"), Out);
				ExportBrush(*this, Ent, Out);
				ExportEntityActor(Ent, Out, 0.f);
				Out.Log(NAME_Remove, TEXT("END Actor"));
			}
			delete Ent.pendingModel;
		}

		DumpBspLights(Out);
		ExpMapRef = NULL;
	}

	Out.Log(TEXT("END Map"));
	unguard;
}
