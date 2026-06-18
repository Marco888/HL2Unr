class FX_BulletTracer extends XSpriteEmitter
	transient;

simulated function DrawTrace( vector Start, vector End )
{
	local float Dist;

	End -= Start;
	Dist = VSize(End);
	if( Dist<150.f )
		return;
	SetLocation(Start);
	End /= LifetimeRange.Min;
	BoxVelocity.X.Min = End.X;
	BoxVelocity.X.Max = End.X;
	BoxVelocity.Y.Min = End.Y;
	BoxVelocity.Y.Max = End.Y;
	BoxVelocity.Z.Min = End.Z;
	BoxVelocity.Z.Max = End.Z;
	SpawnParticles(1);
}

defaultproperties
{
	ParticleRotation=SPR_AbsFacingVelocity
	MaxParticles=10
	FadeInTime=0.1
	FadeOutTime=0.9
	ParticleTextures(0)=Texture'GenFX.LensFlar.Dot_B'
	LifetimeRange=(Min=0.075,Max=0.075)
	StartingScale=(Min=0.05,Max=0.06)
	Scale3DRange=(Z=(Min=7.0,Max=7.0))
	BoxVelocity=(X=(Min=800.0,Max=800.0))
	ParticleColor=(Z=(Min=0.05,Max=0.1))
	bRespawnParticles=False
	bSpawnInitParticles=False
}