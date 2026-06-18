Class HL_SoundTrigger extends HL_Triggers;

var() VoiceLineObject VoxLine;
var() Sound SoundEffect;
var() float EffectRadius; // Maximum distance in units at which the sound plays for a client.
var() float EffectVolume,EffectPitch;
var() name SourceEntityName; // If specified, the sound will play from this entity instead of the ambient_generic. If the target is an NPC capable of lipsynching, and phoneme data is found within the sound, the target will lip-sync to it.

var() bool sf_PlayEverywhere; // Sound plays at a constant volume, no matter how far away the listener is from the sound source.
var() bool sf_StartPlaying;
var() bool sf_LoopSound;
var bool bEnabled;

var transient FX_VoiceLinePlayerVOX VOXPlayer;

function PostBeginPlay()
{
	if( sf_StartPlaying )
		Trigger(Self, None);
}
function Trigger( actor Other, pawn EventInstigator )
{
	local PlayerPawn P;
	local Actor A;
	
	if( VoxLine && Other!=Self )
	{
		if( VOXPlayer==None )
		{
			foreach AllActors(class'FX_VoiceLinePlayerVOX',VOXPlayer)
				break;
			if( VOXPlayer==None )
				VOXPlayer = Spawn(class'FX_VoiceLinePlayerVOX');
		}
		A = Self;
		if( SourceEntityName!='' )
		{
			foreach AllActors(class'Actor',A,SourceEntityName)
				break;
		}
		VOXPlayer.PlayVoxLine(A,VoxLine,sf_PlayEverywhere);
	}
	if( SoundEffect==None )
		return;
	
	bEnabled = !bEnabled;
	if( sf_PlayEverywhere )
	{
		foreach AllActors(class'PlayerPawn',P)
			P.ClientPlaySound(SoundEffect);
	}
	else if( SourceEntityName!='' )
	{
		foreach AllActors(class'Actor',A,SourceEntityName)
		{
			if( sf_LoopSound )
			{
				if( bEnabled )
					A.AmbientSound = SoundEffect;
				else A.AmbientSound = None;
			}
			else if( HL_Pawns(A) )
				A.PlaySound(SoundEffect,SLOT_Talk,EffectVolume,false,EffectRadius,EffectPitch*HL_Pawns(A).VoicePitch);
			else A.PlaySound(SoundEffect,SLOT_Misc,EffectVolume,false,EffectRadius,EffectPitch);
		}
	}
	else if( sf_LoopSound )
	{
		if( bEnabled )
		{
			AmbientSound = SoundEffect;
			SoundVolume = EffectVolume*128;
			SoundRadius = EffectRadius/25;
			SoundPitch = EffectPitch*64;
		}
		else AmbientSound = None;
	}
	else PlaySound(SoundEffect,SLOT_Misc,EffectVolume,false,EffectRadius,EffectPitch);
}

defaultproperties
{
	EffectRadius=1000
	EffectVolume=1
	EffectPitch=1
	bStatic=true
}