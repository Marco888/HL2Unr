class HL_Pickups_HP extends Health
	abstract;

var bool bIgnoreMaxHealth;

auto state Pickup
{
	function Touch( actor Other )
	{
		local int HealMax;
		local Pawn P;

		if ( ValidTouch(Other) )
		{
			P = Pawn(Other);
			HealMax = P.default.health;
			if( bSuperHeal ) HealMax = HealMax * 2.0;
			if( bIgnoreMaxHealth && (P.Health>=HealMax) )
				return;
			
			if( P.Health<HealMax )
				P.Health = Min(P.Health+HealingAmount,HealMax);
			PlayPickupMessage(P);
			PlaySound (PickupSound,,2.5);
			if ( Level.Game.Difficulty > 1 )
				Other.MakeNoise(0.1 * Level.Game.Difficulty);
			SetRespawn();
		}
	}
}

defaultproperties
{
	bRenderMultiEnviroMaps=true
	bIgnoreMaxHealth=true
}