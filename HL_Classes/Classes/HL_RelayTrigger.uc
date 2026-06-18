Class HL_RelayTrigger extends HL_Triggers;

var() name KillTarget; // Removes the specified target when this entity is triggered.
var() float Delay; // Usually the time in seconds before an entity should trigger its target (after being triggered itself). Under other SmartEdit names, delay might also be the time to wait before performing some other action.
var() byte TriggerState; /* Activate the target with this trigger state:
    0 = Off
    1 = On
    2 = Toggle */

function Trigger( actor Other, pawn EventInstigator )
{
	Instigator = EventInstigator;
	if( Delay>0.f )
		SetTimer(Delay,false);
	else Timer();
}
function Timer()
{
	if( Event!='' )
		TriggerEvent(Event,Self,Instigator);
	if( KillTarget!='' )
		KillTargets();
}
final function KillTargets()
{
	local Actor A;
	
	foreach AllActors(class'Actor',A,KillTarget)
	{
		if( A.bStatic || A.bNoDelete )
			A.KilledBy(Instigator);
		else A.Destroy();
	}
}

defaultproperties
{
	Texture=Texture'S_SpecialEvent'
}