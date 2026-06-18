// func_tracktrain
Class HL_TrainMover extends HL_MoverBase;

var() float TrainSpeed,MaxSpeed;
var() float TurnDistance;
var() float TrackHeight;

var() byte MoveSoundMinPitch,MoveSoundMaxPitch;

var() bool sf_NoPitch; // Only rotate by Yaw angle.
var() bool sf_NoUserControl;
var() bool sf_FixedOrientation; // Never rotate.
var() bool sf_SoundPitchBySpeed; // Use max speed for pitch shifting move sound.
var() bool sf_StartActive; // Initially start moving.

var vector TrainOffset;
var HL_TrainTrack PreviousTrack,CurrentTrack;
var int MoveDirection;

struct FTrainMovePart
{
	var rotator Dir;
	var vector Pos;
	var float Rate;
};
var FTrainMovePart MoveParts[4];
var byte MoveCounter,NumMoveCache;
var bool bCustomTrackMotion;
var transient bool bInnerTriggered;

// Pending traintrack motion.
var Sound CustomAmbientSound;
var vector TrackVStart,TrackVEnd;
var rotator TrackRStart,TrackREnd;
var bool bForceFullRotation;

simulated function PostBeginPlay()
{
	local HL_TrainTrack T;
	
	Super.PostBeginPlay();
	if( MaxSpeed==0.f )
		MaxSpeed = TrainSpeed;
	TrainOffset.Z = TrackHeight;
	
	if( Event!='' )
	{
		foreach AllActors(class'HL_TrainTrack',T,Event)
		{
			SetCurrentTrack(T);
			break;
		}
	}
}
simulated final function SetCurrentTrack( HL_TrainTrack T )
{
	local vector V;
	local rotator R;

	PreviousTrack = None;
	CurrentTrack = T;
	T.GetPointCoords(Self, V, R);
	SetLocation(V,R);
}

final function BeginNewMotion( HL_TrainTrack Prev )
{
	local vector MoveDir;
	local float PathDist;
	
	PreviousTrack = Prev;
	bInnerTriggered = true;
	
	if( !Prev.PrepareMove(Self) )
	{
		SetPhysics(PHYS_None);
		bInterpolating = true;
		FinishedOpening();
		return;
	}
	
	if( !bOpening )
		DoOpen();
	
	// Setup move info.
	MoveParts[0].Dir = Rotation;
	MoveParts[0].Pos = TrackVStart;
	
	MoveDir = TrackVEnd-TrackVStart;
	PathDist = VSize(MoveDir);
	
	if( bForceFullRotation || Rotation==TrackREnd )
	{
		MoveParts[1].Dir = TrackREnd;
		MoveParts[1].Pos = TrackVEnd;
		MoveParts[1].Rate = 1.f / (FMax(PathDist,0.001f)/TrainSpeed);
		
		NumMoveCache = 2;
	}
	else
	{
		if( (PathDist*0.5f)<=TurnDistance )
		{
			MoveParts[1].Dir = LerpRotation(TrackRStart,TrackREnd,0.5f);
			MoveParts[1].Pos = TrackVEnd;
			MoveParts[1].Rate = 1.f / (FMax(PathDist,0.001f)/TrainSpeed);
			
			NumMoveCache = 2;
		}
		else
		{
			MoveDir = Normal(MoveDir) * TurnDistance;
			
			// Setup move info.
			MoveParts[1].Dir = TrackRStart;
			MoveParts[1].Pos = TrackVStart + MoveDir;
			MoveParts[1].Rate = 1.f / (FMax(TurnDistance,0.001f)/TrainSpeed);
			
			MoveParts[2].Dir = TrackRStart;
			MoveParts[2].Pos = TrackVEnd - MoveDir;
			MoveParts[2].Rate = 1.f / (FMax(PathDist-(TurnDistance*2.f),0.00001f)/TrainSpeed);
			
			MoveParts[3].Dir = LerpRotation(TrackRStart,TrackREnd,0.5f);
			MoveParts[3].Pos = TrackVEnd;
			MoveParts[3].Rate = 1.f / (FMax(TurnDistance,0.001f)/TrainSpeed);
			
			NumMoveCache = 4;
		}
	}
	MoveCounter = 1;
	
	OldPos = Location;
	OldRot = Rotation;
	PhysAlpha = 0.f;
	PhysRate = MoveParts[1].Rate;
	KeyNum = 1;
	PrevKeyNum = 0;
	KeyPos[1] = MoveParts[1].Pos - BasePos;
	KeyRot[1] = MoveParts[1].Dir - BaseRot;
	SetPhysics(PHYS_MovingBrush);
	bInterpolating = true;
}

