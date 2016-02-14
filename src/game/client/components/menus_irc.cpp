#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/keys.h>

#include "irc.h"
#include "menus.h"

void CMenus::ConKeyShortcutIRC(IConsole::IResult *pResult, void *pUserData)
{
	CMenus *pSelf = (CMenus *)pUserData;
	if(pSelf->Client()->State() == IClient::STATE_ONLINE)
	{
		if(pResult->GetInteger(0) != 0)
			pSelf->m_IRCActive ^= 1;
	}
}

// stolen from H-Client :3
void CMenus::RenderIrc(CUIRect MainView)
{
	if(!m_IRCActive)
	{
		m_IRCWasActive = false;
		return;
	}

	m_IRCWasActive = true;

	// small0r
	MainView.x = 50;
	MainView.y = 50;
	MainView.w -= 100;
	MainView.h -= 100;

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	Graphics()->BlendNormal();

	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActiveIngame-vec4(0.0f, 0.0f, 0.0f, 0.2f), CUI::CORNER_ALL, 5.0f);

	MainView.HSplitTop(15.0f, 0, &MainView);
	MainView.VSplitLeft(15.0f, 0, &MainView);

	MainView.Margin(5.0f, &MainView);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActiveIngame-vec4(0.0f, 0.0f, 0.0f, 0.2f), CUI::CORNER_ALL, 5.0f);

	CUIRect MainIrc, EntryBox, Button;
	MainView.Margin(10.0f, &MainIrc);

	/*if (m_GamePagePanel != PANEL_CHAT && UI()->MouseInside(&MainView) && Input()->KeyPressed(KEY_MOUSE_1))
	 {
	 m_GamePagePanel = PANEL_CHAT;
	 }*/

	if(m_pClient->Irc()->GetState() == IIrc::STATE_DISCONNECTED)
	{
		EntryBox.x = MainIrc.x + (MainIrc.w / 2.0f - 300.0f / 2.0f);
		EntryBox.w = 300.0f;
		EntryBox.y = MainIrc.y + (MainIrc.h / 2.0f - 55.0f / 2.0f);
		EntryBox.h = 55.0f;

		RenderTools()->DrawUIRect(&EntryBox, ms_ColorTabbarActive-vec4(0.0f, 0.0f, 0.0f, 0.2f), CUI::CORNER_ALL, 10.0f);
		EntryBox.Margin(5.0f, &EntryBox);

		EntryBox.HSplitTop(18.0f, &Button, &EntryBox);
		CUIRect Label;
		Button.VSplitLeft(40.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Nick:"), 14.0f, -1);
		static float OffsetNick;
		if(g_Config.m_ClIRCNick[0] == 0)
		{
			str_copy(g_Config.m_ClIRCNick, g_Config.m_PlayerName, sizeof(g_Config.m_ClIRCNick));
			str_irc_sanitize(g_Config.m_ClIRCNick);
		} //TODO_ here?
		DoEditBox(&g_Config.m_ClIRCNick, &Button, g_Config.m_ClIRCNick, sizeof(g_Config.m_ClIRCNick), 12.0f, &OffsetNick,
				false, CUI::CORNER_ALL);

		EntryBox.HSplitTop(5.0f, 0x0, &EntryBox);
		EntryBox.HSplitTop(20.0f, &Button, &EntryBox);
		static float s_ButtonConnect = 0;
		if(DoButton_Menu(&s_ButtonConnect, Localize("Connect"), 0, &Button))
			m_pClient->m_pIrcBind->Connect();
	}
	else if(m_pClient->Irc()->GetState() == IIrc::STATE_CONNECTING)
	{
		EntryBox.x = MainIrc.x + (MainIrc.w / 2.0f - 300.0f / 2.0f);
		EntryBox.w = 300.0f;
		EntryBox.y = MainIrc.y + (MainIrc.h / 2.0f - 25.0f / 2.0f);
		EntryBox.h = 25.0f;

		RenderTools()->DrawUIRect(&EntryBox, ms_ColorTabbarActive-vec4(0.0f, 0.0f, 0.0f, 0.2f), CUI::CORNER_ALL, 10.0f);
		EntryBox.Margin(5.0f, &EntryBox);
		UI()->DoLabelScaled(&EntryBox, Localize("Connecting, please wait..."), 14.0f, -1);
	}
	else if(m_pClient->Irc()->GetState() == IIrc::STATE_CONNECTED)
	{
		CUIRect ButtonBox, InputBox;

		// channel list
		MainIrc.HSplitTop(20.0f, &ButtonBox, &EntryBox);
		ButtonBox.VSplitRight(80.0f, &ButtonBox, &Button);
		static float s_ButtonDisc = 0;
		if(DoButton_Menu(&s_ButtonDisc, Localize("Disconnect"), 0, &Button))
			m_pClient->m_pIrcBind->Disconnect(g_Config.m_ClIRCLeaveMsg);

		// scroll through the tabs
		if(UI()->MouseInside(&ButtonBox))
		{
			if(m_pClient->Input()->KeyDown(KEY_MOUSE_WHEEL_UP))
				m_pClient->Irc()->NextRoom();
			else if(m_pClient->Input()->KeyDown(KEY_MOUSE_WHEEL_DOWN))
				m_pClient->Irc()->PrevRoom();
		}

		float LW = (ButtonBox.w - ButtonBox.x) / m_pClient->Irc()->GetNumComs();
		static int s_ButsID[64];
		for(int i = 0; i < m_pClient->Irc()->GetNumComs(); i++)
		{
			CIrcCom *pCom = m_pClient->Irc()->GetCom(i);

		//	if(pCom == m_pClient->Irc()->GetActiveCom())
				ButtonBox.VSplitLeft(LW - 25.0f, &Button, &ButtonBox);
		//	else
		//	{
		//		ButtonBox.VSplitLeft(LW, &Button, &ButtonBox);
		//		Button.VSplitRight(2.0f, &Button, 0x0);
		//	}

			// close using middle mouse button
			if(UI()->MouseInside(&Button) && m_pClient->Input()->KeyDown(KEY_MOUSE_3) &&
					m_pClient->Irc()->CanCloseCom(m_pClient->Irc()->GetCom(i)))
				m_pClient->Irc()->Part(g_Config.m_ClIRCLeaveMsg, m_pClient->Irc()->GetCom(i));

			if(pCom->GetType() == CIrcCom::TYPE_CHANNEL)
			{
				CComChan *pChan = static_cast<CComChan*>(pCom);
				static float FadeVal[64] = { 0.0f };
				static bool Add[64] = { true };

				if(Add[i])
					smooth_set(&FadeVal[i], 1.0f, 70.0f, 0);
				else
					smooth_set(&FadeVal[i], 0.0f, 70.0f, 0);
				if(FadeVal[i] >= 0.8f) Add[i] = false;
				if(FadeVal[i] <= 0.2f) Add[i] = true;

				char aTab[255];
				if(pCom->m_UnreadMsg)
				{
					str_format(aTab, sizeof(aTab), "%s [%d]", pChan->m_Channel, pCom->m_NumUnreadMsg);
					if(DoButton_MenuTab(&s_ButsID[i], aTab, pCom->m_UnreadMsg, &Button, i==m_pClient->Irc()->GetNumComs()-1?CUI::CORNER_R:0, vec4(0.0f, FadeVal[i], 0.0f, 1.0f)))
						m_pClient->Irc()->SetActiveCom(i);
				}
				else
				{
					FadeVal[i] = 0.0f; Add[i] = true;
					str_copy(aTab, pChan->m_Channel, sizeof(aTab));
					if(DoButton_MenuTab(&s_ButsID[i], aTab, pCom == m_pClient->Irc()->GetActiveCom(), &Button, i==m_pClient->Irc()->GetNumComs()-1?CUI::CORNER_R:0))
						m_pClient->Irc()->SetActiveCom(i);
				}
			}
			else if(pCom->GetType() == CIrcCom::TYPE_QUERY)
			{
				CComQuery *pQuery = static_cast<CComQuery*>(pCom);
				static float FadeVal[64] = { 0.0f };
				static bool Add[64] = { true };

				if(Add[i])
					smooth_set(&FadeVal[i], 1.0f, 70.0f, 0);
				else
					smooth_set(&FadeVal[i], 0.0f, 70.0f, 0);
				if(FadeVal[i] >= 0.8f) Add[i] = false;
				if(FadeVal[i] <= 0.2f) Add[i] = true;

				char aTab[255];
				if(pCom->m_UnreadMsg)
				{
					str_format(aTab, sizeof(aTab), "%s [%d]", pQuery->m_User, pCom->m_NumUnreadMsg);
					if(DoButton_MenuTab(&s_ButsID[i], aTab, pCom->m_UnreadMsg, &Button, i==m_pClient->Irc()->GetNumComs()-1?CUI::CORNER_R:0, vec4(0.0f, FadeVal[i], 0.0f, 1.0f)))
						m_pClient->Irc()->SetActiveCom(i);
				}
				else
				{
					FadeVal[i] = 0.0f; Add[i] = true;
					str_copy(aTab, pQuery->m_User, sizeof(aTab));
					if(DoButton_MenuTab(&s_ButsID[i], aTab, pCom == m_pClient->Irc()->GetActiveCom(), &Button, i==m_pClient->Irc()->GetNumComs()-1?CUI::CORNER_R:0))
						m_pClient->Irc()->SetActiveCom(i);
				}
			}

			if(i > 0 && pCom == m_pClient->Irc()->GetActiveCom() && m_pClient->Irc()->GetNumComs() > 2 && str_comp_nocase(((CComChan*)pCom)->m_Channel, "#AllTheHaxx"))
			{
				Button.VSplitRight(ButtonBox.h, 0, &Button);
				Button.Margin(3.0f, &Button);
				Button.x -= 5.0f; Button.h = Button.w;
				static int sCloseButton = 0;
				if(DoButton_Menu(&sCloseButton, "×", 0, &Button, 0, CUI::CORNER_ALL, ms_ColorTabbarActive+vec4(0.3f,0.3f,0.3f,0)))
					m_pClient->Irc()->Part(g_Config.m_ClIRCLeaveMsg);
			}
		}

		// Input Box
		EntryBox.HSplitBottom(20.0f, &EntryBox, &InputBox);
		InputBox.VSplitRight(50.0f, &InputBox, &Button);
		//Button.VSplitLeft(5.0f, 0x0, &Button);
		static char aEntryText[500];
		static float s_Offset;
		DoEditBox(&aEntryText, &InputBox, aEntryText, sizeof(aEntryText), 12.0f, &s_Offset, false, CUI::CORNER_L);
		static float s_ButtonSend = 0;
		if(DoButton_Menu(&s_ButtonSend, Localize("Send"), 0, &Button, 0, CUI::CORNER_R, vec4(1,1,1,0.6f))
				|| m_EnterPressed)
		{
			if(aEntryText[0] == '/'/* || (m_pClient->Irc()->GetActiveCom()->GetType() == CIrcCom::TYPE_QUERY &&
					str_comp_nocase(((CComQuery*)m_pClient->Irc()->GetActiveCom())->m_User, "@Status") == 0)*/)
			{
				std::string strCmdRaw;
				//if(str_comp_nocase(((CComQuery*)m_pClient->Irc()->GetActiveCom())->m_User, "@Status") == 0)
				//	strCmdRaw = aEntryText;
				//else
					strCmdRaw = aEntryText + 1;
				char aCmd[32] = { 0 }, aCmdParams[255] = { 0 };
				size_t del = strCmdRaw.find_first_of(" ");
				if(del != std::string::npos)
				{
					str_copy(aCmd, strCmdRaw.substr(0, del).c_str(), sizeof(aCmd));
					str_copy(aCmdParams, strCmdRaw.substr(del + 1).c_str(), sizeof(aCmdParams));
				}
				else
					str_copy(aCmd, strCmdRaw.c_str(), sizeof(aCmd));

				if(aCmd[0] != 0)
					m_pClient->Irc()->ExecuteCommand(aCmd, aCmdParams);
			}
			else
				m_pClient->Irc()->SendMsg(0x0, aEntryText);

			aEntryText[0] = 0;
		}

		//Channel/Query
		CIrcCom *pCom = m_pClient->Irc()->GetActiveCom();
		if(!pCom)
			return;

		if(pCom->GetType() == CIrcCom::TYPE_CHANNEL)
		{
			CComChan *pChan = static_cast<CComChan*>(pCom);

			CUIRect Chat, UserList;
			EntryBox.Margin(5.0f, &EntryBox);
			EntryBox.VSplitRight(150.0f, &Chat, &UserList);

			static int Selected = 0;
			static int s_UsersList = 0;
			static float s_UsersScrollValue = 0;
			char aBuff[50];
			str_format(aBuff, sizeof(aBuff), "Total: %d", pChan->m_Users.size());
			UiDoListboxStart(&s_UsersList, &UserList, 18.0f, "Users", aBuff, pChan->m_Users.size(), 1, Selected,
					s_UsersScrollValue, CUI::CORNER_TR);

			int o = 0;
			std::list<std::string>::iterator it = pChan->m_Users.begin();
			while(it != pChan->m_Users.end())
			{
				CListboxItem Item = UiDoListboxNextItem(&(*it));

				if(Item.m_Visible)
				{
					if(Selected == o)
					{
						CUIRect Label, ButtonQS;
						Item.m_Rect.VSplitRight(Item.m_Rect.h, &Label, &ButtonQS);
						UI()->DoLabelScaled(&Label, (*it).c_str(), 12.0f, -1);
						if(UI()->DoButtonLogic(&Item.m_Selected, "", Selected, &Label))
						{
							std::list<std::string>::iterator it = pChan->m_Users.begin();
							std::advance(it, o);

							if(str_comp_nocase(it->c_str()+1, m_pClient->Irc()->GetNick()) != 0)
								m_pClient->Irc()->OpenQuery(it->c_str());
						}

						//DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_CONNECT, &ButtonQS,
						//		vec4(0.47f, 0.58f, 0.72f, 1.0f));
						if(UI()->DoButtonLogic(&Item.m_Visible, "", Selected, &ButtonQS))
						{
							std::list<std::string>::iterator it = pChan->m_Users.begin();
							std::advance(it, o);

							m_pClient->Irc()->SendGetServer(it->c_str());
						}
					}
					else
						UI()->DoLabelScaled(&Item.m_Rect, it->c_str(), 12.0f, -1);
				}

				o++;
				it++;
			}
			Selected = UiDoListboxEnd(&s_UsersScrollValue, 0);

			static int s_Chat = 0;
			static float s_ChatScrollValue = 100.0f;
			UiDoListboxStart(&s_Chat, &Chat, 12.0f,
					pChan->m_Topic.c_str()[0] ? pChan->m_Topic.c_str() : "", "",
					pChan->m_Buffer.size(), 1, -1, s_ChatScrollValue, CUI::CORNER_TL);
			for(size_t i = 0; i < pChan->m_Buffer.size(); i++)
			{
				CListboxItem Item = UiDoListboxNextItem(&pChan->m_Buffer[i]);

				if(Item.m_Visible)
					UI()->DoLabelScaled(&Item.m_Rect, pChan->m_Buffer[i].c_str(), 10.0f, -1);
			}
			UiDoListboxEnd(&s_ChatScrollValue, 0);
		}
		else if(pCom->GetType() == CIrcCom::TYPE_QUERY)
		{
			CComQuery *pQuery = static_cast<CComQuery*>(pCom);
			CUIRect Chat;
			EntryBox.Margin(5.0f, &Chat);

			static int s_Chat = 0;
			static float s_ChatScrollValue = 100.0f;
			UiDoListboxStart(&s_Chat, &Chat, 12.0f, pQuery->m_User, "", pQuery->m_Buffer.size(), 1, -1,
					s_ChatScrollValue);
			for(size_t i = 0; i < pQuery->m_Buffer.size(); i++)
			{
				CListboxItem Item = UiDoListboxNextItem(&pQuery->m_Buffer[i]);

				if(Item.m_Visible)
					UI()->DoLabelScaled(&Item.m_Rect, pQuery->m_Buffer[i].c_str(), 10.0f, -1);
			}
			UiDoListboxEnd(&s_ChatScrollValue, 0);

			if(str_comp_nocase(pQuery->m_User, "@status") != 0)
			{
				CUIRect ButtonQS;
				Chat.VSplitRight(32.0f, 0x0, &ButtonQS);
				ButtonQS.h = 32.0f;
				ButtonQS.x -= 20.0f;
				ButtonQS.y += 25.0f;
				RenderTools()->DrawUIRect(&ButtonQS, vec4(0.2f, 0.6f, 0.4f, UI()->MouseInside(&ButtonQS) ? 1.0f : 0.6f),
						CUI::CORNER_ALL, 15.0f);
				ButtonQS.x += 5.0f;
				ButtonQS.y += 7.0f;
				UI()->DoLabelScaled(&ButtonQS, "Join", 12.0f, -1);
				//DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_CONNECT, &ButtonQS, vec4(0.47f, 0.58f, 0.72f, 1.0f));
				static int s_ButtonQSLog = 0;
				if(UI()->DoButtonLogic(&s_ButtonQSLog, "", 0, &ButtonQS))
				{
					m_pClient->Irc()->SendGetServer(pQuery->m_User);
				}
			}
		}
	}
}
