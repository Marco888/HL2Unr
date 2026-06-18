class HL_Explosion extends ExplosionChain;

var() bool bRepeat; // Repeatable.
var() bool bNoFireball; // Don't create a fireball.
var() bool bNoSmoke; // Don't create smoke.
var() bool bNoScorch; // Don't spawn decals.
var() bool bNoSparks; // Don't create sparks.

state Exploding
{
ignores TakeDamage;

	function Timer()
	{
		local SpriteBallExplosion f;

		bExploding = true;
		if( Damage>0 )
			HurtRadius(Damage, Damage*2.5f, 'Exploded', MomentumTransfer, Location);
		f = spawn(class'SpriteBallExplosion',,,Location + vect(0,0,1)*16,rotang(90,0,0));
		f.DrawScale = RandRange(0.9f,1.1f)*Size;
		if( bRepeat )
		{
			bExploding = false;
			GoToState('');
		}
		else GoToState('Inactive');
	}
}

defaultproperties
{
	bCollideActors=false
	bCollideWorld=false
	DelayTime=0.075
	bHidden=true
}