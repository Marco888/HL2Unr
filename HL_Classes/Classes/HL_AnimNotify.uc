Class HL_AnimNotify extends AnimationNotify;

function OnPlayVoiceSound( Sound Sound, float Volume, float Pitch )
{
	if( Owner && HL_Pawns(Owner).IKLipSync )
		HL_Pawns(Owner).IKLipSync.StartLIPSyncTrack(Sound,Pitch);
}
