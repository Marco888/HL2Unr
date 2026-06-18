Class M_Barney extends HL_HumanFriendly;

var M_BarneyGun BarnGun;
var transient FX_BarneyMuzzle MuzzleFX;
var float HolsterTimer;
var(Human) repnotify bool bIsHolstered;

var(Human) name SpecialAnimEvent; // Event to send when specific animations trigger it.

replication
{
	// Variables the server should send to the client.
	reliable if ( Role==ROLE_Authority )
		bIsHolstered;
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( Level.NetMode!=NM_DedicatedServer )
	{
		if( !BarnGun )
			BarnGun = Spawn(class'M_BarneyGun');
		UpdateWeaponBase();
	}
}
simulated function Destroyed()
{
	Super.Destroyed();
	if( BarnGun )
	{
		BarnGun.Destroy();
		BarnGun = None;
	}
	if( MuzzleFX )
		MuzzleFX.Destroy();
}
simulated function OnRepNotify( name Property )
{
	if( Property=='bIsHolstered' )
		UpdateWeaponBase();
	else Super.OnRepNotify(Property);
}

// Barney doesn't greet.
function bool StrikeConvo( Pawn Other )
{
	return false;
}

simulated function UpdateWeaponBase()
{
	if( bIsHolstered )
		AttachActorToBone(BarnGun,GetBoneIndex("W_Holster"));
	else AttachActorToBone(BarnGun,GetBoneIndex("W_Hand"));
}

simulated function TickAimer( vector TargetPos )
{
	local rotator R;
	
	R.Roll = rotator(TargetPos-Location).Pitch;
	if( R.Roll>32768 )
		R.Roll-=65536;
	R.Roll = Clamp(R.Roll,-9100,11476) / 3;
	SetBoneRotation(GetBoneIndex("Bip01 Spine1"),R,AimAlpha,1);
	SetBoneRotation(GetBoneIndex("Bip01 Spine2"),R,AimAlpha,1);
	SetBoneRotation(GetBoneIndex("Bip01 Spine3"),R,AimAlpha,1);
}
simulated function EndAiming()
{
	SetBoneRotation(GetBoneIndex("Bip01 Spine1"),rot(0,0,0),0.f);
	SetBoneRotation(GetBoneIndex("Bip01 Spine2"),rot(0,0,0),0.f);
	SetBoneRotation(GetBoneIndex("Bip01 Spine3"),rot(0,0,0),0.f);
}

function DoHolster()
{
	if( !bIsHolstered )
	{
		bIsHolstered = true;
		if( BarnGun )
			UpdateWeaponBase();
	}
}
function DoUnholster()
{
	if( bIsHolstered )
	{
		bIsHolstered = false;
		if( BarnGun )
			UpdateWeaponBase();
	}
}

function AnimationEvent()
{
	TriggerEvent(SpecialAnimEvent,Self,Self);
}
simulated function DoorBang()
{
	if( Level.NetMode!=NM_DedicatedServer )
		PlaySound(Sound'latchunlocked2',SLOT_Misc);
}
simulated function ButtonPushed()
{
	if( Level.NetMode!=NM_DedicatedServer )
		PlaySound(Sound'blip1',SLOT_Misc);
}

function PlayWaiting()
{
	local int i;
	
	if( !bIsHolstered )
	{
		if( Enemy==None && HolsterTimer<Level.TimeSeconds )
			TweenAnim('shootgun',0.5f);
		else PlayAnim('disarm');
		return;
	}
	i = Rand(15);
	if( i==1 )
		LoopAnim('Idle2',RandRange(0.9,1.1));
	else if( i==2 )
		LoopAnim('Idle3',RandRange(0.9,1.1));
	else if( i==3 )
		LoopAnim('Idle4',RandRange(0.9,1.1));
	else LoopAnim('Idle1',RandRange(0.9,1.1));
}
function TweenToWaiting(float tweentime)
{
	if( bIsHolstered )
		TweenAnim('Idle1',tweentime);
	else TweenAnim('shootgun',tweentime);
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

function PlayRangedAttack()
{
	HolsterTimer = Level.TimeSeconds + RandRange(1.f,2.f);
	PlayAnim('shootgun');
	Acceleration = vect(0,0,0);
	PlaySound(Sound'ba_attack2',SLOT_Misc);
	if( Level.NetMode!=NM_DedicatedServer )
		FireWeaponFX();
	TraceAttack();
}
simulated function FireWeaponFX()
{
	if( (Level.TimeSeconds-LastRenderedTime)<5.f )
	{
		if( !MuzzleFX )
		{
			MuzzleFX = Spawn(class'FX_BarneyMuzzle');
			AttachActorToBone(MuzzleFX,GetBoneIndex("W_Hand"));
		}
		MuzzleFX.PulseFX();
	}
}
simulated function DrawTrace( Actor Other, vector HL, vector HN, vector End )
{
	local vector V;

	if( !TraceFX )
		TraceFX = Spawn(class'FX_BulletTracer');
	V = GetBoneLocation(GetBoneIndex("W_Hand"));
	TraceFX.DrawTrace(V,HL);
	Super.DrawTrace(Other,HL,HN,End);
}

function bool PrepareRangedAttack()
{
	if( bIsHolstered )
	{
		PlayAnim('draw');
		return true;
	}
	return false;
}

function EAttitude AttitudeToCreature(Pawn Other)
{
	if ( HL_HumanFriendly(Other) )
		return ATTITUDE_Friendly;
	return ATTITUDE_Hate;
}

defaultproperties
{
	Mesh=Mesh'BarneyM'
	Health=35
	MenuName="Barney"
	Texture=Texture'BX_Chrome1'
	AnimSequence="Idle1"
	bIsHolstered=True
	bHasRangedAttack=True
	TimeBetweenAttacks=0.4
	WalkingSpeed=0.2
}