Class HL_Pawns extends ScriptedPawn
	abstract;

#exec OBJ LOAD FILE="HL_Ambience"

var repnotify Actor NetLookAt;
var transient IK_HeadTurn IKHead;
var transient IK_LipSync IKLipSync;
var HL_LookAtController HeadController;
var transient FX_VoiceLinePlayer VoicePlayerActor;

var float ForceMoveTime;
var HL_ScriptedSequence ActiveScript;
var HL_ScriptedSequence ScriptQue;
var VoiceLineObject ActiveVoiceSeq;
var int ScriptLoops;
var bool bIdleSequence,bShouldStartOnFloor;

var	nowarn float VoicePitch;

var() enum ERenderFX
{
	RFX_Normal,
	RFX_SlowPulse,
	RFX_FastPulse,
	RFX_Distort,
	RFX_Hologram,
} RenderFX;

replication
{
	// Variables the server should send to the client.
	reliable if ( Role==ROLE_Authority )
		NetLookAt;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( Level.NetMode!=NM_DedicatedServer )
	{
		IKHead = IK_HeadTurn(GetIKSolver(class'IK_HeadTurn'));
		IKLipSync = IK_LipSync(GetIKSolver(class'IK_LipSync'));
		AnimationNotify = new (Outer) Class'HL_AnimNotify';
	}
}
function PostLoadGame()
{
	Super.PostLoadGame();
	IKHead = IK_HeadTurn(GetIKSolver(class'IK_HeadTurn'));
	IKLipSync = IK_LipSync(GetIKSolver(class'IK_LipSync'));
}

simulated function OnRepNotify( name Property )
{
	if( Property=='NetLookAt' )
		AnimLookAtActor();
}

simulated final function AnimLookAtActor()
{
	if( !IKHead )
		return;
	if( NetLookAt )
	{
		if( !HeadController || HeadController.bDeleteMe )
			HeadController = Spawn(class'HL_LookAtController',Self);
	}
	else if( HeadController )
	{
		HeadController.Destroy();
		HeadController = None;
	}
}

function SetRenderFX( ERenderFX newFX )
{
	RenderFX = newFX;
}

final function LookAtActor( Actor Other, optional float LookTime )
{
	NetLookAt = Other;
	SetTimer(LookTime,false,'StopLooking');
	if( Level.NetMode!=NM_DedicatedServer )
		AnimLookAtActor();
}
final function StopLooking()
{
	NetLookAt = None;
	if( Level.NetMode!=NM_DedicatedServer )
		AnimLookAtActor();
}

simulated function Destroyed()
{
	IKHead = None;
	IKLipSync = None;
	if( HeadController )
	{
		HeadController.Destroy();
		HeadController = None;
	}
	Super.Destroyed();
}
simulated function NotifyIKSolver( IK_SolverBase Solver, bool bDelete )
{
	if( bDelete )
	{
		if( IKHead==Solver )
			IKHead = None;
		else if( IKLipSync==Solver )
			IKLipSync = None;
	}
	else
	{
		if( IK_HeadTurn(Solver) )
			IKHead = IK_HeadTurn(Solver);
		else if( IK_LipSync(Solver) )
			IKLipSync = IK_LipSync(Solver);
	}
	Super.NotifyIKSolver(Solver,bDelete);
}

final function FX_VoiceLinePlayer GetVoicePlayer()
{
	if( VoicePlayerActor==None )
		VoicePlayerActor = Spawn(class'FX_VoiceLinePlayer',Self);
	return VoicePlayerActor;
}

// Return if this is a friendly NPC following a player atm.
function bool IsFollowingPlayer()
{
	return false;
}

// Return if we can start playing a voice sentence now.
function bool CanPlaySentence( bool bForce )
{
	return true;
}

function bool BarnacleCanGrip()
{
	return false;
}

function bool CanStartSequence( HL_ScriptedSequence S, int InterruptLevel )
{
	return true;
}

