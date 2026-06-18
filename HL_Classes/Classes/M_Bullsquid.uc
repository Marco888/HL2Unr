Class M_Bullsquid extends HL_Pawns;

var() int BiteDamage,SlapDamage;
var(Sounds) Sound AttackSounds[3],IdleTalk[5],DieSounds[3];
var Pawn LastBiteTarget;

function PlayWaiting()
{
	if( !bQuiet && Rand(3)==0 )
		PlaySound(IdleTalk[Rand(5)],SLOT_Talk,0.75f,,,VoicePitch);

	switch( Rand(4) )
	{
	case 0:
		LoopAnim('InspectDown',RandRange(0.9,1.1));
		break;
	default:
		LoopAnim('Idle',RandRange(0.9,1.1));
	}
}
function TweenToWaiting(float tweentime)
{
	TweenAnim('Idle',tweentime);
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
	switch( Rand(2) )
	{
	case 0:
		PlaySound(Sound'HL_Ambience.bullchicken.bc_attackgrowl', SLOT_Talk);
		break;
	case 1:
		PlaySound(Sound'HL_Ambience.bullchicken.bc_attackgrowl2', SLOT_Talk);
		break;
	default:
		PlaySound(Sound'HL_Ambience.bullchicken.bc_attackgrowl2', SLOT_Talk);
	}

	if( Target && Target.bIsPawn && Pawn(Target).Health<=SlapDamage )
		PlayAnim('Whip');
	else PlayAnim('Bite');
}

function PlayRangedAttack()
{
	PlayAnim('Range');
	Acceleration = vect(0,0,0);
	PlaySound(AttackSounds[Rand(3)],SLOT_Misc);
}

function EAttitude AttitudeToCreature(Pawn Other)
{
	if ( M_Bullsquid(Other) )
		return ATTITUDE_Friendly;
	if ( M_Headcrab(Other) || HL_Human(Other) )
		return ATTITUDE_Hate;
	return ATTITUDE_Ignore;
}

function PlayHeadDeath(name DamageType)
{
	PlayBigDeath(DamageType);
}

function PlayBigDeath(name DamageType)
{
	if( Rand(2)==0 )
	{
		PlaySound(DieSounds[Rand(2)],SLOT_Pain);
		PlayAnim('Die');
	}
	else
	{
		PlaySound(DieSounds[2],SLOT_Pain);
		PlayAnim('Die1');
	}
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
	PlayAnim('Flinchb');
}

function PlayHeadHit(float tweentime)
{
	PlayAnim('Flinchs');
}

function PlayLeftHit(float tweentime)
{
	PlayAnim('Flinchb');
}

function PlayRightHit(float tweentime)
{
	PlayAnim('Flinchs');
}

function SquidBiteTarget()
{
	if( AnimSequence=='Whip' )
	{
		if ( MeleeDamageTarget(SlapDamage, (SlapDamage * 500 * Normal(Target.Location - Location))) )
		{
			switch( Rand(3) )
			{
			case 0:
				PlaySound(Sound'claw_strike1', SLOT_Interact);
				break;
			case 1:
				PlaySound(Sound'claw_strike2', SLOT_Interact);
				break;
			default:
				PlaySound(Sound'claw_strike3', SLOT_Interact);
			}
		}
	}
	else if( MeleeDamageTarget(BiteDamage, (BiteDamage * 100 * Normal(Target.Location - Location))) )
	{
		LastBiteTarget = Pawn(Target);
	}
}
function SquidBiteThrow()
{
	if( LastBiteTarget )
		LastBiteTarget.AddVelocity(Normal(LastBiteTarget.Location - Location) * 150.f);
	switch( Rand(2) )
	{
	case 0:
		PlaySound(Sound'bc_bite2', SLOT_Interact);
		break;
	default:
		PlaySound(Sound'bc_bite3', SLOT_Interact);
	}
}
function SquidSpit()
{
	Spawn(class'DispersionAmmo');
}

defaultproperties
{
	Mesh=Mesh'BullsquidM'
	Health=60
	MenuName="Bullsquid"
	AnimSequence="Idle1"
	bHasRangedAttack=True
	WalkingSpeed=0.33
	CollisionHeight=26
	CollisionRadius=24
	MeleeRange=50
	Mass=120
	Buoyancy=120
	bIsCrawler=True
	BiteDamage=25
	SlapDamage=35
	
	AttackSounds(0)=Sound'HL_Ambience.bullchicken.bc_attack1'
	AttackSounds(1)=Sound'HL_Ambience.bullchicken.bc_attack2'
	AttackSounds(2)=Sound'HL_Ambience.bullchicken.bc_attack3'
	IdleTalk(0)=Sound'HL_Ambience.bullchicken.bc_idle1'
	IdleTalk(1)=Sound'HL_Ambience.bullchicken.bc_idle2'
	IdleTalk(2)=Sound'HL_Ambience.bullchicken.bc_idle3'
	IdleTalk(3)=Sound'HL_Ambience.bullchicken.bc_idle4'
	IdleTalk(4)=Sound'HL_Ambience.bullchicken.bc_idle5'
	DieSounds(0)=Sound'HL_Ambience.bullchicken.bc_die1'
	DieSounds(1)=Sound'HL_Ambience.bullchicken.bc_die3'
	DieSounds(2)=Sound'HL_Ambience.bullchicken.bc_die2'
}