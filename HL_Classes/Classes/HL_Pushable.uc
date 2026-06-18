// func_pushable / func_breakable
Class HL_Pushable extends HL_MoverBase;

var() int Health;
var() Texture.ESurfaceTypes MaterialType;
var() class<Actor> Contents;
var() float ExplosionMag; // A non-zero value here creates an explosion when the object breaks.

var() bool bPushable; // If enable, behave like func_pushable, otherwise func_breakable.
var() bool bTriggerBreakable; // Entity can only be activated (broken) by being triggered.
var() bool bTouchBreakable; // Brush will break on touch.
var() bool bPressureBreakable; // Brush will break when pressured (e.g. player walking on it).
var() bool bCrowbarBreakable; // Whack it with a crowbar and it will break instantly (regardless of strength).

var repnotify bool bIsBroken;

function Sound GetBreakSound()
{
	switch (MaterialType)
	{
	case EST_Glass:
		switch (Rand(2))
		{
		case 0:
			return Sound'HL_Ambience.debris.bustglass1';
		default:
			return Sound'HL_Ambience.debris.bustglass2';
		}
	case EST_Wood:
		switch (Rand(2))
		{
		case 0:
			return Sound'HL_Ambience.debris.bustcrate1';
		default:
			return Sound'HL_Ambience.debris.bustcrate2';
		}
	case EST_Custom00: // computer
	case EST_Metal:
		switch (Rand(2))
		{
		case 0:
			return Sound'HL_Ambience.debris.bustmetal1';
		default:
			return Sound'HL_Ambience.debris.bustmetal2';
		}
	case EST_Flesh:
		switch (Rand(2))
		{
		case 0:
			return Sound'HL_Ambience.debris.bustflesh1';
		default:
			return Sound'HL_Ambience.debris.bustflesh2';
		}
	case EST_Rock:
	case EST_Custom01: // cinderblock
		switch (Rand(2))
		{
		case 0:
			return Sound'HL_Ambience.debris.bustconcrete1';
		default:
			return Sound'HL_Ambience.debris.bustconcrete2';
		}
	case EST_Custom02: // ceiling tile
		return Sound'HL_Ambience.debris.bustceiling';
	}
}
function Sound GetDamageSound()
{
	switch (MaterialType)
	{
	case EST_Custom00: // computer
	case EST_Glass:
		switch (Rand(3))
		{
		case 0:
			return Sound'HL_Ambience.debris.glass1';
		case 1:
			return Sound'HL_Ambience.debris.glass2';
		default:
			return Sound'HL_Ambience.debris.glass3';
		}
	case EST_Metal:
		switch (Rand(3))
		{
		case 0:
			return Sound'HL_Ambience.debris.metal1';
		case 1:
			return Sound'HL_Ambience.debris.metal2';
		default:
			return Sound'HL_Ambience.debris.metal3';
		}
	case EST_Flesh:
		switch (Rand(3))
		{
		case 0:
			return Sound'HL_Ambience.debris.flesh1';
		case 1:
			return Sound'HL_Ambience.debris.flesh2';
		case 2:
			return Sound'HL_Ambience.debris.flesh3';
		case 3:
			return Sound'HL_Ambience.debris.flesh5';
		case 4:
			return Sound'HL_Ambience.debris.flesh6';
		default:
			return Sound'HL_Ambience.debris.flesh7';
		}
	case EST_Rock:
	case EST_Custom01: // cinderblock
		switch (Rand(3))
		{
		case 0:
			return Sound'HL_Ambience.debris.concrete1';
		case 1:
			return Sound'HL_Ambience.debris.concrete2';
		default:
			return Sound'HL_Ambience.debris.concrete3';
		}
	case EST_Custom02: // ceiling tile
		return None; // UNDONE: no ceiling tile shard sound yet
	default: // default is wood.
		switch (Rand(3))
		{
		case 0:
			return Sound'HL_Ambience.debris.wood1';
		case 1:
			return Sound'HL_Ambience.debris.wood2';
		default:
			return Sound'HL_Ambience.debris.wood3';
		}
	}
}

function DoBreak( vector Momentum )
{
	local BoundingBox B;
	local int i;
	local Fragment s;
	local class<Fragment> FragType;
	local float Size;
	local vector Pos;
	
	switch (MaterialType)
	{
	case EST_Glass:
		FragType = class'GlassFragments';
		break;
	case EST_Wood:
		FragType = class'WoodFragments';
		break;
	case EST_Custom00: // computer
	case EST_Metal:
		FragType = class'Fragment1';
		break;
	case EST_Flesh:
		FragType = class'FX_MeatFragments';
		break;
	default:
		FragType = class'WallFragments';
		break;
	}
	
	B = GetBoundingBox();
	Size = VSize(B.Max-B.Min) / 50.f;
	for( i=Rand(3); i<6 ; i++ )
	{
		Pos.X = RandRange(B.Min.X,B.Max.X);
		Pos.Y = RandRange(B.Min.Y,B.Max.Y);
		Pos.Z = RandRange(B.Min.Z,B.Max.Z);
		s = Spawn(FragType,,,Pos,RotRand(true));
		if( s )
		{
			s.CalcVelocity(Momentum,0);
			s.DrawScale = Size * RandRange(0.5,1.25);
		}
	}
	
	PlaySound(GetBreakSound(),SLOT_None,,,,RandRange(0.9,1.2));
	bIsBroken = true;
	bForceNetUpdate = true;
	HideMover();
	GoToState('Dead');
}

simulated final function HideMover()
{
	SetCollision(false);
	bHidden = true;
	SetPhysics(PHYS_None);
	SetLocation(vect(60000.f,60000.f,-60000.f));
}

state() Deco
{
	function Trigger( actor Other, pawn EventInstigator )
	{
		if( bTriggerBreakable )
			DoBreak(VRand()*15.f);
	}
	function Bump( actor Other )
	{
		if( bTouchBreakable )
		{
			if( Other.bIsPawn && Pawn(Other).bIsPlayer )
			{
				DoBreak(Other.Velocity);
				return;
			}
		}
	}
	function Attach( actor Other )
	{
		if( bPressureBreakable )
		{
			if( Other.bIsPawn && Pawn(Other).bIsPlayer )
			{
				DoBreak(Other.Velocity);
				return;
			}
		}
	}
	function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation, Vector momentum, name damageType)
	{
		if( bDamageTriggered )
		{
			PlaySound(GetDamageSound(),SLOT_None,,,,RandRange(0.9,1.2));
			if( bCrowbarBreakable && damageType=='Crowbar' )
			{
				DoBreak(momentum / 10.f);
				return;
			}
			Health-=Damage;
			if( Health<=0 )
			{
				DoBreak(momentum / 20.f);
				return;
			}
		}
	}
}

state Dead
{
Ignores Trigger,Bump,TakeDamage,Attach;
}

defaultproperties
{
	InitialState="Deco"
	MaterialType=EST_Glass
	bPushable=true
}