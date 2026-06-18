class HL_TriggerMultiple extends Volume;

var HL_Triggers Master;
var() name MasterTag; // Master trigger which is a condition if this is allowed to pass.
var() float RepeatTriggerTime;
var float nextTriggerTime;

var() bool bNotPlayers; // Do not allow players trigger this.
var() bool bAllowMonsters; // Allow monsters trigger this.
var() bool bAllowDecorations; // Only func_pushables/decorations.
var() bool bOnceOnly; // Only trigger once.

function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( MasterTag!='' )
	{
		foreach AllActors(class'HL_Triggers',Master,MasterTag)
			if( Master.IsMasterTrigger() )
				break;
	}
}
function Touch( Actor Other )
{
	if( nextTriggerTime>Level.TimeSeconds )
		return;
	if( Other.bIsPawn )
	{
		if( Pawn(Other).Health<=0 || (bNotPlayers && Pawn(Other).bIsPlayer) || (!bAllowMonsters && !Pawn(Other).bIsPlayer) )
			return;
	}
	else if( !bAllowDecorations || (HL_Pushable(Other)==None && Decoration(Other)==None) )
		return;
	if( Master && !Master.IsTriggered(Pawn(Other)) )
		return;

	TriggerEvent(Event, Self, Pawn(Other));
	if( bOnceOnly )
		SetCollision(false);
	else if( RepeatTriggerTime>0.f )
		nextTriggerTime = Level.TimeSeconds + RepeatTriggerTime;
}

defaultproperties
{
	RemoteRole=ROLE_None
}
