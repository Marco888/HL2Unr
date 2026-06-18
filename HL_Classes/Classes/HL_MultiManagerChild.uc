class HL_MultiManagerChild extends Info
	NoUserCreate;

var HL_MultiManager Parent;
var int EventNum;

auto state Dispatch
{
Begin:
	Sleep(0.f); // Wait 1 tick.
	for( EventNum=0; EventNum<Parent.Events.Size(); ++EventNum )
	{
		if( Parent.Events[EventNum].Delay>0.f )
			Sleep(Parent.Events[EventNum].Delay);
		TriggerEvent(Parent.Events[EventNum].Event,Parent,Instigator);
	}
	Destroy();
}

defaultproperties
{
	RemoteRole=ROLE_None
}