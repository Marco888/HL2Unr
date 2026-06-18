Class D_Hair extends D_Forklift;

simulated function PostBeginPlay()
{
	Role = ROLE_Authority;
	LoopAnim(AnimSequence,RandRange(0.9f,1.1f));
}
function Trigger( actor Other, pawn EventInstigator );

defaultproperties
{
	AnimSequence="Idle"
	Mesh=HairMesh
	bLoopAnim=true
	bInitialAnim=true
	bAllowRootMotionXY=false
	bUnlit=true
	CollisionHeight=4
	CollisionRadius=4
	PrePivot=(Z=-4.5)
}