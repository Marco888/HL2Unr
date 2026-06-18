Class HL_HUD_Overlay extends HudOverlay;

struct FMsgQue
{
	var HL_TriggerMessage Msg;
	var float Time;
	var bool bNewMsg;
};
var array<FMsgQue> MsgList;
var bool bLocalOwner;

var HL_EnvFade ScreenFade;
var float ScreenFadeTime;

static final function HL_HUD_Overlay GetOverlay( PlayerPawn P )
{
	local HL_HUD_Overlay H;
	
	if( P.Player==None )
		return None;
	foreach P.ChildActors(class'HL_HUD_Overlay',H)
		return H;
	return P.Spawn(class'HL_HUD_Overlay',P);
}
function OwnerChanged()
{
	if( Owner==None )
		Destroy();
}

simulated function PostBeginPlay()
{
	local PlayerPawn P;
	
	if( Level.NetMode==NM_Client )
	{
		bLocalOwner = true;
		P = GetLocalPlayerPawn();
		myHUD = P.myHUD;
		SetOwner(P);
		if( myHUD )
			myHUD.Overlays.Add(Self);
		else SetTimer(0.05,false);
	}
	else if( Level.NetMode!=NM_DedicatedServer )
	{
		P = PlayerPawn(Owner);
		if( Viewport(P.Player) )
		{
			bLocalOwner = true;
			myHUD = P.myHUD;
			if( myHUD )
				myHUD.Overlays.Add(Self);
			else SetTimer(0.05,false);
		}
	}
}
simulated function Timer()
{
	PostBeginPlay();
}

reliable client function ShowScreenMsg( HL_TriggerMessage M )
{
	local int i;

	if( !bLocalOwner || !M )
		return;
	i = MsgList.Size();
	MsgList[i].Msg = M;
	MsgList[i].Time = Level.RealTimeSeconds;
	MsgList[i].bNewMsg = true;
	bPostRender = true;
}
reliable client function SetScreenFade( HL_EnvFade F )
{
	if( !bLocalOwner || !F )
		return;
	ScreenFade = F;
	ScreenFadeTime = Level.RealTimeSeconds;
	bPostRender = true;
}

