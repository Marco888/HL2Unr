Class M_ASlave extends HL_Pawns;

var() int ClawDamage,BoltDamage;
var(Sounds) Sound AttackSounds[3],IdleTalk[8];

var int BoltIndex;

function PlayWaiting()
{
	if( !bQuiet && Rand(3)==0 )
		PlaySound(IdleTalk[Rand(8)],SLOT_Talk,0.75f,,,VoicePitch);

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
	LoopAnim('Run1');
}

function PlayWalking()
{
	LoopAnim('Walk1');
}

function TweenToRunning(float tweentime)
{
	TweenAnim('Run1',tweentime);
}

function TweenToWalking(float tweentime)
{
	TweenAnim('Walk1',tweentime);
}

function PlayRangedAttack()
{
	PlayAnim('ZapAttack1');
	Acceleration = vect(0,0,0);
	PlaySound(AttackSounds[Rand(3)],SLOT_Misc);
}

function PlayMeleeAttack()
{
	PlayAnim('Attack1');
	Acceleration = vect(0,0,0);
}

function EAttitude AttitudeToCreature(Pawn Other)
{
	if ( M_ASlave(Other) )
		return ATTITUDE_Friendly;
	if ( HL_Human(Other) )
		return ATTITUDE_Hate;
	return ATTITUDE_Ignore;
}

function PlayHeadDeath(name DamageType)
{
	PlayAnim('DieHeadShot',1.f,0.1);
}

function PlayBigDeath(name DamageType)
{
	switch( Rand(4) )
	{
	case 0:
		PlayAnim('DieSimple',1.f,0.1);
		break;
	case 1:
		PlayAnim('DieForward',1.f,0.1);
		break;
	default:
		PlayAnim('DieBackward',1.f,0.1);
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
	PlayAnim('DieSimple',1.f,0.1);
}

function PlayGutHit(float tweentime)
{
	PlayAnim('llflinch');
}

function PlayHeadHit(float tweentime)
{
	PlayAnim('flinch2');
}

function PlayLeftHit(float tweentime)
{
	PlayAnim('laflinch');
}

function PlayRightHit(float tweentime)
{
	PlayAnim('raflinch');
}

function ClawDamageTarget()
{
	if ( MeleeDamageTarget(ClawDamage, (ClawDamage * 500 * Normal(Target.Location - Location))) )
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
	else
	{
		switch( Rand(2) )
		{
		case 0:
			PlaySound(Sound'claw_miss1', SLOT_Interact);
			break;
		default:
			PlaySound(Sound'claw_miss1', SLOT_Interact);
		}
	}
}
simulated function ElectroBolt()
{
	if( Level.NetMode!=NM_DedicatedServer )
	{
		PlaySound(Sound'HL_Ambience.debris.zap4', SLOT_Interact,,,,1.f+(BoltIndex*0.1f));
		++BoltIndex;
	}
}
simulated function CleanupBolts()
{
	if( Level.NetMode!=NM_DedicatedServer )
	{
		BoltIndex = 0;
	}
}
simulated function PerformBoltAttack()
{
	if( Level.NetMode!=NM_DedicatedServer )
	{
		CleanupBolts();
		PlaySound(Sound'HL_Ambience.hassault.hw_shoot1', SLOT_Interact,,,,RandRange(1.3f,1.6f));
		PlaySound(Sound'HL_Ambience.weapons.electro4', SLOT_Talk,,,,RandRange(1.4f,1.6f));
	}
	if( Level.NetMode!=NM_Client && Target )
		Target.TakeDamage(BoltDamage, Self, Target.Location, vect(0,0,0), 'Zapped');
}

defaultproperties
{
	Mesh=Mesh'ASlaveM'
	Health=40
	MenuName="Alien Slave"
	Texture=Texture'ASV_Chrome_1'
	AnimSequence="Idle3"
	bHasRangedAttack=True
	TimeBetweenAttacks=0.4
	WalkingSpeed=0.2
	CollisionHeight=31
	CollisionRadius=18
	MeleeRange=40
	ClawDamage=10
	BoltDamage=12
	
	AttackSounds(0)=Sound'HL_Ambience.ASlave.slv_alert1'
	AttackSounds(1)=Sound'HL_Ambience.ASlave.slv_alert3'
	AttackSounds(2)=Sound'HL_Ambience.ASlave.slv_alert4'
	IdleTalk(0)=Sound'HL_Ambience.ASlave.slv_word1'
	IdleTalk(1)=Sound'HL_Ambience.ASlave.slv_word2'
	IdleTalk(2)=Sound'HL_Ambience.ASlave.slv_word3'
	IdleTalk(3)=Sound'HL_Ambience.ASlave.slv_word4'
	IdleTalk(4)=Sound'HL_Ambience.ASlave.slv_word5'
	IdleTalk(5)=Sound'HL_Ambience.ASlave.slv_word6'
	IdleTalk(6)=Sound'HL_Ambience.ASlave.slv_word7'
	IdleTalk(7)=Sound'HL_Ambience.ASlave.slv_word8'
}