class INV_HevSuit extends HL_Pickups;

var() bool bShortPickupAnnouncer;
var() VoiceLineObject ShortMessage,LongMessage,PowerLevels[20];

var transient map<VoiceLineObject,float> LastVoiceTime;
var transient VoiceLineObject PlayingVoice,PendingVoice[3];
var transient FX_VoiceLinePlayerHEV VoicePlayer;
var transient byte NumPendingVoice;

const WARN_5_MINS = {60.f * 5.f};
const WARN_10_MINS = {60.f * 10.f};
const WARN_30_MINS = {60.f * 30.f};

replication
{
	reliable if ( Role==ROLE_Authority )
		ClientPlayVoiceLine;
}

function PostBeginPlay()
{
	Charge = 0;
	Super.PostBeginPlay();
}

function inventory SpawnCopy( pawn Other )
{
	local INV_HevSuit Copy;

	Copy = INV_HevSuit(Super(Inventory).SpawnCopy(Other));
	Copy.bShortPickupAnnouncer = bShortPickupAnnouncer;
	return Copy;
}

function TravelPostAccept()
{
	bDisplayableInv = (Charge>0);
	bIsAnArmor = bDisplayableInv;
}

simulated function Destroyed()
{
	Super.Destroyed();
	if( VoicePlayer )
	{
		VoicePlayer.Destroy();
		VoicePlayer = None;
	}
}

function PickupFunction(Pawn Other)
{
	PlayVoiceLine(bShortPickupAnnouncer ? ShortMessage : LongMessage);
}

event float BotDesireability(Pawn Bot)
{
	return Bot.FindInventoryType(Class) ? -1.f : MaxDesireability;
}

function bool HandlePickupQuery( inventory Item )
{
	if( item.Class==Class )
		return true;
	else if( INV_SuitArmor(item)!=None && ChargeBattery(Item.Charge) )
	{
		Pawn(Owner).ClientMessage(Item.PickupMessage, 'Pickup');
		Item.PlaySound(Item.PickupSound,,2.0);
		Item.SetReSpawn();
		return true;
	}
	return (bool(Inventory) && Inventory.HandlePickupQuery(Item));
}

function bool ChargeBattery( int Amount )
{
	local int i;
	local float pct;

	if( Charge>=Default.Charge )
		return false;
	bDisplayableInv = True;
	bIsAnArmor = True;
	Charge = Min(Charge+Amount, Default.Charge);
	
	// Suit reports new power level
	pct = (float(Charge) / float(Default.Charge)) * 100.f + 0.5f;
	i = Clamp((int(pct) / 5) - 1, 0, 19);
	PlayVoiceLine(PowerLevels[i],30.f);
	return true;
}

simulated function PostNetReceive()
{
	bDisplayableInv = bIsAnArmor;
}

function inventory PrioritizeArmor( int Damage, name DamageType, vector HitLocation )
{
	local Inventory FirstArmor, InsertAfter;

	if ( Inventory != None )
		FirstArmor = Inventory.PrioritizeArmor(Damage, DamageType, HitLocation);
	else
		FirstArmor = None;
	if ( FirstArmor == None )
	{
		nextArmor = None;
		return self;
	}

	// insert this armor into the prioritized armor list
	if ( FirstArmor.ArmorPriority(DamageType) < ArmorPriority(DamageType) )
	{
		nextArmor = FirstArmor;
		return self;
	}
	InsertAfter = FirstArmor;
	while ( (InsertAfter.nextArmor != None)
			&& (InsertAfter.nextArmor.ArmorPriority(DamageType) > ArmorPriority(DamageType)) )
		InsertAfter = InsertAfter.nextArmor;

	nextArmor = InsertAfter.nextArmor;
	InsertAfter.nextArmor = self;
	return FirstArmor;
}
function int ArmorAbsorbDamage(int Damage, name DamageType, vector HitLocation)
{
	local int ArmorDamage;
	local bool ftrivial,fmajor,fcritical;

	if( DamageType=='Drowned' ) return Damage;

	if( bIsAnArmor && DamageType!='Fell' )
	{
		ArmorDamage = (Damage * ArmorAbsorption) / 100;
		if ( ArmorDamage >= Charge )
		{
			ArmorDamage = Charge;
			Charge = 0;
			bDisplayableInv = False;
			bIsAnArmor = False;
		}
		else
			Charge -= ArmorDamage;
		Damage-=ArmorDamage;
	}
	if( Damage<=0 )
		return 0;

	ArmorDamage = Instigator.Health-Damage;
	
	// how bad is it, doc?
	ftrivial = (ArmorDamage > 75 || Damage < 5);
	fmajor = (Damage > 25);
	fcritical = (ArmorDamage < 30);
	
	// Enviroment based damages.
	switch( DamageType )
	{
	case 'Bumped':
		if (fmajor)
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG4', 30.f); // minor fracture
		break;
	case 'Fell':
	case 'Crushed':
		if (fmajor)
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG5', 30.f); // major fracture
		else
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG4', 30.f); // minor fracture
		break;
	case 'Shot':
		if (Damage > 5)
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG6', 30.f); // blood loss detected
		break;
	case 'Hacked':
		if (fmajor)
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG1', 30.f); // major laceration
		else
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG0', 30.f); // minor laceration
		break;
	case 'Zapped':
		if (fmajor)
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG2', 60.f); // internal bleeding
		break;
	case 'Poisoned':
		PlayVoiceLine(VoiceLineObject'VX_HEV_DMG3', 60.f); // blood toxins detected
		break;
	case 'Corroded':
		PlayVoiceLine(VoiceLineObject'VX_HEV_DET1', 60.f); // hazardous chemicals detected
		break;
	case 'Toxin':
		PlayVoiceLine(VoiceLineObject'VX_HEV_DET0', 60.f); // biohazard detected
		break;
	case 'Radiation':
		PlayVoiceLine(VoiceLineObject'VX_HEV_DET2', 60.f); // radiation detected
		break;
	}
	
	if (!ftrivial && fmajor && Instigator.Health >= 75)
	{
		// first time we take major damage...
		// turn automedic on if not on
		PlayVoiceLine(VoiceLineObject'VX_HEV_MED1', WARN_30_MINS); // automedic on

		// give morphine shot if not given recently
		PlayVoiceLine(VoiceLineObject'VX_HEV_HEAL7', WARN_30_MINS); // morphine shot
	}
	else if (!ftrivial && fcritical && Instigator.Health < 75)
	{
		// already took major damage, now it's critical...
		if (ArmorDamage < 6)
			PlayVoiceLine(VoiceLineObject'VX_HEV_HLTH3', WARN_10_MINS); // near death
		else if (ArmorDamage < 20)
			PlayVoiceLine(VoiceLineObject'VX_HEV_HLTH2', WARN_10_MINS); // health critical

		// give critical health warnings
		if (Rand(3)==0 && Instigator.Health < 50)
			PlayVoiceLine(VoiceLineObject'VX_HEV_DMG7', WARN_5_MINS); //seek medical attention
	}
	// if we're taking time based damage, warn about its continuing effects
	else if (Damage=='Zapped' && Instigator.Health < 75)
	{
		if (Instigator.Health < 50)
		{
			if (Rand(3)==0)
				PlayVoiceLine(VoiceLineObject'VX_HEV_DMG7', WARN_5_MINS); //seek medical attention
		}
		else
			PlayVoiceLine(VoiceLineObject'VX_HEV_HLTH1', WARN_10_MINS); // health dropping
	}
	
	return Damage;
}
function int ArmorPriority(name DamageType)
{
	if ( DamageType == 'Drowned' || DamageType=='Fell' )
		return 0;
	return bIsAnArmor ? AbsorptionPriority : 0;
}

