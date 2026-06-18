class FX_VoiceLinePlayer extends Info
	transient;

var VoiceLineObject Voice;
var float VolumeMod,PitchMod;
var int VoiceIndex,MaxIndex;

var private float NextSpeakTime;
var private int LastLevelIndex;

event OwnerChanged()
{
	if( Owner==None )
		Destroy();
}

final function float CalcVoiceDuration( VoiceLineObject V, optional float Pit )
{
	local int i;
	local float Result;

	if( V==None )
		return 0.1f;
	if( Pit==0.f )
		Pit = 1.f;
	for( i=(V.Line.Size()-1); i>=0; --i )
		Result += GetSoundDuration(V.Line[i].Sound) * V.Line[i].Pitch * V.Line[i].Duration;
	return Result*Pit;
}

final function AbortVoiceLine()
{
	Owner.PlaySound(Sound'_Comma', SLOT_Talk, VolumeMod);
	SetTimer(0.f,false);
}
final function PlayVoiceLine( VoiceLineObject V, optional float Vol, optional float Pit )
{
	if( V==None )
	{
		AbortVoiceLine();
		return;
	}
	VolumeMod = (Vol==0.f) ? 1.f : Vol;
	PitchMod = (Pit==0.f) ? 1.f : Pit;
	MaxIndex = V.Line.Size();
	VoiceIndex = 0;
	Voice = V;
	SetTimer(0.001f,false);
}
function Timer()
{
	local float p;
	
	if( VoiceIndex>=MaxIndex )
	{
		Owner.ExecFunctionStr('FinishedSpeech',"");
		return;
	}
	p = Voice.Line[VoiceIndex].Pitch*PitchMod;
	Owner.PlaySound(Voice.Line[VoiceIndex].Sound, SLOT_Talk, Voice.Line[VoiceIndex].Volume*VolumeMod,,, p);
	SetTimer(GetSoundDuration(Voice.Line[VoiceIndex].Sound)*p*Voice.Line[VoiceIndex].Duration, false);
	++VoiceIndex;
}

static final function bool GlobalCanSpeak( LevelInfo inLevel )
{
	if( Default.LastLevelIndex!=inLevel.ObjectIndex )
		return true;
	return (inLevel.TimeSeconds>Default.NextSpeakTime);
}
static final function GlobalSetSpeakTime( LevelInfo inLevel, float PauseTime )
{
	Default.LastLevelIndex = inLevel.ObjectIndex;
	Default.NextSpeakTime = inLevel.TimeSeconds+PauseTime;
}

defaultproperties
{
	RemoteRole=ROLE_None
}