function StartSequence( HL_ScriptedSequence S, bool bIdle )
{
	ActiveScript = S;
	bIdleSequence = bIdle;
	S.ApplyScriptTo(Self, bIdle);
	GoToState('Scripting');
}

// Voiceline sequence.
function StartVoiceSequence( VoiceLineObject V, Actor LookTarget, bool bAllowConcurrency )
{
	ActiveVoiceSeq = V;
	GetVoicePlayer().PlayVoiceLine(V,, VoicePitch);
}
function AbortVoiceSequence()
{
	ActiveVoiceSeq = None;
	if( VoicePlayerActor )
		VoicePlayerActor.AbortVoiceLine();
}
function FinishedSpeech()
{
	ActiveVoiceSeq = None;
}

function bool StartAnimSequence( name ScSeq, bool bLoop )
{
	if( ScSeq!='' && HasAnim(ScSeq) )
	{
		if( bLoop )
			LoopAnim(ScSeq);
		else PlayAnim(ScSeq);
		return true;
	}
	return false;
}

state MovementTest
{
Ignores EnemyAcquired,TryToDuck,TryToCrouch,TakeDamage;

	final function InitPatrols()
	{
		WalkBob = Location + vector(Rotation)*1000.f;
		HidingSpot = Location;
	}
Begin:
	SetPhysics(PHYS_Falling);
	WaitForLanding();
	InitPatrols();
	while( true )
	{
		Acceleration = vect(0,0,0);
		TweenToWalking(0.3);
		FinishAnim();
		PlayWalking();
		MoveTo(WalkBob,WalkingSpeed);
		
		Acceleration = vect(0,0,0);
		TweenToRunning(0.15);
		FinishAnim();
		PlayRunning();
		MoveTo(HidingSpot,1.f);
	}
}

