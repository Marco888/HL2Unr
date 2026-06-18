class HL_Decal extends HL_Triggers;

var() ERenderStyle DecalStyle;
var() Texture DecalTexture;
var() float DecalScale;

var const vector Dirs[6];
var FX_DecalActor ActiveDecal;
var repnotify bool bEnabled;

replication
{
	unreliable if ( Role==ROLE_Authority )
		bEnabled;
}

simulated function PostBeginPlay()
{
	if( (Tag=='' || Tag==Class.Name) && Level.NetMode!=NM_DedicatedServer )
		SpawnDecal();
}
function Trigger( Actor Other, Pawn EventInstigator )
{
	RemoteRole = ROLE_SimulatedProxy;
	bAlwaysRelevant = true;
	bForceNetUpdate = true;
	bEnabled = true;
	if( Level.NetMode!=NM_DedicatedServer )
		SpawnDecal();
}
simulated function OnRepNotify( name Property )
{
	if( Property=='bEnabled' )
		SpawnDecal();
}
simulated final function SpawnDecal()
{
	local vector HL,HN,End;
	local int i,j;
	local float BestDist,Dist;

	if( ActiveDecal || DecalTexture==None )
		return;
	
	// Trace for closest wall.
	j = -1;
	for( i=0; i<6; ++i )
	{
		End = Location + Dirs[i]*16.f;
		if( Trace(HL,HN,End,Location,false)==None )
			continue;
		Dist = VSizeSq(HL-Location);
		if( j==-1 || BestDist>Dist )
		{
			j = i;
			BestDist = Dist;
		}
	}
	if( j==-1 )
		return;
	
	ActiveDecal = Spawn(class'FX_DecalActor',,,HL+HN*4.f,rotator(HN));
	ActiveDecal.Texture = DecalTexture;
	ActiveDecal.Style = DecalStyle;
	ActiveDecal.DrawScale = DecalScale;
	ActiveDecal.AttachToSurface();
}

defaultproperties
{
	bNoDelete=true
	bSkipActorReplication=true
	DrawScale=0.5
	Texture=Texture'UnrealShare.Icons.UIProjectorIcon'
	DecalStyle=STY_Modulated
	DecalTexture=Texture'S_Pawn'
	DecalScale=1
	NetUpdateFrequency=2
	
	Dirs(0)=(X=1)
	Dirs(1)=(X=-1)
	Dirs(2)=(Y=1)
	Dirs(3)=(Y=-1)
	Dirs(4)=(Z=1)
	Dirs(5)=(Z=-1)
}
