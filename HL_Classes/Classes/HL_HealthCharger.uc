class HL_HealthCharger extends HL_TriggerButton;

var() Sound ChargeSound,BeginCharge,ChargeFinished;
var() int chargerJuice[3]; // Easy/Medium/Hard amount it can heal.
var int currentJuice;
var bool bCharging;

function BeginPlay()
{
	local int i;
	
	if( Level.Game )
	{
		i = int(Level.Game.Difficulty);
		currentJuice = chargerJuice[Min(i,2)];
	}
}
function GrabbedBy( Pawn Other );

final function bool CanHeal( Pawn Other )
{
	return (Other.bIsPlayer && Other.Health>0 && Other.Health<Other.Default.Health);
}
function Touch( Actor Other )
{
	if( bTouchActivate && Other.bIsPawn && CanHeal(Pawn(Other)) )
		Activate(Pawn(Other));
}
function Activate( Pawn Other )
{
	if( currentJuice==0 || Other.FindInventoryType(Class'INV_HevSuit')==None )
	{
		if( nextTriggerTime<Level.TimeSeconds )
		{
			PlaySound(ChargeFinished);
			nextTriggerTime = Level.TimeSeconds + 0.25f;
		}
		return;
	}
	if( !bCharging )
	{
		if( nextTriggerTime<Level.TimeSeconds )
		{
			PlaySound(BeginCharge);
			nextTriggerTime = Level.TimeSeconds + 0.25f;
		}
		AmbientSound = ChargeSound;
		bCharging = true;
		SetTimer(0.1,true);
	}
}
function Timer()
{
	local Pawn P,Best;
	local int Score,BestScore;
	
	foreach TouchingActors(class'Pawn',P)
	{
		if( CanHeal(P) && bool(P.FindInventoryType(Class'INV_HevSuit')) )
		{
			// If multiple players are touching this, desire one with lowest health.
			Score = P.Health+Rand(50);
			if( !Best || Score<BestScore )
			{
				Best = P;
				BestScore = Score;
			}
		}
	}
	if( Best )
	{
		++Best.Health;
		--currentJuice;
		if( currentJuice<=0 )
		{
			SwitchToMaterial(1);
			PlaySound(ChargeFinished);
			AmbientSound = None;
			SetTimer(0,false);
			bCharging = false;
			return;
		}
	}
	else
	{
		AmbientSound = None;
		bCharging = false;
		SetTimer(0,false);
	}
}

defaultproperties
{
	chargerJuice[0]=50
	chargerJuice[1]=40
	chargerJuice[2]=25
	ChargeSound=Sound'HL_Ambient.items.medcharge4'
	BeginCharge=Sound'HL_Ambient.items.medshot4'
	ChargeFinished=Sound'HL_Ambient.items.medshotno1'
	bTouchActivate=true
	SoundVolume=255
	SoundRadius=16
	bStatic=false
	bNoDelete=true
}