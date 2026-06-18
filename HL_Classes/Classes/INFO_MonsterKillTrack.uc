class INFO_MonsterKillTrack extends Info
	NoUserCreate;

var HL_MonsterMaker Notify;

function Trigger( actor Other, pawn EventInstigator )
{
	if( Notify )
		Notify.ChildKilled();
}

defaultproperties
{
	RemoteRole=ROLE_None
}