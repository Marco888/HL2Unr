Class INFO_HLGameRules extends GameRules;

var private int LastValidRules;
var HL_InitialItems SpawnWeapons;

static final function INFO_HLGameRules GetHLRules( GameInfo Game )
{
	local GameRules G;
	local INFO_HLGameRules D;
	
	// Read from cache.
	if( Default.LastValidRules>0 )
	{
		D = INFO_HLGameRules(FindObjectIndex(Default.LastValidRules));
		if( D && D.Level.Game==Game )
			return D;
	}
	for( G=Game.GameRules; G; G=G.NextRules )
		if( INFO_HLGameRules(G) )
		{
			Default.LastValidRules = G.ObjectIndex;
			return INFO_HLGameRules(G);
		}
	D = Game.Spawn(class'INFO_HLGameRules');
	D.AddSelfToGame();
	Default.LastValidRules = D.ObjectIndex;
	return D;
}

function ModifyPlayer( Pawn Other )
{
	if( SpawnWeapons )
		SpawnWeapons.GiveWeapons(Other);
}

defaultproperties
{
	bNotifySpawnPoint=true
}