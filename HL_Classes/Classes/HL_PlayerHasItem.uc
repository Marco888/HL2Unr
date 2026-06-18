// Custom trigger for checking if player has specific item
class HL_PlayerHasItem extends HL_Triggers;

var() class<Inventory> ItemClass;
var bool bCompleted;

function bool IsMasterTrigger()
{
	return true;
}
function bool IsTriggered( Pawn EventInstigator )
{
	if( !bCompleted )
	{
		if( EventInstigator==None || EventInstigator.FindInventoryType(ItemClass)==None )
			return false;
		bCompleted = true;
	}
	return true;
}

defaultproperties
{
	ItemClass=class'INV_HevSuit'
}