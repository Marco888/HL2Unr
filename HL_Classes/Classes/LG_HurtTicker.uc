class LG_HurtTicker extends Info
	NoUserCreate;

var Pawn PawnOwner;
var HL_TriggerHurt ActiveVolume, PassiveVolume;
var byte HurtCounter;

function PostBeginPlay()
{
	PawnOwner = Pawn(Owner);
	if( PawnOwner==None )
		Error("No Owner?");
	SetTimer(0.5,true);
}

function OwnerChanged()
{
	if( Owner==None )
		Destroy();
}

final function SetVolume( HL_TriggerHurt V )
{
	if( ActiveVolume==None && ActiveVolume==None )
	{
		ActiveVolume = V;
		Timer();
	}
	else
	{
		PassiveVolume = None;
		ActiveVolume = V;
	}
}
final function bool StillTouchingVolume()
{
	local HL_TriggerHurt V;
	
	foreach PawnOwner.TouchingActors(class'HL_TriggerHurt',V)
		if( V==ActiveVolume )
			return true;
	return false;
}

function Timer()
{
	if( PawnOwner==None || PawnOwner.Health<=0 )
	{
		Destroy();
		return;
	}
	if( ActiveVolume )
	{
		SetTimer(ActiveVolume.ReDamageDelay, true);
		if( !StillTouchingVolume() )
		{
			if( PawnOwner.bIsPlayer )
			{
				HurtCounter = Default.HurtCounter;
				ActiveVolume.DealDamage(PawnOwner);
				PassiveVolume = ActiveVolume;
				ActiveVolume = None;
			}
			else
			{
				Destroy();
				return;
			}
		}
		else ActiveVolume.DealDamage(PawnOwner);
	}
	else
	{
		--HurtCounter;
		if( PassiveVolume==None || HurtCounter==0 )
		{
			Destroy();
			return;
		}
		PassiveVolume.DealDamage(PawnOwner);
	}
}

static final function SetHurtVolume( Pawn P, HL_TriggerHurt V )
{
	local LG_HurtTicker H;
	
	foreach P.ChildActors(class'LG_HurtTicker',H)
		break;
	if( !H )
		H = P.Spawn(class'LG_HurtTicker',P);
	H.ActiveVolume = V;
}

defaultproperties
{
	RemoteRole=ROLE_None
	HurtCounter=4
}