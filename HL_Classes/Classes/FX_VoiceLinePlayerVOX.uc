class FX_VoiceLinePlayerVOX extends FX_VoiceLinePlayer
	transient;

var Actor VoicePlayer;
var float NextVoxTimer,PlayRadius;
var bool bPlayingVoice;

final function PlayVoxLine( Actor Source, VoiceLineObject V, optional bool bFullRadius )
{
	if( bPlayingVoice )
		VoicePlayer.PlaySound(Sound'_Comma', SLOT_Talk, 2.f,,PlayRadius);
	VoicePlayer = Source;
	bPlayingVoice = true;
	NextVoxTimer = Level.TimeSeconds + CalcVoiceDuration(V);
	if( bFullRadius )
		PlayRadius = 999999.f;
	else PlayRadius = 1600.f;
	PlayVoiceLine(V);
}
function Timer()
{
	local float p;
	
	if( VoiceIndex>=MaxIndex )
	{
		bPlayingVoice = false;
		return;
	}
	p = Voice.Line[VoiceIndex].Pitch;
	VoicePlayer.PlaySound(Voice.Line[VoiceIndex].Sound, SLOT_Talk, Voice.Line[VoiceIndex].Volume,,PlayRadius,p);
	SetTimer(GetSoundDuration(Voice.Line[VoiceIndex].Sound)*p*Voice.Line[VoiceIndex].Duration, false);
	++VoiceIndex;
}
