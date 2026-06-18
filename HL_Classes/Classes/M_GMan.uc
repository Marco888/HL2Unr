Class M_GMan extends HL_HumanFriendly;

function PlayWaiting()
{
	local int i;
	
	i = Rand(13);
	if( i<=3 )
		LoopAnim('idle01',RandRange(0.9,1.1));
	else if( i<=7 )
		LoopAnim('idle02',RandRange(0.9,1.1));
	else if( i==8 )
		LoopAnim('idlebrush',RandRange(0.9,1.1));
	else if( i==9 )
		LoopAnim('idlelook',RandRange(0.9,1.1));
	else LoopAnim('stand',RandRange(0.9,1.1));
}
function TweenToWaiting(float tweentime)
{
	TweenAnim('idle01',tweentime);
}

function PlayRunning()
{
	LoopAnim('Walk');
}

function PlayWalking()
{
	LoopAnim('Walk');
}

function TweenToRunning(float tweentime)
{
	TweenAnim('Walk',tweentime);
}

function TweenToWalking(float tweentime)
{
	TweenAnim('Walk',tweentime);
}

function EAttitude AttitudeWithFear()
{
	return ATTITUDE_Friendly;
}
function EAttitude AttitudeToCreature(Pawn Other)
{
	return ATTITUDE_Friendly;
}
function damageAttitudeTo(pawn Other)
{
}

defaultproperties
{
	Mesh=Mesh'GManM'
	Health=100
	MenuName="G-Man"
	AnimSequence="idle01"
	WalkingPct=1
	WalkingSpeed=1
	GroundSpeed=115
}