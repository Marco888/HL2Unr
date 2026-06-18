
#include "HLExporter.h"

// Bitmap compression types.
enum EBitmapCompression
{
	BCBI_RGB = 0,
	BCBI_RLE8 = 1,
	BCBI_RLE4 = 2,
	BCBI_BITFIELDS = 3,
	BCBI_JPEG = 4,
	BCBI_PNG = 5,
};

// .BMP subheader.
struct FBitmapInfoHeader
{
	DWORD biSize GCC_PACK(1);
	DWORD biWidth GCC_PACK(1);
	DWORD biHeight GCC_PACK(1);
	_WORD biPlanes GCC_PACK(1);
	_WORD biBitCount GCC_PACK(1);
	DWORD biCompression GCC_PACK(1);
	DWORD biSizeImage GCC_PACK(1);
	DWORD biXPelsPerMeter GCC_PACK(1);
	DWORD biYPelsPerMeter GCC_PACK(1);
	DWORD biClrUsed GCC_PACK(1);
	DWORD biClrImportant GCC_PACK(1);
};

// XYZ coordinates of an endpoint.
struct _CIEXYZ
{
	INT ciexyzX;
	INT ciexyzY;
	INT ciexyzZ;
};

// XYZ coordinates for red, green and blue endpoints.
struct _CIEXYZTRIPLE
{
	_CIEXYZ ciexyzRed;
	_CIEXYZ ciexyzGreen;
	_CIEXYZ ciexyzBlue;
};

// .BMP v5 subheader.
struct FBitmapV5InfoHeader
{
	DWORD        bV5Size GCC_PACK(1);
	INT          bV5Width GCC_PACK(1);
	INT          bV5Height GCC_PACK(1);
	_WORD        bV5Planes GCC_PACK(1);
	_WORD        bV5BitCount GCC_PACK(1);
	DWORD        bV5Compression GCC_PACK(1);
	DWORD        bV5SizeImage GCC_PACK(1);
	INT          bV5XPelsPerMeter GCC_PACK(1);
	INT          bV5YPelsPerMeter GCC_PACK(1);
	DWORD        bV5ClrUsed GCC_PACK(1);
	DWORD        bV5ClrImportant GCC_PACK(1);
	DWORD        bV5RedMask GCC_PACK(1);
	DWORD        bV5GreenMask GCC_PACK(1);
	DWORD        bV5BlueMask GCC_PACK(1);
	DWORD        bV5AlphaMask GCC_PACK(1);
	DWORD        bV5CSType GCC_PACK(1);
	_CIEXYZTRIPLE bV5Endpoints GCC_PACK(1); // This member is ignored unless the bV5CSType member specifies LCS_CALIBRATED_RGB.
	DWORD        bV5GammaRed GCC_PACK(1);
	DWORD        bV5GammaGreen GCC_PACK(1);
	DWORD        bV5GammaBlue GCC_PACK(1);
	DWORD        bV5Intent GCC_PACK(1);
	DWORD        bV5ProfileData GCC_PACK(1);
	DWORD        bV5ProfileSize GCC_PACK(1);
	DWORD        bV5Reserved GCC_PACK(1);
};

