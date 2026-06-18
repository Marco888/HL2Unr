Class HL_TriggerMessage extends HL_Triggers;

var() localized string Message; // Message to be shown.
var() color MessageColor;
var() color MessageHiColor;
var() vector2d MessagePosition;
var() float MessageFadeIn,MessageFadeOut,MessageDisplayTime,CharsPerSec,HighlightFadeTime;

var() Sound MessageSound; // When the message is shown, this sound effect will be played, originating from this entity.
var() float MessageVolume; // Volume of the sound effect.
var() enum ESoundAtten
{
	MSA_Small,
	MSA_Medium,
	MSA_Large,
	MSA_Everywhere,
} MessageAttenuate; // How big the radius of the sound is. 

var() bool sf_PlayOnce;
var() bool sf_AllClients;

var transient array<string> MsgList;
var transient int MaxSize;
var transient float TotalMsgTime,CharFadeInTime;
var transient vector2d TextPosition,TextSize;
var transient bool bMsgInit,bHasHighlightColor;

function Trigger( actor Other, pawn EventInstigator )
{
	local PlayerPawn P;
	local HL_HUD_Overlay M;
	
	if( sf_PlayOnce )
		Disable('Trigger');
	if( sf_AllClients )
	{
		foreach AllActors(class'PlayerPawn',P)
		{
			M = Class'HL_HUD_Overlay'.Static.GetOverlay(P);
			if( M )
				M.ShowScreenMsg(Self);
		}
	}
	else if( PlayerPawn(EventInstigator) )
	{
		M = Class'HL_HUD_Overlay'.Static.GetOverlay(PlayerPawn(EventInstigator));
		if( M )
			M.ShowScreenMsg(Self);
	}
}
simulated final function InitMessages( Canvas Canvas )
{
	local int i,j;
	local string S;
	local float XL,YL;

	bMsgInit = true;
	bHasHighlightColor = (MessageHiColor.R || MessageHiColor.G || MessageHiColor.B);
	MaxSize = 0;
	S = Message;
	TextSize.X = 0.f;
	TextSize.Y = 0.f;
	while( true )
	{
		i = InStr(S,"|");
		if( i==-1 )
		{
			MaxSize += Len(S);
			MsgList[j++] = S;
			break;
		}
		else
		{
			MaxSize += i;
			MsgList[j++] = Left(S,i);
			S = Mid(S,i+1);
		}
	}
	for( i=0; i<j; ++i )
	{
		Canvas.TextSize(MsgList[i],XL,YL);
		TextSize.X = FMax(TextSize.X,XL);
		TextSize.Y+=YL;
	}
	if( CharsPerSec<=0.f )
		CharFadeInTime = MessageFadeIn;
	else
	{
		CharsPerSec = FClamp(CharsPerSec,1.f,100000.f);
		CharFadeInTime = (float(MaxSize)/CharsPerSec) + MessageFadeIn;
		if( bHasHighlightColor )
			CharFadeInTime+=HighlightFadeTime;
	}
	TotalMsgTime = CharFadeInTime + MessageDisplayTime + MessageFadeOut;
	
	if( MessagePosition.X<0.f )
		TextPosition.X = (Canvas.ClipX-TextSize.X)*0.5f;
	else TextPosition.X = (MessagePosition.X*Canvas.ClipX);

	if( MessagePosition.Y<0.f )
		TextPosition.Y = (Canvas.ClipY-TextSize.Y)*0.4f;
	else TextPosition.Y = (MessagePosition.Y*Canvas.ClipY);
}
simulated final function bool MessagesOverlap( HL_TriggerMessage M )
{
	return ((TextPosition.X>=M.TextPosition.X) && (TextPosition.X<=(M.TextPosition.X+M.TextSize.X)) || (M.TextPosition.X>=TextPosition.X) && (M.TextPosition.X<=(TextPosition.X+TextSize.X))
			&& (TextPosition.Y>=M.TextPosition.Y) && (TextPosition.Y<=(M.TextPosition.Y+M.TextSize.Y)) || (M.TextPosition.Y>=TextPosition.Y) && (M.TextPosition.Y<=(TextPosition.Y+TextSize.Y)));
}

defaultproperties
{
	bStatic=true
	MessageColor=(R=100,G=100,B=100)
	MessageHiColor=(R=255,G=255,B=255)
	MessagePosition=(X=-1,Y=-1)
	MessageFadeIn=0.1
	MessageFadeOut=0.25
	MessageDisplayTime=3
	MessageVolume=1
	MessageAttenuate=MSA_Medium
	HighlightFadeTime=0.15
	CharsPerSec=20
	
	sf_AllClients=true
}