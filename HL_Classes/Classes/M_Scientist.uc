Class M_Scientist extends HL_HumanFriendly;

var(Human) byte BodyType;

simulated function PostBeginPlay()
{
	if( Level.NetMode!=NM_Client )
	{
		switch( BodyType )
		{
		case 0:
			VoicePitch = 1.05f;
			Mesh = SkeletalMesh'Scientist3M';
			break;	//glasses
		case 1:
			break;	//einstein
		case 2:
			VoicePitch = 0.95f;
			Mesh = SkeletalMesh'Scientist2M';
			break;	//luther
		default:
			Mesh = SkeletalMesh'Scientist4M'; //slick
		}
	}
	Super.PostBeginPlay();
}

function PlayWaiting()
{
	local int i;
	
	i = Rand(18);
	if( i==1 )
		LoopAnim('Idle3',RandRange(0.9,1.1));
	else if( i==2 )
		LoopAnim('Idle4',RandRange(0.9,1.1));
	else if( i==3 )
		LoopAnim('Idle5',RandRange(0.9,1.1));
	else if( i==4 )
		LoopAnim('Idle6',RandRange(0.9,1.1));
	else if( i==5 )
		LoopAnim('Idle7',RandRange(0.9,1.1));
	else LoopAnim('Idle1',RandRange(0.9,1.1));
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

function PlayFearSound()
{
	if ( (Threaten != None) && (FRand() < 0.4) )
	{
		PlaySound(Threaten, SLOT_Talk,, true);
		return;
	}
	if (Fear != None)
		PlaySound(Fear, SLOT_Talk,, true);
}

function TweenToFighter(float tweentime)
{
	TweenAnim('Crouch', tweentime);
}

function PlayThreatening()
{
	local int i;

	Acceleration = vect(0,0,0);
	if( AnimSequence=='crouch_idle' || AnimSequence=='crouch_idle2' || AnimSequence=='crouch_idle3' || AnimSequence=='crouch_idle3_2' || (AnimSequence=='Crouch' && AnimFrame>0.5) )
	{
		i = Rand(4);
		switch( i )
		{
		case 0:
			LoopAnim('crouch_idle', RandRange(0.8,1.1), 0.1);
			break;
		case 1:
			LoopAnim('crouch_idle2', RandRange(0.8,1.1), 0.1);
			break;
		case 2:
			LoopAnim('crouch_idle3', RandRange(0.8,1.1), 0.1);
			break;
		default:
			LoopAnim('crouch_idle3_2', RandRange(0.8,1.1), 0.1);
		}
	}
	else PlayAnim('Crouch');
}

function PlayHeadDeath(name DamageType)
{
	PlayAnim('headshot',1.f,0.1);
}

function PlayBigDeath(name DamageType)
{
	local int i;
	
	i = Rand(4);
	switch( i )
	{
	case 0:
		PlayAnim('diesimple',1.f,0.1);
		break;
	case 1:
		PlayAnim('diebackward',1.f,0.1);
		break;
	case 2:
		PlayAnim('dieforward',1.f,0.1);
		break;
	default:
		PlayAnim('dieforward1',1.f,0.1);
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
	PlayAnim('Gutshot',1.f,0.1);
}

function PlayLanded(float impactVel)
{
	TweenAnim('panic', 0.1);
}

function PlayMeleeAttack()
{
	PlayThreatening();
}

function PlayRangedAttack()
{
	PlayThreatening();
}

function PlayGutHit(float tweentime)
{
	PlayAnim('llflinch');
}

function PlayHeadHit(float tweentime)
{
	PlayAnim('flinch1');
}

function PlayLeftHit(float tweentime)
{
	PlayAnim('laflinch');
}

function PlayRightHit(float tweentime)
{
	PlayAnim('raflinch');
}

function EAttitude AttitudeWithFear()
{
	return ATTITUDE_Fear;
}
function EAttitude AttitudeToCreature(Pawn Other)
{
	if ( HL_HumanFriendly(Other) )
		return ATTITUDE_Friendly;
	return ATTITUDE_Fear;
}
function damageAttitudeTo(pawn Other)
{
	if ( (Other == Self) || (Other == None) || Other.bIsAmbientCreature )
		return;
	if ( Other.bIsPlayer ) //change attitude to player
		AttitudeToPlayer = ATTITUDE_Fear;
	SetEnemy(Other);
}

defaultproperties
{
	Mesh=Mesh'Scientist1M'
	Health=35
	MenuName="Scientist"
	AnimSequence="Idle1"
	bIsWuss=true
	WalkingSpeed=0.25
	PreDistGreet.Add(VX_SC_PIDLE0)
	PreDistGreet.Add(VX_SC_PIDLE1)
	PreDistGreet.Add(VX_SC_PIDLE2)
	PreDistGreet.Add(VX_SC_PIDLE3)
	PreDistGreet.Add(VX_SC_PIDLE4)
	PreDistGreet.Add(VX_SC_PIDLE5)
	PreDistGreet.Add(VX_SC_PIDLE6)
	PreDistGreet.Add(VX_SC_PIDLE7)
	PreDistGreet.Add(VX_SC_PIDLE8)
	PreDistGreet.Add(VX_SC_PIDLE9)
	PreDistGreet.Add(VX_SC_PIDLE10)
	IdleGreet.Add(VX_SC_IDLE0)
	IdleGreet.Add(VX_SC_IDLE1)
	IdleGreet.Add(VX_SC_IDLE2)
	IdleGreet.Add(VX_SC_IDLE3)
	IdleGreet.Add(VX_SC_IDLE4)
	IdleGreet.Add(VX_SC_IDLE5)
	IdleGreet.Add(VX_SC_IDLE6)
	IdleGreet.Add(VX_SC_IDLE7)
	IdleGreet.Add(VX_SC_IDLE8)
	IdleGreet.Add(VX_SC_IDLE9)
	IdleGreet.Add(VX_SC_IDLE10)
	IdleGreet.Add(VX_SC_IDLE11)
	IdleGreet.Add(VX_SC_IDLE12)
	IdleGreet.Add(VX_SC_IDLE13)
}