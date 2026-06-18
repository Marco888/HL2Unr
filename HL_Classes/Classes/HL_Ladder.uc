class HL_Ladder extends LadderTrigger;

simulated function PostBeginPlay()
{
	local BoundingBox B;

	B = GetBoundingBox();
	if( (B.Max.X-B.Min.X)<(B.Max.Y-B.Min.Y) )
		SideAxis = vect(0,1,0);
	else SideAxis = vect(1,0,0);
}

defaultproperties
{
	MaxGrabVelocity=1500
	ClimbSpeed=0.8
	SideStepSpeedMod=0.75
	bUnarmedClimbing=False
	bMustFaceForward=False
	bAllowSideStep=true
	ClimbingNoise=Sound'pl_ladder1'
}
