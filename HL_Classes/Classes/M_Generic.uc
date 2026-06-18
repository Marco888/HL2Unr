// Generic NPC that does nothing.
Class M_Generic extends HL_Pawns;

function PlayWaiting()
{
	LoopAnim('Idle',RandRange(0.9,1.1));
}
function TweenToWaiting(float tweentime)
{
	TweenAnim('Idle',tweentime);
}

function PlayRunning()
{
	LoopAnim('run2');
}
function PlayWalking()
{
	LoopAnim('boxwalk');
}

function TweenToRunning(float tweentime)
{
	TweenAnim('run2',tweentime);
}
function TweenToWalking(float tweentime)
{
	TweenAnim('boxwalk',tweentime);
}

defaultproperties
{
	AnimSequence="idle"
	Mesh=LoaderM
	bIsAmbientCreature=true
	Physics=PHYS_None
	CollisionRadius=12.0
	CollisionHeight=36.0
	AttitudeToPlayer=ATTITUDE_Friendly
	SightRadius=0
}