state Scripting
{
Ignores EnemyAcquired,TryToDuck,TryToCrouch;

	function StartSequence( HL_ScriptedSequence S, bool bIdle )
	{
		if( ActiveScript==S )
		{
			if( bIdleSequence )
			{
				if( !bIdle )
				{
					bIdleSequence = false;
					GoToState('Scripting','Begin');
				}
			}
			else if( bIdle )
			{
				bIdleSequence = true;
				GoToState('Scripting','Begin');
			}
			return;
		}
		else if( ActiveScript.sf_PriorityScript && !S.sf_PriorityScript )
		{
			ScriptQue = S;
			return;
		}
		ActiveScript = S;
		bIdleSequence = bIdle;
		S.ApplyScriptTo(Self, bIdle);
		GoToState('Scripting','Begin');
	}
	
	function StartRoaming() // Never go to roaming from here.
	{
		GotoState('Waiting');
	}
	
	// Never actually die during sequence...
	function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation, Vector momentum, name damageType)
	{
		local int actualDamage;
		local bool bAlreadyDead;
		local GameRules GR;

		if ( Level.Game && Level.Game.GameRules )
		{
			for (GR = Level.Game.GameRules; GR != none; GR = GR.NextRules)
				if (GR.bModifyDamage && GR.PreventDamage(self, instigatedBy, Damage, hitlocation, damageType, momentum))
					return;
			for (GR = Level.Game.GameRules; GR != none; GR = GR.NextRules)
				if (GR.bModifyDamage)
					GR.ModifyDamage(self, instigatedBy, Damage, hitlocation, damageType, momentum);
		}
		LastDamageInstigator = instigatedBy;
		LastDamageHitLocation = HitLocation;
		LastDamageMomentum = momentum;
		LastDamageType = damageType;
		LastDamageTime = Level.TimeSeconds;
		bLastDamageSpawnedBlood = false;

		bAlreadyDead = (Health <= 0);

		actualDamage = Level.Game.ReduceDamage(Damage, DamageType, self, instigatedBy);
		if ( bIsPlayer )
		{
			if (ReducedDamageType == 'All') //God mode
				actualDamage = 0;
			else if (Inventory != None) //then check if carrying armor
				actualDamage = Inventory.ReduceDamage(actualDamage, DamageType, HitLocation);
		}
		else
		{
			if ( (InstigatedBy != None) && (InstigatedBy.IsA(Class.Name) || IsA(InstigatedBy.Class.Name)) )
				actualDamage = actualDamage * FMin(1 - ReducedDamagePct, 0.35);
			else if ( (ReducedDamageType == 'All') || ((ReducedDamageType != '') && (ReducedDamageType == damageType)) )
				actualDamage = float(actualDamage) * (1 - ReducedDamagePct);
			if (Inventory != None)
				actualDamage = Inventory.ReduceDamage(actualDamage, DamageType, HitLocation);
		}

		if( ActiveScript.sf_NoInterruptions )
			Health = Max(Health-actualDamage,1);
		else Health-=actualDamage;
		
		if (CarriedDecoration != None)
			DropDecoration();
		if ( HitLocation == vect(0,0,0) )
			HitLocation = Location;

		if (Health > 0)
		{
			if (instigatedBy != None)
				damageAttitudeTo(instigatedBy);
			PlayHit(actualDamage, hitLocation, damageType, Momentum.Z);
		}
		else if ( !bAlreadyDead )
		{
			NextState = '';
			PlayDeathHit(actualDamage, hitLocation, damageType);
			if ( actualDamage > mass )
				Health = -1 * actualDamage;
			Enemy = instigatedBy;
			Died(instigatedBy, damageType, HitLocation);
		}
		else
		{
			if ( bIsPlayer )
			{
				HidePlayer();
				GotoState('Dying');
			}
			else Destroy();
		}
		MakeNoise(1.0);
	}
	function bool CanStartSequence( HL_ScriptedSequence S, int InterruptLevel )
	{
		return true; //bIdleSequence || S.sf_OverrideAI;
	}
	final function bool KeepMovingToPoint( vector V )
	{
		local float Dist;
		
		V-=Location;
		V.Z = 0.f;
		Dist = VSize2D(V);
		if( ForceMoveTime<Level.TimeSeconds || Dist<5.f )
		{
			Acceleration = vect(0,0,0);
			MoveSmooth(V);
			return false;
		}
		Acceleration = Normal2D(V)*FMin(Dist*2.f,GroundSpeed);
		return true;
	}
	final function bool NeedsToMoveTo( vector V )
	{
		V-=Location;
		if( Abs(V.Z)>CollisionHeight || VSize2DSq(V)>Square(5.f) )
			return true;
		V.Z = 0.f;
		MoveSmooth(V);
		return false;
	}
	final function bool NeedsToTurn( vector V )
	{
		local rotator R;
		
		R.Yaw = Rotation.Yaw;
		return (Normal2D(V-Location) Dot vector(R))<0.85f;
	}

