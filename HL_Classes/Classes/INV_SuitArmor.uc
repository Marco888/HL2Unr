class INV_SuitArmor extends HL_Pickups;

event float BotDesireability(Pawn Bot)
{
	local Inventory I;

	I = Bot.FindInventoryType(class'INV_HevSuit');
	if( I==None )
		return -1.f;
	return (I.Charge<90) ? MaxDesireability : -1.f;
}

auto state Pickup
{
	function Touch( actor Other )
	{
		ValidTouch(Other); // Suit should handle picking this up.
	}
}

defaultproperties
{
	Mesh=W_Battery
	PickupViewMesh=W_Battery
	PickupMessage="You got a battery pack"
	ItemName="Battery Pack"
	CollisionRadius=7
	CollisionHeight=7
	Charge=15
	PickupSound=Sound'HL_Ambience.Items.gunpickup2'
}
