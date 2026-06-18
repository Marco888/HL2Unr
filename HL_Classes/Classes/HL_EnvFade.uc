Class HL_EnvFade extends HL_Triggers;

var() float Duration; // The time that it will take to fade the screen in or out.
var() float HoldTime; // The time to hold the faded in/out state.
var() float FadeOpacity; // Alpha of the fade, where 0 = fully transparent and 1 = fully opaque.
var() color FadeColor; // Fade color.
var() float ReverseFadeDuration; // The duration of the reverse fade.

var() bool sf_FadeFrom; //  Screen fades from the specified color instead of to it.
var() bool sf_StayOut; //  Fade remains indefinitely until another fade deactivates it.

function Trigger( actor Other, pawn EventInstigator )
{
	local PlayerPawn P;
	local HL_HUD_Overlay M;
	
	foreach AllActors(class'PlayerPawn',P)
	{
		M = Class'HL_HUD_Overlay'.Static.GetOverlay(P);
		if( M )
			M.SetScreenFade(Self);
	}
}

defaultproperties
{
	bStatic=true
}