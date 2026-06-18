// A scripted_sequence typically takes a Target NPC which can be set to either move to the sequence's position or play the sequence's animation at the NPC's current position. When the NPC is ready to begin the sequence, it will play the specified animation(s) in a special scripted state which may or may not be interrupted depending on the scripted_sequence's settings. 
Class HL_ScriptedSequence extends HL_Triggers;

var() name m_iszEntity; // The name or class name (such as 'npc_zombie') of an NPC to use for this script.
var() name m_iszIdle; // The name of the sequence (such as "idle01") or activity (such as 'ACT_IDLE') to play before the action animation if the NPC must wait for the script to be triggered. Use "Start on Spawn" flag or "MoveToPosition" input to play this idle animation.
var() name m_iszEntry; // The name of the sequence (such as 'reload02') or activity (such as 'ACT_RELOAD') to play when the sequence starts, before transitioning to play the main action sequence.
var() name m_iszPlay; // The name of the main sequence (such as 'reload02') or activity (such as 'ACT_RELOAD') to play.
var() name m_iszPostIdle; // The name of the sequence (such as 'idle01') or activity (such as 'ACT_IDLE') to play after the action animation.
var() name m_iszCustomMove; // Used in conjunction with the 'Custom movement' setting for the 'Move to Position' property, specifies the sequence (such as 'crouch_run01') or activity (such as 'ACT_RUN') to use while moving to the scripted position.
var() name m_iszNextScript; // The name of the script to run immediately after this script completes. The NPC will not return to AI between the two scripts.

var() float m_flRadius; // Radius to search within for an NPC to use. 0 searches everywhere.

var() bool m_LoopActionAnim; // Repeat permamently "m_iszPlay"? See "m_iszPlay"
var() float m_flRepeat; // How long NPC will repeat "m_iszPlay". See "m_iszPlay". Useless with "m_LoopActionAnim"
var() bool m_bIgnoreGravity; // If this is set to 'Yes', the NPC will not be subject to gravity while playing this script
var() bool m_bDisableNPCCollisions; // Useful for when NPCs playing scripts must interpenetrate while riding on trains, elevators, etc. This only disables collisions between the NPCs in the script and must be enabled on BOTH scripted_sequences.

var() bool sf_Repeatable;
var() bool sf_StartOnSpawn; // Makes the script search for a NPC and move to its position upon spawning, playing the Pre-Action Idle Animation until BeginSequence is received.
var() bool sf_NoInterruptions; //  Prevents the NPC from being interrupted by damage, etc.
var() bool sf_OverrideAI; //  Overrides the NPC's current state to play the script, regardless of whether they're in combat or otherwise.
var() bool sf_NoTeleportAtEnd; // Prevents the script from teleporting the NPC to the last animation's end position when the script is finished.
var() bool sf_LoopInPostIdle; // Makes the NPC loop the Post-Action Idle Animation after it finishes playing the Action Animation.
var() bool sf_PriorityScript; // Stops other scripts from stealing this script's spot in a NPC's queue.
var() bool sf_SearchCyclically; // UNUSED: Searches for the next entity from the last one found.

var() enum EMoveType
{
	MTT_None,
	MTT_Walk,
	MTT_Run,
	MTT_Custom,
	MTT_Instant,
	MTT_TurnTo
} m_fMoveTo;

var vector LookAtFocus;
var HL_ScriptedSequence NextScript;

function PostBeginPlay()
{
	LookAtFocus = Location+vector(Rotation)*2000.f;
	if( m_iszNextScript!='' )
	{
		foreach AllActors(class'HL_ScriptedSequence',NextScript,m_iszNextScript)
			break;
	}
	if( sf_StartOnSpawn || Tag==Class.Name || Tag=='' || m_iszIdle!='' )
		SetTimer(0.001,false);
}
function Timer()
{
	TriggerSequence(true);
}
function Trigger( Actor Other, Pawn EventInstigator )
{
	if( !sf_Repeatable )
		Disable('Trigger');
	TriggerSequence(false);
}
function TriggerSequence( bool bIdle )
{
	local HL_Pawns P;
	
	foreach AllActors(class'HL_Pawns',P,m_iszEntity,,false)
	{
		if( P.Class.Name==m_iszEntity || !P.CanStartSequence(Self,1) )
			continue;
		P.StartSequence(Self,bIdle);
		return;
	}
	if( m_flRadius>0.f )
	{
		foreach RadiusActors(class'HL_Pawns',P,m_flRadius)
			if( P.Class.Name==m_iszEntity && P.CanStartSequence(Self,0) )
			{
				P.StartSequence(Self,bIdle);
				return;
			}
	}
}
function ApplyScriptTo( HL_Pawns P, bool bIdle )
{
	local float orgH,orgR;

	if( m_fMoveTo==MTT_Instant )
	{
		P.DesiredRotation = Rotation;
		orgH = P.CollisionHeight;
		orgR = P.CollisionRadius;
		P.SetCollisionSize(0,0);
		P.SetLocation(Location,Rotation);
		P.SetCollisionSize(orgR,orgH);
	}
	if( bIdle && m_iszIdle!='' )
		P.StartAnimSequence(m_iszIdle,true);
}

defaultproperties
{
	CollisionHeight=39
	CollisionRadius=16
	bDirectional=true
}