// .pcx file header.
#if defined(_MSC_VER) || defined(HAVE_PRAGMA_PACK)
#pragma pack(push,1)
#endif
class FPCXFileHeader
{
public:
	BYTE	Manufacturer GCC_PACK(1);		// Always 10.
	BYTE	Version GCC_PACK(1);			// PCX file version.
	BYTE	Encoding GCC_PACK(1);			// 1=run-length, 0=none.
	BYTE	BitsPerPixel GCC_PACK(1);		// 1,2,4, or 8.
	_WORD	XMin GCC_PACK(1);				// Dimensions of the image.
	_WORD	YMin GCC_PACK(1);				// Dimensions of the image.
	_WORD	XMax GCC_PACK(1);				// Dimensions of the image.
	_WORD	YMax GCC_PACK(1);				// Dimensions of the image.
	_WORD	XDotsPerInch GCC_PACK(1);		// Horizontal printer resolution.
	_WORD	YDotsPerInch GCC_PACK(1);		// Vertical printer resolution.
	BYTE	OldColorMap[48] GCC_PACK(1);	// Old colormap info data.
	BYTE	Reserved1 GCC_PACK(1);			// Must be 0.
	BYTE	NumPlanes GCC_PACK(1);			// Number of color planes (1, 3, 4, etc).
	_WORD	BytesPerLine GCC_PACK(1);		// Number of bytes per scanline.
	_WORD	PaletteType GCC_PACK(1);		// How to interpret palette: 1=color, 2=gray.
	_WORD	HScreenSize GCC_PACK(1);		// Horizontal monitor size.
	_WORD	VScreenSize GCC_PACK(1);		// Vertical monitor size.
	BYTE	Reserved2[54] GCC_PACK(1);		// Must be 0.
	friend FArchive& operator<<(FArchive& Ar, FPCXFileHeader& H)
	{
		guard(FPCXFileHeader << );
		Ar << H.Manufacturer << H.Version << H.Encoding << H.BitsPerPixel;
		Ar << H.XMin << H.YMin << H.XMax << H.YMax << H.XDotsPerInch << H.YDotsPerInch;
		INT i;
		for (i = 0; i < ARRAY_COUNT(H.OldColorMap); i++)
			Ar << H.OldColorMap[i];
		Ar << H.Reserved1 << H.NumPlanes;
		Ar << H.BytesPerLine << H.PaletteType << H.HScreenSize << H.VScreenSize;
		for (i = 0; i < ARRAY_COUNT(H.Reserved2); i++)
			Ar << H.Reserved2[i];
		return Ar;
		unguard;
	}
};
#if defined(_MSC_VER) || defined(HAVE_PRAGMA_PACK)
#pragma pack(pop)
#endif

constexpr _WORD BMPHeader = 'B' | ('M' << 8);

UBOOL UHLTexImporter::LoadTexture(FMipmap& OutMip, TArray<FColor>& OutPalette, UBOOL bIsMasked, const TCHAR* FileName)
{
	guard(UHLTexImporter::LoadTexture);
	TArray<BYTE> Buffer;
	if (!appLoadFileToArray(Buffer, FileName))
	{
		GWarn->Logf(TEXT("Failed to open file: %ls"), FileName);
		return FALSE;
	}

	const BITMAPFILEHEADER& bmf = *reinterpret_cast<BITMAPFILEHEADER*>(&Buffer(0));
	if (bmf.bfType != BMPHeader)
	{
		GWarn->Logf(TEXT("BMP file has invalid header: %ls"), FileName);
		return FALSE;
	}

	const FBitmapV5InfoHeader& bmV5hdr = *reinterpret_cast<FBitmapV5InfoHeader*>(&Buffer(sizeof(BITMAPFILEHEADER)));

	// This is a .bmp type data stream.
	if (bmV5hdr.bV5Compression && bmV5hdr.bV5Compression != BCBI_RGB && bmV5hdr.bV5Compression != BCBI_BITFIELDS)
	{
		GWarn->Logf(TEXT("Rejecting RLE compression of BMP 'Name: %ls' image with compression %i"), *FileName, bmV5hdr.bV5Compression);
		return FALSE;
	}

	if (bmV5hdr.bV5Planes != 1 || bmV5hdr.bV5BitCount != 8)
	{
		GWarn->Logf(TEXT("BMP uses an unsupported format (planes %i/bitcount %i)"), bmV5hdr.bV5Planes, bmV5hdr.bV5BitCount);
		return FALSE;
	}

	const INT ResX = bmV5hdr.bV5Width;
	const INT ResY = bmV5hdr.bV5Height;

	// Do palette.
	const BYTE* bmpal = &Buffer(sizeof(BITMAPFILEHEADER) + sizeof(FBitmapInfoHeader));
	OutPalette.Empty(NUM_PAL_COLORS);
	for (INT i = 0; i < Min<INT>(NUM_PAL_COLORS, bmV5hdr.bV5ClrUsed ? bmV5hdr.bV5ClrUsed : NUM_PAL_COLORS); i++)
		new(OutPalette)FColor(bmpal[i * 4 + 2], bmpal[i * 4 + 1], bmpal[i * 4 + 0], 255);
	while (OutPalette.Num() < NUM_PAL_COLORS)
		new(OutPalette)FColor(0, 0, 0, 255);

	OutMip.USize = ResX;
	OutMip.VSize = ResY;
	OutMip.DataArray.SetSize(ResX * ResY);
	BYTE* Dest = &OutMip.DataArray(0);
	INT DestSize = OutMip.DataArray.Num();

	// Copy upside-down scanlines.
	for (INT y = 0; y < ResY; y++)
	{
		appMemcpy
		(
			Dest + ((ResY - 1 - y) * ResX),
			&Buffer(bmf.bfOffBits + y * Align(ResX, 4)),
			ResX
		);
	}

	if (bIsMasked)
	{
		OutPalette(NUM_PAL_COLORS - 1) = OutPalette(0);
		OutPalette(0) = FColor(0, 0, 0, 0);
		for (INT i = 0; i < DestSize; ++i)
		{
			if (Dest[i] == 0)
				Dest[i] = 255;
			else if (Dest[i] == 255)
				Dest[i] = 0;
		}
	}
	return TRUE;
	unguard;
}

