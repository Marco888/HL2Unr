
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

constexpr _WORD BMPHeader = 'B' | ('M' << 8);

UTexture* UHLTexImporter::ImportTexture(UPackage* InParent, const TCHAR* FileName)
{
	guard(UHLTexImporter::ImportTexture);
	FString ObjectName = FString::GetFilenameOnlyStr(FileName);
	const UBOOL IsMaskedTex = (**ObjectName == '{');
	ObjectName = MakeSafeName(*ObjectName);
	debugf(TEXT("Import '%ls' to %ls as '%ls'"), FileName, InParent->GetPathName(), *ObjectName);

	TArray<BYTE> Buffer;
	if (!appLoadFileToArray(Buffer, FileName))
	{
		GWarn->Logf(TEXT("Failed to open file: %ls"), FileName);
		return nullptr;
	}

	const BITMAPFILEHEADER& bmf = *reinterpret_cast<BITMAPFILEHEADER*>(&Buffer(0));
	if (bmf.bfType != BMPHeader)
	{
		GWarn->Logf(TEXT("BMP file has invalid header: %ls"), FileName);
		return nullptr;
	}

	//const FBitmapInfoHeader& bmhdr = *reinterpret_cast<FBitmapInfoHeader*>(&Buffer(sizeof(FBitmapFileHeader)));
	const FBitmapV5InfoHeader& bmV5hdr = *reinterpret_cast<FBitmapV5InfoHeader*>(&Buffer(sizeof(BITMAPFILEHEADER)));

	// This is a .bmp type data stream.
	if (bmV5hdr.bV5Compression && bmV5hdr.bV5Compression != BCBI_RGB && bmV5hdr.bV5Compression != BCBI_BITFIELDS)
	{
		GWarn->Logf(TEXT("Rejecting RLE compression of BMP 'Name: %ls' image with compression %i"), *FileName, bmV5hdr.bV5Compression);
		return nullptr;
	}
	if (bmV5hdr.bV5Planes == 1 && bmV5hdr.bV5BitCount == 8)
	{
		const INT ResX = bmV5hdr.bV5Width;
		const INT ResY = bmV5hdr.bV5Height;
		INT TexResX = FNextPowerOfTwo(ResX);
		INT TexResY = FNextPowerOfTwo(ResY);

		// Set texture properties.
		UTexture* Texture = new (InParent, *ObjectName, (RF_Public | RF_Standalone)) UTexture(TEXF_P8, TexResX, TexResY);

		// Do palette.
		UPalette* Palette = new(InParent->TopOuter(), NAME_None, RF_Public)UPalette;
		const BYTE* bmpal = &Buffer(sizeof(BITMAPFILEHEADER) + sizeof(FBitmapInfoHeader));
		Palette->Colors.Empty(NUM_PAL_COLORS);
		for (INT i = 0; i < Min<INT>(NUM_PAL_COLORS, bmV5hdr.bV5ClrUsed ? bmV5hdr.bV5ClrUsed : NUM_PAL_COLORS); i++)
			new(Palette->Colors)FColor(bmpal[i * 4 + 2], bmpal[i * 4 + 1], bmpal[i * 4 + 0], 255);
		while (Palette->Colors.Num() < NUM_PAL_COLORS)
			new(Palette->Colors)FColor(0, 0, 0, 255);

		BYTE* Dest = &Texture->Mips(0).DataArray(0);
		INT DestSize = Texture->Mips(0).DataArray.Num();
		TArray<BYTE> ReadBits;
		if ((ResX != TexResX) || (ResY != TexResY))
		{
			ReadBits.SetSize(ResX * ResY);
			Dest = &ReadBits(0);
			DestSize = ReadBits.Num();
		}
		const INT NumPix = TexResX * TexResY;

		// Copy upside-down scanlines.
		for (INT y = 0; y < (INT)bmV5hdr.bV5Height; y++)
		{
			appMemcpy
			(
				Dest + ((bmV5hdr.bV5Height - 1 - y) * bmV5hdr.bV5Width),
				&Buffer(bmf.bfOffBits + y * Align(bmV5hdr.bV5Width, 4)),
				bmV5hdr.bV5Width
			);
		}

		if (IsMaskedTex)
		{
			Texture->PolyFlags |= PF_Masked;
			Palette->Colors(NUM_PAL_COLORS - 1) = Palette->Colors(0);
			Palette->Colors(0) = FColor(0, 0, 0, 0);
			for (INT i = 0; i < DestSize; ++i)
			{
				if (Dest[i] == 0)
					Dest[i] = 255;
				else if (Dest[i] == 255)
					Dest[i] = 0;
			}
		}

		if ((ResX != TexResX) || (ResY != TexResY))
		{
			debugf(TEXT(" Upscale image: %i->%i x %i->%i"), ResX, TexResX, ResY, TexResY);
			// Must resize image first.

			const FLOAT xs = FLOAT(ResX) / FLOAT(TexResX);
			const FLOAT ys = FLOAT(ResY) / FLOAT(TexResY);
			INT i, x, y;
			FPlane* PalPlanes = new FPlane[NUM_PAL_COLORS];
			guard(GetPalette);
			for (i = 0; i < NUM_PAL_COLORS; ++i)
				PalPlanes[i] = FPlane(Palette->Colors(i).Vector(), 1.f);
			if (IsMaskedTex)
				PalPlanes[0].W = 0.f;
			unguard;

			FPlane* CPlanes = new FPlane[ReadBits.Num()];
			const BYTE* OPix = &ReadBits(0);
			guard(GetPixels);
			for (i = 0; i < ReadBits.Num(); ++i)
				CPlanes[i] = PalPlanes[OPix[i]];
			unguard;

			BYTE* wPix = &Texture->Mips(0).DataArray(0);
			FPlane NColor;
			FLOAT remainx, remainy;
			FLOAT fx = 0.f, fy = 0.f;
			const INT xLimit = ResX - 1;
			const INT yLimit = ResY - 1;
			guard(BestPixels);
			for (y = 0; y < TexResY; ++y, fy += ys)
			{
				INT yBase = appFloor(fy);
				remainy = fy - FLOAT(yBase);
				if (yBase >= yLimit)
				{
					yBase = yLimit;
					remainy = 0.f;
				}
				yBase *= ResX;
				const INT nYBase = y * TexResX;
				for (x = 0, fx = 0.f; x < TexResX; ++x, fx += xs)
				{
					INT xBase = appFloor(fx);
					remainx = fx - FLOAT(xBase);
					if (xBase >= xLimit)
					{
						xBase = xLimit;
						remainx = 0.f;
					}
					if (remainx < 0.01f && remainy < 0.01f)
					{
						wPix[x + nYBase] = OPix[xBase + yBase];
						continue;
					}
					const FPlane& A = CPlanes[xBase + yBase];
					if (remainy < 0.01f)
					{
						const FPlane& B = CPlanes[xBase + yBase + 1];
						NColor = (A * (1.f - remainx)) + (B * remainx);
					}
					else if (remainx < 0.01f)
					{
						const FPlane& B = CPlanes[xBase + yBase + ResX];
						NColor = (A * (1.f - remainy)) + (B * remainy);
					}
					else
					{
						const FPlane& B = CPlanes[xBase + yBase + 1];
						const FPlane& C = CPlanes[xBase + yBase + ResX];
						const FPlane& D = CPlanes[xBase + yBase + ResX + 1];
						const FPlane SideX = (A * (1.f - remainx)) + (B * remainx);
						const FPlane SideY = (C * (1.f - remainx)) + (D * remainx);
						NColor = (SideX * (1.f - remainy)) + (SideY * remainy);
					}
					if (IsMaskedTex)
					{
						if (NColor.W < 0.5f)
							wPix[x + nYBase] = 0;
						else
						{
							FLOAT BestScore = 0.f;
							for (i = 1; i < NUM_PAL_COLORS; ++i)
							{
								const FLOAT Score = PalPlanes[i].DistSquared(NColor);
								if (i == 1 || BestScore > Score)
								{
									BestScore = Score;
									wPix[x + nYBase] = i;
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
								wPix[x + nYBase] = i;
							}
						}
					}
				}
			}
			unguard;
			delete[] PalPlanes;
			delete[] CPlanes;
		}

		Texture->Palette = Palette;
		Texture->CreateMips(TRUE, TRUE);
		Texture->Palette = Palette->ReplaceWithExisting();
		return Texture;
	}
	else
	{
		GWarn->Logf(TEXT("BMP uses an unsupported format (planes %i/bitcount %i)"), bmV5hdr.bV5Planes, bmV5hdr.bV5BitCount);
	}
	return nullptr;
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
		else *S++ = *inName;
		++inName;
	}
	*S = 0;
	return Result;
	unguard;
}
