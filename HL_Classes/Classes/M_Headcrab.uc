Class M_Headcrab extends HL_Pawns;

var() int ClawDamage;
var(Sounds) Sound AttackSounds[3],IdleTalk[5],DieSounds[2];

function PlayWaiting()
{
	if( !bQuiet && Rand(3)==0 )
		PlaySound(IdleTalk[Rand(5)],SLOT_Talk,0.75f,,,VoicePitch);

	switch( Rand(3) )
	{
	case 0:
		LoopAnim('Idle1',RandRange(0.9,1.1));
		break;
	case 1:
		LoopAnim('Idle2',RandRange(0.9,1.1));
		break;
	default:
		LoopAnim('Idle3',RandRange(0.9,1.1));
	}
}
function TweenToWaiting(float tweentime)
{
	TweenAnim('Idle1',tweentime);
}

function PlayRunning()
{
	LoopAnim('Run');
}

function PlayWalking()
{
	LoopAnim('Walk');
}

function TweenToRunning(float tweentime)
{
	TweenAnim('Run',tweentime);
}

function TweenToWalking(float tweentime)
{
	TweenAnim('Walk',tweentime);
}

function PlayMeleeAttack()
{
	local float dist;
	local vector AimDir;

	if ( Target==None || Physics==PHYS_Falling )
	{
		PlayWaiting();
		return;
	}
	dist = VSize(Target.Location - Location);

	PlaySound(AttackSounds[Rand(3)],SLOT_Misc);
	Enable('Bump');
	switch( Rand(3) )
	{
	case 0:
		PlayAnim('Jump');
		break;
	case 1:
		PlayAnim('Jump_Variation1');
		break;
	default:
		PlayAnim('Jump_Variation2');
	}
	
	if( (Target.Location.Z-Target.CollisionHeight)>Location.Z ) // Aim higher when lunging uphill.
		AimDir = Target.Location + Target.CollisionHeight * vect(0,0,1.25);
	else AimDir = Target.Location + Target.CollisionHeight * vect(0,0,0.75);
	
	if( Rand(3) )
		AimDir+=Target.Velocity*((dist/600.f)*(FRand()*0.4f+0.75f));
	Velocity = 600 * Normal(AimDir - Location);
	
	if (dist > CollisionRadius + Target.CollisionRadius + 35)
		Velocity.Z += 0.7 * dist;
	SetPhysics(PHYS_Falling);
	GoToState('MeleeAttack','WaitForAttack'); // 227j: Don't let challenge anim interrupt this melee move.
}

function EAttitude AttitudeToCreature(Pawn Other)
{
	if ( M_Headcrab(Other) )
		return ATTITUDE_Friendly;
	return ATTITUDE_Hate;
}

function PlayHeadDeath(name DamageType)
{
	PlayBigDeath(DamageType);
}

function PlayBigDeath(name DamageType)
{
	PlaySound(DieSounds[Rand(2)],SLOT_Pain);
	PlayAnim('DieBack',1.f,0.1);
}

function PlayLeftDeath(name DamageType)
{
	PlayBigDeath(DamageType);
}

function PlayRightDeath(name DamageType)
{
	PlayBigDeath(DamageType);
}

function PlayGutDeath(name DamageType)
{
	PlayBigDeath(DamageType);
}

function PlayGutHit(float tweentime)
{
	PlayAnim('Flinch');
}

function PlayHeadHit(float tweentime)
{
	PlayAnim('Flinch');
}

function PlayLeftHit(float tweentime)
{
	PlayAnim('Flinch');
}

function PlayRightHit(float tweentime)
{
	PlayAnim('Flinch');
}

function bool MeleeDamageTarget(int hitdamage, vector pushdir)
{
	local vector HitLocation;

	if ( Target==self )
		Target = none;
	if ( !Target && HasAliveEnemy() )   // allow non pawn targets
		Target = Enemy;
	if ( !IsLiveActor(Target) || (Target.bIsPawn && Pawn(Target).Health<=0) )
		return false;

	if ( FastTrace(Target.Location, Location) && Target.TraceThisActor(Target.Location,Location,HitLocation) )
	{
		Target.TakeDamage(hitdamage, Self, HitLocation, pushdir, 'hacked');
		return true;
	}
	return false;
}

function bool IsInMeleeRange( Pawn Other )
{
	return VSizeSq(Location - Other.Location) < Square(MeleeRange + CollisionRadius + Other.CollisionRadius);
}

state MeleeAttack
{
ignores SeePlayer, HearNoise, Falling;

	singular function Bump(actor Other)
	{
		Disable('Bump');
		if ( (Other == Target) && (AnimSequence=='Jump' || AnimSequence=='Jump_Variation1' || AnimSequence=='Jump_Variation2') && MeleeDamageTarget(ClawDamage, vect(0,0,0)))
			PlaySound(Sound'hc_headbite', SLOT_Interact);
	}
}

defaultproperties
{
	Mesh=Mesh'HeadcrabM'
	Health=15
	MenuName="Headcrab"
	AnimSequence="Idle3"
	bHasRangedAttack=False
	WalkingSpeed=0.33
	CollisionHeight=8
	CollisionRadius=12
	MeleeRange=300
	ClawDamage=10
	Mass=15
	Buoyancy=15
	bIsCrawler=True
	
	AttackSounds(0)=Sound'HL_Ambience.Headcrab.hc_attack1'
	AttackSounds(1)=Sound'HL_Ambience.Headcrab.hc_attack2'
	AttackSounds(2)=Sound'HL_Ambience.Headcrab.hc_attack3'
	IdleTalk(0)=Sound'HL_Ambience.Headcrab.hc_idle1'
	IdleTalk(1)=Sound'HL_Ambience.Headcrab.hc_idle2'
	IdleTalk(2)=Sound'HL_Ambience.Headcrab.hc_idle3'
	IdleTalk(3)=Sound'HL_Ambience.Headcrab.hc_idle4'
	IdleTalk(4)=Sound'HL_Ambience.Headcrab.hc_idle5'
	DieSounds(0)=Sound'HL_Ambience.Headcrab.hc_die1'
	DieSounds(1)=Sound'HL_Ambience.Headcrab.hc_die2'
}