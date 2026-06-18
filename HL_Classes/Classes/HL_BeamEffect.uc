Class HL_BeamEffect extends NetworkBeamEmitter;

var() float BeamRadius; // If the Start Entity and/or Ending Entity values are omitted, this radius determines the area within which the endpoints will randomly strike. A new random position will be appointed for every strike.
var() name BeamStartTag,BeamEndTag; // Entity that the beam starts/ends at. If omitted the entity will instead use a random position on any surface within its Radius value. A new random position will be appointed for every strike. See usage note above.
var() float BeamLifeTime; // Amount of time before the beam dies. Setting to zero will make the beam stay forever.
var() float RestrikeTime; // Refire time between random strikes of the beam. Only used if the 'Random Strike' spawnflag is set.
var() vector BeamEndPointVec; // If a Ending Entity entity is not specified, use this point as the destination instead. If the beam moves, the destination point will move along with it.
var() int BeamDamage; // How much damage this beam does per second to things it hits when it is continually on, or instantaneously if it strikes.

var() enum ETouchType
{
	BTT_None,
	BTT_Players,
	BTT_NPC,
	BTT_PlayersNPC,
	BTT_PlayersNPCProps,
} TouchType;
var() enum EBeamClipMode
{
	CLIP_None,
	CLIP_BSP,
	CLIP_AllColliding,
} ClipMode;

var() bool bInitiallyOn; // Beam is initially on.
var() bool bToggleBeam; // Used to toggle the beam on and off.
var() bool bRandomStrike; // When the beam has died, it will strike again randomly within the time set by Strike again time (secs).

var bool bBeamActive;
var Actor BeamStartActor,BeamEndActor;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	BeamTarget.SetSize(1);
	if( BeamStartTag!='' )
	{
		foreach AllActors(class'Actor',BeamStartActor,BeamStartTag)
			break;
		if( Level.NetMode!=NM_DedicatedServer )
		{
			SetLocation(BeamStartActor.Location);
			SetBase(BeamStartActor);
		}
	}
	if( BeamEndTag!='' )
	{
		foreach AllActors(class'Actor',BeamEndActor,BeamEndTag)
			break;
		
		if( BeamEndActor )
		{
			if( ClipMode==CLIP_None )
			{
				BeamTargetType = BEAM_BeamActor;
				BeamTarget[0].TargetActor = BeamEndActor;
				BeamTarget[0].Offset = vect(0,0,0);
			}
			else
			{
				BeamTarget[0].Offset = BeamEndActor.Location;
			}
		}
	}
	if( BeamDamage<=0 || Level.NetMode==NM_Client )
		TouchType = BTT_None;

	bBeamActive = bInitiallyOn;
	if( bToggleBeam )
	{
		if( !bBeamActive )
			bDisabled = true;
		LifetimeRange.Min = 0.06f;
		LifetimeRange.Max = 0.1f;
		InitialState = 'ToggledBeam';
	}
	else
	{
		if( BeamLifeTime<=0.f )
		{
			LifetimeRange.Min = 0.1f;
			LifetimeRange.Max = 0.15f;
			bSpawnInitParticles = false;
		}
		else
		{
			LifetimeRange.Min = BeamLifeTime;
			LifetimeRange.Max = BeamLifeTime+0.2f;
			bRespawnParticles = false;
			bSpawnInitParticles = false;
		}
		InitialState = 'SingleBeam';
	}
}
simulated function OnRepNotify( name Property )
{
	if( Property=='RepCounter' )
	{
		if( bToggleBeam )
		{
			if( RepCounter==1 )
				EnableEmitter();
			else DisableEmitter();
		}
		else FireEmitter();
	}
}

simulated function Timer()
{
	TraceDamage(0.05f);
}
simulated function EnableEmitter()
{
	if( Level.NetMode!=NM_Client )
	{
		RepCounter = 1;
		bForceNetUpdate = true;
	}
	bBeamActive = true;
	TraceDamage(0.05f);
	SetTimer(0.05,true);
	if( Level.NetMode==NM_DedicatedServer )
		return;
	bDisabled = false;
	SpawnParticles(1);
}
simulated function DisableEmitter()
{
	if( Level.NetMode!=NM_Client )
	{
		RepCounter = 0;
		bForceNetUpdate = true;
	}
	bBeamActive = false;
	SetTimer(0,false);
	if( Level.NetMode==NM_DedicatedServer )
		return;
	if( !bDisabled )
		bDisabled = true;
}
simulated function FireEmitter()
{
	if( Level.NetMode!=NM_Client )
	{
		if ( ++RepCounter==255 )
			RepCounter = 1;
		bForceNetUpdate = true;
	}
	TraceDamage(1.f);
	if( Level.NetMode!=NM_DedicatedServer )
		SpawnParticles(1);
}

