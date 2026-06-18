Class D_Protozoa extends D_Forklift;

simulated function PostBeginPlay()
{
	Role = ROLE_Authority;
	LoopAnim(AnimSequence,RandRange(0.9f,1.1f));
}
function Trigger( actor Other, pawn EventInstigator );

defaultproperties
{
	AnimSequence="Idle"
	Mesh=ProtozoaMesh
	bLoopAnim=true
	bInitialAnim=true
	bAllowRootMotionXY=false
	Style=STY_Translucent
	AmbientGlow=255
}