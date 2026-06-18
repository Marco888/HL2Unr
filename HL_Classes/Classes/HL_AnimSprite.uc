class HL_AnimSprite extends ScaledSprite;

var repnotify bool bNetTriggered;
var int numKeyFrames,curKeyFrame;

replication
{
	reliable if ( Role==ROLE_Authority )
		bNetTriggered;
}

simulated function PostBeginPlay()
{
	Role = ROLE_Authority;
	LifeSpan = 0.f;
	if( !bHidden && DrawType==DT_SpriteAnimOnce )
		PlayOnceAnim();
}
function Trigger( actor Other, pawn EventInstigator )
{
	RemoteRole = ROLE_SimulatedProxy;
	bAlwaysRelevant = true;
	bNetTriggered = true;
	bHidden = false;
	SetTimer(0.5,false,'ResetNetwork');
	
	if( Level.NetMode!=NM_DedicatedServer && DrawType==DT_SpriteAnimOnce )
		PlayOnceAnim();
}
simulated final function PlayOnceAnim()
{
	local Texture T;
	
	bHidden = false;
	numKeyFrames = 0;
	for( T=Texture; T; T=T.AnimNext )
		++numKeyFrames;
	LifeSpan = 1000.f;
	curKeyFrame = 0;
	if( numKeyFrames>0 )
		SetTimer(1.f/FMax(Texture.MaxFrameRate,1.f),true);
}
simulated function Timer()
{
	++curKeyFrame;
	if( curKeyFrame==numKeyFrames )
	{
		SetTimer(0.f,false);
		bHidden = true;
		LifeSpan = 0.f;
		return;
	}
	LifeSpan = (1.f - (float(curKeyFrame) / float(numKeyFrames))) * 1000.f;
}
function ResetNetwork()
{
	bNetTriggered = false;
}

simulated function OnRepNotify( name Property )
{
	if( bNetTriggered )
	{
		bNetTriggered = false;
		PlayOnceAnim();
	}
}

defaultproperties
{
	LifeSpan=1000
	bStatic=false
	RemoteRole=ROLE_None
	bNoDelete=true
}
