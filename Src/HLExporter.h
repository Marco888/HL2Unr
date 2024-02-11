#pragma once

#include "Engine.h"
#include "HLBSP.h"

constexpr INT DefaultLightRadius = 36;

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

	UTexture* ImportTexture(UPackage* InParent, const TCHAR* FileName);

	static const TCHAR* MakeSafeName(const TCHAR* inName);
};
