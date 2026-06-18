class HL_CoopPlayerStart extends PlayerStart;

function PostBeginPlay()
{
	if( !bEnabled )
	{
		bSinglePlayerStart = false;
		bCoopStart = false;
	}
}
function Trigger( actor Other, pawn EventInstigator )
{
	local PlayerStart P;

	bEnabled = true;
	bSinglePlayerStart = true;
	bCoopStart = true;
	
	foreach AllActors(class'PlayerStart',P)
		if( P.Tag!=Tag )
		{
			P.bEnabled = false;
			P.bSinglePlayerStart = false;
			P.bCoopStart = false;
		}
}

defaultproperties
{
	bEnabled=false
}