function PlayVoiceLine( VoiceLineObject Line, optional float Delay )
{
	if( Line==None || PlayerPawn(Owner)==None )
		return;
	if( Delay>0.f )
	{
		if( LastVoiceTime[Line]>Level.TimeSeconds )
			return;
		LastVoiceTime[Line] = Level.TimeSeconds + Delay;
	}
	ClientPlayVoiceLine(Line);
}
simulated function ClientPlayVoiceLine( VoiceLineObject Line )
{
	if( Line==None || Level.NetMode==NM_DedicatedServer || PlayerPawn(Owner)==None || Viewport(PlayerPawn(Owner).Player)==None )
		return;

	if( PlayingVoice )
	{
		if( NumPendingVoice<ArrayCount(PendingVoice) )
			PendingVoice[NumPendingVoice++] = Line;
		return;
	}
	if( VoicePlayer==None )
	{
		VoicePlayer = Spawn(class'FX_VoiceLinePlayerHEV',Owner);
		VoicePlayer.SuitOwner = Self;
	}
	PlayingVoice = Line;
	VoicePlayer.PlayVoiceLine(Line);
}
simulated function FinishedSpeech()
{
	if( NumPendingVoice )
	{
		PlayingVoice = PickNextQue();
		VoicePlayer.PlayVoiceLine(PlayingVoice);
	}
	else PlayingVoice = None;
}
simulated final function VoiceLineObject PickNextQue()
{
	local byte i;
	local VoiceLineObject Result;
	
	if( NumPendingVoice==0 )
		return None;
	Result = PendingVoice[0];
	if( NumPendingVoice>1 )
	{
		for( i=1; i<NumPendingVoice; ++i )
			PendingVoice[i-1] = PendingVoice[i];
	}
	--NumPendingVoice;
	return Result;
}

defaultproperties
{
	Mesh=W_Suit
	PickupViewMesh=W_Suit
	PickupMessage="You got the H.E.V. Suit"
	ItemName="H.E.V. Suit"
	CollisionRadius=12
	CollisionHeight=30
	RespawnTime=1.5
	Charge=100
	Icon=Texture'UnrealShare.Icons.I_Armor'
	ArmorAbsorption=80
	AbsorptionPriority=7
	bNetNotify=true
	PickupSound=Sound'UnrealShare.Pickups.suitsnd'
	
	ShortMessage=VX_HEV_A0
	LongMessage=VX_HEV_AAx
	PowerLevels(0)=VX_HEV_0P
	PowerLevels(1)=VX_HEV_1P
	PowerLevels(2)=VX_HEV_2P
	PowerLevels(3)=VX_HEV_3P
	PowerLevels(4)=VX_HEV_4P
	PowerLevels(5)=VX_HEV_5P
	PowerLevels(6)=VX_HEV_6P
	PowerLevels(7)=VX_HEV_7P
	PowerLevels(8)=VX_HEV_8P
	PowerLevels(9)=VX_HEV_9P
	PowerLevels(10)=VX_HEV_10P
	PowerLevels(11)=VX_HEV_11P
	PowerLevels(12)=VX_HEV_12P
	PowerLevels(13)=VX_HEV_13P
	PowerLevels(14)=VX_HEV_14P
	PowerLevels(15)=VX_HEV_15P
	PowerLevels(16)=VX_HEV_16P
	PowerLevels(17)=VX_HEV_17P
	PowerLevels(18)=VX_HEV_18P
	PowerLevels(19)=VX_HEV_19P
}
