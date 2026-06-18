class INV_SodaCan extends HL_Pickups_HP;

var Texture SkinList[6];

function PlayPickupMessage(Pawn Other)
{
	Other.ClientMessage(PickupMessage, 'Pickup');
}
function SetSkin( byte nSkin )
{
	if( nSkin>=ArrayCount(SkinList) )
		nSkin = Rand(ArrayCount(SkinList));
	if( nSkin>0 )
		MultiSkins[1] = SkinList[nSkin];
}

auto state Pickup
{
	// Landed on ground.
	function Landed(Vector HitNormal)
	{
		local vector X,Y;
		local rotator R;
		
		R.Yaw = Rotation.Yaw;
		Y = Normal(HitNormal Cross vector(R));
		X = Normal(Y Cross HitNormal);
		SetRotation(OrthoRotation(X,Y,HitNormal));
		PlaySound(Sound'HL_Ambience.weapons.g_bounce3');
		SetCollisionSize(18,10);
		SetTimer(2.0, false);
	}
}

defaultproperties
{
	Mesh=SodaCanSM
	PickupViewMesh=SodaCanSM
	PickupMessage="You got a soda pop"
	ItemName="Soda can"
	CollisionRadius=1
	CollisionHeight=4
	HealingAmount=1
	bIgnoreMaxHealth=false
	Physics=PHYS_Falling
	bCollideWorld=true
	RespawnTime=0
	
	SkinList(0)=Texture'CAN_gensoda1'
	SkinList(1)=Texture'CAN_gensoda2'
	SkinList(2)=Texture'CAN_gensoda3'
	SkinList(3)=Texture'CAN_gensoda4'
	SkinList(4)=Texture'CAN_gensoda5'
	SkinList(5)=Texture'CAN_gensoda6'
}
