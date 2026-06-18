// func_rotating
Class HL_MoveRotating extends HL_MoverBase;

var() float m_flFanFriction; // Accel/deaccel speed when shutting of or turning on.
var() float m_flFanSpeed; // Speed as in real degrees per second.
var() float m_flPitch; // Fan sound pitch.
var() vector RotationAxis;
var float curSpeed, initialVol;

var() bool bHurtOnTouch, bInitiallyActive;

simulated function PostBeginPlay()
{
	initialVol = float(SoundVolume);
	if( bInitiallyActive && !bCollideActors && (Tag=='' || Tag==Class.Name) )
	{
		Role = ROLE_Authority;
		bOpening = true;
		if( Level.NetMode!=NM_DedicatedServer )
			UpdateSpeed(m_flFanSpeed);
	}
	else
	{
		if( Level.NetMode!=NM_Client )
		{
			RemoteRole = ROLE_SimulatedProxy;
			bOpening = bInitiallyActive;
			if( bInitiallyActive )
				UpdateSpeed(m_flFanSpeed);
		}
	}
}
simulated final function UpdateSpeed( float newSpeed )
{
	curSpeed = newSpeed;
	if( newSpeed==0.f )
	{
		RotationRate = rot(0,0,0);
		SetPhysics(PHYS_None);
		if( Level.NetMode!=NM_DedicatedServer )
			AmbientSound = None;
	}
	else
	{
		newSpeed *= {65536.f / 360.f};
		RotationRate.Pitch = RotationAxis.X*newSpeed;
		RotationRate.Yaw = RotationAxis.Y*newSpeed;
		RotationRate.Roll = RotationAxis.Z*newSpeed;
		SetPhysics(PHYS_Rotating);
		
		if( Level.NetMode!=NM_DedicatedServer )
		{
			AmbientSound = MoveAmbientSound;
			if( AmbientSound )
			{
				SoundPitch = Max(curSpeed / m_flFanSpeed * m_flPitch * 64.f,10);
				SoundVolume = (curSpeed / m_flFanSpeed * initialVol);
			}
		}
	}
}
function bool EncroachingOn( actor Other )
{
	if( Other.bIsPawn )
		Bump(Other);
	return Super.EncroachingOn(Other);
}
function Trigger( actor Other, pawn EventInstigator )
{
	bOpening = !bOpening;
	Enable('Tick');
}
function Bump( actor Other )
{
	local int dmg;
	
	// calculate damage based on rotation speed
	dmg = curSpeed / 10;
	if( dmg>0 )
		Other.TakeDamage(dmg, None, Other.Location, Normal(Other.Location-Location)*dmg*1000.f, 'Crushed');
}

function Tick( float Delta )
{
	if( bOpening )
	{
		if( curSpeed>=m_flFanSpeed )
		{
			Disable('Tick');
			return;
		}
		UpdateSpeed(FMin(curSpeed+m_flFanFriction*10.f*Delta,m_flFanSpeed));
	}
	else
	{
		if( curSpeed<=0.f )
		{
			Disable('Tick');
			return;
		}
		UpdateSpeed(FMax(curSpeed-m_flFanFriction*10.f*Delta,0.f));
	}
}

defaultproperties
{
	InitialState=""
	m_flFanFriction=1
	m_flFanSpeed=1
	m_flPitch=1
	Physics=PHYS_None
	MoverEncroachType=ME_IgnoreWhenEncroach
	bRepAmbientSound=false
	RemoteRole=ROLE_None
}