Begin:
	if( bIdleSequence )
	{
		Acceleration = vect(0,0,0);
		Stop;
	}
	else
	{
RunScript:
		if( StartAnimSequence(ActiveScript.m_iszEntry,false) )
			FinishAnim();
		
MoveToScript:
		if( NeedsToMoveTo(ActiveScript.Location) )
		{
			if( ActiveScript.m_fMoveTo==MTT_Walk || ActiveScript.m_fMoveTo==MTT_Run )
			{
				SetPhysics(PHYS_Walking);
				if( ActiveScript.m_fMoveTo==MTT_Walk )
					TweenToWalking(0.3);
				else TweenToRunning(0.15);
				FinishAnim();
RepeatMove:
				if( ActiveScript.m_fMoveTo==MTT_Walk )
					PlayWalking();
				else PlayRunning();
				
				if( ActorReachable(ActiveScript) )
				{
					MoveToward(ActiveScript,(ActiveScript.m_fMoveTo==MTT_Walk) ? WalkingSpeed : 1.f);
					if( ActiveScript.m_iszPlay!='' || ActiveScript.m_LoopActionAnim!='' || ActiveScript.m_iszPostIdle!='' )
					{
						ForceMoveTime = Level.TimeSeconds+0.2f;
						while( KeepMovingToPoint(ActiveScript.Location) )
							Sleep(0.01f);
					}
				}
				else
				{
					MoveTarget = FindPathToward(ActiveScript);
					if( MoveTarget )
					{
						MoveToward(MoveTarget,(ActiveScript.m_fMoveTo==MTT_Walk) ? WalkingSpeed : 1.f);
						GoTo'RepeatMove';
					}
					if( FastTrace(ActiveScript.Location,Location) )
						MoveToward(ActiveScript,(ActiveScript.m_fMoveTo==MTT_Walk) ? WalkingSpeed : 1.f);
					else GoTo'TeleportMove';
				}
				Acceleration = vect(0,0,0);
				if( NeedsToTurn(ActiveScript.LookAtFocus) )
				{
					PlayTurning();
					Sleep(0.1f);
				}
				TurnTo(ActiveScript.LookAtFocus);
			}
			else if( ActiveScript.m_fMoveTo==MTT_Instant )
			{
TeleportMove:
				DesiredRotation = ActiveScript.Rotation;
				SetLocation(ActiveScript.Location,DesiredRotation);
			}
			else if( ActiveScript.m_fMoveTo==MTT_TurnTo )
			{
				Acceleration = vect(0,0,0);
				if( NeedsToTurn(ActiveScript.LookAtFocus) )
				{
					PlayTurning();
					Sleep(0.1f);
				}
				TurnTo(ActiveScript.LookAtFocus);
			}
		}
		else if( ActiveScript.m_fMoveTo==MTT_Instant )
		{
			DesiredRotation = ActiveScript.Rotation;
			SetLocation(ActiveScript.Location,DesiredRotation);
		}
		else
		{
			Acceleration = vect(0,0,0);
			if( NeedsToTurn(ActiveScript.LookAtFocus) )
			{
				PlayTurning();
				Sleep(0.1f);
			}
			TurnTo(ActiveScript.LookAtFocus);
		}
		Acceleration = vect(0,0,0);
		
RepeatScript:
		if( StartAnimSequence(ActiveScript.m_iszPlay,ActiveScript.m_LoopActionAnim) )
		{
			if( ActiveScript.m_LoopActionAnim )
				Stop;
			FinishAnim();
		}
		if( StartAnimSequence(ActiveScript.m_iszPostIdle,false) )
			FinishAnim();
		
		if( ActiveScript.Event!='' )
			TriggerEvent(ActiveScript.Event,ActiveScript,Self);
		if( ActiveScript.NextScript )
		{
			ActiveScript = ActiveScript.NextScript;
			GoTo'RunScript';
		}
		if( ScriptQue )
		{
			ActiveScript = ScriptQue;
			GoTo'RunScript';
		}
		ActiveScript = None;
		WhatToDoNext('','');
	}
}

State Dying
{
ignores SeePlayer, EnemyNotVisible, HearNoise, KilledBy, Trigger, Bump, HitWall, HeadZoneChange, FootZoneChange, ZoneChange, Falling, WarnTarget, Died, LongFall, PainTimer;

	// Prevent from speaking while dead.
	function bool CanPlaySentence( bool bForce )
	{
		return false;
	}
	function bool BarnacleCanGrip()
	{
		return false;
	}
	function bool CanStartSequence( HL_ScriptedSequence S, int InterruptLevel )
	{
		return false;
	}
}

defaultproperties
{
	DrawType=DT_Mesh
	Accelrate=3000
	VoicePitch=1
	bLeadTarget=False
	bShouldStartOnFloor=true
	//Orders="MovementTest"
}