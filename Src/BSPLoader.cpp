
#include "HLExporter.h"

constexpr INT ExpIDBSPHead = IDBSPHEADER;

enum EExpEntFlags : DWORD
{
	EXP_None = 0,
	EXP_Mover = 0x01,
	EXP_NoRotation = 0x02,
	EXP_TriggerLight = 0x04,
};

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
	}

	EntData.DumpEntities();
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
	unguard;
}
void FEntListData::DumpEntities() const
{
	guard(FEntListData::DumpEntities);
	debugf(TEXT("===== LIST OF ENTITIES ====="));
	FName EntClass(TEXT("classname"));
	const FString* S;
	for (INT i = 0; i < EntList.Num(); ++i)
	{
		const FEntity& Entity = *EntList(i);
		S = Entity.KeyMap.Find(EntClass);
		if (S)
			debugf(TEXT(" Class: '%ls'"), **S);
		else debugf(TEXT(" Class: <UNKNOWN CLASS>"));

		for (TMap<FName, FString>::TConstIterator It(Entity.KeyMap); It; ++It)
		{
			if (It.Key() != EntClass)
				debugf(TEXT("   '%ls'='%ls'"), *It.Key(), *It.Value());
		}
	}
	debugf(TEXT("============================"));
	unguard;
}
FEntListData::FEntity* FEntListData::FindEntity(const TCHAR* EntName) const
{
	guard(FEntListData::FindEntity);
	FName NAME_ClassName(TEXT("classname"));
	for (INT i = 0; i < EntList.Num(); ++i)
	{
		FString* Res = EntList(i)->KeyMap.Find(NAME_ClassName);
		if (Res && (*Res) == EntName)
			return EntList(i);
	}
	return nullptr;
	unguard;
}

void CMapLoadHelper::LoadEntities()
{
	guard(CMapLoadHelper::LoadEntities);
	const BSPLUMP& Lump = bspHeader.lump[LUMP_ENTITIES];
	Loader.Seek(Lump.nOffset);
	TCHAR* WStr;

	{
		FScopedMemMark Mark(GMem);
		ANSICHAR* Str = new(GMem) ANSICHAR[Lump.nLength + 1];
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
inline INT ToUnrealAngles(const INT InAng)
{
	return appRound(DOUBLE(InAng) * (65536.0 / 360.0));
}

static void ExportEntityActor(FEntListData::FEntity& Ent, FOutputDevice& Out, const FLOAT EntHeight = 0.f, const DWORD ExpFlags = EXP_None)
{
	FString* Res;
	if (!(ExpFlags & EXP_NoRotation))
	{
		FRotator Rot(0, 0, 0);
		Res = Ent.KeyMap.Find(FName(TEXT("angles")));
		if (Res)
		{
			const TCHAR* Str = **Res;
			Rot.Pitch = ToUnrealAngles(HLTitleManager::ParseNextInt(Str, 0));
			Rot.Yaw = ToUnrealAngles(HLTitleManager::ParseNextInt(Str, 0));
			Rot.Roll = ToUnrealAngles(HLTitleManager::ParseNextInt(Str, 0));
			Res = Ent.KeyMap.Find(TEXT("pitch"));
			if (Res)
				Rot.Pitch = ToUnrealAngles(Res->Int());
		}
		else
		{
			Res = Ent.KeyMap.Find(TEXT("angle"));
			if (Res)
				Rot.Yaw = -ToUnrealAngles(Res->Int());
			Res = Ent.KeyMap.Find(TEXT("pitch"));
			if (Res)
				Rot.Pitch = ToUnrealAngles(Res->Int());
		}
		if (!Rot.IsZero())
		{
			Out.Logf(TEXT("Rotation=(Yaw=%i,Pitch=%i,Roll=%i)"), Rot.Yaw, Rot.Pitch, Rot.Roll);
			if (ExpFlags & EXP_Mover)
				Out.Logf(TEXT("BaseRot=(Yaw=%i,Pitch=%i,Roll=%i)"), Rot.Yaw, Rot.Pitch, Rot.Roll);
		}
	}

	Res = Ent.KeyMap.Find(FName(TEXT("origin")));
	if (Res)
	{
		FVector Pos = ToUnrealScale(GetVectorOf(**Res));
		Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), Pos.X, Pos.Y, Pos.Z + EntHeight);
		if (ExpFlags & EXP_Mover)
			Out.Logf(TEXT("BasePos=(X=%f,Y=%f,Z=%f)"), Pos.X, Pos.Y, Pos.Z + EntHeight);
	}
	Res = Ent.KeyMap.Find(FName(TEXT("targetname")));
	if (Res)
		Out.Logf(TEXT("Tag=\"%ls\""), **Res);
	Res = Ent.KeyMap.Find(FName(TEXT("target")));
	if (Res)
		Out.Logf(TEXT("Event=\"%ls\""), **Res);

	if (ExpFlags & EXP_TriggerLight)
	{
		Res = Ent.KeyMap.Find(TEXT("spawnflags"));
		if (Res && (Res->Int() & 1))
			Out.Log(TEXT("bInitiallyOn=False"));
		else Out.Log(TEXT("bInitiallyOn=True"));
		Out.Log(TEXT("InitialState=\"TriggerToggle\""));
	}
}

inline UBOOL CanMergeFaces(const FPoly& A, const FPoly& B)
{
	return (A.ItemName == B.ItemName) && (A.PolyFlags == B.PolyFlags) && (A.TextureU.DistSquared(B.TextureU) < 0.1f) && (A.TextureV.DistSquared(B.TextureV) < 0.1f);
}

struct CModel
{
private:
	CMapLoadHelper& Data;
	TArray<FPoly*> Polys;
	UBOOL bWasInverted{};
	FBox Bounds;
	FVector Centroid;

public:
	CModel(CMapLoadHelper& D)
		: Data(D)
	{}
	~CModel()
	{
		for (INT i = 0; i < Polys.Num(); ++i)
			delete Polys(i);
	}

