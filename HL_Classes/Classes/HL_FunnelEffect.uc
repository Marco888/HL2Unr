class HL_FunnelEffect extends HL_Triggers;

var() bool bReverseEffect;

function Trigger( actor Other, pawn EventInstigator )
{
	if( bReverseEffect )
		Spawn(class'FX_FunnelOut');
	else Spawn(class'FX_FunnelIn');
}
