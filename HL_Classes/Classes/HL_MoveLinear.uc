// func_door
Class HL_MoveLinear extends HL_MoverBase;

var() Sound LockedSound;
var() array<VoiceLineObject> LockedLines;
var float nextLockedSound;

var() bool bReverseRotate;
var() bool bDirectionalOpen; // Is a directional rotating door where door will rotate to keyframe 2 or 3 depending on where player is standing.
var() bool bPlayersOnly; // Door cant be opened by NPC's.
var() bool bStartsOpen; // Initially at opened state.

function BumpLocked( Pawn Other )
{
	if( nextLockedSound<Level.TimeSeconds )
	{
		PlaySound(LockedSound);
		nextLockedSound = Level.TimeSeconds+3.f;
	}
}

state() BumpOpenTimed
{
	function Trigger( actor Other, pawn EventInstigator )
	{
		SavedTrigger = Other;
		Instigator = EventInstigator;
		if ( SavedTrigger != None )
			SavedTrigger.BeginEvent();
		GotoState(,'Open');
	}

Begin:
	Stop;
Open:
	Disable('Trigger');
	Disable('Bump');
	if ( DelayTime > 0 )
	{
		bDelaying = true;
		Sleep(DelayTime);
	}
	DoOpen();
	FinishInterpolation();
	FinishedOpening();
	Sleep( StayOpenTime );
	if ( bTriggerOnceOnly )
		GotoState('');
Close:
	DoClose();
	FinishInterpolation();
	FinishedClosing();
	Enable('Trigger');
	Enable('Bump');
}
state() BumpToggle
{
	function Trigger( actor Other, pawn EventInstigator )
	{
		SavedTrigger = Other;
		Instigator = EventInstigator;
		if ( SavedTrigger != None )
			SavedTrigger.BeginEvent();
		if ( KeyNum==0 || KeyNum<PrevKeyNum )
			GotoState(,'Open');
		else
			GotoState(,'Close');
	}
Begin:
	if( bStartsOpen )
		InterpolateTo(NumKeys-1,0.f);
	Stop;
Open:
	Disable('Bump');
	if ( DelayTime > 0 )
	{
		bDelaying = true;
		Sleep(DelayTime);
	}
	DoOpen();
	FinishInterpolation();
	FinishedOpening();
	Enable('Bump');
	Stop;
	
Close:
	Disable('Bump');
	DoClose();
	FinishInterpolation();
	FinishedClosing();
	Enable('Bump');
}

state() TriggerToggle
{
Begin:
	if( bStartsOpen )
		InterpolateTo(NumKeys-1,0.f);
	Stop;
Open:
	if ( DelayTime > 0 )
	{
		bDelaying = true;
		Sleep(DelayTime);
	}
	DoOpen();
	FinishInterpolation();
	FinishedOpening();
	if ( SavedTrigger!=None )
		SavedTrigger.EndEvent();
	Stop;
Close:
	DoClose();
	FinishInterpolation();
	FinishedClosing();
}

state() TriggeredLoop
{
	function Trigger( actor Other, pawn EventInstigator )
	{
		GotoState('ConstantLoop');
	}
}

defaultproperties
{
	InitialState="TriggerOpenTimed"
}