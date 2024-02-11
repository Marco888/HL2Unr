#pragma once


// little-endian "VBSP"
#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'V')		

// MINBSPVERSION is the minimum acceptable version.  The engine will load MINBSPVERSION through BSPVERSION
#define MINBSPVERSION 19
#define BSPVERSION 20


// This needs to match the value in gl_lightmap.h
// Need to dynamically allocate the weights and light values in radial_t to make this variable.
#define MAX_BRUSH_LIGHTMAP_DIM_WITHOUT_BORDER 32
// This is one more than what vbsp cuts for to allow for rounding errors
#define MAX_BRUSH_LIGHTMAP_DIM_INCLUDING_BORDER	35

// We can have larger lightmaps on displacements
#define MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER	125
#define MAX_DISP_LIGHTMAP_DIM_INCLUDING_BORDER	128


// This is the actual max.. (change if you change the brush lightmap dim or disp lightmap dim
#define MAX_LIGHTMAP_DIM_WITHOUT_BORDER		MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER
#define MAX_LIGHTMAP_DIM_INCLUDING_BORDER	MAX_DISP_LIGHTMAP_DIM_INCLUDING_BORDER

#define	MAX_LIGHTSTYLES	64


// upper design bounds
#define MIN_MAP_DISP_POWER		2	// Minimum and maximum power a displacement can be.
#define MAX_MAP_DISP_POWER		4	

// Max # of neighboring displacement touching a displacement's corner.
#define MAX_DISP_CORNER_NEIGHBORS	4

#define NUM_DISP_POWER_VERTS(power)	( ((1 << (power)) + 1) * ((1 << (power)) + 1) )
#define NUM_DISP_POWER_TRIS(power)	( (1 << (power)) * (1 << (power)) * 2 )

// Common limits
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	MAX_MAP_MODELS					1024
#define	MAX_MAP_BRUSHES					8192
#define	MAX_MAP_ENTITIES				8192
#define	MAX_MAP_TEXINFO					12288
#define MAX_MAP_TEXDATA					2048
#define MAX_MAP_DISPINFO				2048
#define MAX_MAP_DISP_VERTS				( MAX_MAP_DISPINFO * ((1<<MAX_MAP_DISP_POWER)+1) * ((1<<MAX_MAP_DISP_POWER)+1) )
#define MAX_MAP_DISP_TRIS				( (1 << MAX_MAP_DISP_POWER) * (1 << MAX_MAP_DISP_POWER) * 2 )
#define MAX_DISPVERTS					NUM_DISP_POWER_VERTS( MAX_MAP_DISP_POWER )
#define MAX_DISPTRIS					NUM_DISP_POWER_TRIS( MAX_MAP_DISP_POWER )
#define	MAX_MAP_AREAS					256
#define MAX_MAP_AREA_BYTES				(MAX_MAP_AREAS/8)
#define	MAX_MAP_AREAPORTALS				1024
// Planes come in pairs, thus an even number.
#define	MAX_MAP_PLANES					65536
#define	MAX_MAP_NODES					65536
#define	MAX_MAP_BRUSHSIDES				65536
#define	MAX_MAP_LEAFS					65536
#define	MAX_MAP_VERTS					65536
#define MAX_MAP_VERTNORMALS				256000
#define MAX_MAP_VERTNORMALINDICES		256000
#define	MAX_MAP_FACES					65536
#define	MAX_MAP_LEAFFACES				65536
#define	MAX_MAP_LEAFBRUSHES 			65536
#define	MAX_MAP_PORTALS					65536
#define MAX_MAP_CLUSTERS				65536
#define MAX_MAP_LEAFWATERDATA			32768
#define MAX_MAP_PORTALVERTS				128000
#define	MAX_MAP_EDGES					256000
#define	MAX_MAP_SURFEDGES				512000
#define	MAX_MAP_LIGHTING				0x1000000
#define	MAX_MAP_VISIBILITY				0x1000000			// increased BSPVERSION 7
#define	MAX_MAP_TEXTURES				1024
#define MAX_MAP_WORLDLIGHTS				8192
#define MAX_MAP_CUBEMAPSAMPLES			1024
#define MAX_MAP_OVERLAYS				512 
#define MAX_MAP_WATEROVERLAYS			16384
#define MAX_MAP_TEXDATA_STRING_DATA		256000
#define MAX_MAP_TEXDATA_STRING_TABLE	65536
// this is stuff for trilist/tristrips, etc.
#define MAX_MAP_PRIMITIVES				32768
#define MAX_MAP_PRIMVERTS				65536
#define MAX_MAP_PRIMINDICES				65536

// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024


// ------------------------------------------------------------------------------------------------ //
// Displacement neighbor rules
// ------------------------------------------------------------------------------------------------ //
//
// Each displacement is considered to be in its own space:
//
//               NEIGHBOREDGE_TOP
//
//                   1 --- 2
//                   |     |
// NEIGHBOREDGE_LEFT |     | NEIGHBOREDGE_RIGHT
//                   |     |
//                   0 --- 3
//
//   			NEIGHBOREDGE_BOTTOM
//
//
// Edge edge of a displacement can have up to two neighbors. If it only has one neighbor
// and the neighbor fills the edge, then SubNeighbor 0 uses CORNER_TO_CORNER (and SubNeighbor 1
// is undefined).
//
// CORNER_TO_MIDPOINT means that it spans [bottom edge,midpoint] or [left edge,midpoint] depending
// on which edge you're on.
//
// MIDPOINT_TO_CORNER means that it spans [midpoint,top edge] or [midpoint,right edge] depending
// on which edge you're on.
//
// Here's an illustration (where C2M=CORNER_TO_MIDPOINT and M2C=MIDPOINT_TO_CORNER
//
//
//				 C2M			  M2C
//
//       1 --------------> x --------------> 2
//
//       ^                                   ^
//       |                                   |
//       |                                   |
//  M2C  |                                   |	M2C
//       |                                   |
//       |                                   |
//
//       x                 x                 x 
//
//       ^                                   ^
//       |                                   |
//       |                                   |
//  C2M  |                                   |	C2M
//       |                                   |
//       |                                   |
// 
//       0 --------------> x --------------> 3
//
//               C2M			  M2C
//
//
// The CHILDNODE_ defines can be used to refer to a node's child nodes (this is for when you're
// recursing into the node tree inside a displacement):
//
// ---------
// |   |   |
// | 1 | 0 |
// |   |   |
// |---x---|
// |   |   |
// | 2 | 3 |
// |   |   |
// ---------
// 
// ------------------------------------------------------------------------------------------------ //

// These can be used to index g_ChildNodeIndexMul.
enum
{
	CHILDNODE_UPPER_RIGHT=0,
	CHILDNODE_UPPER_LEFT=1,
	CHILDNODE_LOWER_LEFT=2,
	CHILDNODE_LOWER_RIGHT=3
};


// Corner indices. Used to index m_CornerNeighbors.
enum
{
	CORNER_LOWER_LEFT=0,
	CORNER_UPPER_LEFT=1,
	CORNER_UPPER_RIGHT=2,
	CORNER_LOWER_RIGHT=3
};


// These edge indices must match the edge indices of the CCoreDispSurface.
enum
{
	NEIGHBOREDGE_LEFT=0,
	NEIGHBOREDGE_TOP=1,
	NEIGHBOREDGE_RIGHT=2,
	NEIGHBOREDGE_BOTTOM=3
};


// These denote where one dispinfo fits on another.
// Note: tables are generated based on these indices so make sure to update
//       them if these indices are changed.
typedef enum
{
	CORNER_TO_CORNER=0,
	CORNER_TO_MIDPOINT=1,
	MIDPOINT_TO_CORNER=2
} NeighborSpan;


// These define relative orientations of displacement neighbors.
typedef enum
{
	ORIENTATION_CCW_0=0,
	ORIENTATION_CCW_90=1,
	ORIENTATION_CCW_180=2,
	ORIENTATION_CCW_270=3
} NeighborOrientation;


