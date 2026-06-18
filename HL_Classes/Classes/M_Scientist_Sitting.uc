Class M_Scientist_Sitting extends M_Scientist;

var float NextLookTimer;

singular function Falling()
{
	SetMovementPhysics();
}

function SetMovementPhysics()
{
	SetPhysics(PHYS_None);
}
function bool SetEnemy( Pawn NewEnemy )
{
	return false;
}
function AddVelocity( vector NewVelocity );

function PreSetMovement()
{
	bCanJump = false;
	bCanWalk = false;
	bCanSwim = false;
	bCanFly = false;
	MinHitWall = -0.6;
	bCanOpenDoors = false;
	bCanDoSpecial = false;
}

final function bool IsAtLeftSide( const out vector Pos )
{
	local vector X,Y,Z;
	
	GetAxes(Rotation,X,Y,Z);
	X = Normal2D(Pos-Location);
	return ((X Dot Y)<0.f);
}

function PlayWaiting()
{
	local int i;
	
	if( ReactTarget && NextLookTimer<Level.TimeSeconds && Rand(2)==0 && VSizeSq(ReactTarget.Location-Location)<Square(200.f) )
	{
		NextLookTimer = Level.TimeSeconds+5.f;
		PlayAnim(IsAtLeftSide(ReactTarget.Location) ? 'sitlookleft' : 'sitlookright',RandRange(0.8,1.1));
		return;
	}
	i = Rand(7);
	if( i<=2 )
		LoopAnim('sitting3',RandRange(0.8,1.1));
	else if( i==3 )
		LoopAnim('sitscared',RandRange(0.9,1.1));
	else LoopAnim('sitting2',RandRange(0.8,1.1));
}
function TweenToWaiting(float tweentime)
{
	TweenAnim('sitting2',tweentime);
}

function StartRoaming() // Nope!
{
	GotoState('Waiting');
}

function PlayTakeHit(float tweentime, vector HitLoc, int damage);

// Can't be bothered right now.
function bool HasAliveEnemy()
{
	Enemy = None;
	return false;
}

defaultproperties
{
	AnimSequence="sitting2"
	Health=50
	bPreDisaster=true
	Physics=PHYS_None
	SightCheckType=SEE_PlayersOnly
	bCollideWhenPlacing=false
	bCollideWorld=false
	bShouldStartOnFloor=false
}