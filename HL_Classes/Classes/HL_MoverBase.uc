// Base class of Half-Life movers.
Class HL_MoverBase extends Mover
	abstract;

function BumpLocked( Pawn Other )
{
}

state() BumpOpenTimed
{
	function Bump( actor Other )
	{
		switch( BumpType )
		{
		case BT_PlayerBump:
			if ( !Other.bIsPawn && !Pawn(Other).bIsPlayer )
				return;
			break;
		case BT_PawnBump:
			if ( !Other.bIsPawn || (Other.Mass < 10) )
				return;
			break;
		}
		if( Tag!='' && Tag!=Class.Name )
		{
			if( Other.bIsPawn && Pawn(Other).bIsPlayer )
				BumpLocked(Pawn(Other));
			return;
		}
		Global.Bump( Other );
		SavedTrigger = None;
		Instigator = Pawn(Other);
		GotoState( 'BumpOpenTimed', 'Open' );
	}
}
state() BumpToggle
{
	function bool HandleDoor(pawn Other)
	{
		if ( (BumpType == BT_PlayerBump) && !Other.bIsPlayer )
			return false;

		Bump(Other);
		WaitingPawn = Other;
		Other.SpecialPause = 2.5;
		return true;
	}

	function Bump( actor Other )
	{
		switch( BumpType )
		{
		case BT_PlayerBump:
			if ( !Other.bIsPawn && !Pawn(Other).bIsPlayer )
				return;
			break;
		case BT_PawnBump:
			if ( !Other.bIsPawn || (Other.Mass < 10) )
				return;
			break;
		}
		if( Tag!='' && Tag!=Class.Name )
		{
			if( Other.bIsPawn && Pawn(Other).bIsPlayer )
				BumpLocked(Pawn(Other));
			return;
		}
		Global.Bump( Other );
		SavedTrigger = None;
		Instigator = Pawn(Other);
		if ( KeyNum==0 || KeyNum<PrevKeyNum )
			GotoState(,'Open');
		else GotoState(,'Close');
	}
Begin:
	Stop;
Open:
	Disable( 'Bump' );
	if ( DelayTime > 0 )
	{
		bDelaying = true;
		Sleep(DelayTime);
	}
	DoOpen();
	FinishInterpolation();
	FinishedOpening();
	Enable( 'Bump' );
	Stop;
	
Close:
	Disable( 'Bump' );
	DoClose();
	FinishInterpolation();
	FinishedClosing();
	Enable( 'Bump' );
}

defaultproperties
{
	bDirectionalPushOff=true
	bReplicateSimMove=false
	MoverGlideType=MV_MoveByTime
	bUseMeshCollision=true
	BumpType=BT_PlayerBump
	CollisionHeight=1
	CollisionRadius=1
}