inline INT FPrevPowerOfTwo(INT N)
{
	for (INT i = 1; i < 16; ++i)
	{
		if (N & (1 << i))
			return 1 << (i - 1);
	}
	return N;
}

#define GET_PIX(index) PalPlanes[SrcBits[index]]

void UHLTexImporter::ConvertToPowTwo(FMipmap& Mip, TArray<FColor>& Palette, UBOOL bIsMasked, UBOOL bUseClosest)
{
	guard(UHLTexImporter::ConvertToPowTwo);
	const INT ResX = Mip.USize;
	const INT ResY = Mip.VSize;
	INT TexResX = FNextPowerOfTwo(ResX);
	INT TexResY = FNextPowerOfTwo(ResY);

	// Check if already correct aspect ratio.
	if ((ResX == TexResX) && (ResY == TexResY))
		return;

	if (bUseClosest || 1)
	{
		if (ResX != TexResX && ResX > 30)
		{
			FLOAT diff = FLOAT(ResX) / FLOAT(TexResX);
			if (diff < 0.5f)
			{
				debugf(TEXT("DOWNSCALE %i -> %i"), TexResX, FPrevPowerOfTwo(TexResX));
				TexResX = FPrevPowerOfTwo(TexResX);
			}
		}
		if (ResY != TexResY && ResY > 30)
		{
			FLOAT diff = FLOAT(ResY) / FLOAT(TexResY);
			if (diff < 0.5f)
			{
				debugf(TEXT("DOWNSCALE %i -> %i"), TexResY, FPrevPowerOfTwo(TexResY));
				TexResY = FPrevPowerOfTwo(TexResY);
			}
		}
	}

	FScopedMemMark Mark(GMem);
	debugf(TEXT(" Scale image: %i->%i x %i->%i"), ResX, TexResX, ResY, TexResY);

	// Setup new bit arrays.
	const INT SrcBitSize = Mip.DataArray.Num();
	Mip.USize = TexResX;
	Mip.VSize = TexResY;
	BYTE* SrcBits = New<BYTE>(GMem, SrcBitSize);
	appMemcpy(SrcBits, Mip.DataArray.GetData(), SrcBitSize);
	Mip.DataArray.SetSize(TexResX * TexResY);
	BYTE* DestBits = &Mip.DataArray(0);

	// Vectorize palette.
	FPlane* PalPlanes = New<FPlane>(GMem, NUM_PAL_COLORS);
	guard(GetPalette);
	for (INT i = 0; i < NUM_PAL_COLORS; ++i)
		PalPlanes[i] = FPlane(Palette(i).Vector(), 1.f);
	if (bIsMasked)
		PalPlanes[0] = FPlane(0.f, 0.f, 0.f, 0.f);
	unguard;

	guard(MatchPixels);
	FPlane NColor;
	DOUBLE remainx, remainy;
	DOUBLE fx = 0.0, fy = 0.0;
	const INT xLimit = ResX - 1;
	const INT yLimit = ResY - 1;
	const DOUBLE deltaX = DOUBLE(ResX - 1) / DOUBLE(TexResX - 1);
	const DOUBLE deltaY = DOUBLE(ResY - 1) / DOUBLE(TexResY - 1);
	INT i, x, y;

	for (y = 0; y < TexResY; ++y, fy += deltaY)
	{
		INT yBase = (INT)floor(fy);
		remainy = fy - DOUBLE(yBase);
		if (yBase >= yLimit)
		{
			yBase = yLimit;
			remainy = 0.f;
		}
		yBase *= ResX;
		for (x = 0, fx = 0.0; x < TexResX; ++x, fx += deltaX, ++DestBits)
		{
			INT xBase = (INT)floor(fx);
			remainx = fx - DOUBLE(xBase);
			if (xBase >= xLimit)
			{
				xBase = xLimit;
				remainx = 0.f;
			}
			xBase += yBase;
			if (remainx < 0.01 && remainy < 0.01)
			{
				*DestBits = SrcBits[xBase];
				continue;
			}
			const FPlane& A = GET_PIX(xBase);
			if (remainy < 0.01)
			{
				const FPlane& B = GET_PIX(xBase + 1);
				NColor = (A * (1.f - remainx)) + (B * remainx);
			}
			else if (remainx < 0.01)
			{
				const FPlane& B = GET_PIX(xBase + ResX);
				NColor = (A * (1.f - remainy)) + (B * remainy);
			}
			else
			{
				const FPlane& B = GET_PIX(xBase + 1);
				const FPlane& C = GET_PIX(xBase + ResX);
				const FPlane& D = GET_PIX(xBase + ResX + 1);
				const FPlane SideX = (A * (1.0 - remainx)) + (B * remainx);
				const FPlane SideY = (C * (1.0 - remainx)) + (D * remainx);
				NColor = (SideX * (1.0 - remainy)) + (SideY * remainy);
			}
			if (bIsMasked)
			{
				if (NColor.W < 0.5f)
					*DestBits = 0;
				else
				{
					FLOAT BestScore = 0.f;
					for (i = 1; i < NUM_PAL_COLORS; ++i)
					{
						const FLOAT Score = PalPlanes[i].DistSquared(NColor);
						if (i == 1 || BestScore > Score)
						{
							BestScore = Score;
							*DestBits = i;
						}
					}
				}
			}
			else
			{
				FLOAT BestScore = 0.f;
				for (i = 0; i < NUM_PAL_COLORS; ++i)
				{
					const FLOAT Score = PalPlanes[i].DistSquared(NColor);
					if (i == 0 || BestScore > Score)
					{
						BestScore = Score;
						*DestBits = i;
					}
				}
			}
		}
	}
	unguard;
	unguard;
}
void UHLTexImporter::CenterToPowTwo(FMipmap& Mip, UBOOL bIsMasked)
{
	guard(UHLTexImporter::CenterToPowTwo);
	const INT ResX = Mip.USize;
	const INT ResY = Mip.VSize;
	const INT TexResX = FNextPowerOfTwo(ResX);
	const INT TexResY = FNextPowerOfTwo(ResY);

	// Check if already correct aspect ratio.
	if ((ResX == TexResX) && (ResY == TexResY))
		return;

	FScopedMemMark Mark(GMem);
	debugf(TEXT(" Center image: %i->%i x %i->%i"), ResX, TexResX, ResY, TexResY);

	// Setup new bit arrays.
	const INT SrcBitSize = Mip.DataArray.Num();
	Mip.USize = TexResX;
	Mip.VSize = TexResY;
	BYTE* SrcBits = New<BYTE>(GMem, SrcBitSize);
	appMemcpy(SrcBits, Mip.DataArray.GetData(), SrcBitSize);
	Mip.DataArray.SetSize(TexResX * TexResY);
	BYTE* DestBits = &Mip.DataArray(0);
	BYTE EdgeColor = bIsMasked ? 0 : SrcBits[0];
	appMemset(DestBits, (INT)EdgeColor, Mip.DataArray.Num()); // Init to mask or edge pixel color.

	// Get center point.
	const INT xStart = (TexResX - ResX) / 2;
	const INT yStart = (TexResY - ResY) / 2;

	DestBits += (yStart * TexResX) + xStart;
	for (INT y = 0; y < ResY; ++y, DestBits += TexResX, SrcBits += ResX)
		appMemcpy(DestBits, SrcBits, ResX);
	unguard;
}
UTexture* UHLTexImporter::ImportTexture(UPackage* InParent, const TCHAR* FileName)
{
	guard(UHLTexImporter::ImportTexture);
	FString ObjectName = FString::GetFilenameOnlyStr(FileName);
	const UBOOL IsMaskedTex = (appStrchr(*ObjectName, '{') != NULL);
	ObjectName = MakeSafeName(*ObjectName);
	debugf(TEXT("Import '%ls' to %ls as '%ls'"), FileName, InParent->GetPathName(), *ObjectName);

	FMipmap TempMip;
	TArray<FColor> TempPalette;
	if (!LoadTexture(TempMip, TempPalette, IsMaskedTex, FileName))
		return nullptr;

	ConvertToPowTwo(TempMip, TempPalette, IsMaskedTex);

	// Set texture properties.
	UTexture* Texture = new (InParent, *ObjectName, (RF_Public | RF_Standalone)) UTexture(TEXF_P8, TempMip.USize, TempMip.VSize);
	auto& Mip = Texture->Mips(0);
	check(Mip.DataArray.Num() == TempMip.DataArray.Num());
	appMemcpy(Mip.DataArray.GetData(), TempMip.DataArray.GetData(), TempMip.DataArray.Num());
	Mip.DataPtr = &Mip.DataArray(0);

	// Do palette.
	UPalette* Palette = new(InParent->TopOuter(), NAME_None, RF_Public)UPalette;
	Palette->Colors.ExchangeArray(&TempPalette);
	check(Palette->Colors.Num() == NUM_PAL_COLORS);

	if (IsMaskedTex)
		Texture->PolyFlags |= PF_Masked;

	Texture->Palette = Palette;
	Texture->CreateMips(TRUE, TRUE);
	Texture->Palette = Palette->ReplaceWithExisting();
	return Texture;
	unguard;
}
UBOOL UHLTexImporter::ExportPCX(const FMipmap& Mip, const FColor* Palette, const TCHAR* FileName)
{
	guard(UHLTexImporter::ExportPCX);
	FArchive* Ar = GFileManager->CreateFileWriter(FileName);
	if (!Ar)
	{
		warnf(TEXT("Couldn't create file writer for: %ls"), FileName);
		return FALSE;
	}

	// Set all PCX file header properties.
	FPCXFileHeader PCX;
	appMemzero(&PCX, sizeof(PCX));
	PCX.Manufacturer = 10;
	PCX.Version = 05;
	PCX.Encoding = 1;
	PCX.BitsPerPixel = 8;
	PCX.XMin = 0;
	PCX.YMin = 0;
	PCX.XMax = Mip.USize - 1;
	PCX.YMax = Mip.VSize - 1;
	PCX.XDotsPerInch = 72;
	PCX.YDotsPerInch = 72;
	PCX.BytesPerLine = Mip.USize;
	PCX.PaletteType = 0;
	PCX.HScreenSize = 0;
	PCX.VScreenSize = 0;

	// Copy all RLE bytes.
	BYTE RleCode = 0xc1;

	debugf(TEXT(" - Exporting P8 as 8bpp pcx (%ls)"), FileName);
	PCX.NumPlanes = 1;
	*Ar << PCX;
	const BYTE* ScreenPtr = &Mip.DataArray(0);
	INT i;
	for (i = 0; i < Mip.USize * Mip.VSize; i++)
	{
		if ((*ScreenPtr & 0xc0) == 0xc0)
			*Ar << RleCode;
		BYTE pix = *ScreenPtr++;
		*Ar << pix;
	}

	// Write PCX trailer then palette.
	BYTE Extra = 12;
	*Ar << Extra;
	for (i = 0; i < NUM_PAL_COLORS; i++)
	{
		BYTE R = Palette[i].R, G = Palette[i].G, B = Palette[i].B;
		*Ar << R << G << B;
	}
	delete Ar;
	return TRUE;
	unguard;
}
const TCHAR* UHLTexImporter::MakeSafeName(const TCHAR* inName)
{
	guard(UHLTexImporter::MakeSafeName);
	TCHAR* Result = appStaticString1024();
	TCHAR* S = Result;
	while (*inName)
	{
		if (*inName == '!')
		{
			*S++ = 'e';
			*S++ = 'x';
			*S++ = '_';
		}
		else if (*inName == '+')
		{
			*S++ = 'p';
			*S++ = 's';
			*S++ = '_';
		}
		else if (*inName == '-')
		{
			*S++ = 'n';
			*S++ = 'q';
			*S++ = '_';
		}
		else if (*inName == '~')
		{
			*S++ = '_';
			*S++ = 's';
			*S++ = '_';
		}
		else if (*inName == '{')
		{
			*S++ = 'l';
			*S++ = 'b';
			*S++ = '_';
		}
		else if (*inName == '}')
		{
			*S++ = 'r';
			*S++ = 'b';
			*S++ = '_';
		}
		else if (*inName == '#')
		{
			*S++ = '_';
			*S++ = 'h';
			*S++ = 't';
			*S++ = '_';
		}
		else *S++ = *inName;
		++inName;
	}
	*S = 0;
	return Result;
	unguard;
}
