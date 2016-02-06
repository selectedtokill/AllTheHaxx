#include <base/math.h>
#include <base/tl/array.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/mapitems.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>

#include <engine/external/astar-jps/AStar.h>
#include <engine/graphics.h>

#include "astar.h"
#include "effects.h"
#include "hud.h"

// TODO: make all of this shit optional

CAStar::CAStar()
{
	m_pField = NULL;
	m_PathFound = false;
	m_MapReloaded = false;
	OnReset();
}

CAStar::~CAStar()
{

}

void CAStar::OnReset() // is being called right after OnMapLoad()
{
	m_Path.clear();
	m_LineItems.clear();
	m_PathFound = false;
}

void CAStar::OnPlayerDeath()
{
	if(!m_PathFound)
		return;

	// fitness calculation
	int ClosestNode = 1337*1337;
	int ClosestID = -1;
	for(int i = m_Path.size(); i >= 0; i--)
	{
		// better make a SAFE CAAAAAAALLLL
		if(distance(m_Path[i], m_LastPos) < ClosestNode && !Collision()->IntersectLine(m_Path[i], m_LastPos, 0x0, 0x0, false))
		{
			ClosestNode = distance(m_Path[i], m_LastPos);
			ClosestID = i;
		}
	}

	if(g_Config.m_ClNotifications && ClosestID != -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Fitness Score: %i (%.2f%%)", m_Path.size()-ClosestID, (((float)m_Path.size()-(float)ClosestID)/(float)m_Path.size())*100.0f);
		m_pClient->m_pHud->PushNotification(aBuf);
	}
}

void CAStar::OnRender()
{
	// find the path one second after joining to be buffered
	{
		static int64 activationTime = 0;
		if(m_MapReloaded)
		{
			activationTime = time_get();
		}

		if(activationTime && time_get() > activationTime+time_freq())
		{
			FillGrid(true);
			BuildPath();
			activationTime = 0;
		}
		else
			m_MapReloaded = false;
	}

	const CNetObj_Character * pPlayerChar = m_pClient->m_Snap.m_pLocalCharacter;
	const CNetObj_Character * pPrevChar = m_pClient->m_Snap.m_pLocalPrevCharacter;

	if (pPlayerChar && pPrevChar)
		m_LastPos = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), Client()->IntraGameTick());

	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	// my dong is strong
	static int64 s_LastRender = 0;
	if(time_get()-s_LastRender < time_freq()*0.19f)
		return;

	// visualize the path
	for(int i = 0; i < m_Path.size(); i++)
		m_pClient->m_pEffects->BulletTrail(m_Path[i]); // TODO: redo this nicer

/*	TODO: implement lines
	Graphics()->LinesBegin();

	Graphics()->SetColor(1,0,0,1);
	Graphics()->LinesDraw(m_LineItems.base_ptr(), m_LineItems.size());
	Graphics()->LinesEnd();
*/
	s_LastRender = time_get();
}

int CAStar::GetStart()
{
	int NumTiles = 0;
	int aTiles[128];

	for(int y = 0; y < Collision()->GetHeight(); y++)
	{
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			// hack. but apperently this can indeed happen
			if(x < Collision()->GetWidth() && y < Collision()->GetHeight() && Collision()->GetIndex(x, y) == TILE_BEGIN)
				aTiles[NumTiles++] = Collision()->GetIndex(vec2(x*32, y*32));
		}
	}

	if(NumTiles)
		return aTiles[NumTiles/2]; // some tile from the middle
	else
		return -1;
}

int CAStar::GetFinish()
{
	int NumTiles = 0;
	int aTiles[128];

	for(int y = 0; y < Collision()->GetHeight(); y++)
	{
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			// hack. but apperently this can indeed happen
			if(x < Collision()->GetWidth() && y < Collision()->GetHeight() && Collision()->GetIndex(x, y) == TILE_END)
				aTiles[NumTiles++] = Collision()->GetIndex(vec2(x*32, y*32));
		}
	}

	if(NumTiles)
		return aTiles[NumTiles/2]; // some tile from the middle
	else
		return -1;
}

void CAStar::BuildPath()
{
	int Start = GetStart();
	int Finish = GetFinish();
	int SolutionLength = 0;
	int *pSolution = astar_compute((const char *)m_pField, &SolutionLength, Collision()->GetWidth(), Collision()->GetHeight(), Start, Finish);
	dbg_msg("path", "start=%i finish=%i solution length=%i", Start, Finish, SolutionLength);

	bool NoFreeze = true; // avoid freeze?
	if(SolutionLength == -1) // try again ignoring freeze
	{
		FillGrid(false);
		pSolution = astar_compute((const char *)m_pField, &SolutionLength, Collision()->GetWidth(), Collision()->GetHeight(), Start, Finish);
		bool NoFreeze = false;
	}

	if(g_Config.m_ClNotifications)
	{
		if(SolutionLength != -1)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Found path. Length: %i.%s", SolutionLength, NoFreeze?"":" (Ignored freeze)");
			m_pClient->m_pHud->PushNotification(aBuf);
		}
		else
			m_pClient->m_pHud->PushNotification("No possible path found.");
	}
	
	if(pSolution)
	{
		if(SolutionLength > 0)
		{
			m_PathFound = true;

			for(int i = 0; i < SolutionLength; i++)
				m_Path.add(Collision()->GetPos(pSolution[i]));

		/*	for(int i = 0; i < m_Path.size()-1; i++) // TODO: how to get screenpos from mappos?
			{
				IGraphics::CLineItem l;
				l.m_X0 = m_Path[i].x;
				l.m_X1 = m_Path[i+1].x;
				l.m_Y0 = m_Path[i].y;
				l.m_Y1 = m_Path[i+1].y;
				m_LineItems.add(l);
			}*/
		}
		free(pSolution);
	}
}

void CAStar::FillGrid(bool NoFreeze)
{
	if(m_pField)
		mem_free(m_pField);

	m_pField = (char *)mem_alloc(Collision()->GetWidth() * Collision()->GetHeight() * sizeof(char), 1);
	for(int y = 0; y < Collision()->GetHeight(); y++)
	{
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			if(Collision()->CheckPoint(x * 32, y * 32) || (NoFreeze && Collision()->GetIndex(x, y) == TILE_FREEZE))
				m_pField[Collision()->GetIndex(vec2(x*32, y*32))] = 0;
			else
				m_pField[Collision()->GetIndex(vec2(x*32, y*32))] = 1;
		}
	}
}

void CAStar::OnMapLoad()
{
	m_MapReloaded = true;
}
