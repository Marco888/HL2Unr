// Decorative NPC or something.
Class D_Forklift extends Decoration;

var repnotify bool bNetTriggered;
var() bool bLoopAnim; // Animation should loop.
var() bool bInitialAnim; // Animation should start instantly on map start.
var() bool bFinishAnimLoop; // Animation end should loop.
var() name TriggerAnim; // Animation to start, if not set, use AnimSequence.
var() name AnimFinishAnim; // Animation to start once finished.

replication
{
	unreliable if ( Role==ROLE_Authority )
		bNetTriggered;
}

simulated function PostBeginPlay()
{
	if( bInitialAnim )
		OnStartAnim();
	else if( TriggerAnim!='' )
		LoopAnim(TriggerAnim);
}
simulated function OnRepNotify( name Property )
{
	if( Property=='bNetTriggered' )
		OnStartAnim();
}
simulated function OnStartAnim()
{
	local name N;
	
	bStasis = false;
	if( TriggerAnim!='' )
		N = TriggerAnim;
	else N = AnimSequence;
	if( bLoopAnim )
		LoopAnim(N);
	else PlayAnim(N);
}
simulated function AnimEnd()
{
	if( !bAnimLoop && AnimFinishAnim!='' && AnimSequence!=AnimFinishAnim )
	{
		bStasis = true;
		if( bFinishAnimLoop )
			LoopAnim(AnimFinishAnim);
		else PlayAnim(AnimFinishAnim);
	}
}
function Trigger( actor Other, pawn EventInstigator )
{
	Disable('Trigger');
	RemoteRole = ROLE_SimulatedProxy;
	bNetTriggered = true;
	bForceNetUpdate = true;
	OnStartAnim();
}
function Destroyed();
function Bump( actor Other );
function EdNoteAddedActor( vector HitLocation, vector HitNormal );
function GrabbedBy( Pawn Other );
function ZoneChange( ZoneInfo NewZone );

simulated event SkeletalRootMotion( vector Delta, float DeltaTime )
{
	Move(Delta*1.25f);
}

defaultproperties
{
	AnimSequence="idle"
	Mesh=ForkliftM
	Physics=PHYS_None
	bCollideWhenPlacing=false
	bCollideWorld=false
	bCollideActors=false
	bNoDelete=true
	bStatic=false
	RemoteRole=ROLE_None
	bSkipActorReplication=true
	NetUpdateFrequency=1
	bAlwaysRelevant=true
	bRepAnimations=false
	bOnlyDirtyReplication=true
	bCarriedItem=true
	bMovable=true
	DrawType=DT_Mesh
	bRenderMultiEnviroMaps=true
	bAllowRootMotionXY=true
}