class FX_VoiceLinePlayerHEV extends FX_VoiceLinePlayer;

var INV_HevSuit SuitOwner;

simulated function Timer()
{
	local float p;
	
	if( VoiceIndex>=MaxIndex )
	{
		SuitOwner.FinishedSpeech();
		return;
	}
	p = Voice.Line[VoiceIndex].Pitch*PitchMod;
	PlaySound(Voice.Line[VoiceIndex].Sound, SLOT_Talk, Voice.Line[VoiceIndex].Volume*VolumeMod,,, p);
	SetTimer(GetSoundDuration(Voice.Line[VoiceIndex].Sound)*p*Voice.Line[VoiceIndex].Duration, false);
	++VoiceIndex;
}

defaultproperties
{
	Physics=PHYS_Trailer
	Mass=0
	DrawType=DT_None
}