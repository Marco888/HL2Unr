class FX_TeleportEffect extends XEmitter
	transient;

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( Level.NetMode==NM_DedicatedServer )
		LifeSpan = 0.5f;
}

defaultproperties
{
	MaxParticles=1
	ParticlesPerSec=1000.0
	FadeInTime=0.1
	FadeOutTime=0.8
	PartSpriteForwardZ=48.0
	ParticleTextures(0)=Texture'HL_Assets.FX.FX_fexplo1'
	Begin Object Class=Emitter Name=Emitter1
		MaxParticles=1
		ParticlesPerSec=1000.0
		FadeInTime=0.1
		FadeOutTime=0.75
		PartSpriteForwardZ=42.0
		SpawnSound=(Sounds[0]=Sound'HL_Ambience.debris.beamstart7',SndPitch=(Min=0.95,Max=1.05),SndRadius=(Min=1600.0,Max=1800.0),SndVolume=(Min=0.8,Max=1.0),SndCount=1)
		ParticleTextures(0)=Texture'HL_Assets.FX.FX_c_tele1'
		LifetimeRange=(Min=1.5,Max=1.7)
		ParticleColor=(X=(Min=0.7,Max=0.75),Z=(Min=0.8,Max=0.8))
		SpriteAnimationType=SAN_PlayOnce
		bRespawnParticles=False
		bUseRandomTex=True
	End Object
	IdleCombiner(0)=Emitter'Emitter1'
	Begin Object Class=BeamEmitter Name=BeamEmitter0
		NoiseSwapTime=0.025
		BeamPointScaling(0)=1.0
		BeamPointScaling(1)=0.8
		BeamPointScaling(2)=0.2
		NoiseRange=(X=(Min=-5.0,Max=5.0),Y=(Min=-5.0,Max=5.0),Z=(Min=-5.0,Max=5.0))
		Segments=8
		bDynamicNoise=True
		bDoBeamNoise=True
		MaxParticles=20
		ParticlesPerSec=30.0
		ParticleTextures(0)=Texture'HL_Assets.FX.FX_zbeam5'
		LifetimeRange=(Min=0.75,Max=1.0)
		StartingScale=(Min=2.0,Max=3.0)
		SphereCylVelocity=(Min=600.0,Max=800.0)
		ParticleColor=(X=(Min=0.5,Max=0.8),Z=(Min=0.35,Max=0.48))
		SpawnVelType=SP_Sphere
		bRespawnParticles=False
		bDirectional=False
	End Object
	IdleCombiner(1)=BeamEmitter'BeamEmitter0'
	Begin Object Class=Emitter Name=Emitter2
		MaxParticles=35
		ParticlesPerSec=40.0
		FadeInTime=0.7
		FadeOutTime=0.9
		ParticleTextures(0)=Texture'GenFX.LensFlar.Dot_B'
		TimeScale(0)=(DrawScaling=2.0,Time=1.0)
		SpeedScale(0)=(VelocityScale=9.0,Time=1.0)
		LifetimeRange=(Min=0.5,Max=0.6)
		StartingScale=(Min=0.05,Max=0.08)
		SphereCylinderRange=(Min=250.0,Max=250.0)
		SphereCylVelocity=(Min=-80.0,Max=-80.0)
		ParticleColor=(X=(Min=0.5,Max=0.9),Z=(Min=0.3,Max=0.4))
		SpawnPosType=SP_Sphere
		SpawnVelType=SP_Sphere
		bRespawnParticles=False
		bCylRangeBasedOnPos=True
	End Object
	IdleCombiner(2)=Emitter'Emitter2'
	LifetimeRange=(Min=2.0,Max=2.0)
	StartingScale=(Min=2.5,Max=3.0)
	SpawnSound=(Sounds[0]=Sound'HL_Ambience.debris.beamstart2',SndPitch=(Min=0.95,Max=1.05),SndRadius=(Min=1600.0,Max=1800.0),SndVolume=(Max=1.5),SndCount=1)
	ParticleColor=(X=(Min=0.32,Max=0.34),Y=(Min=0.95),Z=(Min=0.6,Max=0.62))
	SpriteAnimationType=SAN_PlayOnce
	bRespawnParticles=False
	bAutoDestroy=True
	bBoxVisibility=True
	bStasisEmitter=False
	bNoUpdateOnInvis=False
	LifeSpan=2.0
	RemoteRole=ROLE_SimulatedProxy
	bNetTemporary=True
}