simulated event PostRender( canvas Canvas )
{
	if( ScreenFade )
		RenderScreenFade(Canvas);
	if( MsgList.Size() )
	{
		Canvas.PushCanvasScale(class'HUD'.Default.HudScaler);
		RenderScreenMsg(Canvas);
		Canvas.PopCanvasScale();
	}
	Canvas.Style = ERenderStyle.STY_Normal;
	if( MsgList.Size()==0 && !ScreenFade )
		bPostRender = false;
}
simulated final function RenderScreenFade( Canvas Canvas )
{
	local float T,Opaq;
	
	T = Level.RealTimeSeconds-ScreenFadeTime;
	Canvas.DrawColor = ScreenFade.FadeColor;
	Opaq = ScreenFade.FadeOpacity;
	
	if( ScreenFade.sf_FadeFrom )
	{
		T-=ScreenFade.HoldTime;
		if( T>0.f )
		{
			if( T<ScreenFade.Duration )
				Opaq *= (1.f - (T/ScreenFade.Duration));
			else
			{
				ScreenFade = None;
				return;
			}
		}
	}
	else if( T<ScreenFade.Duration )
		Opaq *= (1.f - (T/ScreenFade.Duration));
	else if( !ScreenFade.sf_StayOut )
	{
		T-=(ScreenFade.Duration+ScreenFade.HoldTime);
		if( T>0.f )
		{
			if( T<ScreenFade.Duration )
				Opaq *= (1.f - (T/ScreenFade.Duration));
			else
			{
				ScreenFade = None;
				return;
			}
		}
	}
	
	Canvas.Style = ERenderStyle.STY_AlphaBlend;
	Canvas.DrawColor.A = int(Opaq*255.f);
	Canvas.SetPos(0.f,0.f);
	Canvas.DrawTile(Texture'WhiteTexture', Canvas.ClipX, Canvas.ClipY, 0, 0, 1, 1);
}
simulated final function RenderScreenMsg( Canvas Canvas )
{
	local int i,n,j,l,nl,endchar;
	local float T,CT,SHT,HT,XL,YL,BaseX,XPos,YPos,ChDelta;
	local HL_TriggerMessage M,MB;
	local string S;
	
	n = MsgList.Size();
	Canvas.Style = ERenderStyle.STY_Translucent;
	Canvas.Font = Font'WhiteFont';
	for( i=0; i<n; ++i )
	{
		M = MsgList[i].Msg;
		if( !M.bMsgInit )
			M.InitMessages(Canvas);
		if( MsgList[i].bNewMsg )
		{
			MsgList[i].bNewMsg = false;
			
			// Kill any messages underneath that is overlapping.
			for( j=0; j<i; ++j )
			{
				MB = MsgList[j].Msg;
				if( M.MessagesOverlap(MB) )
				{
					T = Level.RealTimeSeconds-MsgList[j].Time;
					MsgList[j].Time = FMin(MsgList[j].Time,Level.RealTimeSeconds-(MB.TotalMsgTime-MB.MessageFadeOut));
				}
			}
		}
		T = Level.RealTimeSeconds-MsgList[i].Time;
		if( T>=M.TotalMsgTime )
		{
			MsgList.Remove(i--);
			--n;
			continue;
		}
		BaseX = M.TextPosition.X;
		YPos = M.TextPosition.Y;
		nl = M.MsgList.Size();
		if( T<M.CharFadeInTime )
		{
			SHT = M.MessageFadeIn*0.45f;
			HT = M.MessageFadeIn;
			if( M.bHasHighlightColor )
				HT+=M.HighlightFadeTime;
			
			if( M.CharsPerSec<=0.f )
			{
				if( M.bHasHighlightColor )
				{
					if( T<SHT )
						Canvas.DrawColor = M.MessageHiColor*(T/SHT);
					else if( T<(HT-SHT) )
						Canvas.DrawColor = M.MessageHiColor;
					else if( T<HT )
					{
						CT = (T-(HT-SHT))/SHT;
						Canvas.DrawColor = (M.MessageHiColor*(1.f-CT)) + (M.MessageColor*CT);
					}
					else Canvas.DrawColor = M.MessageColor;
				}
				else
				{
					CT = T/M.CharFadeInTime;
					Canvas.DrawColor = (M.MessageColor*CT);
				}
				
				Canvas.TextSize("ABC",XL,YL);
				for( l=0; l<nl; ++l )
				{
					Canvas.SetPos(BaseX,YPos);
					Canvas.DrawText(M.MsgList[l]);
					YPos+=YL;
				}
			}
			else
			{
				ChDelta = 1.f/M.CharsPerSec;
				for( l=0; (l<nl && T>0.f); ++l )
				{
					XPos = BaseX;
					endchar = Len(M.MsgList[l]);
					for( j=0; (j<endchar && T>0.f); ++j, T-=ChDelta )
					{
						S = Mid(M.MsgList[l],j,1);
						if( S==" " )
						{
							Canvas.TextSize(S,XL,YL);
							XPos+=XL;
							continue;
						}
						if( M.bHasHighlightColor )
						{
							if( T<SHT )
								Canvas.DrawColor = M.MessageHiColor*(T/SHT);
							else if( T<(HT-SHT) )
								Canvas.DrawColor = M.MessageHiColor;
							else if( T<HT )
							{
								CT = (T-(HT-SHT))/SHT;
								Canvas.DrawColor = (M.MessageHiColor*(1.f-CT)) + (M.MessageColor*CT);
							}
							else Canvas.DrawColor = M.MessageColor;
						}
						else
						{
							if( T<HT )
							{
								CT = T/HT;
								Canvas.DrawColor = (M.MessageColor*CT);
							}
							else Canvas.DrawColor = M.MessageColor;
						}
						
						Canvas.SetPos(XPos,YPos);
						Canvas.DrawText(S);
						Canvas.TextSize(S,XL,YL);
						XPos+=XL;
					}
					Canvas.TextSize("ABC",XL,YL);
					YPos+=YL;
				}
			}
		}
		else
		{
			T = M.TotalMsgTime-T;
			if( T<M.MessageFadeOut )
				Canvas.DrawColor = M.MessageColor * (T/M.MessageFadeOut);
			else Canvas.DrawColor = M.MessageColor;
			Canvas.TextSize("ABC",XL,YL);
			for( l=0; l<nl; ++l )
			{
				Canvas.SetPos(BaseX,YPos);
				Canvas.DrawText(M.MsgList[l]);
				YPos+=YL;
			}
		}
	}
}

defaultproperties
{
	bSkipActorReplication=true
	NetUpdateFrequency=0.25
	RemoteRole=ROLE_SimulatedProxy
	bOnlyDirtyReplication=true
	bPostRender=false
}