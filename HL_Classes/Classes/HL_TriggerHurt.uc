class HL_TriggerHurt extends Volume;

var() name DamageType;
var() int Damage;
var() float ReDamageDelay; // Delay seconds between damage ticks.

var() bool bRadiationDamage; // Will cause geiger counter to tick when close to this.
var() bool bPlayerOnly; // Only damage player.
var() bool bMonsterOnly; // Only damage NPC's.
var() bool bInitiallyActive;
var() bool bDamageOnceOnly; // Only deal damage once only.

var transient FX_GeigerTicker GeigerCounter;
var vector AABBCenter;
var float AABBExtent;

simulated function PostBeginPlay()
{
	if( Level.NetMode==NM_Client )
	{
		if( bRadiationDamage )
			InitialState = 'GeigerTicker';
	}
	else
	{
		if( bInitiallyActive )
			SetCollision(true);
		if( bRadiationDamage )
		{
			bAlwaysRelevant = true;
			RemoteRole = ROLE_SimulatedProxy;
			if( Level.NetMode!=NM_DedicatedServer )
				InitialState = 'GeigerTicker';
		}
	}
}
function Trigger( Actor Other, Pawn EventInstigator )
{
	Instigator = EventInstigator;
	SetCollision(!bCollideActors);
	if( bCollideActors )
		CheckEncroachments();
}
function Touch( Actor Other )
{
	if( !Other.bIsPawn || Pawn(Other).Health<=0 || (bPlayerOnly && !Pawn(Other).bIsPlayer) || (bMonsterOnly && Pawn(Other).bIsPlayer) )
		return;
	class'LG_HurtTicker'.Static.SetHurtVolume(Pawn(Other),Self);
	
	if( bDamageOnceOnly )
		SetCollision(false);
}

function DealDamage( Pawn Other )
{
	Other.TakeDamage(Damage, Instigator, Other.Location, vect(0,0,0), DamageType);
}

simulated final function TickRadiation()
{
	local float Dist;
	local PlayerPawn P;
	
	P = GetLocalPlayerPawn();
	if( P==None || P.Health<=0 || !FastTrace(AABBCenter,P.Location) )
		return;
	Dist = VSize(AABBCenter-Location) - AABBExtent;
	if( Dist<800.f )
	{
		Dist = (800.f - FMax(Dist,0.f)) / {800.f/5.f};
		if( GeigerCounter==None )
			GeigerCounter = class'FX_GeigerTicker'.Static.GetTicker();
		GeigerCounter.SetRadiationLevel(Dist);
	}
}
simulated final function InitAABB()
{
	local BoundingBox B;

	B = GetBoundingBox();
	AABBCenter = (B.Min+B.Max) * 0.5f;
	AABBExtent = VSize(B.Max-AABBCenter);
}

simulated state GeigerTicker
{
Begin:
	Sleep(RandRange(0.1,1.0));
	InitAABB();
	while( true )
	{
		if( bCollideActors )
			TickRadiation();
		Sleep(0.25f);
	}
}

defaultproperties
{
	RemoteRole=ROLE_None
	bStatic=false
	bCollideActors=false
	Damage=10
	ReDamageDelay=0.5
	bInitiallyActive=true
}
