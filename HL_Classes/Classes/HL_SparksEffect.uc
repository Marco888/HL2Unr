class HL_SparksEffect extends NetSpriteEmitter;

defaultproperties
{
	ParticleRotation=SPR_AbsFacingVelocity
	MaxParticles=15
	FadeInTime=0.1
	FadeOutTime=0.1
	MaxCoronaDistance=800.0
	CoronaTexture=Texture'GenFX.LensFlar.1'
	ParticleTextures(0)=Texture'GenFX.LensFlar.Dot_B'
	LifetimeRange=(Min=0.6,Max=0.7)
	StartingScale=(Min=0.15,Max=0.2)
	Scale3DRange=(Y=(Min=0.1,Max=0.2),Z=(Min=2.0,Max=3.0))
	ParticleAcceleration=(Z=(Min=-750.0,Max=-900.0))
	BoxVelocity=(X=(Min=200.0,Max=300.0),Y=(Min=-50.0,Max=50.0),Z=(Min=-50.0,Max=50.0))
	ParticleColor=(Z=(Min=0.3,Max=0.4))
	CoronaColor=(Z=(Min=0.2,Max=0.3))
	ParticleCollision=ECT_HitWalls
	bDisabled=True
	bVelRelativeToRotation=True
	bParticleCoronaEnabled=True
	TriggerAction=ETR_SpawnParticles
	SpawnParts=(Min=2,Max=3)
	bDirectional=True
}