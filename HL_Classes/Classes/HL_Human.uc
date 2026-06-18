Class HL_Human extends HL_Pawns
	abstract;

var(Human) int BulletDamage;
var repnotify Actor NetAttackTarget;
var transient repnotify vector TracertHitPos;
var INFO_NPCAimActor NPCTicker;
var float AimAlpha;
var transient FX_BulletTracer TraceFX;

var bool bInBarnaclePin; // Grabbed by barnacle.

replication
{
	// Variables the server should send to the client.
	reliable if ( Role==ROLE_Authority )
		NetAttackTarget,TracertHitPos;
}

function bool PrepareRangedAttack()
{
	return false;
}
simulated function Destroyed()
{
	if( NPCTicker )
	{
		NPCTicker.Destroy();
		NPCTicker = None;
	}
	if( TraceFX )
		TraceFX.Destroy();
	Super.Destroyed();
}

function bool BarnacleCanGrip()
{
	return true;
}

final function bool PlayerInPvs()
{
	local Pawn P;
	
	foreach AllActors(class'Pawn',P,'Player')
	{
		if( P.Health>0 && (VSizeSq(P.Location-Location)<Square(600.f) || LineOfSightTo(P)) )
			return true;
	}
	return false;
}

function bool CanPlaySentence( bool bForce )
{
	if( bForce )
		return true;

	// if in the grip of a barnacle, don't speak
	if( bInBarnaclePin || bQuiet )
		return false;

	// if someone else is talking, don't speak
	if( !Class'FX_VoiceLinePlayer'.Static.GlobalCanSpeak(Level) )
		return false;

	// if player is not in pvs, don't speak
	if( !PlayerInPvs() )
		return false;

	return true;
}

function StartVoiceSequence( VoiceLineObject V, Actor LookTarget, bool bAllowConcurrency )
{
	local float T;
	local HL_Human P;

	Super.StartVoiceSequence(V,LookTarget,bAllowConcurrency);
	T = GetVoicePlayer().CalcVoiceDuration(V, VoicePitch);
	Class'FX_VoiceLinePlayer'.Static.GlobalSetSpeakTime(Level,T+0.5f);
	if( LookTarget )
		LookAtActor(LookTarget, T);
	
	if( !bAllowConcurrency )
	{
		foreach AllActors(class'HL_Human',P)
		{
			if( P==Self || P.Health<=0 || P.ActiveVoiceSeq==None || P.AttitudeToPlayer!=AttitudeToPlayer || VSizeSq(P.Location-Location)>Square(1500.f) )
				continue;
			P.AbortVoiceSequence();
		}
	}
}

simulated function OnRepNotify( name Property )
{
	if( Property=='NetAttackTarget' )
	{
		if( NetAttackTarget==None )
		{
			if( NPCTicker )
				NPCTicker.bFinishAim = true;
		}
		else if( NPCTicker )
			NPCTicker.bFinishAim = false;
		else NPCTicker = Spawn(class'INFO_NPCAimActor',Self);
	}
	else if( Property=='TracertHitPos' && !bNetBeginPlay )
	{
		if( TracertHitPos!=vect(0,0,0) )
			TraceAttack();
	}
	else Super.OnRepNotify(Property);
}
function TickAimer( vector TargetPos );
function EndAiming();

simulated function TraceAttack()
{
	local rotator R;
	local vector HL,HN,End,X;
	local Actor A;
	
	if( Level.NetMode==NM_Client )
	{
		X = Normal(TracertHitPos-Location);
		End = TracertHitPos+X*200.f;
		A = TraceShot(HL,HN,End,TracertHitPos-X*50.f);
	}
	else
	{
		R = AdjustAim(9999.f,Location,150,false,false);
		X = Normal(vector(R)+VRand()*0.05f);
		End = Location + X*10000.f;
		A = TraceShot(HL,HN,End,Location);
		
		if( !A )
			HL = End;
		TracertHitPos = HL;
	}
	if( Level.NetMode!=NM_DedicatedServer )
		DrawTrace(A,HL,HN,End);
	if( A && Level.NetMode!=NM_Client )
		A.TakeDamage(BulletDamage, Self, HL, ((FRand() < 0.2) ? 6000.f : 3000.f)*X, 'shot');
}
simulated function DrawTrace( Actor Other, vector HL, vector HN, vector End )
{
	local Actor A;
	
	if( Other )
	{
		if( Other==Level || Other.bWorldGeometry || Other.bIsMover || Other.Brush )
			A = Spawn(class'WallHitEffect',,, HL+HN*9, rotator(HN));
		else if( !Other.bIsPawn && !Other.IsA('Carcass') )
			A = Spawn(class'SpriteSmokePuff',,,HL+HN*9);
		if( A )
			A.RemoteRole = ROLE_None;
	}
}

function bool SetEnemy(Pawn NewEnemy)
{
	if( !bHurtEntry )
	{
		bHurtEntry = true;
		// Mutually allow non-humans see this if humans can see them.
		if( ScriptedPawn(NewEnemy) && NewEnemy.SightCheckType!=SEE_All && NewEnemy.CanSee(Self) )
			NewEnemy.SeePlayer(Self);
		bHurtEntry = false;
	}
	return Super.SetEnemy(NewEnemy);
}

state RangedAttack
{
ignores SeePlayer, HearNoise, Bump, AnimEnd;

	function Timer()
	{
		if( NetAttackTarget!=Enemy )
		{
			NetAttackTarget = Enemy;
			if( Level.NetMode!=NM_DedicatedServer )
			{
				if( NetAttackTarget==None )
				{
					if( NPCTicker )
						NPCTicker.bFinishAim = true;
				}
				else if( NPCTicker )
					NPCTicker.bFinishAim = false;
				else NPCTicker = Spawn(class'INFO_NPCAimActor',Self);
			}
		}
		if( Enemy )
		{
			DesiredRotation = rotator(Enemy.Location-Location);
			DesiredRotation.Pitch = 0;
		}
	}
	function BeginState()
	{
		bReadyToAttack = false;
		SetTimer(0.1,true);
		Timer();
	}
	function EndState()
	{
		bFiringPaused = false;
		NetAttackTarget = None;
		if( NPCTicker )
			NPCTicker.bFinishAim = true;
	}
	
	// don't talk if you're in combat
	function bool CanPlaySentence( bool bForce )
	{
		return bForce;
	}

Challenge:
FaceTarget:
ReadyToAttack:
Firing:
DoneFiring:
Begin:
	if (!HasAliveEnemy() || Enemy.Level!=Level)
		GotoState('Attacking');
	Acceleration = vect(0,0,0); //stop
	DesiredRotation = Rotator(Enemy.Location - Location);
	if( !PrepareRangedAttack() )
		TweenToFighter(0.15);
	FinishAnim();
	
	while( CanFireAtEnemy() )
	{
		PlayRangedAttack();
		Sleep(TimeBetweenAttacks);
		
		if( Enemy && MeleeRange>0.f && VSize(Location - Enemy.Location) < 0.9 * MeleeRange + CollisionRadius + Enemy.CollisionRadius )
			GotoState('MeleeAttack', 'ReadyToAttack');
	}
	FinishAnim();
	GotoState('Attacking');
}

defaultproperties
{
	CollisionHeight=39
	CollisionRadius=16
	SightCheckType=SEE_All
	BulletDamage=8
}