class HL_TriggerPush extends Volume;

var() bool bPushOnceOnly, bInitiallyActive;
var() float PushSpeed;
var() rotator PushDirection;
var vector PushVelocity;

simulated function PostBeginPlay()
{
	if( Level.NetMode==NM_Client )
		SetCollision(false);
	else
	{
		PushVelocity = vector(PushDirection) * PushSpeed;
		if( !bInitiallyActive )
			SetCollision(false);
	}
}
function Touch( Actor Other )
{
	if( !Other.bIsPawn )
		return;
	if( bPushOnceOnly )
	{
		SetPendingTouch(Other);
		SetCollision(false);
	}
	else Enable('Tick');
}
function PostTouch( actor Other )
{
	if( Other.bIsPawn )
		Pawn(Other).AddVelocity(PushVelocity);
	else Other.Velocity+=PushVelocity;
}
function Trigger( Actor Other, Pawn EventInstigator )
{
	SetCollision(!bCollideActors);
	if( bCollideActors )
		CheckEncroachments();
	else Disable('Tick');
}
function Tick( float Delta )
{
	local Pawn P;
	local bool bFound;
	
	foreach TouchingActors(class'Pawn',P)
	{
		if( P.Health<=0 )
			continue;
		P.AddVelocity(PushVelocity * Delta);
		bFound = true;
	}
	if( !bFound )
		Disable('Tick');
}

defaultproperties
{
	PushSpeed=110
	RemoteRole=ROLE_None
	bInitiallyActive=true
	bStatic=false
	bNoDelete=true
}