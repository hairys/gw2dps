#include "stdafx.h"
#include "gw2dps.h"

// Switches //
bool help = false;
bool targetSelected = true;
bool targetLock = false;
bool dpsAllowNegative = false; // for threadDps/threadKillTimer only
bool logDps = true;
bool logDpsDetails = false;
bool logKillTimer = false;
bool logKillTimerDetails = false;
bool logKillTimerToFile = false;
bool logHits = false;
bool logHitsToFile = false;
bool logAttackRate = false;
bool logAttackRateToFile = false;

bool alliesList = false;

// Settings //
int AttackRateChainHits = 1;
int AttackRateChainTime = 0; // not used atm
double dpsPollingRate = 250; // ms
string logDpsFile = "gw2dpsLog-Dps.txt";
string logHitsFile = "gw2dpsLog-Hits.txt";
string logAttackRateFile = "gw2dpsLog-AttackRate.txt";
int wvwBonus = 0;

// Threads //
#include "gw2dps.threadHotkeys.cpp"
#include "gw2dps.threadDps.cpp"
#include "gw2dps.threadKillTimer.cpp"
#include "gw2dps.threadHits.cpp"
#include "gw2dps.threadAttackRate.cpp"

void ESP()
{
	// Element Anchors
	Anchor aLeft, aTopRight, aRight, aTopLeft, aCenter, aBottom;

	aLeft.x = 100;
	aLeft.y = 75;

	aTopLeft.x = round((GetWindowWidth() / 2 - 316 - 179) / 2 + 316);
	aTopLeft.y = 8;

	aTopRight.x = round((GetWindowWidth() / 2 - 294 - 179)/2 + GetWindowWidth() / 2 + 179);
	aTopRight.y = 8;

	aRight.x = GetWindowWidth() - 10;
	aRight.y = 8;

	aCenter.x = round(GetWindowWidth() * float(0.5));
	aCenter.y = round(GetWindowHeight() * float(0.25));

	aBottom.x = round(GetWindowWidth() * float(0.5));
	aBottom.y = round(GetWindowHeight() - float(85));

	
	if (help){
		stringstream ss;
		ss << format("[%i] Selected/Locked Target Info (Alt S)\n") % targetSelected;
		ss << format("[%i] Lock On Target (Alt L)\n") % targetLock;
		ss << format("[%i] Allow Negative DPS (Alt N)\n") % dpsAllowNegative;
		ss << format("\n");
		ss << format("[%i] DPS Meter (Alt D)\n") % logDps;
		ss << format("[%i] DPS Meter History (Alt Shift D)\n") % logDpsDetails;
		ss << format("\n");
		ss << format("[%i] Kill Timer (Alt Num7)\n") % logKillTimer;
		ss << format("[%i] Kill Timer Details (Alt Num1)\n") % logKillTimerDetails;
		//ss << format("[%i] Kill Timer Writes to a File (Alt Num4)\n") % logKillTimerToFile;
		ss << format("\n");
		ss << format("[%i] Monitor Hits (Alt Num8)\n") % logHits;
		ss << format("[%i] Record Damage Hits to File (Alt Num5)\n") % logHitsToFile;
		ss << format("\n");
		ss << format("[%i] Monitor Attack Rate (Alt Num9)\n") % logAttackRate;
		ss << format("[%i] Record Attack Rate to File (Alt Num6)\n") % logAttackRateToFile;
		ss << format("[%i] Adjust Attack Rate Threshold (Alt PgUp/PgDown)\n") % AttackRateChainHits;
		ss << format("\n");
		ss << format("[%i] Nearby Ally Players List (Alt C)\n") % alliesList;
		ss << format("[%i] Adjust WvW HP Bonus (Alt Home/End)\n") % wvwBonus;

		StrInfo strInfo;
		strInfo = StringInfo(ss.str());
		float x = round(aCenter.x - strInfo.x / 2);
		float y = round(aCenter.y - strInfo.y / 2);

		DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
		DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
		font.Draw(x, y, fontColor, ss.str());
	}

	// Font Draw Debug
	if (0) {
		stringstream ss;
		ss << format("Selected: 18,140 / 18,140 [100%s]") % "%%";
		ss << format("Locked: 18,140 / 18,140 [100%s]") % "%%";

		StrInfo strInfo;
		strInfo = StringInfo(ss.str());
		float x = 0;
		float y = float(strInfo.lineCount * lineHeight + 1);
		padX = 0;
		padY = 0;

		DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, 0xffffffff);
		font.Draw(x, y, 0xff000000, ss.str());

		return;
	}

	// Targets & Agents //
	Character me = GetOwnCharacter();
	if (me.IsValid()){
		self.cHealth = int(me.GetCurrentHealth());
		self.mHealth = int(me.GetMaxHealth());
		if (self.mHealth > 0)
			self.pHealth = int(100.f * float(self.cHealth) / float(self.mHealth));
		else
			self.pHealth = 0;

		self.lvl = me.GetScaledLevel();
		self.lvlActual = me.GetLevel();
		self.alive = me.IsAlive();
	}
	Agent agLocked = GetLockedSelection();
	if (agLocked.IsValid())
	{
		if (agLocked.GetAgentId() != selected.id)
			selected = {};

		int agType = agLocked.GetType();
		if (agType == GW2::AGENT_CATEGORY_CHAR) // char
		{
			selected.valid = true;
			selected.id = agLocked.GetAgentId();
			selected.type = agType;

			Character chLocked = agLocked.GetCharacter();
			selected.cHealth = int(chLocked.GetCurrentHealth());
			selected.mHealth = int(chLocked.GetMaxHealth());
			if (selected.mHealth > 0)
				selected.pHealth = int(100.f * float(selected.cHealth) / float(selected.mHealth));
			else
				selected.pHealth = 0;
			selected.lvl = chLocked.GetScaledLevel();
			selected.lvlActual = chLocked.GetLevel();
		}
		else if (agType == GW2::AGENT_TYPE_GADGET) // object
		{
			selected.valid = true;
			selected.id = agLocked.GetAgentId();
			selected.type = agType;

			unsigned long shift = *(unsigned long*)agLocked.m_ptr;
			shift = *(unsigned long*)(shift + 0x30);
			shift = *(unsigned long*)(shift + 0x164);
			if (shift)
			{
				selected.cHealth = int(*(float*)(shift + 0x8));
				selected.mHealth = int(*(float*)(shift + 0xC));
			}
			if (selected.mHealth > 0)
				selected.pHealth = int(100.f * float(selected.cHealth) / float(selected.mHealth));
			else
				selected.pHealth = 0;
			//selected.lvl = chLocked.GetScaledLevel();
			//selected.lvlActual = chLocked.GetLevel();
		}
		else if (agType == GW2::AGENT_TYPE_GADGET_ATTACK_TARGET) // world boss
		{
			selected.valid = true;
			selected.id = agLocked.GetAgentId();
			selected.type = agType;

			unsigned long shift = *(unsigned long*)agLocked.m_ptr;
			shift = *(unsigned long*)(shift + 0x30);
			shift = *(unsigned long*)(shift + 0x28);
			shift = *(unsigned long*)(shift + 0x178);
			if (shift)
			{
				selected.cHealth = int(*(float*)(shift + 0x8));
				selected.mHealth = int(*(float*)(shift + 0xC));
			}
			if (selected.mHealth > 0)
				selected.pHealth = int(100.f * float(selected.cHealth) / float(selected.mHealth));
			else
				selected.pHealth = 0;

			//selected.lvl = chLocked.GetScaledLevel();
			//selected.lvlActual = chLocked.GetLevel();
		}
		else
			selected = {};

		if (selected.mHealth == 0)
			selected = {};
	}
	else
		selected = {};

	// Locked Target (ID)
	if (targetLock)
	{
		if (!locked.valid && selected.valid)
			targetLockID = selected.id;
	}
	else
	{
		if (!selected.valid)
		{
			locked = {};
			targetLockID = 0;
		}
		else
			targetLockID = selected.id;
	}

	// compile agents data
	Allies allies;
	Agent ag;
	while (ag.BeNext())
	{
		// Locked Target (Data)
		if (targetLockID == ag.GetAgentId())
		{
			int agType = ag.GetType();
			if (agType == GW2::AGENT_CATEGORY_CHAR) // char
			{
				locked.valid = true;
				locked.id = ag.GetAgentId();
				locked.type = agType;

				Character ch = ag.GetCharacter();
				locked.cHealth = int(ch.GetCurrentHealth());
				locked.mHealth = int(ch.GetMaxHealth());
				if (locked.mHealth > 0)
					locked.pHealth = int(100.f * float(locked.cHealth) / float(locked.mHealth));
				else
					locked.pHealth = 0;
				locked.lvl = ch.GetScaledLevel();
				locked.lvlActual = ch.GetLevel();
			}
			else if (agType == GW2::AGENT_TYPE_GADGET) // struct
			{
				locked.valid = true;
				locked.id = ag.GetAgentId();
				locked.type = agType;

				unsigned long shift = *(unsigned long*)ag.m_ptr;
				shift = *(unsigned long*)(shift + 0x30);
				shift = *(unsigned long*)(shift + 0x164);
				if (shift)
				{
					locked.cHealth = int(*(float*)(shift + 0x8));
					locked.mHealth = int(*(float*)(shift + 0xC));
				}
				if (locked.mHealth > 0)
					locked.pHealth = int(100.f * float(locked.cHealth) / float(locked.mHealth));
				else
					locked.pHealth = 0;
				//locked.lvl = ch.GetScaledLevel();
				//locked.lvlActual = ch.GetLevel();
			}
			else if (agType == GW2::AGENT_TYPE_GADGET_ATTACK_TARGET) // world boss
			{
				locked.valid = true;
				locked.id = ag.GetAgentId();
				locked.type = agType;

				unsigned long shift = *(unsigned long*)ag.m_ptr;
				shift = *(unsigned long*)(shift + 0x30);
				shift = *(unsigned long*)(shift + 0x28);
				shift = *(unsigned long*)(shift + 0x178);
				if (shift)
				{
					locked.cHealth = int(*(float*)(shift + 0x8));
					locked.mHealth = int(*(float*)(shift + 0xC));
				}
				if (locked.mHealth > 0)
					locked.pHealth = int(100.f * float(locked.cHealth) / float(locked.mHealth));
				else
					locked.pHealth = 0;
				//locked.lvl = ch.GetScaledLevel();
				//locked.lvlActual = ch.GetLevel();
			}

			if (locked.cHealth == 0 && locked.mHealth != 78870) // don't clear if 78870 (indestructible golem) or targetLocked
			{
				if (targetLock)
					locked.alive = false;
				else
					locked = {};
			}
		}

		// Allies list
		if (alliesList) {
			Character ch = ag.GetCharacter();
			
			// collect only valid allies (and yourself)
			bool chValid = true;

			if (!ch.IsValid())
				chValid = false;

			//if (ch.IsControlled())
				//chValid = false;

			if (!ch.IsPlayer() || ch.GetAttitude() != GW2::ATTITUDE_FRIENDLY)
				chValid = false;

			// gather char data
			if (chValid){
				Ally ally;
				ally.id = ag.GetAgentId();
				
				ally.profession = ch.GetProfession();
				ally.mHealth = int(round(ch.GetMaxHealth() / (100 + wvwBonus) * 100));
				//ally.cHealth = int(ch.GetCurrentHealth());
				//if (ally.mHealth > 0)
				//ally.pHealth = int(100.f * float(ally.cHealth) / float(ally.mHealth));
				//else
				//ally.pHealth = 0;
				ally.lvl = ch.GetScaledLevel();
				ally.lvlActual = ch.GetLevel();
				ally.name = ch.GetName();

				baseHpReturn base = baseHp(ally.lvl, ally.profession);
				ally.vitality = int(round((ally.mHealth - base.health) / 10));
				ally.traits = (916.f / base.vitality) * ((ally.mHealth - base.health) / 100.f / 5.f);
				ally.traits = round(ally.traits * 100) / 100; // round to 0.00

				switch (ally.profession)
				{
				case GW2::PROFESSION_WARRIOR:
					allies.war.push_back(ally);
					break;
				case GW2::PROFESSION_NECROMANCER:
					allies.necro.push_back(ally);
					break;

				case GW2::PROFESSION_ENGINEER:
					allies.engi.push_back(ally);
					break;
				case GW2::PROFESSION_RANGER:
					allies.ranger.push_back(ally);
					break;
				case GW2::PROFESSION_MESMER:
					allies.mes.push_back(ally);
					break;

				case GW2::PROFESSION_GUARDIAN:
					allies.guard.push_back(ally);
					break;
				case GW2::PROFESSION_THIEF:
					allies.thief.push_back(ally);
					break;
				case GW2::PROFESSION_ELEMENTALIST:
					allies.ele.push_back(ally);
					break;
				}
			}
		}
	}

	// Bottom Element //
	{
		stringstream ss;
		StrInfo strInfo;

		if (self.alive)
		{
			ss << format("%i") % self.pHealth;

			strInfo = StringInfo(ss.str());
			float x = round(aBottom.x - strInfo.x / 2);
			float y = round(aBottom.y - lineHeight);

			//DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
			//DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
			font.Draw(x, y, fontColor, ss.str());
		}
	}

	// Left Element //
	{
		if (alliesList)
		{
			stringstream ss;
			stringstream sp;
			stringstream sn;
			stringstream sh;
			stringstream sv;
			stringstream st;

			
			ss << format("Nearby Ally Players (WvW HP Bonus: %i%s)") % wvwBonus % "%%";
			sp << format("Class");
			sn << format("Name");
			sh << format("Health");
			sv << format("Vitality");
			st << format("Traits");

			bool listEmpty = true;
			if (!allies.war.empty())
			{
				for (auto & ally : allies.war) {
					sp << format("\nWar:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}
			if (!allies.guard.empty())
			{
				for (auto & ally : allies.guard) {
					sp << format("\nGuard:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}

			if (!allies.ele.empty())
			{
				for (auto & ally : allies.ele) {
					sp << format("\nEle:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}
			if (!allies.mes.empty())
			{
				for (auto & ally : allies.mes) {
					sp << format("\nMes:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}

			if (!allies.thief.empty())
			{
				for (auto & ally : allies.thief) {
					sp << format("\nThief:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}
			if (!allies.ranger.empty())
			{
				for (auto & ally : allies.ranger) {
					sp << format("\nRanger:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}
			if (!allies.engi.empty())
			{
				for (auto & ally : allies.engi) {
					sp << format("\nEngi:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}
			if (!allies.necro.empty())
			{
				for (auto & ally : allies.necro) {
					sp << format("\nNecro:");
					sn << format("\n%s") % ally.name;
					sh << format("\n%i hp") % ally.mHealth;
					sv << format("\n%+i") % ally.vitality;
					st << format("\n%+g") % ally.traits;
				}
				listEmpty = false;
			}
			if (listEmpty)
			{
				sp << format("\n...");
				sn << format("\n...");
				sh << format("\n...");
				sv << format("\n...");
				st << format("\n...");
			}

			
			// CharName max width
			stringstream sx;
			sx << "WWWWWWWWWWWWWWWWWWW";
			StrInfo strInfo;
			strInfo = StringInfo(sx.str());

			float spOffset = 0;
			float snOffset = spOffset + 65;
			float shOffset = snOffset + strInfo.x;
			float svOffset = shOffset + 85;
			float stOffset = svOffset + 70;
			float sxOffset = stOffset + 60;

			float x = round(aLeft.x);
			float y = round(aLeft.y);

			strInfo = StringInfo(sp.str());
			int lineCount = int(strInfo.lineCount) + 2;

			// render the list
			DrawRectFilled(x - padX, y - padY, sxOffset + padX * 2, float(lineCount * lineHeight) + padY * 2, backColor - 0x44000000);
			DrawRect(x - padX, y - padY, sxOffset + padX * 2, float(lineCount * lineHeight) + padY * 2, borderColor);

			int lineShiftY = 2;
			for (int i = 3; i < lineCount; i++) {
				DrawLine(x - padX, y - padY + i * lineHeight + lineShiftY, x + sxOffset + padX, y - padY + i * lineHeight + lineShiftY, borderColor);
			}
			font.Draw(x + spOffset, y, fontColor, ss.str()); y += 2 * lineHeight;
			font.Draw(x + spOffset, y, fontColor, sp.str());
			font.Draw(x + snOffset, y, fontColor, sn.str());
			font.Draw(x + shOffset, y, fontColor, sh.str());
			font.Draw(x + svOffset, y, fontColor, sv.str());
			font.Draw(x + stOffset, y, fontColor, st.str());
		}
	}

	// TopLeft Element //
	{
		stringstream ss;
		StrInfo strInfo;

		if (targetSelected)
		{
			if (selected.valid)
			{
				Character chLocked = agLocked.GetCharacter();

				ss << format("Selected: %i / %i [%i%s]") % selected.cHealth % selected.mHealth % selected.pHealth % "%%";

				strInfo = StringInfo(ss.str());
				float x = round(aTopLeft.x - strInfo.x / 2);
				float y = round(aTopLeft.y);

				DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
				DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
				font.Draw(x, y, fontColor, ss.str());

				// Prepare for Next Element
				ss.str("");
				aTopLeft.y += strInfo.lineCount * lineHeight + padY * 2;
			}

			if (targetLock && locked.valid)
			{
				ss << format("Locked: %i / %i [%i%s]") % locked.cHealth % locked.mHealth % locked.pHealth % "%%";

				strInfo = StringInfo(ss.str());
				float x = round(aTopLeft.x - strInfo.x / 2);
				float y = round(aTopLeft.y);

				DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
				DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
				font.Draw(x, y, fontColor, ss.str());
			}

			// Prepare for Next Element
			ss.str("");
			aTopLeft.y += strInfo.lineCount * lineHeight + padY * 2;
		}
	}

	// TopRight Elements //
	{
		if (logDps)
		{
			// separate ss vars
			stringstream ss;
			StrInfo strInfo;

			float aAdjustX = 120; // adjust anchor -120

			if (!bufferDps.empty())
			{
				double average[6] {}; // for 1s & 5s
				size_t samples = 0;

				// DP1s
				samples = 4; // 1s/250ms=4
				if (samples > bufferDps.size())
					samples = bufferDps.size();
				average[1] = 0;
				for (size_t i = 0; i < samples; i++)
					average[1] += bufferDps[i];
				average[1] = average[1] / samples * (1000 / dpsPollingRate);

				// DP5s
				samples = 20; // 5s/250ms=20
				if (samples > bufferDps.size())
					samples = bufferDps.size();
				average[5] = 0;
				for (size_t i = 0; i < samples; i++)
					average[5] += bufferDps[i];
				average[5] = average[5] / samples * (1000 / dpsPollingRate);

				// Prepare String
				ss << format("DP1s: %0.0f\n") % average[1];
				ss << format("DP5s: %0.0f\n") % average[5];
				if (logDpsDetails)
				{
					for (size_t i = 0; i < bufferDps.size(); i++)
						ss << format("\nDP250ms: %i") % bufferDps[i];
				}
			}
			else
			{
				ss << format("DP1s: ...\n");
				ss << format("DP5s: ...");
			}

			strInfo = StringInfo(ss.str());
			if (logDpsDetails && !bufferDps.empty() && strInfo.x < aAdjustX)
				strInfo.x = aAdjustX; // box min-width with history stream
			float x = round(aTopRight.x - aAdjustX / 2); // perma anchor offset
			float y = round(aTopRight.y);

			// Draw
			DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
			DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
			font.Draw(x, y, fontColor, ss.str());

			// Prepare for Next Element
			//ss.str("");
			//aTopRight.y += strInfo.lineCount * lineHeight + padY * 2;
			aTopRight.x -= aAdjustX / 2 + padX + 2;
		}

		if (logKillTimer)
		{
			// separate ss vars
			stringstream ss;
			StrInfo strInfo;

			// Prepare String
			if (bufferKillTimer.time > 0)
			{
				ss << format("Timer: %s") % SecondsToString(bufferKillTimer.time);
				if (logKillTimerDetails)
					ss << format("\nDPS: %0.2f") % bufferKillTimer.dps;
			}
			else
			{
				ss << format("Timer: 0.0s");
				if (logKillTimerDetails)
					ss << format("\nDPS: 0.0");
			}

			strInfo = StringInfo(ss.str());
			float x = 0;
			float y = round(aTopRight.y);
			if (logDps)
				x = round(aTopRight.x - strInfo.x - padX); // perma anchor offset with logDps
			else
				x = round(aTopRight.x - strInfo.x / 2); // center otherwise
			

			// Draw
			DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
			DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
			font.Draw(x, y, fontColor, ss.str());

			// Prepare for Next Element
			//ss.str("");
			aTopRight.y += strInfo.lineCount * lineHeight + padY * 2;
			//aTopRight.x -= 0;
		}
		
	}

	// Right Elements //
	{
		if (logAttackRate)
		{
			stringstream ss;
			StrInfo strInfo;

			if (logAttackRateToFile)
				ss << format("� Recording �\n");
			else
				ss << format("� Monitoring �\n");
			ss << format("� Attack Rate �\n");
			ss << format("\n");
			ss << format("Threshold: %i hits\n") % AttackRateChainHits;

			if (!bufferAttackRate.empty())
			{
				double min = *min_element(bufferAttackRate.begin(), bufferAttackRate.end());
				double max = *max_element(bufferAttackRate.begin(), bufferAttackRate.end());
				double average = 0;
				for (size_t i = 0; i < bufferAttackRate.size(); i++)
					average += bufferAttackRate[i];
				average = average / bufferAttackRate.size();

				ss << format("Min: %0.3fs\n") % min;
				ss << format("Avg: %0.3fs\n") % average;
				ss << format("Max: %0.3fs\n") % max;

				ss << format("\n");
				ss << format("History");
				for (size_t i = 0; i < bufferAttackRate.size(); i++)
					ss << format("\n� %0.3fs") % bufferAttackRate[i];
			}
			else
			{
				ss << format("Minimum: ...\n");
				ss << format("Average: ...\n");
				ss << format("Maximum: ...\n");
			}

			strInfo = StringInfo(ss.str());

			float aAdjustX = 120; // adjust anchor -120
			if (strInfo.x < aAdjustX)
				strInfo.x = aAdjustX; // perma box min-width
			float x = round(aRight.x - strInfo.x);
			float y = round(aRight.y);

			// Draw
			DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
			DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
			font.Draw(x, y, fontColor, ss.str());

			// Prepare for Next Element
			//ss.str("");
			//aTopRight.y += strInfo.lineCount * lineHeight + padY * 2;
			aRight.x = x - padX * 2 - 5;
		}

		if (logHits)
		{
			stringstream ss;
			StrInfo strInfo;

			if (logHitsToFile)
				ss << format("� Recording �\n");
			else
				ss << format("� Monitoring �\n");

			ss << format("� Damage Hits �\n");
			ss << format("\n");
			ss << format("Counter: %i\n") % threadHitsCounter;
			
			if (!bufferHits.empty())
			{
				//double min = *min_element(bufferHits.begin(), bufferHits.end());
				//double max = *max_element(bufferHits.begin(), bufferHits.end());
				double average = 0;
				for (size_t i = 0; i < bufferHits.size(); i++)
					average += bufferHits[i];
				average = average / bufferHits.size();

				ss << format("Avg: %0.1f\n") % average;
				
				ss << format("\n");
				ss << format("History");
				for (size_t i = 0; i < bufferHits.size(); i++)
					ss << format("\n� %i") % bufferHits[i];
			}
			else
			{
				ss << format("Counter: ...\n");
				ss << format("Average: ...");
			}

			strInfo = StringInfo(ss.str());
			
			float aAdjustX = 120; // adjust anchor -120
			if (strInfo.x < aAdjustX)
				strInfo.x = aAdjustX; // perma box min-width
			float x = round(aRight.x - strInfo.x);
			float y = round(aRight.y);

			// Draw
			DrawRectFilled(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, backColor - 0x44000000);
			DrawRect(x - padX, y - padY, strInfo.x + padX * 2, strInfo.y + padY * 2, borderColor);
			font.Draw(x, y, fontColor, ss.str());

			// Prepare for Next Element
			//ss.str("");
			//aTopRight.y += strInfo.lineCount * lineHeight + padY * 2;
			aRight.x = x - padX * 2 - 5;
		}	
	}
}

class TestDll : public GW2LIB::Main
{
public:
	bool init() override
	{
		locale::global(locale("en-US"));

		EnableEsp(ESP);
		NewThread(threadHotKeys);
		NewThread(threadDps);
		NewThread(threadKillTimer);
		NewThread(threadHits);
		NewThread(threadAttackRate);
		
		if (!font.Init(lineHeight, "Verdana"))
		{
			DbgOut("Could not create Font");
			return false;
		}

		return true;
	}
};
TestDll g_testDll;