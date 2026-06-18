Class HL_MultiManager extends HL_Triggers;

struct FTrigPairs
{
	var() name Event;
	var() float Delay;
};
var() array<FTrigPairs> Events;
var() bool bMultiThreaded; // Allow this manager to be triggered multiple times.
var int EventNum;

function Trigger( actor Other, pawn EventInstigator )
{
	Instigator = EventInstigator;
	GoToState('Dispatch');
}

state Dispatch
{
	function Trigger( actor Other, pawn EventInstigator )
	{
		local HL_MultiManagerChild C;
		
		if( bMultiThreaded )
		{
			C = Spawn(class'HL_MultiManagerChild');
			C.Parent = Self;
			C.Instigator = EventInstigator;
		}
		else GoToState('Dispatch', 'Begin');
	}
Begin:
	Sleep(0.f); // Wait 1 tick.
	for( EventNum=0; EventNum<Events.Size(); ++EventNum )
	{
		if( Events[EventNum].Delay>0.f )
			Sleep(Events[EventNum].Delay);
		TriggerEvent(Events[EventNum].Event,Self,Instigator);
	}
	GoToState('');
}

event DrawEditorSelection( Canvas C )
{
	local int j;
	local Actor A;

	for ( j=0; j<Events.Size(); ++j )
		if( Events[j].Event!='' )
		{
			foreach AllActors(Class'Actor',A,Events[j].Event)
				C.Draw3DLine(MakeColor(255,0,255),Location,A.Location);
		}
}

defaultproperties
{
	Texture=Texture'S_Dispatcher'
	bEditorSelectRender=True
}
