class HL_Emitter extends NetworkEmitter;

defaultproperties
{
	FadeInTime=0.025
	FadeOutTime=0.975
	MinBounceVelocity=10.0
	ParticleAcceleration=(Z=(Min=-850.0,Max=-900.0))
	ParticleBounchyness=(X=0.75,Y=0.75,Z=0.25)
	ParticleCollision=ECT_HitWalls
	bVelRelativeToRotation=True
	bRespawnParticles=False
	bDisabled=True
	TriggerAction=ETR_ToggleDisabled
}