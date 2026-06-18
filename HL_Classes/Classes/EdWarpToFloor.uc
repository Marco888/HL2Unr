class EdWarpToFloor extends BrushBuilder;

event bool Build()
{
	local Inventory I;
	local HL_Pawns H;
	local NavigationPoint N;
	
	foreach AllObjects(class'Inventory',I)
		DropToFloor(I);
	foreach AllObjects(class'HL_Pawns',H)
		if( H.bShouldStartOnFloor )
			DropToFloor(H);
	foreach AllObjects(class'NavigationPoint',N)
		if( N.Brush==None && Teleporter(N)==None )
			DropToFloor(N,true);
	return false;
}

final function DropToFloor( Actor A, optional bool bZero )
{
	local vector Ext,End,HL,HN;
	
	End = A.Location;
	if( bZero )
		End.Z-=(A.CollisionHeight*2.f);
	else
	{
		Ext = A.GetExtent();
		End.Z-=A.CollisionHeight;
	}
	if( A.Trace(HL,HN,End,A.Location,false,Ext)==None )
		return;
	if( bZero )
		HL.Z+=A.CollisionHeight;
	A.SetLocation(HL);
}

defaultproperties
{
	BitmapImage=Texture'Engine.S_Player'
	ToolTip="Drop HL entities to floor"
}