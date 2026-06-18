/* Multisource behaves exactly the opposite as multi_manager. Instead of receiving a single input and triggering multiple outputs, multisource recieves multiple inputs, and will trigger its target if and only if every entity that targets the multisource has triggered said multisource. For basic use cases, this is all you need to know.

This entity is also the only entity that can be used as a master.
(Entities that specify a master cannot be enabled unless their master is enabled.)

It can used be create mechanisms. For example: a keycard for a slot. */
class HL_MultiSource extends HL_Triggers_Master;

struct FSouceEntity
{
	var Actor Other;
	var bool bTriggered;
};
var array<FSouceEntity> m_rgEntities;
var bool bCompleted;

function PostBeginPlay()
{
	local Actor A;
	local int i;
	
	if( Tag!='' )
	{
		foreach AllActors(class'Actor',A,,Tag)
		{
			m_rgEntities[i].Other = A;
			++i;
		}
	}
}
function bool IsTriggered( Pawn EventInstigator )
{
	return bCompleted;
}
function Trigger( Actor Other, Pawn EventInstigator )
{
	local int i;
	
	i = m_rgEntities.Find(Other,Other);
	if( i!=-1 )
		m_rgEntities[i].bTriggered = !m_rgEntities[i].bTriggered;
	bCompleted = false;
	
	for( i=(m_rgEntities.Size()-1); i>=0; --i )
		if( !m_rgEntities[i].bTriggered )
			return;

	bCompleted = true;
	TriggerEvent(Event,Self,EventInstigator);
	SendChangeNotify();
}