//=============================================================================

enum
{
	LUMP_ENTITIES					= 0,	// *
	LUMP_PLANES						= 1,	// *
	LUMP_TEXDATA					= 2,	// *
	LUMP_VERTEXES					= 3,	// *
	LUMP_VISIBILITY					= 4,	// *
	LUMP_NODES						= 5,	// *
	LUMP_TEXINFO					= 6,	// *
	LUMP_FACES						= 7,	// *
	LUMP_LIGHTING					= 8,	// *
	LUMP_OCCLUSION					= 9,
	LUMP_LEAFS						= 10,	// *
	LUMP_FACEIDS					= 11,
	LUMP_EDGES						= 12,	// *
	LUMP_SURFEDGES					= 13,	// *
	LUMP_MODELS						= 14,	// *
	LUMP_WORLDLIGHTS				= 15,	// 
	LUMP_LEAFFACES					= 16,	// *
	LUMP_LEAFBRUSHES				= 17,	// *
	LUMP_BRUSHES					= 18,	// *
	LUMP_BRUSHSIDES					= 19,	// *
	LUMP_AREAS						= 20,	// *
	LUMP_AREAPORTALS				= 21,	// *
	LUMP_UNUSED0					= 22,
	LUMP_UNUSED1					= 23,
	LUMP_UNUSED2					= 24,
	LUMP_UNUSED3					= 25,
	LUMP_DISPINFO					= 26,
	LUMP_ORIGINALFACES				= 27,
	LUMP_PHYSDISP					= 28,
	LUMP_PHYSCOLLIDE				= 29,
	LUMP_VERTNORMALS				= 30,
	LUMP_VERTNORMALINDICES			= 31,
	LUMP_DISP_LIGHTMAP_ALPHAS		= 32,
	LUMP_DISP_VERTS					= 33,		// CDispVerts
	LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,	// For each displacement
												//     For each lightmap sample
												//         byte for index
												//         if 255, then index = next byte + 255
												//         3 bytes for barycentric coordinates
	// The game lump is a method of adding game-specific lumps
	// FIXME: Eventually, all lumps could use the game lump system
	LUMP_GAME_LUMP					= 35,
	LUMP_LEAFWATERDATA				= 36,
	LUMP_PRIMITIVES					= 37,
	LUMP_PRIMVERTS					= 38,
	LUMP_PRIMINDICES				= 39,
	// A pak file can be embedded in a .bsp now, and the file system will search the pak
	//  file first for any referenced names, before deferring to the game directory 
	//  file system/pak files and finally the base directory file system/pak files.
	LUMP_PAKFILE					= 40,
	LUMP_CLIPPORTALVERTS			= 41,
	// A map can have a number of cubemap entities in it which cause cubemap renders
	// to be taken after running vrad.
	LUMP_CUBEMAPS					= 42,
	LUMP_TEXDATA_STRING_DATA		= 43,
	LUMP_TEXDATA_STRING_TABLE		= 44,
	LUMP_OVERLAYS					= 45,
	LUMP_LEAFMINDISTTOWATER			= 46,
	LUMP_FACE_MACRO_TEXTURE_INFO	= 47,
	LUMP_DISP_TRIS					= 48,
	LUMP_PHYSCOLLIDESURFACE			= 49,	// deprecated.  We no longer use win32-specific havok compression on terrain
	LUMP_WATEROVERLAYS              = 50,
	LUMP_LEAF_AMBIENT_INDEX_HDR		= 51,	// index of LUMP_LEAF_AMBIENT_LIGHTING_HDR
	LUMP_LEAF_AMBIENT_INDEX         = 52,	// index of LUMP_LEAF_AMBIENT_LIGHTING

	// optional lumps for HDR
	LUMP_LIGHTING_HDR				= 53,
	LUMP_WORLDLIGHTS_HDR			= 54,
	LUMP_LEAF_AMBIENT_LIGHTING_HDR	= 55,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.
	LUMP_LEAF_AMBIENT_LIGHTING		= 56,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.

