Class D_Fungus extends D_Forklift;

simulated function PostBeginPlay()
{
	LoopAnim(AnimSequence,RandRange(0.9f,1.1f));
	SetCollisionSize(ScaledDefaultCollisionRadius(), ScaledDefaultCollisionHeight());
}
function Trigger( actor Other, pawn EventInstigator );

defaultproperties
{
	AnimSequence="Idle"
	Mesh=FungusLMesh
	bLoopAnim=true
	bInitialAnim=true
	bAllowRootMotionXY=false
	CollisionRadius=100.0
	CollisionHeight=140.0
	bWorldGeometry=True
	bCollideActors=True
	bBlockActors=True
	bBlockPlayers=True
	bProjTarget=True
	bPathCollision=True
	bBlockRigidBodyPhys=True
}
