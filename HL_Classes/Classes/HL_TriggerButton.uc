class HL_TriggerButton extends HL_Triggers;

var repnotify byte RepIndex;
var float nextTriggerTime;
var Texture TextureStates[2];
var HL_Triggers Master;

var() name MasterTag; // Master trigger which is a condition if this is allowed to pass.
var() export MaterialSequence ButtonMaterial;
var() Sound PressedSound, LockedSound;
var() array<VoiceLineObject> LockedLines;
var() float RetriggerTime;
var() bool bTouchActivate, bOnceOnly, bToggleButton, bInitiallyActive, bButtonSparks, bResetSwitchMaterial;

replication
{
	reliable if ( Role==ROLE_Authority )
		RepIndex;
}

simulated function PostBeginPlay()
{
	if( Level.NetMode!=NM_DedicatedServer && ButtonMaterial )
	{
		TextureStates[0] = ButtonMaterial.SequenceItems[0].Material;
		TextureStates[1] = ButtonMaterial.SequenceItems[1].Material;
		ButtonMaterial.Paused = true;
		ButtonMaterial.Loop = false;
		ButtonMaterial.CurrentTime = 0.f;
		ButtonMaterial.SequenceItems.SetSize(2);
		ButtonMaterial.SequenceItems[1].DisplayTime = 1.f;
		ButtonMaterial.SequenceItems[0].DisplayTime = 1.f;
		ButtonMaterial.SequenceItems[1].FadeOutTime = 0.f;
		ButtonMaterial.SequenceItems[0].FadeOutTime = 0.f;
		ButtonMaterial.SequenceItems[1].Material = TextureStates[0];
		ButtonMaterial.SequenceItems[0].Material = TextureStates[0];
	}
	if( Level.NetMode!=NM_Client )
	{
		if( MasterTag!='' )
		{
			foreach AllActors(class'HL_Triggers',Master,MasterTag)
				if( Master.IsMasterTrigger() )
					break;
		}
		if( bInitiallyActive )
			SwitchToMaterial(1);
	}
}

function GrabbedBy( Pawn Other )
{
	local vector Start,End;
	
	if( bTouchActivate )
		return;
	Start = Other.Location + vect(0,0,1)*Other.BaseEyeHeight;
	End = Start + vector(Other.ViewRotation)*200.f;
	if( TraceThisActor(End,Start) )
		Activate(Other);
}
function Touch( Actor Other )
{
	if( bTouchActivate && Other.bIsPawn && Pawn(Other).bIsPlayer )
		Activate(Pawn(Other));
}
function Activate( Pawn Other )
{
	if( nextTriggerTime>Level.TimeSeconds )
		return;
	if( Master && !Master.IsTriggered(Other) )
	{
		PlaySound(LockedSound);
		nextTriggerTime = Level.TimeSeconds + 5.f;
		return;
	}
	bInitiallyActive = !bInitiallyActive;
	PlaySound(PressedSound);
	nextTriggerTime = Level.TimeSeconds + RetriggerTime;
	TriggerEvent(Event, Self, Other);
	if( ButtonMaterial )
	{
		SwitchToMaterial(bInitiallyActive ? 1 : 0);
		if( bResetSwitchMaterial && RetriggerTime>0 )
			SetTimer(RetriggerTime,false);
	}
	if( bOnceOnly )
		SetCollision(false);
}
function Timer()
{
	bInitiallyActive = !bInitiallyActive;
	SwitchToMaterial(bInitiallyActive ? 1 : 0);
}

simulated function OnRepNotify( name Property )
{
	if( Property=='RepIndex' )
		SwitchToMaterial(RepIndex);
}
simulated final function SwitchToMaterial( byte Num )
{
	if( Level.NetMode!=NM_Client )
	{
		RepIndex = Num;
		bForceNetUpdate = true;
	}
	if( Level.NetMode!=NM_DedicatedServer && ButtonMaterial )
	{
		ButtonMaterial.SequenceItems[0].Material = TextureStates[Num];
		ButtonMaterial.SequenceItems[1].Material = TextureStates[Num];
	}
}

defaultproperties
{
	bCollideActors=true
	bProjTarget=true
	RemoteRole=ROLE_SimulatedProxy
	bStatic=true
	bAlwaysRelevant=true
	NetUpdateFrequency=2
	RetriggerTime=1
	bResetSwitchMaterial=True
}