final function BeginMoveTo( int TrackDir )
{
	if( CurrentTrack )
	{
		MoveDirection = TrackDir;
		BeginNewMotion(CurrentTrack);
	}
}

function SetCustomTrackMotion( bool bCustom, optional Sound NewAmbient )
{
	if( bCustomTrackMotion!=bCustom )
	{
		bCustomTrackMotion = bCustom;
		if( bOpening )
		{
			if( bCustom )
				PlaySound( OpenedSound, SLOT_None );
			else PlaySound( OpeningSound, SLOT_None );
		}
		else if( !bCustom )
			PlaySound( OpeningSound, SLOT_None );
	}
	CustomAmbientSound = NewAmbient;
	if( bCustom )
		AmbientSound = NewAmbient;
	else AmbientSound = MoveAmbientSound;
}

state() Train
{
	function DoOpen()
	{
		bOpening = true;
		bDelaying = false;
		PlaySound( OpeningSound, SLOT_None );
		if( bCustomTrackMotion )
			AmbientSound = CustomAmbientSound;
		else AmbientSound = MoveAmbientSound;
	}
	function Trigger( actor Other, pawn EventInstigator )
	{
		Instigator = EventInstigator;
		if( bOpening )
		{
			// Pause move.
			MakeGroupStop();
			FinishedOpening();
		}
		else BeginMoveTo(1);
	}
	function FinishedOpening()
	{
		bOpening = false;
		PlaySound( OpenedSound, SLOT_None );
		AmbientSound = None;
		FinishNotify();
	}
	function InterpolateEnd( actor Other )
	{
		if( ++MoveCounter>=NumMoveCache ) // Reached keypoint.
		{
			if( CurrentTrack )
			{
				bInnerTriggered = false;
				CurrentTrack.MotionFinished(Self);
				if( !bInnerTriggered && !CurrentTrack.InitiateMove(Self) )
					FinishedOpening();
			}
			else FinishedOpening();
		}
		else // Keep moving...
		{
			OldPos = MoveParts[MoveCounter-1].Pos;
			OldRot = MoveParts[MoveCounter-1].Dir;
			PhysAlpha = 0.f;
			PhysRate = MoveParts[MoveCounter].Rate;
			KeyNum = 1;
			PrevKeyNum = 0;
			KeyPos[1] = MoveParts[MoveCounter].Pos - BasePos;
			KeyRot[1] = MoveParts[MoveCounter].Dir - BaseRot;
			SetPhysics(PHYS_MovingBrush);
			bInterpolating = true;
		}
	}
	function MakeGroupReturn()
	{
		// Abort move and reverse course.
		MoveDirection = -MoveDirection;
		BeginMoveTo(MoveDirection);

		if ( Follower )
			Follower.MakeGroupReturn();
	}
Begin:
	if( sf_StartActive )
		Trigger(Self, None);
	Stop;
PauseMove:
	Sleep(OtherTime);
	BeginMoveTo(1);
	DoOpen();
}

defaultproperties
{
	TurnDistance=50
	MoveSoundMinPitch=100
	MoveSoundMaxPitch=100
	bUseShortestRotation=True
	InitialState="Train"
	bDirectionalPushOff=True
	bAdvancedCamUpdate=True
	MoveDirection=1
}