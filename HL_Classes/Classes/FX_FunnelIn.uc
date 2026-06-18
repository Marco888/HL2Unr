class FX_FunnelIn extends XSpriteEmitter
	transient;

simulated function BeginPlay()
{
	if( Level.NetMode==NM_DedicatedServer )
		LifeSpan = 0.75;
}

defaultproperties
{
	InitialRot=(Z=(Max=1.0))
	MaxParticles=64
	ParticlesPerSec=150.0
	FadeInTime=0.2
	FadeOutTime=0.75
	ParticleTextures(0)=Texture'UnrealShare.SKEffect.Skj_a02'
	ParticleTextures(1)=Texture'UnrealShare.SKEffect.Skj_a03'
	SpeedScale(0)=(VelocityScale=5.0,Time=1.0)
	LifetimeRange=(Min=2.0,Max=2.5)
	StartingScale=(Max=2.0)
	BoxLocation=(X=(Min=-450.0,Max=450.0),Y=(Min=-450.0,Max=450.0),Z=(Min=400.0,Max=550.0))
	SphereCylVelocity=(Min=-50.0,Max=-50.0)
	ParticleColor=(X=(Min=0.9),Z=(Min=0.2,Max=0.75))
	SpawnVelType=SP_Sphere
	bRespawnParticles=False
	bUseRandomTex=True
	bCylRangeBasedOnPos=True
	bNoUpdateOnInvis=False
	bStasisEmitter=False
	
	LifeSpan=5.0
	RemoteRole=ROLE_SimulatedProxy
	bNetTemporary=True
}
