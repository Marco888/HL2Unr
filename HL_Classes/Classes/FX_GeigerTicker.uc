class FX_GeigerTicker extends Info
	transient;

var PlayerPawn LocalPlayer;
var float RadiationLevel;

function PostBeginPlay()
{
	LocalPlayer = PlayerPawn(Owner);
	if( LocalPlayer==None )
		Error("No Player?");
}
static final function FX_GeigerTicker GetTicker()
{
	local PlayerPawn P;
	local FX_GeigerTicker T;
	
	P = GetLocalPlayerPawn();
	if( P==None )
		return None;
	foreach P.ChildActors(class'FX_GeigerTicker',T)
		return T;
	return P.Spawn(class'FX_GeigerTicker',P);
}

final function SetRadiationLevel( float NewLevel )
{
	RadiationLevel = FMax(RadiationLevel, NewLevel);
	if( RadiationLevel>0.f )
		Enable('Tick');
}

simulated function Tick( float Delta )
{
	RadiationLevel-=Delta;
	if( RadiationLevel<=0.f )
	{
		RadiationLevel = 0.f;
		Disable('Tick');
		return;
	}
}

defaultproperties
{
	RemoteRole=ROLE_None
}