Class HL_HumanFriendly extends HL_Human
	abstract;

var(Human) bool bPreDisaster; // Inherit some pre-disaster behaviour.
var Pawn ReactTarget;
var float NextGreetTimer;
var(Human) array<VoiceLineObject> PreDistGreet, IdleGreet;

function SeePlayer(Actor SeenPlayer)
{
	if (SetEnemy(Pawn(SeenPlayer)))
		LastSeenPos = Enemy.Location;
	else StrikeConvo(Pawn(SeenPlayer));
}

function EAttitude AttitudeToCreature(Pawn Other)
{
	if( HL_HumanFriendly(Other) )
		return ATTITUDE_Friendly;
	return ATTITUDE_Hate;
}

function bool StrikeConvo( Pawn Other )
{
	if( Other.Health<=0 || !Other.bIsPlayer || AttitudeToPlayer!=ATTITUDE_Friendly )
		return false;
	if( ReactTarget==Other )
	{
		if( NextGreetTimer<Level.TimeSeconds )
		{
			if( CanPlaySentence(false) )
				GreetPlayer();
			NextGreetTimer = Level.TimeSeconds+RandRange(300.f,600.f);
		}
		return true;
	}
	if( ReactTarget && (VSizeSq(ReactTarget.Location-Location)*0.75f)<VSizeSq(Other.Location-Location) )
		return false;
	ReactTarget = Other;
	if( NextGreetTimer<Level.TimeSeconds )
	{
		if( CanPlaySentence(false) )
			GreetPlayer();
		NextGreetTimer = Level.TimeSeconds+RandRange(300.f,600.f);
	}
	return true;
}

function GreetPlayer()
{
	if( bPreDisaster )
		StartVoiceSequence(PreDistGreet[Rand(PreDistGreet.Size())], ReactTarget, true);
	else StartVoiceSequence(IdleGreet[Rand(IdleGreet.Size())], ReactTarget, true);
}

function Killed(pawn Killer, pawn Other, name damageType)
{
	if( ReactTarget==Other )
		ReactTarget = None;
	Super.Killed(Killer,Other,damageType);
}

state Scripting
{
Ignores EnemyAcquired,TryToDuck,TryToCrouch;

	function bool StrikeConvo( Pawn Other )
	{
		return false;
	}
}

defaultproperties
{
	AttitudeToPlayer=ATTITUDE_Friendly
	SightCheckType=SEE_All
}