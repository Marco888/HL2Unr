class HL_Triggers_Master extends HL_Triggers
	abstract;

var array<Actor> NotifyList;

function bool IsMasterTrigger()
{
	return true;
}
function NotifyMasterChange( Actor Other )
{
	NotifyList.Add(Other);
}

final function SendChangeNotify()
{
	local Actor A;
	
	foreach NotifyList(A)
		A.ExecFunctionStr('MasterCondition',"");
}

defaultproperties
{
	RemoteRole=ROLE_None
	bCollideActors=false
}