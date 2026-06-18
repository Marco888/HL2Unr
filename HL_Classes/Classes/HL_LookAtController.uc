Class HL_LookAtController extends Info;

var HL_Pawns PawnOwner;

function PostBeginPlay()
{
	PawnOwner = HL_Pawns(Owner);
	if( !PawnOwner )
		Error("No owner actor!");
	else if( PawnOwner.IKHead )
		PawnOwner.IKHead.SetEnabled(true,0.25f);
}
function Destroyed()
{
	if( PawnOwner && PawnOwner.IKHead )
		PawnOwner.IKHead.SetEnabled(false,0.25f);
}
function Tick( float Delta )
{
	if( PawnOwner.NetLookAt )
		PawnOwner.IKHead.ViewPosition = GetLookVect();
}
final function vector GetLookVect()
{
	local vector V;
	
	V = PawnOwner.NetLookAt.Location;
	//V.Z+=(PawnOwner.NetLookAt.CollisionHeight*0.8f);
	return V;
}

defaultproperties
{
	RemoteRole=ROLE_None
}