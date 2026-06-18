class HL_AnimEntity extends Decoration;

var() float EntAnimRate;

simulated function PostBeginPlay()
{
	Role = ROLE_Authority;
	if( Level.NetMode!=NM_DedicatedServer )
		LoopAnim(AnimSequence,EntAnimRate);
}

event Bump( Actor Other );
event Trigger( Actor Other, Pawn EventInstigator );
event TakeDamage( int Damage, Pawn EventInstigator, vector HitLocation, vector Momentum, name DamageType);
function GrabbedBy( Pawn Other );

defaultproperties
{
	RemoteRole=ROLE_None
	DrawType=DT_Mesh
	bStatic=false
	bNoDelete=true
	bMovable=false
	Mesh=Mesh'WoodenBoxM'
	bCollideActors=false
	AnimSequence="All"
	bGameRelevant=true
	Texture=Texture'S_Actor'
	EntAnimRate=1
}