class HL_InitialItems extends Info;

var() bool bStartWithHEV;
var() array<class<Weapon> > Weapons;
var() class<Weapon> DefaultWeapon;

function PostBeginPlay()
{
	local INFO_HLGameRules G;
	
	Level.Game.DefaultWeapon = DefaultWeapon;
	G = class'INFO_HLGameRules'.Static.GetHLRules(Level.Game);
	G.SpawnWeapons = Self;
}
function GiveWeapons( Pawn Other )
{
	local class<Weapon> WC;
	local Weapon W;
	local INV_HevSuit S;
	
	foreach Weapons(WC)
	{
		if( !WC || Other.FindInventoryType(WC) )
			continue;
		W = Other.Spawn(WC);
		if( !W )
			continue;
		W.RespawnTime = 0;
		W.PickupSound = None;
		W.PickupMessage = "";
		W.Touch(Other);
		W.PickupSound = W.Default.PickupSound;
		W.PickupMessage = W.Default.PickupMessage;
		if( !W.bCarriedItem )
			W.Destroy();
	}
	if( bStartWithHEV && Other.FindInventoryType(Class'INV_HevSuit')==None )
	{
		S = Spawn(class'INV_HevSuit',,,Other.Location);
		S.RespawnTime = 0.0;
		S.bHeldItem = true;
		S.GiveTo(Other);
	}
}

defaultproperties
{
	RemoteRole=ROLE_None
	bStartWithHEV=true
	DefaultWeapon=class'DispersionPistol'
	Weapons(0)=class'Automag'
}