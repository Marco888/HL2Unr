// trigger_teleport is a brush entity available in all GoldSrc games. This entity teleports entities that touch its volume. 
class HL_TriggerTeleport extends Volume;

var HL_Triggers Master;
var Actor TeleportTarget;

var() name MasterTag; // Master trigger which is a condition if this is allowed to pass.
var() bool bAllowPlayers, bAllowMonsters;

function PostBeginPlay()
{
	if( MasterTag!='' )
	{
		foreach AllActors(class'HL_Triggers',Master,MasterTag)
			if( Master.IsMasterTrigger() )
			{
				Master.NotifyMasterChange(Self);
				break;
			}
	}
	if( Event!='' )
	{
		foreach AllActors(class'Actor',TeleportTarget,Event)
			break;
	}
	if( !TeleportTarget )
	{
		Error("No target?");
		SetCollision(false);
	}
}

function MasterCondition()
{
	local Pawn P;
	
	foreach AllActors(class'Pawn',P)
		if( Encompasses(P.Location) )
			Touch(P);
	foreach TouchingActors(class'Pawn',P)
		Touch(P);
}

// Disabled by Relay.
function KilledBy( pawn EventInstigator )
{
	SetCollision(false);
}

function Touch( Actor Other )
{
	// Only teleport monsters or clients
	if( !Other.bCanTeleport || !Other.bIsPawn || Pawn(Other).Health<=0 )
		return;
	// check type.
	if( (!bAllowPlayers && Pawn(Other).bIsPlayer) || (!bAllowMonsters && !Pawn(Other).bIsPlayer) )
		return;
	if( Master && !Master.IsTriggered(Pawn(Other)) )
		return;
	SetPendingTouch(Other);
}
function PostTouch( actor Other )
{
	local vector V;

	if( TeleportTarget==None )
		return;

	Other.bCanTeleport = false;
	Other.SetCollision(false);
	V = TeleportTarget.Location;
	V.Z+=Other.CollisionHeight;
	if( !Other.SetLocation(V,TeleportTarget.Rotation) )
		Other.SetLocation(TeleportTarget.Location,TeleportTarget.Rotation);
	Other.SetCollision(true);
	if( PlayerPawn(Other)!=None )
		PlayerPawn(Other).ClientSetRotation(TeleportTarget.Rotation);
	Other.Velocity = vect(0,0,0);
	Other.bCanTeleport = true;
}

defaultproperties
{
	RemoteRole=ROLE_None
	bStatic=false
	bCollideActors=true
	bAllowPlayers=true
}