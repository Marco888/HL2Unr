Class HL_Speaker extends HL_Triggers;

var() bool bInitiallyActive, bOnceOnly;
var() float AnnouncementDelay[2]; // Delay in random minutes between announcements.
var() array<VoiceLineObject> VoxLines;

var transient FX_VoiceLinePlayerVOX VOXPlayer;

function PostBeginPlay()
{
	if( VoxLines.Size()==0 )
		Warn("VOX without voices?");
	else if( bInitiallyActive )
		SetTimer(RandRange(AnnouncementDelay[0],AnnouncementDelay[1])*60.f,false);
}
function Trigger( actor Other, pawn EventInstigator )
{
	bInitiallyActive = !bInitiallyActive;
	if( bInitiallyActive )
		SetTimer(0.1f,false);
	else SetTimer(0.f,false);
}
function Timer()
{
	local VoiceLineObject V;

	if( VOXPlayer==None )
	{
		foreach AllActors(class'FX_VoiceLinePlayerVOX',VOXPlayer)
			break;
		if( VOXPlayer==None )
			VOXPlayer = Spawn(class'FX_VoiceLinePlayerVOX');
	}
	
	if( !Class'FX_VoiceLinePlayer'.Static.GlobalCanSpeak(Level) || VOXPlayer.NextVoxTimer>Level.TimeSeconds )
	{
		// Wait for the talkmonster to finish first.
		SetTimer(RandRange(5.f,10.f), false);
		return;
	}
	if( !bOnceOnly )
		SetTimer(RandRange(AnnouncementDelay[0],AnnouncementDelay[1])*60.f,false);

	V = VoxLines[Rand(VoxLines.Size())];
	if( V )
	{
		VOXPlayer.NextVoxTimer = Level.TimeSeconds + VOXPlayer.CalcVoiceDuration(V) + 5.f;
		VOXPlayer.VoicePlayer = Self;
		VOXPlayer.PlayVoiceLine(V);
		Class'FX_VoiceLinePlayer'.Static.GlobalSetSpeakTime(Level, 5.f);
	}
}

defaultproperties
{
	AnnouncementDelay(0)=0.25
	AnnouncementDelay(1)=2.25
	bInitiallyActive=true
	bNoDelete=true
}