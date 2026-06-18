// Keep players locked inside this volume.
// Useful for train ride at start of game.
class StrictMovementVolume extends Volume;

var() vector RestartOffset;
var vector prevLocation;
var rotator prevRotation;

simulated function PostBeginPlay()
{
	local Actor A;

	if( AttachTag!='' )
	{
		foreach AllActors(class'Actor',A,AttachTag)
		{
			SetOwner(A);
			prevLocation = A.Location;
			prevRotation = A.Rotation;
		}
	}
}
simulated final function vector GetTeleportLocation()
{
	return Location + (RestartOffset >> Rotation);
}
function DrawEditorSelection( Canvas C )
{
	C.Draw3DLine(MakeColor(255,255,64), GetTeleportLocation(), Location);
}
function UnTouch( Actor Other )
{
	if( Other.bIsPawn && Other.bCollideActors && PlayerPawn(Other)!=None && Pawn(Other).Health>0 )
	{
		Other.SetCollision(false);
		Other.SetLocation(GetTeleportLocation());
		Other.SetCollision(true);
	}
}
simulated function Tick( float Delta )
{
	if( Owner==None )
	{
		Disable('Tick');
		return;
	}
	if( prevLocation!=Owner.Location || prevRotation!=Owner.Rotation )
		UpdateDeltaMove();
}
simulated final function UpdateDeltaMove()
{
	local vector V,Delta;
	local rotator R;
	local PlayerPawn P;
	local bool bDeltaRot;
	
	bDeltaRot = (prevRotation!=Owner.Rotation);
	V = Owner.Location-prevLocation;
	prevLocation = Owner.Location;
	if( bDeltaRot )
	{
		R = Owner.Rotation-prevRotation;
		prevRotation = Owner.Rotation;
	}
	
	foreach TouchingActors(class'PlayerPawn',P)
		if( P.Physics==PHYS_Falling && P.bCollideActors && P.Health>0 )
		{
			if( bDeltaRot )
			{
				Delta = P.Location + V - Owner.Location;
				Delta = (Delta >> R) - Delta + V;
				P.ViewRotation += R;
			}
			else Delta = V;
			P.MoveSmooth(Delta);
		}
}

defaultproperties
{
	bStatic=false
	bNoDelete=true
	bEditorSelectRender=true
}