	LUMP_XZIPPAKFILE				= 57,   // deprecated. xbox 1: xzip version of pak file
	LUMP_FACES_HDR					= 58,	// HDR maps may have different face data.
	LUMP_MAP_FLAGS                  = 59,   // extended level-wide flags. not present in all levels
	LUMP_OVERLAY_FADES				= 60,	// Fade distances for overlays
};


// Lumps that have versions are listed here
enum
{
	LUMP_LIGHTING_VERSION          = 1,
	LUMP_FACES_VERSION             = 1,
	LUMP_OCCLUSION_VERSION         = 2,
	LUMP_LEAFS_VERSION			   = 1,
	LUMP_LEAF_AMBIENT_LIGHTING_VERSION = 1,
};

#define	HEADER_LUMPS		64

const TCHAR* GetCCString(const char* InChr, const INT Len);

struct lump_t
{
	int		fileofs, filelen;
	int		version;		// default to zero
	char	fourCC[4];		// default to ( char )0, ( char )0, ( char )0, ( char )0

	friend FArchive& operator<<(FArchive& Ar, lump_t& l)
	{
		Ar << l.fileofs << l.filelen << l.version;
		Ar.Serialize(l.fourCC, sizeof(lump_t::fourCC));
		return Ar;
	}
	inline const TCHAR* GetName() const
	{
		return GetCCString(fourCC, sizeof(fourCC));
	}
};
struct dheader_t
{
	int			ident;
	int			version;
	lump_t		lumps[HEADER_LUMPS];
	int			mapRevision;				// the map's revision (iteration, version) number (added BSPVERSION 6)

	friend FArchive& operator<<(FArchive& Ar, dheader_t& h)
	{
		Ar << h.ident << h.version;
		for (INT i = 0; i < HEADER_LUMPS; ++i)
			Ar << h.lumps[i];
		return Ar << h.mapRevision;
	}
};
struct dplane_t
{
	FVector	normal;
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate

	INT signbits;

	friend FArchive& operator<<(FArchive& Ar, dplane_t& p)
	{
		Ar << p.normal << p.dist << p.type;
		p.CalcSignBits();
		return Ar;
	}
	inline void CalcSignBits()
	{
		INT bits = 0;
		const FLOAT* axis = &normal.X;
		for (INT j = 0; j < 3; j++)
		{
			if (axis[j] < 0)
				bits |= 1 << j;
		}
		signbits = bits;
	}
};
constexpr INT dplane_t_size = sizeof(FLOAT) * 4 + sizeof(INT);

struct dtexdata_t
{
	FVector		reflectivity;
	int			nameStringTableID;				// index into g_StringTable for the texture name
	int			width, height;					// source image
	int			view_width, view_height;		//

	friend FArchive& operator<<(FArchive& Ar, dtexdata_t& t)
	{
		return Ar << t.reflectivity << t.nameStringTableID << t.width << t.height << t.view_width << t.view_height;
	}
};
constexpr INT dtexdata_t_size = sizeof(FLOAT) * 3 + sizeof(INT) * 5;

#define MAXTEXTURENAME 16
#define MIPLEVELS 4
struct BSPMIPTEX
{
	char szName[MAXTEXTURENAME];  // Name of texture
	INT nWidth, nHeight;     // Extends of the texture
	INT nOffsets[MIPLEVELS]; // Offsets to texture mipmaps BSPMIPTEX;

	friend FArchive& operator<<(FArchive& Ar, BSPMIPTEX& t)
	{
		Ar.Serialize(t.szName,sizeof(BSPMIPTEX::szName));
		Ar << t.nWidth << t.nHeight;
		Ar.Serialize(t.nOffsets, sizeof(BSPMIPTEX::nOffsets));
		return Ar;
	}
	inline const TCHAR* GetName() const
	{
		return GetCCString(szName, sizeof(szName));
	}
};

