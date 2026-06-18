class FX_BarneyMuzzle extends XSpriteEmitter
	transient;

simulated function PulseFX()
{
	SpawnParticles(3);
	LightType = LT_Steady;
	SetTimer(0.05,false);
}
simulated function Timer()
{
	LightType = LT_None;
}

defaultproperties
{
	InitialRot=(Z=(Max=1.0))
	MaxParticles=4
	ParticlesPerSec=100.0
	FadeInTime=0.1
	FadeOutTime=0.5
	ParticleTextures(0)=Texture'GenFX.LensFlar.flare6'
	TimeScale(0)=(DrawScaling=0.25,Time=1.0)
	LifetimeRange=(Min=0.06,Max=0.05)
	StartingScale=(Min=0.2,Max=0.25)
	BoxLocation=(X=(Min=12.0,Max=12.0),Z=(Min=3.0,Max=3.0))
	BoxVelocity=(X=(Min=64.0,Max=128.0))
	bRespawnParticles=False
	bSpawnInitParticles=False
	bUseRelativeLocation=True
	
	LightBrightness=255
	LightHue=43
	LightSaturation=218
	LightRadius=6
}