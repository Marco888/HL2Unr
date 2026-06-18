// A trigger_auto will automatically fire the specified entity whenever the map loads.
// In coop, wait until players enter the game.
class HL_TriggerAuto extends HL_Triggers;

var() bool bInitiallyActive;
var() float TriggerDelay;

function Timer()
{
	local PlayerPawn P;
	
	if( Level.NetMode==NM_StandAlone )
		P = GetLocalPlayerPawn();
	else
	{
		foreach AllActors(class'PlayerPawn',P)
			break;
	}
	TriggerEvent(Event,Self,P);
	Destroy();
}
auto state DelayedTrigger
{
	event Trigger( Actor Other, Pawn EventInstigator )
	{
		if( TriggerDelay>0.f )
			SetTimer(TriggerDelay,false);
		else Timer();
		GoToState('');
	}
Begin:
	if( !bInitiallyActive )
		Stop;
	if( Level.NetMode!=NM_StandAlone )
	{
		while( Level.Game.NumPlayers==0 )
			Sleep(0.25);
	}
	if( TriggerDelay>0.f )
		SetTimer(TriggerDelay,false);
	else Timer();
	GoToState('');
}

defaultproperties
{
	bInitiallyActive=true
}