struct BSPLUMP
{
	int32_t nOffset; // File offset to data
	int32_t nLength; // Length of data

	friend FArchive& operator<<(FArchive& Ar, BSPLUMP& l)
	{
		return Ar << l.nOffset << l.nLength;
	}
};
struct BSPHEADER
{
	int32_t nVersion;           // Must be 30 for a valid HL BSP file
	BSPLUMP lump[HEADER_LUMPS]; // Stores the directory of lumps

	friend FArchive& operator<<(FArchive& Ar, BSPHEADER& h)
	{
		Ar << h.nVersion;
		for (INT i = 0; i < ARRAY_COUNT(BSPHEADER::lump); ++i)
			Ar << h.lump[i];
		return Ar;
	}
};

constexpr INT vertex_size = sizeof(FLOAT) * 3;

struct BSPFACE
{
	_WORD iPlane;			// Plane the face is parallel to
	_WORD nPlaneSide;		// Set if different normals orientation
	INT iFirstEdge;			// Index of the first surfedge
	_WORD nEdges;			// Number of consecutive surfedges
	_WORD iTextureInfo;		// Index of the texture info structure
	BYTE nStyles[4];		// Specify lighting styles
	INT nLightmapOffset;	// Offsets into the raw lightmap data

	INT iTag;

	friend FArchive& operator<<(FArchive& Ar, BSPFACE& f)
	{
		f.iTag = 0;
		Ar << f.iPlane << f.nPlaneSide << f.iFirstEdge << f.nEdges << f.iTextureInfo;
		Ar.Serialize(f.nStyles, sizeof(BSPFACE::nStyles));
		Ar << f.nLightmapOffset;
		return Ar;
	}
};
constexpr INT BSPFACE_size = sizeof(_WORD) * 4 + sizeof(INT) * 2 + sizeof(BSPFACE::nStyles);

struct BSPEDGE
{
	_WORD iVertex[2]; // Indices into vertex array

	INT iTag;

	friend FArchive& operator<<(FArchive& Ar, BSPEDGE& e)
	{
		e.iTag = 0;
		Ar.Serialize(e.iVertex, sizeof(BSPEDGE::iVertex));
		return Ar;
	}
};
constexpr INT BSPEDGE_size = sizeof(_WORD) * 2;

#define MAX_MAP_HULLS 4

struct BSPMODEL
{
	FLOAT nMins[3], nMaxs[3];          // Defines bounding box
	FVector vOrigin;                  // Coordinates to move the // coordinate system
	INT iHeadnodes[MAX_MAP_HULLS]; // Index into nodes array
	INT nVisLeafs;                 // ???
	INT iFirstFace, nFaces;        // Index and count into faces

	friend FArchive& operator<<(FArchive& Ar, BSPMODEL& m)
	{
		Ar.Serialize(m.nMins, sizeof(BSPMODEL::nMins));
		Ar.Serialize(m.nMins, sizeof(BSPMODEL::nMaxs));
		Ar << m.vOrigin;
		Ar.Serialize(m.iHeadnodes, sizeof(BSPMODEL::iHeadnodes));
		Ar << m.nVisLeafs << m.iFirstFace << m.nFaces;
		return Ar;
	}
};
constexpr INT BSPMODEL_size = sizeof(FLOAT) * 9 + sizeof(INT) * (MAX_MAP_HULLS + 3);

struct BSPTEXTUREINFO
{
	FVector vS;
	FLOAT fSShift;    // Texture shift in s direction
	FVector vT;
	FLOAT fTShift;    // Texture shift in t direction
	INT iMiptex; // Index into textures array
	INT nFlags;  // Texture flags, seem to always be 0

	friend FArchive& operator<<(FArchive& Ar, BSPTEXTUREINFO& t)
	{
		return Ar << t.vS << t.fSShift << t.vT << t.fTShift << t.iMiptex << t.nFlags;
	}
};
constexpr INT BSPTEXTUREINFO_size = sizeof(FLOAT) * (3*2+2) + sizeof(INT) * 2;

