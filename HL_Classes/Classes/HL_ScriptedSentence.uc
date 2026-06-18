// scripted_sentence is a point entity available in Half-Life. (also in Ricochet) This entity allows you to make a monster say something. Available sentence names can be found in the sound\sentences.txt file. 
Class HL_ScriptedSentence extends HL_Triggers;

var() name m_iszEntity; // The name or class name (such as 'npc_zombie') of an NPC to use for this script.
var() name Listener; // The name of the entity that the speaker will look towards while speaking ('player' = nearest player).
var() export editinline VoiceLineObject VoiceLine; // Voiceline to speak.

var() float m_flRadius; // If speaker type is set to the monster's classname, the scripted_sentence will try to find the monster only in this radius (in units, relative to the scripted_sentence origin).
var() float StartDelay; // Delay in seconds before firing the targeted entity.
var() float SentanceDuration; // The duration in seconds before the monster becomes available for usage by a player once the scripted sentence has started playing.
var() float RefireTime; // Delay before trying to find the appropriate monster again once the scripted_sentence is activated but failed to find the speaker.

var() bool bOnceOnly;
var() bool bFollowersOnly;
var() bool bAllowInterrupt;
var() bool bConCurrent; // If false, all other nearby talkers are silenced.

function Trigger( Actor Other, Pawn EventInstigator )
{
	if( bOnceOnly )
		Disable('Trigger');
	TriggerSequence();
}
function TriggerSequence()
{
	local HL_Pawns P;
	
	foreach AllActors(class'HL_Pawns',P,m_iszEntity,,false)
		if( P.Class.Name!=m_iszEntity && AcceptableSpeaker(P) )
		{
			P.StartVoiceSequence(VoiceLine, FindLookTarget(P), bConCurrent);
			return;
		}
	if( m_flRadius>0.f )
	{
		foreach RadiusActors(class'HL_Pawns',P,m_flRadius)
			if( P.Class.Name==m_iszEntity && AcceptableSpeaker(P) )
			{
				P.StartVoiceSequence(VoiceLine, FindLookTarget(P), bConCurrent);
				return;
			}
	}
	if( RefireTime>0.f )
		SetTimer(RefireTime,false,'TriggerSequence');
}
function bool AcceptableSpeaker( HL_Pawns P )
{
	if( bFollowersOnly && !P.IsFollowingPlayer() )
		return false;
	return P.CanPlaySentence(bAllowInterrupt);
}
function Actor FindLookTarget( HL_Pawns Other )
{
	local Pawn P,Best;
	local float Score,BestScore;

	foreach AllActors(class'Pawn',P,Listener)
	{
		if( P.Health<=0 )
			continue;
		Score = VSizeSq(P.Location-Other.Location);
		if( Score<Square(1000.f) && Other.LineOfSightTo(P) )
			Score*=0.75f;
		if( Best==None || Score<BestScore )
		{
			Best = P;
			BestScore = Score;
		}
	}
	return Best;
}

defaultproperties
{
	Listener="Player"
}