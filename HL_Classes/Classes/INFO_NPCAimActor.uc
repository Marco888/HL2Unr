class INFO_NPCAimActor extends Info
	NoUserCreate;

var HL_Human NPCOwner;
var vector AimTarget;
var bool bFinishAim;

function PostBeginPlay()
{
	NPCOwner = HL_Human(Owner);
	if( NPCOwner==None )
		Error("Invalid owner");
}
function Tick( float Delta )
{
	if( !NPCOwner || NPCOwner.bDeleteMe || NPCOwner.Health<=0 )
		Destroy();
	else if( bFinishAim )
	{
		if( NPCOwner.AimAlpha>0.f && (Level.TimeSeconds-NPCOwner.LastRenderedTime)<1.f )
		{
			NPCOwner.AimAlpha = FMax(NPCOwner.AimAlpha - Delta*3.f,0.f);
			NPCOwner.TickAimer(AimTarget);
		}
		else
		{
			NPCOwner.EndAiming();
			Destroy();
		}
	}
	else if( NPCOwner.NetAttackTarget )
	{
		AimTarget = NPCOwner.NetAttackTarget.Location;
		if( NPCOwner.AimAlpha<1.f )
			NPCOwner.AimAlpha = FMin(NPCOwner.AimAlpha + Delta*3.f,1.f);
		if( (Level.TimeSeconds-NPCOwner.LastRenderedTime)<1.f )
			NPCOwner.TickAimer(AimTarget);
	}
}
function Destroyed()
{
	if( NPCOwner && NPCOwner.NPCTicker==Self )
		NPCOwner.NPCTicker = None;
}

defaultproperties
{
	RemoteRole=ROLE_None
}