	inline FVector GetCentroid() const
	{
		return Centroid;
	}
	void MergePolys(UBOOL bKeepTex = TRUE, UBOOL bOnlyUnlit = FALSE)
	{
		guard(CModel::MergePolys);
		INT i, j;
		UBOOL bDidMerge = TRUE;
		while (bDidMerge)
		{
			bDidMerge = FALSE;
			for (i = 0; i < Polys.Num(); ++i)
			{
				FPoly* A = Polys(i);
				for (j = (i + 1); j < Polys.Num(); ++j)
				{
					FPoly* B = Polys(j);
					if (bKeepTex && !CanMergeFaces(*A, *B))
						continue;
					if (bOnlyUnlit && !(A->PolyFlags & PF_Unlit))
						continue;
					if (!A->MergePolys(*B))
						continue;
					delete B;
					Polys.Remove(j);
					bDidMerge = TRUE;
					break;
				}
			}
		}
		unguard;
	}
	inline FPoly* AddPoly()
	{
		FPoly* Result = new FPoly;
		Polys.AddItem(Result);
		return Result;
	}
	inline void RemoveLastPoly()
	{
		delete Polys.Pop();
	}
	UBOOL DumpOutput(FOutputDevice& Out, UBOOL bExportTextures, UBOOL bAllowCenter = TRUE) const
	{
		guard(CModel::DumpOutput);
		UBOOL bHadLightmaps = FALSE;
		INT j;
		for (INT i = 0; i < Polys.Num(); ++i)
		{
			const FPoly& P = *Polys(i);
			if (bExportTextures)
			{
				if (P.PolyFlags != PF_None)
				{
					Out.Logf(NAME_Add, TEXT("BEGIN Polygon Texture=%ls Flags=%u"), *P.ItemName, P.PolyFlags);
					bHadLightmaps = (bHadLightmaps || ((P.PolyFlags & PF_Unlit) != 0));
				}
				else Out.Logf(NAME_Add, TEXT("BEGIN Polygon Texture=%ls"), *P.ItemName);
			}
			else Out.Log(NAME_Add, TEXT("BEGIN Polygon"));
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
		return bHadLightmaps;
		unguard;
	}
	void DumpLights(FOutputDevice& Out) const
	{
		guard(CModel::DumpLights);
		UBOOL bHadLightmaps = FALSE;
		INT j;
		for (INT i = 0; i < Polys.Num(); ++i)
		{
			const FPoly& P = *Polys(i);
			if (P.PolyFlags & PF_Unlit)
			{
				const FColor* C = Data.TexLightMap.Find(P.ItemName);
				if (!C)
					continue;
				const FVector SurfNormal = bWasInverted ? (-P.Normal) : P.Normal;
				FVector CenterPoint(SurfNormal * 5.f);
				const FLOAT Div = 1.f / FLOAT(P.NumVertices);
				for (j = 0; j < P.NumVertices; ++j)
					CenterPoint += (P.Vertex[j] * Div);
				const FRotator LightRot = SurfNormal.Rotation();

				Data.ExportEntity(TEXT("Light"), Out);
				Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), CenterPoint.X, CenterPoint.Y, CenterPoint.Z);
				Out.Logf(TEXT("Rotation=(Yaw=%i,Pitch=%i,Roll=%i)"), LightRot.Yaw, LightRot.Pitch, LightRot.Roll);
				Out.Log(TEXT("LightEffect=LE_Spotlight"));
				Out.Log(TEXT("bDirectional=True"));
				const FVector HSL = C->GetHSL();
				Out.Logf(TEXT("LightHue=%i"), appRound(HSL.X * 255.f));
				Out.Logf(TEXT("LightSaturation=%i"), appRound(HSL.Y * 255.f));
				Out.Logf(TEXT("LightBrightness=%i"), appRound(FLOAT(C->A) * (3.f / 4.f)));
				Out.Log(TEXT("LightRadius=42"));
				Out.Log(TEXT("LightCone=132"));
				Out.Log(NAME_Remove, TEXT("END Actor"));

				CenterPoint += SurfNormal * 4.f;
				Data.ExportEntity(TEXT("Light"), Out);
				Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), CenterPoint.X, CenterPoint.Y, CenterPoint.Z);
				Out.Logf(TEXT("LightHue=%i"), appRound(HSL.X * 255.f));
				Out.Logf(TEXT("LightSaturation=%i"), appRound(HSL.Y * 255.f));
				Out.Logf(TEXT("LightBrightness=%i"), INT(C->A / 4));
				Out.Log(TEXT("LightRadius=48"));
				Out.Log(NAME_Remove, TEXT("END Actor"));
			}
		}
		unguard;
	}
	inline void InverseModel()
	{
		guardSlow(CModel::InverseModel);
		for (INT i = 0; i < Polys.Num(); ++i)
			Polys(i)->Reverse();
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
				for (i = 0; i < Polys.Num(); ++i)
				{
					const FPoly& P = *Polys(i);
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
				TotalNormal += (Polys(BestSurf)->Normal | Axis);
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
		for (INT i = 0; i < Polys.Num(); ++i)
		{
			const FPoly& P = *Polys(i);
			for (j = 0; j < P.NumVertices; ++j)
				Bounds += P.Vertex[j];
		}
		Centroid = Bounds.GetCentroid().GridSnap(FVector(8.f, 8.f, 8.f));
		unguard;
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
					if (P->ItemName == NAME_Black) // Black pit, no lighting needed.
						P->PolyFlags = PF_Unlit;
					if (P->ItemName == SkyName) // Black pit, no lighting needed.
						P->PolyFlags = PF_FakeBackdrop;

					// Bleh complex way of figuring out exact HL texture coordinates...
					FVector2D UVs[3];
					for (j = 0; j < 3; ++j)
					{
						UVs[j].X = ((ToUnrealUVMap(P->Vertex[j]) | tex.vS) + tex.fSShift) * mTex.ScaleX;
						UVs[j].Y = ((ToUnrealUVMap(P->Vertex[j]) | tex.vT) + tex.fTShift) * mTex.ScaleY;
					}
					ProjectUVToCoords(P->Vertex[0], UVs[0], P->Vertex[1], UVs[1], P->Vertex[2], UVs[2], P->Base, P->TextureU, P->TextureV);
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

void CMapLoadHelper::WriteModel(const BSPMODEL& Model, FOutputDevice& Out, const UBOOL bWorld, const UBOOL bAddTextures, const UBOOL bCenterModel, const UBOOL bIsMover)
{
	guard(CMapLoadHelper::WriteModel);
	Out.Logf(NAME_Add, TEXT("BEGIN Brush Name=Brush%i"), iModelIndex);
	Out.Log(NAME_Add, TEXT("BEGIN PolyList"));

	CModelBuilder Builder(this);
	Builder.AddSurfs(Model, FALSE, bAddTextures);
	if (Builder.Models.Num())
	{
		Builder.Models(0)->MergePolys(bAddTextures);
		Builder.Models(0)->DumpOutput(Out, bAddTextures, bCenterModel);
	}
	Out.Log(NAME_Remove, TEXT("END PolyList"));
	Out.Log(NAME_Remove, TEXT("END Brush"));
	Out.Logf(TEXT("Brush=Model'Brush%i'"), iModelIndex);
	if (bCenterModel)
	{
		const FVector V = Builder.Models(0)->GetCentroid();
		Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), V.X, V.Y, V.Z);
		if (bIsMover)
			Out.Logf(TEXT("BasePos=(X=%f,Y=%f,Z=%f)"), V.X, V.Y, V.Z);
	}
	++iModelIndex;
	unguard;
}

