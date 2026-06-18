// func_trackchange is a brush entity. This entity allows you to create a rotating platform that will switch a func_tracktrain's path, used mainly with player-controllable trains. 
Class HL_TrainTrackChange extends HL_TrainTrack;

var HL_TrainTrack TopTrack,BottomTrack;
var() name TopTrackTag,BottomTrackTag,TrainTag;

var() int RotationOffset; // This is the amount in degrees the track will turn on its way up / down.
var() float MovementHeight; // Altitude to ascend or descend (use negative values for the latter, or 0 to disable altitude change).

var() Sound UseSound; // Sound to play while on this.

var() bool bStartFromTrackNode; // Override Half-Life behaviour and make it start from track point.
var() bool bStartBottom; // Moving track will start in the bottom position.
var() bool bRotateOnly; // Track will not move up and down.
var() bool bRotateX; // Rotate around the X axis.
var() bool bRotateY; // Rotate around the Y axis.
var bool bCurrentlyBottom;

simulated function BeginPlay()
{
	bCurrentlyBottom = bStartBottom;
	if( TopTrackTag!='' )
	{
		foreach AllActors(class'HL_TrainTrack',TopTrack,TopTrackTag)
			break;
	}
	if( BottomTrackTag!='' )
	{
		foreach AllActors(class'HL_TrainTrack',BottomTrack,BottomTrackTag)
			break;
	}
	if( bCurrentlyBottom )
	{
		PrevTrack = BottomTrack;
		NextTrack = TopTrack;
	}
	else
	{
		PrevTrack = TopTrack;
		NextTrack = BottomTrack;
	}
}

event OnPropertyChange( name Property, name ParentProperty )
{
}

simulated function GetPointCoords( HL_TrainMover T, out vector Pos, out rotator Dir )
{
	if( bStartFromTrackNode && PrevTrack )
		PrevTrack.GetPointCoords(T, Pos, Dir);
	else
	{
		Pos = T.Location;
		Dir = T.Rotation;
	}
}

// Prepare a move from this point to a next point, return false if impossible (no next point).
simulated function bool PrepareMove( HL_TrainMover T )
{
	if( bCurrentlyBottom )
	{
		PrevTrack = BottomTrack;
		NextTrack = TopTrack;
	}
	else
	{
		PrevTrack = TopTrack;
		NextTrack = BottomTrack;
	}
	
	T.SetCustomTrackMotion(true,UseSound);

	T.CurrentTrack = NextTrack;
	GetPointCoords(T,T.TrackVStart,T.TrackRStart);
	T.TrackVEnd = T.Location;
	if( !bRotateOnly )
	{
		if( bCurrentlyBottom )
			T.TrackVEnd.Z += MovementHeight;
		else T.TrackVEnd.Z -= MovementHeight;
	}
	T.TrackREnd = T.TrackRStart;
	if( bCurrentlyBottom )
		T.TrackREnd.Yaw -= RotationOffset;
	else T.TrackREnd.Yaw += RotationOffset;

	if( TrainSpeed>0.f )
		T.TrainSpeed = FMin(TrainSpeed, T.MaxSpeed);
	T.bForceFullRotation = true;
	bCurrentlyBottom = !bCurrentlyBottom;
	return true;
}

function Trigger( Actor Other, Pawn EventInstigator )
{
	local HL_TrainMover T;
	
	T = HL_TrainMover(Other);
	if( T )
		T.BeginNewMotion(Self);
	else if( TrainTag!='' )
	{
		foreach AllActors(class'HL_TrainMover',T,TrainTag)
			T.BeginNewMotion(Self);
	}
}

defaultproperties
{
	OrientationType=OT_None
	RemoteRole=ROLE_None
}