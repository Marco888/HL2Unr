class HL_Triggers extends Triggers
	abstract;

function bool IsTriggered( Pawn EventInstigator )
{
	return true;
}
function bool IsMasterTrigger()
{
	return false;
}
function NotifyMasterChange( Actor Other );

invariant static final function DebugMsg( string S )
{
	GetLocalPlayerPawn().ClientMessage(S);
	Log(GetLocalPlayerPawn().Level.TimeSeconds$": "$S,'Debug');
}

defaultproperties
{
	RemoteRole=ROLE_None
	bCollideActors=false
}