simulated final function TraceDamage( float Delta )
{
	local vector End,HL,HN;
	local Actor A;
	local int Dmg;

	if( BeamEndActor )
	{
		End = BeamEndActor.Location;
		if( BeamTargetType!=BEAM_BeamActor )
			BeamTarget[0].Offset = End;
	}
	else
	{
		End = Location+VRand()*BeamRadius;
		BeamTarget[0].Offset = End;
	}
	
	if( TouchType==BTT_None )
	{
		if( ClipMode==CLIP_None )
			return; // Do not need to trace anything.
		
		foreach TraceActors(Class'Actor',A,HL,HN,End,Location)
		{
			if( ClipMode==CLIP_BSP )
			{
				if( A.bWorldGeometry )
				{
					BeamTarget[0].Offset = HL;
					return;
				}
			}
			else if( A.bWorldGeometry || A.bBlockActors || A.bBlockPlayers || A.bProjTarget )
			{
				BeamTarget[0].Offset = HL;
				return;
			}
		}
	}
	Dmg = Max(float(BeamDamage)*Delta,1);
	if( ClipMode==CLIP_None ) // Only trace for damage.
	{
		foreach TraceActors(Class'Actor',A,HL,HN,End,Location,,TRACE_ProjTargets)
		{
			switch( TouchType )
			{
			case BTT_Players:
				if( A.bIsPawn && Pawn(A).bIsPlayer )
					A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
				break;
			case BTT_NPC:
				if( A.bIsPawn && !Pawn(A).bIsPlayer )
					A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
				break;
			case BTT_PlayersNPC:
				if( A.bIsPawn )
					A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
				break;
			default:
				A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
			}
		}
	}
	else // Trace damage AND halt beam.
	{
		foreach TraceActors(Class'Actor',A,HL,HN,End,Location)
		{
			if( A.bProjTarget || A.bBlockActors )
			{
				switch( TouchType )
				{
				case BTT_Players:
					if( A.bIsPawn && Pawn(A).bIsPlayer )
						A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
					break;
				case BTT_NPC:
					if( A.bIsPawn && !Pawn(A).bIsPlayer )
						A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
					break;
				case BTT_PlayersNPC:
					if( A.bIsPawn )
						A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
					break;
				default:
					A.TakeDamage(Dmg, None, HL, vect(0,0,0), 'zapped');
				}
			}
			if( ClipMode==CLIP_BSP )
			{
				if( A.bWorldGeometry )
				{
					BeamTarget[0].Offset = HL;
					return;
				}
			}
			else if( A.bWorldGeometry || A.bBlockActors || A.bBlockPlayers || A.bProjTarget )
			{
				BeamTarget[0].Offset = HL;
				return;
			}
		}
	}
}

state ToggledBeam
{
	function Trigger( Actor Other, Pawn EventInstigator )
	{
		if( bBeamActive )
			DisableEmitter();
		else EnableEmitter();
	}
Begin:
	if( bBeamActive )
		EnableEmitter();
	else DisableEmitter();
}
state SingleBeam
{
	function Trigger( Actor Other, Pawn EventInstigator )
	{
		bBeamActive = !bBeamActive;
		GoToState(,'Begin');
	}
Begin:
	while( bBeamActive )
	{
		FireEmitter();
		if( bRandomStrike )
			Sleep(RestrikeTime);
		else Stop;
	}
}

defaultproperties
{
	bEndPointFixed=True
	RemoteRole=ROLE_SimulatedProxy
	bAlwaysRelevant=true
	TriggerAction=ETR_ToggleDisabled
	BeamTargetType=BEAM_OffsetAsAbsolute
	bDirectional=false
	BeamTarget(0)=(Offset=(X=1,Y=0,Z=0))
}