static void ParseOptNameParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out)
{
	FString* S = Ent.KeyMap.Find(ValveName);
	if (S)
		Out.Logf(TEXT("%ls=\"%ls\""), ScriptName ? ScriptName : ValveName, **S);
}
static void ParseOptNumberParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out)
{
	FString* S = Ent.KeyMap.Find(ValveName);
	if (S)
		Out.Logf(TEXT("%ls=%ls"), ScriptName ? ScriptName : ValveName, **S);
}
static void ParseOptScaledNumParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out)
{
	FString* S = Ent.KeyMap.Find(ValveName);
	if (S)
		Out.Logf(TEXT("%ls=%f"), ScriptName ? ScriptName : ValveName, S->Float() * ToUnrScale);
}
static void ParseOptBoolParm(const TCHAR* ValveName, const TCHAR* ScriptName, FEntListData::FEntity& Ent, FOutputDevice& Out)
{
	FString* S = Ent.KeyMap.Find(ValveName);
	if (S)
		Out.Logf(TEXT("%ls=%ls"), ScriptName ? ScriptName : ValveName, (S->Int() != 0) ? GTrue : GFalse);
}

#define EXPENTPARMS CMapLoadHelper& Loader, const TCHAR* ClassName, FEntListData::FEntity& Entity, FOutputDevice& Out
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
static void ExportLights(EXPENTPARMS)
{
	if (!appStricmp(ClassName, TEXT("light_spot")))
	{
		Out.Log(TEXT("LightEffect=LE_Spotlight"));
		Out.Log(TEXT("bDirectional=True"));
	}
	FString* Res = Entity.KeyMap.Find(TEXT("_light"));
	if (Res)
	{
		FColor C = GetColorOf(**Res);
		FVector HSL = C.GetHSL();
		Out.Logf(TEXT("LightHue=%i"), appRound(HSL.X * 255.f));
		Out.Logf(TEXT("LightSaturation=%i"), appRound(HSL.Y * 255.f));
		Out.Logf(TEXT("LightBrightness=%i"), INT(C.A));
	}
	Res = Entity.KeyMap.Find(TEXT("_distance"));
	if (Res)
	{
		const FLOAT LightRadii = Res->Float();
		Out.Logf(TEXT("LightRadius=%i"), Min(appRound(LightRadii * 32.f), 255));
	}
	else Out.Logf(TEXT("LightRadius=%i"), DefaultLightRadius);
	Res = Entity.KeyMap.Find(TEXT("_cone2"));
	if (Res)
	{
		const FLOAT LightCone = Res->Float();
		Out.Logf(TEXT("LightCone=%i"), Clamp(appRound(LightCone * (255.f / 90.f)), 10, 255));
	}
}
static void ExportScientist(EXPENTPARMS)
{
	ParseOptNumberParm(TEXT("body"), TEXT("BodyType"), Entity, Out);
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
	FString* Res = Entity.KeyMap.Find(TEXT("spawnflags"));
	if (Res)
	{
		const INT Flags = Res->Int();
		if (Flags & 4)
			Out.Log(TEXT("sf_Repeatable=True"));
		if (Flags & 16)
			Out.Log(TEXT("sf_StartOnSpawn=True"));
		if (Flags & 32)
			Out.Log(TEXT("sf_NoInterruptions=True"));
		if (Flags & 64)
			Out.Log(TEXT("sf_OverrideAI=True"));
		if (Flags & 128)
			Out.Log(TEXT("sf_NoTeleportAtEnd=True"));
		if (Flags & 256)
			Out.Log(TEXT("sf_LoopInPostIdle=True"));
		if (Flags & 512)
			Out.Log(TEXT("sf_PriorityScript=True"));
		if (Flags & 1024)
			Out.Log(TEXT("sf_SearchCyclically=True"));
	}
	Res = Entity.KeyMap.Find(TEXT("m_fMoveTo"));
	if (Res)
	{
		const INT Flags = Res->Int();
		static const TCHAR* MoveTypes[] = { TEXT("MTT_None"), TEXT("MTT_Walk"), TEXT("MTT_Run") , TEXT("MTT_Custom") , TEXT("MTT_Instant") , TEXT("MTT_TurnTo") };
		if (Flags < 6)
			Out.Logf(TEXT("m_fMoveTo=%ls"), MoveTypes[Flags]);
	}
}
static void ExportEnvMessage(EXPENTPARMS)
{
	FString* Res = Entity.KeyMap.Find(TEXT("message"));
	if (Res)
	{
		HLTitleManager::FTitleInfo* T = Loader.Titles.FindTitle(**Res);
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
	Res = Entity.KeyMap.Find(TEXT("messageattenuation"));
	if (Res)
	{
		const INT Flags = Res->Int();
		static const TCHAR* SAttenTypes[] = { TEXT("MSA_Small"), TEXT("MSA_Medium"), TEXT("MSA_Large") , TEXT("MSA_Everywhere") };
		if (Flags < 4)
			Out.Logf(TEXT("MessageAttenuate=%ls"), SAttenTypes[Flags]);
	}
	Res = Entity.KeyMap.Find(TEXT("spawnflags"));
	if (Res)
	{
		const INT Flags = Res->Int();
		if (Flags & 1)
			Out.Log(TEXT("sf_PlayOnce=True"));
		if (Flags & 2)
			Out.Log(TEXT("sf_AllClients=True"));
	}
}
static void ExportMultiManager(EXPENTPARMS)
{
	INT en = 0;
	for (TMap<FName, FString>::TConstIterator It(Entity.KeyMap); It; ++It)
	{
		if (!appStricmp(*It.Key(), TEXT("origin")) || !appStricmp(*It.Key(), TEXT("targetname")) || !appStricmp(*It.Key(), TEXT("classname")))
			continue;
		Out.Logf(TEXT("Events(%i)=(Event=\"%ls\",Delay=%f)"), en++, *It.Key(), It.Value().Float());
	}
}
static void ExportEnvGlov(EXPENTPARMS)
{
	FString* Res = Entity.KeyMap.Find(TEXT("model"));
	if (Res)
		Out.Logf(TEXT("Texture=Texture'%ls'"), *Res->GetFilenameOnly());
	Res = Entity.KeyMap.Find(TEXT("rendercolor"));
	if (Res)
	{
		FColor RC = GetColorOf(**Res);
		Out.Logf(TEXT("ActorRenderColor=(R=%i,G=%i,B=%i,A=255)"), INT(RC.R), INT(RC.G), INT(RC.B));
	}
	Res = Entity.KeyMap.Find(TEXT("rendermode"));
	if (Res)
	{
		const INT Mode = Res->Int();
		if (Mode >= 1 && Mode <= 5)
			Out.Log(TEXT("Style=STY_Translucent"));
	}
	Res = Entity.KeyMap.Find(TEXT("renderamt"));
	if (Res)
	{
		const FLOAT fx = Res->Int();
		Out.Logf(TEXT("ScaleGlow=%f"), (fx / 255.f));
	}
	Res = Entity.KeyMap.Find(TEXT("scale"));
	if (Res)
	{
		const FLOAT fx = Res->Float();
		Out.Logf(TEXT("DrawScale=%f"), fx);
	}
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
	ParseOptScaledNumParm(TEXT("radius"), TEXT("PathRadius"), Entity, Out);
	ParseOptScaledNumParm(TEXT("speed"), TEXT("TrainSpeed"), Entity, Out);
	ParseOptScaledNumParm(TEXT("wait"), TEXT("PauseTime"), Entity, Out);

	if (!appStricmp(ClassName, TEXT("path_corner")))
		Out.Log(TEXT("OrientationType=OT_None"));
	else
	{
		FString* Res = Entity.KeyMap.Find(TEXT("orientationtype"));
		if (Res)
		{
			static const TCHAR* OrNames[] = { TEXT("OT_None"), TEXT("OT_Velocity"), TEXT("OT_Rotation") };
			const INT type = Res->Int();
			if (type < 3)
			{
				Out.Logf(TEXT("OrientationType=%ls"), OrNames[type]);
				if (type == 2)
					Out.Log(TEXT("bDirectional=True"));
			}
		}
	}
}
static void ExportBrush(EXPENTPARMS)
{
	static const DWORD RenderModeFlags[] = { PF_None, PF_Translucent, PF_None, (PF_Translucent | PF_Unlit), PF_Masked, PF_Translucent, PF_Invisible, PF_None, PF_None, PF_Unlit, PF_Invisible };

	const DWORD Solidity = appStricmp(ClassName, TEXT("func_illusionary")) ? PF_Semisolid : PF_NotSolid;
	FString* Res = Entity.KeyMap.Find(TEXT("rendermode"));
	const INT rndMode = Clamp<INT>(Res ? Res->Int() : 0, 0, 10);
	Out.Logf(TEXT("PolyFlags=%i"), Solidity | RenderModeFlags[rndMode]);
	Out.Log(TEXT("CsgOper=CSG_Add"));

	Res = Entity.KeyMap.Find(TEXT("model"));
	if (Res && Res->Left(1) == TEXT("*"))
	{
		const INT mdlIndex = appAtoi((**Res) + 1);
		BSPMODEL* M = Loader.GetModel(mdlIndex);
		if (M)
		{
			FString* Test = Entity.KeyMap.Find(FName(TEXT("origin")));
			Loader.WriteModel(*M, Out, FALSE, TRUE, (Test == nullptr), FALSE);
		}
	}
}
static void ExportMover(EXPENTPARMS)
{
	FString* Mdl = Entity.KeyMap.Find(TEXT("model"));
	BSPMODEL* M = nullptr;
	if (Mdl && Mdl->Left(1) == TEXT("*"))
	{
		const INT mdlIndex = appAtoi((**Mdl) + 1);
		M = Loader.GetModel(mdlIndex);
		if (M)
		{
			FString* Test = Entity.KeyMap.Find(FName(TEXT("origin")));
			Loader.WriteModel(*M, Out, FALSE, TRUE, (Test == nullptr), TRUE);
		}
	}

	if (!appStricmp(ClassName, TEXT("func_tracktrain")) || !appStricmp(ClassName, TEXT("func_train")))
	{
		if (!appStricmp(ClassName, TEXT("func_train")))
			Out.Log(TEXT("sf_FixedOrientation=True"));
		ParseOptScaledNumParm(TEXT("startspeed"), TEXT("MaxSpeed"), Entity, Out);
		ParseOptScaledNumParm(TEXT("speed"), TEXT("TrainSpeed"), Entity, Out);
		ParseOptScaledNumParm(TEXT("wheels"), TEXT("TurnDistance"), Entity, Out);
		ParseOptScaledNumParm(TEXT("height"), TEXT("TrackHeight"), Entity, Out);
		FString* Res = Entity.KeyMap.Find(TEXT("sounds"));
		if (Res)
		{
			const INT SoundType = Res->Int();
			Res = Entity.KeyMap.Find(TEXT("StartSound"));
			if (Res)
				Out.Logf(TEXT("OpeningSound=Sound'%ls'"), **Res);
			else if (SoundType == 1)
				Out.Log(TEXT("OpeningSound=Sound'ttrain_start1'"));
			Res = Entity.KeyMap.Find(TEXT("StopSound"));
			if (Res)
				Out.Logf(TEXT("OpenedSound=Sound'%ls'"), **Res);
			else if (SoundType == 1)
				Out.Log(TEXT("OpenedSound=Sound'ttrain_brake1'"));
			Res = Entity.KeyMap.Find(TEXT("MoveSound"));
			if (Res)
				Out.Logf(TEXT("MoveAmbientSound=Sound'%ls'"), **Res);
			else Out.Logf(TEXT("MoveAmbientSound=Sound'ttrain%i'"), SoundType);
		}
		else
		{
			Res = Entity.KeyMap.Find(TEXT("StartSound"));
			Out.Logf(TEXT("OpeningSound=Sound'%ls'"), Res ? **Res : TEXT("ttrain_start1"));
			Res = Entity.KeyMap.Find(TEXT("StopSound"));
			Out.Logf(TEXT("OpenedSound=Sound'%ls'"), Res ? **Res : TEXT("ttrain_brake1"));
		}

		Res = Entity.KeyMap.Find(TEXT("spawnflags"));
		if (Res)
		{
			const INT flags = Res->Int();
			if (flags & 1)
				Out.Log(TEXT("sf_NoPitch=True"));
			if (flags & 2)
				Out.Log(TEXT("sf_NoUserControl=True"));
			if (flags & 8)
			{
				Out.Log(TEXT("bCollideActors=False"));
				Out.Log(TEXT("bBlockActors=False"));
				Out.Log(TEXT("bBlockPlayers=False"));
			}
			if (flags & 16)
				Out.Log(TEXT("sf_FixedOrientation=True"));
			if (flags & 256)
				Out.Log(TEXT("sf_SoundPitchBySpeed=True"));
			if (flags & 512)
				Out.Log(TEXT("MoverEncroachType=ME_IgnoreWhenEncroach"));
		}
	}
	else if (!appStricmp(ClassName, TEXT("func_door")) || !appStricmp(ClassName, TEXT("func_door_rotating")))
	{
		INT SpawnFlags = 0;
		FString* Res = Entity.KeyMap.Find(TEXT("spawnflags"));
		if (Res)
			SpawnFlags = Res->Int();

		if (!appStricmp(ClassName, TEXT("func_door")))
		{
			FLOAT Speed = 100.f;
			FLOAT Distance = 100.f;
			FRotator R(0, 0, 0);
			Res = Entity.KeyMap.Find(TEXT("angle"));
			if (Res)
				R.Yaw = -ToUnrealAngles(Res->Int());
			Res = Entity.KeyMap.Find(TEXT("speed"));
			if (Res)
				Speed = Res->Float() * ToUnrScale;
			FVector MoveDir = R.Vector();
			if (M)
			{
				FVector BoxSize(M->nMaxs[0] - M->nMins[0], M->nMaxs[1] - M->nMins[1], M->nMaxs[2] - M->nMins[2]);
				Distance = FBoxPushOut(MoveDir, BoxSize) * ToUnrScale;
			}
			Res = Entity.KeyMap.Find(TEXT("lip"));
			if (Res)
				Distance -= (Res->Float() * ToUnrScale);

			const FVector FinalPos = MoveDir * Distance;
			Out.Logf(TEXT("KeyPos(1)=(X=%f,Y=%f,Z=%f)"), FinalPos.X, FinalPos.Y, FinalPos.Z);
			Out.Logf(TEXT("MoveTime=%f"), Distance / Max(Speed, 0.00001f));
		}
		else
		{
			INT RotAng = 0;
			Res = Entity.KeyMap.Find(TEXT("distance"));
			if (Res)
			{
				RotAng = ToUnrealAngles(Res->Int());
				FRotator Rot(0, 0, 0);

				if (SpawnFlags & 64)
					Rot.Roll = RotAng;
				if (SpawnFlags & 128)
					Rot.Pitch = RotAng;
				if ((SpawnFlags & (64 | 128)) == 0)
					Rot.Yaw = -RotAng;
				Out.Logf(TEXT("KeyRot(1)=(Yaw=%i,Pitch=%i,Roll=%i)"), Rot.Yaw, Rot.Pitch, Rot.Roll);
				if ((SpawnFlags & (16 | 64 | 128)) == 0)
				{
					Rot.Yaw = RotAng;
					Out.Logf(TEXT("KeyRot(2)=(Yaw=%i,Pitch=%i,Roll=%i)"), Rot.Yaw, Rot.Pitch, Rot.Roll);
					Out.Log(TEXT("bDirectionalOpen=True"));
					if (SpawnFlags & 2)
						Out.Log(TEXT("bReverseRotate=True"));
				}
			}

			INT Speed = 32000;
			Res = Entity.KeyMap.Find(TEXT("speed"));
			if (Res)
				Speed = ToUnrealAngles(Res->Int());

			Out.Logf(TEXT("MoveTime=%f"), Abs<FLOAT>(RotAng) / Max(FLOAT(Speed), 0.1f));
		}
		if ((SpawnFlags & (32 | 1024)) == 32)
			Out.Log(TEXT("InitialState=\"TriggerToggle\""));
		if (SpawnFlags & 256)
			Out.Log(TEXT("bUseTriggered=True"));
		if (SpawnFlags & 512)
			Out.Log(TEXT("bPlayersOnly=True"));
		if (SpawnFlags & 1024)
			Out.Log(TEXT("InitialState=\"BumpOpenTimed\""));

		Res = Entity.KeyMap.Find(TEXT("wait"));
		if (Res)
		{
			const FLOAT OpenTime = Res->Float();
			if (OpenTime < 0.f)
				Out.Log(TEXT("bTriggerOnceOnly=True"));
			else Out.Logf(TEXT("StayOpenTime=%f"), OpenTime);
		}
	}
	else if (!appStricmp(ClassName, TEXT("func_breakable")) || !appStricmp(ClassName, TEXT("func_pushable")))
	{
		ParseOptNumberParm(TEXT("health"), TEXT("Health"), Entity, Out);
		ParseOptNumberParm(TEXT("explodemagnitude"), TEXT("ExplosionMag"), Entity, Out);

		FString* Res = Entity.KeyMap.Find(TEXT("material"));
		if (Res)
		{
			const INT MatType = Res->Int();
			static const TCHAR* MatNames[] = { TEXT("EST_Glass"), TEXT("EST_Wood") ,TEXT("EST_Metal") ,TEXT("EST_Flesh") ,TEXT("EST_Rock") ,TEXT("EST_Rock") ,TEXT("EST_Metal") ,TEXT("EST_Glass"),TEXT("EST_Rock") };
			if (MatType < ARRAY_COUNT(MatNames))
				Out.Logf(TEXT("MaterialType=%ls"), MatNames[MatType]);
		}
		Res = Entity.KeyMap.Find(TEXT("spawnobject"));
		if (Res)
		{
			const INT ItemType = Res->Int();
			static const TCHAR* ItemClasses[] = { TEXT(""),TEXT("I_Battery"),TEXT("I_HealthKit"),TEXT("W_Pistol"),TEXT("A_Pistol"),TEXT("W_SMG"),TEXT("A_SMG"),TEXT("A_SMG_Grenade"),TEXT("W_Shotgun"),TEXT("A_Shotgun"),TEXT("W_Crossbow"),TEXT("A_Crossbow"),
				TEXT("W_Magnum"),TEXT("A_Magnum"),TEXT("W_RPG"),TEXT("A_RPG"),TEXT("A_Gauss"),TEXT("W_Grenade"),TEXT("W_Tripmine"),TEXT("W_RemoteExp"),TEXT("W_Snark"),TEXT("W_HornetGun") };
			if (ItemType>0 && ItemType < ARRAY_COUNT(ItemClasses))
				Out.Logf(TEXT("Contents=Class'%ls'"), ItemClasses[ItemType]);
		}

		if (!appStricmp(ClassName, TEXT("func_breakable")))
		{
			Out.Log(TEXT("bPushable=False"));
			Res = Entity.KeyMap.Find(TEXT("spawnflags"));
			if (Res)
			{
				const INT flags = Res->Int();

				if (flags & 1)
					Out.Log(TEXT("bTriggerBreakable=True"));
				if (flags & 2)
					Out.Log(TEXT("bTouchBreakable=True"));
				if (flags & 4)
					Out.Log(TEXT("bPressureBreakable=True"));
				if (flags & 256)
					Out.Log(TEXT("bCrowbarBreakable=True"));
			}
		}
	}
	else if (!appStricmp(ClassName, TEXT("func_pendulum")))
	{
		INT SpawnFlags = 0;
		FString* Res = Entity.KeyMap.Find(TEXT("spawnflags"));
		if (Res)
			SpawnFlags = Res->Int();

		if (SpawnFlags & 1)
			Out.Log(TEXT("InitialState=\"ConstantLoop\""));
		else Out.Log(TEXT("InitialState=\"TriggeredLoop\""));
		Out.Log(TEXT("MoverGlideType=MV_GlideByTime"));

		INT RotAng = 0;
		Res = Entity.KeyMap.Find(TEXT("distance"));
		if (Res)
		{
			RotAng = ToUnrealAngles(Res->Int());
			FRotator Rot(0, 0, 0);

			if (SpawnFlags & 64)
				Rot.Roll = RotAng;
			if (SpawnFlags & 128)
				Rot.Pitch = RotAng;
			if ((SpawnFlags & (64 | 128)) == 0)
				Rot.Yaw = -RotAng;
			Out.Logf(TEXT("KeyRot(1)=(Yaw=%i,Pitch=%i,Roll=%i)"), Rot.Yaw*2, Rot.Pitch*2, Rot.Roll*2);
			Out.Logf(TEXT("BaseRot=(Yaw=%i,Pitch=%i,Roll=%i)"), -Rot.Yaw, -Rot.Pitch, -Rot.Roll);
			Out.Logf(TEXT("Rotation=(Yaw=%i,Pitch=%i,Roll=%i)"), -Rot.Yaw, -Rot.Pitch, -Rot.Roll);
		}

		INT Speed = 32000;
		Res = Entity.KeyMap.Find(TEXT("speed"));
		if (Res)
			Speed = ToUnrealAngles(Res->Int());

		Out.Logf(TEXT("MoveTime=%f"), Abs<FLOAT>(RotAng * 2.71828f) / Max(FLOAT(Speed), 0.1f));
		Out.Logf(TEXT("StayOpenTime=0"));
	}
}
static void ExportTeleporter(EXPENTPARMS)
{
	FString* Res = Entity.KeyMap.Find(TEXT("model"));
	if (Res && Res->Left(1) == TEXT("*"))
	{
		const INT mdlIndex = appAtoi((**Res) + 1);
		BSPMODEL* M = Loader.GetModel(mdlIndex);
		if (M)
			Loader.WriteModel(*M, Out, FALSE, FALSE, FALSE, FALSE);
	}

	FString* LM = Entity.KeyMap.Find(TEXT("landmark"));
	Res = Entity.KeyMap.Find(TEXT("Map"));
	if (Res)
		Out.Logf(TEXT("URL=\"%ls#%ls\""), **Res, LM ? **LM : TEXT("go"));
}
static void ExportSoundTrigger(EXPENTPARMS)
{
	FString* Res = Entity.KeyMap.Find(TEXT("Message"));
	if (Res)
	{
		FString FX = Res->GetFilenameOnly();
		Out.Logf(TEXT("SoundEffect=Sound'%ls'"), *FX);
	}
	Res = Entity.KeyMap.Find(TEXT("radius"));
	if (Res)
	{
		const FLOAT r = Res->Float();
		Out.Logf(TEXT("EffectRadius=%f"), (r * ToUnrScale));
	}
	Res = Entity.KeyMap.Find(TEXT("health"));
	if (Res)
	{
		const FLOAT r = Res->Float();
		Out.Logf(TEXT("EffectVolume=%f"), (r / 10.f));
	}
	Res = Entity.KeyMap.Find(TEXT("pitch"));
	if (Res)
	{
		const FLOAT r = Res->Float();
		Out.Logf(TEXT("EffectPitch=%f"), (r / 100.f));
	}
	ParseOptNameParm(TEXT("SourceEntityName"), TEXT("SourceEntityName"), Entity, Out);
	Res = Entity.KeyMap.Find(TEXT("spawnflags"));
	if (Res)
	{
		const INT flags = Res->Int();
		if (flags & 1)
			Out.Log(TEXT("sf_PlayEverywhere=True"));
		if (!(flags & 16))
			Out.Log(TEXT("sf_StartPlaying=True"));
		if (!(flags & 32))
			Out.Log(TEXT("sf_LoopSound=True"));
	}
}
static void ExportEnvFade(EXPENTPARMS)
{
	ParseOptNumberParm(TEXT("duration"), TEXT("Duration"), Entity, Out);
	ParseOptNumberParm(TEXT("holdtime"), TEXT("HoldTime"), Entity, Out);
	FString* Res = Entity.KeyMap.Find(TEXT("renderamt"));
	if (Res)
	{
		const FLOAT r = Res->Float();
		Out.Logf(TEXT("FadeOpacity=%f"), (r / 255.f));
	}
	Res = Entity.KeyMap.Find(TEXT("rendercolor"));
	if (Res)
	{
		FColor C = GetColorOf(**Res);
		Out.Logf(TEXT("FadeColor=(R=%i,G=%i,B=%i)"), INT(C.R), INT(C.G), INT(C.B));
	}
	ParseOptNumberParm(TEXT("ReverseFadeDuration"), TEXT("ReverseFadeDuration"), Entity, Out);
	
	Res = Entity.KeyMap.Find(TEXT("spawnflags"));
	if (Res)
	{
		const INT flags = Res->Int();
		if (flags & 1)
			Out.Log(TEXT("sf_FadeFrom=True"));
		if (flags & 8)
			Out.Log(TEXT("sf_StayOut=True"));
	}
}
static void ExportBeam(EXPENTPARMS)
{
	ParseOptNumberParm(TEXT("StrikeTime"), TEXT("RestrikeTime"), Entity, Out);
	ParseOptNumberParm(TEXT("Radius"), TEXT("BeamRadius"), Entity, Out);
	ParseOptNumberParm(TEXT("life"), TEXT("BeamLifeTime"), Entity, Out);
	ParseOptNumberParm(TEXT("damage"), TEXT("BeamDamage"), Entity, Out);

	FString* Res = Entity.KeyMap.Find(TEXT("BoltWidth"));
	if (Res)
	{
		const FLOAT r = Res->Float() / 10.f;
		Out.Logf(TEXT("StartingScale=(Min=%f,Max=%f)"), (r * ToUnrScale * 0.8), (r * ToUnrScale));
	}
	ParseOptNameParm(TEXT("LightningStart"), TEXT("BeamStartTag"), Entity, Out);
	ParseOptNameParm(TEXT("LightningEnd"), TEXT("BeamEndTag"), Entity, Out);

	Res = Entity.KeyMap.Find(TEXT("TouchType"));
	if (Res)
	{
		const INT t = Res->Int();
		static const TCHAR* TouchTypes[] = { TEXT("BTT_None"), TEXT("BTT_Players") ,TEXT("BTT_NPC") ,TEXT("BTT_PlayersNPC") ,TEXT("BTT_PlayersNPCProps") };
		if (t < ARRAY_COUNT(TouchTypes))
			Out.Logf(TEXT("TouchType=%ls"), TouchTypes[t]);
	}
	Res = Entity.KeyMap.Find(TEXT("ClipStyle"));
	if (Res)
	{
		const INT t = Res->Int();
		static const TCHAR* ClipStyles[] = { TEXT("CLIP_None"), TEXT("CLIP_BSP") ,TEXT("CLIP_AllColliding") };
		if (t < ARRAY_COUNT(ClipStyles))
			Out.Logf(TEXT("ClipMode=%ls"), ClipStyles[t]);
	}
	Res = Entity.KeyMap.Find(TEXT("targetpoint"));
	if (Res)
	{
		const FVector v = ToUnrealScale(GetVectorOf(**Res));
		Out.Logf(TEXT("BeamEndPointVec=(X=%f,Y=%f,Z=%f)"), v.X, v.Y, v.Z);
	}
	Res = Entity.KeyMap.Find(TEXT("Texture"));
	if (Res)
	{
		FString S = Res->GetFilenameOnly();
		Out.Logf(TEXT("ParticleTextures(0)=Texture'%ls'"), *S);
	}
	Res = Entity.KeyMap.Find(TEXT("NoiseAmplitude"));
	if (Res)
	{
		FLOAT a = Res->Float() * 0.15f;
		Out.Logf(TEXT("NoiseRange=(X=(Min=%f,Max=%f),Y=(Min=%f,Max=%f),Z=(Min=%f,Max=%f))"), -a, a, -a, a, -a, a);
		Out.Log(TEXT("bDoBeamNoise=True"));
		Out.Log(TEXT("Segments=6"));
	}
	Res = Entity.KeyMap.Find(TEXT("renderamt"));
	if (Res)
	{
		FLOAT a = Res->Float() / 255.f;
		Out.Logf(TEXT("FadeInMaxAmount=%f"), a);
	}
	Res = Entity.KeyMap.Find(TEXT("rendercolor"));
	if (Res)
	{
		FColor c = GetColorOf(**Res);
		FVector v = c.Vector();
		Out.Logf(TEXT("ParticleColor=(X=(Min=%f,Max=%f),Y=(Min=%f,Max=%f),Z=(Min=%f,Max=%f))"), v.X * 0.9f, v.X, v.Y * 0.9f, v.Y, v.Z * 0.9f, v.Z);
	}

	Res = Entity.KeyMap.Find(TEXT("spawnflags"));
	if (Res)
	{
		const INT flags = Res->Int();
		if (flags & 1)
			Out.Log(TEXT("bInitiallyOn=True"));
		if (flags & 2)
			Out.Log(TEXT("bToggleBeam=True"));
		if (flags & 4)
			Out.Log(TEXT("bRandomStrike=True"));
	}
}

struct FEntityExport
{
	f_ExpEntFunc* Func;
	const TCHAR* UnrealClass;
	const FLOAT EntityHeight;
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

void CMapLoadHelper::ExportMap(FOutputDevice& Out)
{
	guard(CMapLoadHelper::ExportMap);
	Out.Log(TEXT("BEGIN Map"));
	{
		Out.Log(NAME_Add, TEXT("BEGIN Actor Class=LevelInfo Name=LevelInfo0"));
		FEntListData::FEntity* Ent = EntData.FindEntity(TEXT("worldspawn"));
		if (Ent)
		{
			FString* Title = Ent->KeyMap.Find(FName(TEXT("message")));
			if (Title)
			{
				Out.Logf(TEXT("Title=\"%ls - %ls\""), *MapFile, *(*Title));
				FString* EntryMsg = Ent->KeyMap.Find(FName(TEXT("gametitle")));
				if (EntryMsg && *EntryMsg == TEXT("1"))
					Out.Logf(TEXT("LevelEnterText=\"%ls\""), *(*Title));
			}
			else Out.Logf(TEXT("Title=\"%ls\""), *MapFile);
		}

		// Find ambient light
		Ent = EntData.FindEntity(TEXT("light_environment"));
		if (Ent)
		{
			FString* Amb = Ent->KeyMap.Find(FName(TEXT("_ambient")));
			if (Amb)
			{
				FColor C = GetColorOf(**Amb);
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
			Builder.Models(i)->MergePolys(TRUE);
			UBOOL bHadLights = Builder.Models(i)->DumpOutput(Out, TRUE);
			Out.Log(NAME_Remove, TEXT("END PolyList"));
			Out.Log(NAME_Remove, TEXT("END Brush"));
			Out.Logf(TEXT("Brush=Model'Brush%i'"), iModelIndex);
			const FVector Center = Builder.Models(i)->GetCentroid();
			Out.Logf(TEXT("Location=(X=%f,Y=%f,Z=%f)"), Center.X, Center.Y, Center.Z);
			++iModelIndex;
			Out.Log(NAME_Remove, TEXT("END Actor"));

			// Extract lights too.
			if (bHadLights)
				Builder.Models(i)->DumpLights(Out);
		}
	}
	
	{
		FName NAME_ClassName(TEXT("classname"));
		TMap<FString, FEntityExport> ExpMap;
		ExpMap.Set(TEXT("light_spot"), FEntityExport(&ExportLights, TEXT("Light"), 0.f, EXP_TriggerLight));
		ExpMap.Set(TEXT("light"), FEntityExport(&ExportLights, TEXT("Light"), 0.f, EXP_TriggerLight));
		ExpMap.Set(TEXT("light_environment"), FEntityExport(&ExportLights, TEXT("Sunlight")));
		ExpMap.Set(TEXT("monster_scientist"), FEntityExport(&ExportScientist, TEXT("M_Scientist"), 39.f));
		ExpMap.Set(TEXT("monster_sitting_scientist"), FEntityExport(&ExportScientist, TEXT("M_Scientist_Sitting"), 10.f));
		ExpMap.Set(TEXT("monster_barney"), FEntityExport(&ExportNothing, TEXT("M_Barney"), 39.f));
		ExpMap.Set(TEXT("monster_generic"), FEntityExport(&ExportNothing, TEXT("M_Generic")));
		ExpMap.Set(TEXT("info_node"), FEntityExport(&ExportNothing, TEXT("PathNode"), 45.f));
		ExpMap.Set(TEXT("info_player_start"), FEntityExport(&ExportNothing, TEXT("PlayerStart"), 40.f));
		ExpMap.Set(TEXT("path_track"), FEntityExport(&ExportPathTrack, TEXT("HL_TrainTrack")));
		ExpMap.Set(TEXT("path_corner"), FEntityExport(&ExportPathTrack, TEXT("HL_TrainTrack")));
		ExpMap.Set(TEXT("trigger_auto"), FEntityExport(&ExportNothing, TEXT("HL_TriggerAuto")));
		ExpMap.Set(TEXT("scripted_sequence"), FEntityExport(&ExportScriptedSequence, TEXT("HL_ScriptedSequence"), 39.f));
		ExpMap.Set(TEXT("env_message"), FEntityExport(&ExportEnvMessage, TEXT("HL_TriggerMessage")));
		ExpMap.Set(TEXT("multi_manager"), FEntityExport(&ExportMultiManager, TEXT("HL_MultiManager")));
		ExpMap.Set(TEXT("env_glow"), FEntityExport(&ExportEnvGlov, TEXT("ScaledSprite")));
		ExpMap.Set(TEXT("trigger_relay"), FEntityExport(&ExportTriggerRelay, TEXT("HL_RelayTrigger")));
		ExpMap.Set(TEXT("func_wall"), FEntityExport(&ExportBrush, TEXT("Brush")));
		ExpMap.Set(TEXT("func_illusionary"), FEntityExport(&ExportBrush, TEXT("Brush")));
		ExpMap.Set(TEXT("func_breakable"), FEntityExport(&ExportMover, TEXT("HL_Pushable"), 0.f, EXP_Mover));
		ExpMap.Set(TEXT("func_pushable"), FEntityExport(&ExportMover, TEXT("HL_Pushable"), 0.f, EXP_Mover));
		ExpMap.Set(TEXT("func_tracktrain"), FEntityExport(&ExportMover, TEXT("HL_TrainMover"), 0.f, EXP_Mover));
		ExpMap.Set(TEXT("func_train"), FEntityExport(&ExportMover, TEXT("HL_TrainMover"), 0.f, EXP_Mover));
		ExpMap.Set(TEXT("func_door"), FEntityExport(&ExportMover, TEXT("HL_MoveLinear"), 0.f, EXP_Mover | EXP_NoRotation));
		ExpMap.Set(TEXT("func_door_rotating"), FEntityExport(&ExportMover, TEXT("HL_MoveLinear"), 0.f, EXP_Mover));
		ExpMap.Set(TEXT("func_pendulum"), FEntityExport(&ExportMover, TEXT("HL_MoveLinear"), 0.f, EXP_Mover | EXP_NoRotation));
		ExpMap.Set(TEXT("trigger_changelevel"), FEntityExport(&ExportTeleporter, TEXT("Teleporter")));
		ExpMap.Set(TEXT("ambient_generic"), FEntityExport(&ExportSoundTrigger, TEXT("HL_SoundTrigger")));
		ExpMap.Set(TEXT("env_fade"), FEntityExport(&ExportEnvFade, TEXT("HL_EnvFade")));
		ExpMap.Set(TEXT("info_target"), FEntityExport(&ExportNothing, TEXT("HL_TargetPoint")));
		ExpMap.Set(TEXT("env_beam"), FEntityExport(&ExportBeam, TEXT("HL_BeamEffect")));

		for (INT i = 0; i < EntData.EntList.Num(); ++i)
		{
			FEntListData::FEntity& Ent = *EntData.EntList(i);
			FString* Res = Ent.KeyMap.Find(NAME_ClassName);
			if (!Res)
				continue;

			FEntityExport* f = ExpMap.Find(**Res);
			if (f)
			{
				const TCHAR* USClass = f->UnrealClass;
				DWORD ExportFlags = f->ExpFlags;
				if (ExportFlags & EXP_TriggerLight)
				{
					ExportFlags = EXP_None;
					FString* SF = Ent.KeyMap.Find(TEXT("spawnflags"));
					if (SF && (SF->Int() & 1))
					{
						ExportFlags |= EXP_TriggerLight;
						USClass = TEXT("TriggerLight");
					}
					else
					{
						SF = Ent.KeyMap.Find(TEXT("targetname"));
						if (SF)
						{
							ExportFlags |= EXP_TriggerLight;
							USClass = TEXT("TriggerLight");
						}
					}
				}
				ExportEntity(USClass, Out);
				(*f->Func)(*this, **Res, Ent, Out);
				ExportEntityActor(Ent, Out, f->EntityHeight, ExportFlags);
				Out.Log(NAME_Remove, TEXT("END Actor"));
			}
			else debugf(TEXT("Unknown entity: %ls"), **Res);
		}
	}

	Out.Log(TEXT("END Map"));
	unguard;
}
