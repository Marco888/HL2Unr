class HL_MonsterMaker extends HL_Triggers;

var() class<Actor> TeleportEffect; // Effect with bShowSpawnEffect.
var() float EffectDelayTime; // Time it takes between effect start and when monster spawns in (with bShowSpawnEffect).
var() float AutoDespawnTime; // if >0, time seconds before this monster despawns or "teleports away".
var() int MonsterCount; // how many monsters the monstermaker can create (-1 = unlimited)
var() int MaxLiveChildren; // how many monsters the monstermaker can create (-1 = unlimited)
var() float Delay; // if delay is -1, new monster will be made when last monster dies. else, delay is how often (seconds) a new monster will be dookied out.
var() class<HL_Pawns> MonsterType;
var() name MonsterTag; // Give spawned monsters this tag.
var() bool bShowSpawnEffect;

var bool bActive,bInitHeightMap;

var int curMonsters;
var vector EndPosition;
var INFO_MonsterKillTrack Tracker;

function PostBeginPlay()
{
	if( MonsterType==None )
		Error("No monster type?");
}
function Trigger( actor Other, pawn EventInstigator )
{
	bActive = !bActive;
	
	if( !bActive )
		SetTimer(0,false);
	else SpawnMonster();
}
function SpawnMonster()
{
	if( !bActive )
		return;

	// not allowed to make a new one yet. Too many live ones out right now.
	if (MaxLiveChildren > 0 && curMonsters >= MaxLiveChildren)
		return;
	
	// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
	if ( !bInitHeightMap )
		TraceHeight();
	
	// don't build a stack of monsters!
	if( MonstersUnder() )
		return;
	
	if( bShowSpawnEffect )
	{
		Spawn(TeleportEffect);
		SetTimer(EffectDelayTime,false,'GenerateMonster');
	}
	else GenerateMonster();
}
function GenerateMonster()
{
	local HL_Pawns H;
	local INFO_MonsterKillTimer M;

	H = Spawn(MonsterType);
	if( H==None )
	{
		SetTimer(0.5,false); // Try again soon.
		return;
	}

	if( Tracker==None )
	{
		Tracker = Spawn(class'INFO_MonsterKillTrack');
		Tracker.Notify = Self;
		Tracker.Tag = Name;
	}
	H.Event = Name;
	H.Tag = MonsterTag;
	TriggerEvent(Event,Self,H);
	
	if( AutoDespawnTime>0.f )
	{
		M = Spawn(class'INFO_MonsterKillTimer');
		M.SetTimer(AutoDespawnTime,false);
		M.Maker = Self;
		M.Monster = H;
		if( bShowSpawnEffect )
			M.SetTimer(FMax(AutoDespawnTime-EffectDelayTime,0.01f),false,'KillEffect');
	}
	
	curMonsters++; // count this monster
	if( MonsterCount>0 )
	{
		MonsterCount--;
		if (MonsterCount == 0)
			Destroy();
	}
	if( curMonsters < MaxLiveChildren )
		SetTimer(FMax(Delay,0.25f),false);
}
function Destroyed()
{
	if( Tracker )
		Tracker.Destroy();
}
function ChildKilled()
{
	--curMonsters;
	if( bActive )
		SetTimer(FMax(Delay,0.01f),false);
}
function Timer()
{
	SpawnMonster();
}
function PendingKillMonster( HL_Pawns H )
{
	local vector HL,HN,End;

	if( H.bDeleteMe || H.Health<=0 )
		return;
	End = H.Location+H.Velocity*EffectDelayTime;
	if( Trace(HL,HN,End,H.Location,false,H.GetExtent()) )
		End = HL;
	Spawn(TeleportEffect,,,End);
}
function KillMonster( HL_Pawns H )
{
	if( H.bDeleteMe || H.Health<=0 )
		return;
	H.Destroy();
	ChildKilled();
}

final function TraceHeight()
{
	local vector HL,HN;

	bInitHeightMap = true;
	if( Trace(HL,HN,Location - vect(0,0,2048),Location,false,vect(10,10,10))==None )
		HL = Location - vect(0,0,2048);
	EndPosition = HL + vect(0,0,5);
}
final function bool MonstersUnder()
{
	local vector HL,HN;
	local Actor A;

	foreach TraceActors(class'Actor',A,HL,HN,EndPosition,Location,vect(10,10,10))
		if( A.bIsPawn && Pawn(A).Health>0 )
			return true;
	return false;
}

defaultproperties
{
	MonsterCount=1
	MaxLiveChildren=1
	EffectDelayTime=0.15
	TeleportEffect=class'FX_TeleportEffect'
}