struct FEntListData
{
	struct FEntity
	{
		TMap<FName, FString> KeyMap;
	};
	TArray<FEntity*> EntList;

	void Clear();
	~FEntListData()
	{
		Clear();
	}
	void LoadFrom(const TCHAR* String);

	FEntity* FindEntity(const TCHAR* EntName) const;
	void DumpEntities() const;
};

constexpr FLOAT ToUnrScale = 1.1f;
inline FVector ToUnrealScale(const FVector& V)
{
	return FVector(V.X * ToUnrScale, V.Y * -ToUnrScale, V.Z * ToUnrScale);
}
inline FVector ToUnrealUVMap(const FVector& V)
{
	return FVector(V.X / ToUnrScale, V.Y / -ToUnrScale, V.Z / ToUnrScale);
}
inline FVector GetInvVector(const FVector& V)
{
	return FVector(1.f / V.X, 1.f / V.Y, 1.f / V.Z);
}

class HLTitleManager
{
public:
	struct FTitleInfo
	{
		FLOAT XPos, YPos;
		FColor Color, HighlightColor;
		FLOAT FadeIn, FadeOut, DisplayTime, HighlightFadeTime;
		INT MsgType;
		FString Text;

		FTitleInfo(const FString& InStr)
			: Text(InStr)
		{}
	};

private:
	FString TitleFileURL;
	TMap<FString, FTitleInfo*> TitleMap;
	UBOOL bInit;

	void InitTitles();

public:
	HLTitleManager(const TCHAR* MapURL);
	~HLTitleManager();

	FTitleInfo* FindTitle(const TCHAR* Name);

	static UBOOL TextStartsWith(const TCHAR*& Str, const TCHAR* Match);
	static FLOAT ParseNextFloat(const TCHAR*& Str, const FLOAT Default);
	static INT ParseNextInt(const TCHAR*& Str, const INT Default);
};

class CMapLoadHelper
{
	friend struct CModelBuilder;
	friend struct CModel;

	struct FMapTex
	{
		FString Name;
		FLOAT X, Y, OrgX, OrgY;
		FLOAT ScaleX, ScaleY;

		FMapTex(const TCHAR* InName, INT inX, INT inY);
	};
	FArchive& Loader;
	FString MapFile;
	BSPHEADER bspHeader;
	//dheader_t s_MapHeader;

	TMap<FString, INT> ExportCount;
	INT iModelIndex;

	FEntListData EntData;
	TArray<dplane_t> map_planes;
	TArray<FMapTex> map_tex;
	TArray<FVector> map_verts;
	TArray<BSPFACE> map_surfs;
	TArray<BSPEDGE> map_edges;
	TArray<INT> map_surfedges;
	TArray<BSPMODEL> map_models;
	TArray<BSPTEXTUREINFO> map_texinfo;

	TMap<FName, FColor> TexLightMap;

	void LoadEntities();
	void LoadPlanes();
	void LoadTextures();
	void LoadVertices();
	void LoadSurfs();
	void LoadEdges();
	void LoadSurfEdges();
	void LoadModels();
	void LoadTexInfos();

	void ExportEntity(const TCHAR* EntName, FOutputDevice& Out);

public:
	HLTitleManager Titles;

	CMapLoadHelper(FArchive& InAr, const TCHAR* FileURL)
		: Loader(InAr), MapFile(FString::GetFilenameOnlyStr(FileURL)), iModelIndex(2), Titles(FileURL)
	{}
	UBOOL Load();
	void ExportMap(FOutputDevice& Out);

	void WriteModel(const BSPMODEL& Model, FOutputDevice& Out, const UBOOL bWorld, const UBOOL bAddTextures, const UBOOL bCenterModel, const UBOOL bIsMover);

	inline const TCHAR* GetMapName() const
	{
		return *MapFile;
	}
	inline BSPMODEL* GetModel(INT Index)
	{
		return map_models.IsValidIndex(Index) ? &map_models(Index) : nullptr;
	}
};
