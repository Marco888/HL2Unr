class HL_RenderModeTrigger extends HL_Triggers
	DependsOn(HL_Pawns);

var() ERenderStyle TargetStyle;
var() float TargetScaleGlow;
var() color TargetRenderColor;
var() HL_Pawns.ERenderFX TargetRenderFX;

var() bool bChangeStyle, bChangeScaleGlow, bChangeRenderColor, bChangeRenderFX;

function Trigger( actor Other, pawn EventInstigator )
{
	local Actor A;

	if( Event=='' )
		return;
	foreach AllActors(class'Actor',A,Event)
	{
		if( bChangeRenderFX && HL_Pawns(A) )
			HL_Pawns(A).SetRenderFX(TargetRenderFX);
		if( bChangeStyle )
			A.Style = TargetStyle;
		if( bChangeScaleGlow )
			A.ScaleGlow = TargetScaleGlow;
		if( bChangeRenderColor )
			A.ActorRenderColor = TargetRenderColor;
	}
}

defaultproperties
{
	TargetStyle=STY_Normal
	TargetScaleGlow=1
	TargetRenderColor=(R=255,G=255,B=255,A=255)
	TargetRenderFX=RFX_Normal
}