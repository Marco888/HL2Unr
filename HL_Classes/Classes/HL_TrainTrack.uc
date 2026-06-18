Class HL_TrainTrack extends Keypoint;

var() name ReachEvent; // Event to trigger when train enters this node.
var() name AlternativePath; // An alternative path_track to be the next node in the path. Useful for making branching paths. Use the ToggleAlternatePath / EnableAlternatePath inputs to make the alternative path active.
var() float PathRadius; // Used by NPCs who follow track paths (attack chopper/gunship). This tells them the maximum distance they're allowed to be from the path at this node.
var() float TrainSpeed; // When the train reaches this path_track, it will set its speed to this speed. Speeds higher than the max speed of the train are clamped.
var() float PauseTime; // Train should pause here.
var() bool bTriggerOnce; // Run ReachEvent once only.

var() enum EOrientationType
{
	OT_None,
	OT_Velocity,
	OT_Rotation,
} OrientationType; // The way that the path follower faces as it moves through this path track. 

var HL_TrainTrack NextTrack,PrevTrack;
var rotator NextRouteDir;

simulated function BeginPlay()
{
	NextRouteDir = Rotation;
	if( Event!='' )
	{
		foreach AllActors(class'HL_TrainTrack',NextTrack,Event)
			break;
		if( NextTrack )
		{
			NextTrack.PrevTrack = Self;
			if( OrientationType==OT_Velocity )
				NextRouteDir = rotator(NextTrack.Location-Location);
		}
	}
}
simulated function PostBeginPlay()
{
	if( OrientationType==OT_Velocity && !NextTrack && PrevTrack )
		NextRouteDir = PrevTrack.NextRouteDir;
}

event OnPropertyChange( name Property, name ParentProperty )
{
	if( Property=='OrientationType' )
		bDirectional = (OrientationType==OT_Rotation);
}

simulated function GetPointCoords( HL_TrainMover T, out vector Pos, out rotator Dir )
{
	Pos = Location + T.TrainOffset;
	if( OrientationType==OT_None )
		Dir = T.Rotation;
	if( T.sf_NoPitch )
		Dir = Construct<rotator>(Yaw=NextRouteDir.Yaw,Pitch=0,Roll=0);
	else Dir = NextRouteDir;
}

// Prepare a move from this point to a next point, return false if impossible (no next point).
simulated function bool PrepareMove( HL_TrainMover T )
{
	if( !NextTrack )
		return false;

	T.SetCustomTrackMotion(false);
	T.CurrentTrack = NextTrack;
	GetPointCoords(T, T.TrackVStart, T.TrackRStart);
	NextTrack.GetPointCoords(T, T.TrackVEnd, T.TrackREnd);

	if( TrainSpeed>0.f )
		T.TrainSpeed = FMin(TrainSpeed, T.MaxSpeed);
	T.bForceFullRotation = false;
	return true;
}

// Initiate move to this track.
function bool InitiateMove( HL_TrainMover T )
{
	if( !NextTrack )
		return false;

	if( PauseTime>0.f )
	{
		T.OtherTime = PauseTime;
		T.FinishedOpening();
		T.GoToState('Train','PauseMove');
	}
	else T.BeginMoveTo(1);
	return true;
}

function MotionFinished( HL_TrainMover T )
{
	if( ReachEvent!='' )
	{
		TriggerEvent(ReachEvent,T,T.Instigator);
		if( bTriggerOnce )
			ReachEvent = '';
	}
}

defaultproperties
{
	OrientationType=OT_Velocity
	RemoteRole=ROLE_None
}