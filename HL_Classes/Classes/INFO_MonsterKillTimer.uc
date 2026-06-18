class INFO_MonsterKillTimer extends Info
	NoUserCreate;

var HL_Pawns Monster;
var HL_MonsterMaker Maker;

function Timer()
{
	Maker.KillMonster(Monster);
	Destroy();
}
function KillEffect()
{
	Maker.PendingKillMonster(Monster);
}

defaultproperties
{
}