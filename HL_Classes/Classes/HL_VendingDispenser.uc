// Vending machine dispenser.
class HL_VendingDispenser extends HL_Triggers;

var() byte numCans,canSkinNum;
var INV_SodaCan ActiveCan;

function Trigger( Actor Other, Pawn EventInstigator )
{
	if( numCans<=0 || (ActiveCan!=None && !ActiveCan.bDeleteMe) )
		return;
	--numCans;
	ActiveCan = Spawn(class'INV_SodaCan');
	if( ActiveCan )
		ActiveCan.SetSkin(canSkinNum);
}

defaultproperties
{
	numCans=10
	canSkinNum=6
}