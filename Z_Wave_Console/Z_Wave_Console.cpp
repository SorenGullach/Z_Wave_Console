// Z_Wave_Console.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <algorithm>
#include <conio.h>
#include <iostream>
#include <vector>

#include "Logging.h"
#include "Z_Wave.h"

ZW_Logging Log; // a global var used for logging
Z_Wave ZW;

static SHORT kConsoleWidth = 230;
static SHORT kConsoleHeight = 57;
static SHORT FirstContentY = 3;
static SHORT borderY = kConsoleHeight - 5;
static SHORT commandsY = borderY + 1;
static SHORT promptY = commandsY + 1;

static bool NewConsoleSize()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return false;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hOut, &csbi))
		return false;

	return kConsoleWidth != csbi.dwSize.X || kConsoleHeight != csbi.dwSize.Y;
}

static void ClearConsoleBuffer()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hOut, &csbi))
		return;

	DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
	kConsoleWidth = csbi.dwSize.X;
	kConsoleHeight = csbi.dwSize.Y;
	FirstContentY = 3;
	borderY = kConsoleHeight - 5;
	commandsY = borderY + 1;
	promptY = commandsY + 1;

	DWORD written = 0;
	COORD home = { 0, 0 };

	FillConsoleOutputCharacterW(hOut, L' ', cellCount, home, &written);
	FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, home, &written);
	SetConsoleCursorPosition(hOut, home);
}

static std::string MakeUiLine(const std::string& content)
{
	const size_t innerWidth = kConsoleWidth - 2;
	std::string inner = content;

	if (inner.size() > innerWidth)
		inner.resize(innerWidth);
	else
		inner.append(innerWidth - inner.size(), ' ');

	return "|" + inner + "|\n";
}

static WORD DefaultColor()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi{};
	if (GetConsoleScreenBufferInfo(hOut, &csbi))
		return csbi.wAttributes;
	return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}

static void gotoxy(int x, int y)
{
	COORD coord{ static_cast<SHORT>(x), static_cast<SHORT>(y) };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

static void WriteFixedLine(int x, int y, const std::string& s)
{
	gotoxy(x, y);
	std::string out = s;
	if (out.empty() || out.back() != '\n')
		out.push_back('\n');

	if (out.size() < (size_t)kConsoleWidth + 1)
		out.append(((size_t)kConsoleWidth + 1) - out.size(), ' ');
	else
		out.resize((size_t)kConsoleWidth + 1);

	DWORD written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), out.c_str(), (DWORD)out.size(), &written, nullptr);
}

static void WriteFixedLineColor(int x, int y, const std::string& s, WORD color)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD prev = DefaultColor();
	SetConsoleTextAttribute(hOut, color);
	WriteFixedLine(x, y, s);
	SetConsoleTextAttribute(hOut, prev);
}

static std::string MakeUiBorder()
{
	// Total width should be kConsoleWidth characters: '+' + (kConsoleWidth-2)*'-' + '+'
	return "+" + std::string(kConsoleWidth - 2, '-') + "+\n";
}

static WORD ColorForLogLine(const std::string& line)
{
	const WORD green = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
	const WORD red = FOREGROUND_RED | FOREGROUND_INTENSITY;
	const WORD gray = FOREGROUND_INTENSITY;

	if (line.find(" ERR ") != std::string::npos)
		return red;
	if (line.find(" DBG ") != std::string::npos)
		return gray;

	return green; // INFO
}

static void DrawUiFrame()
{
	//	ConfigureConsoleWindow();
	ClearConsoleBuffer();

	const WORD cyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
	const std::string border = MakeUiBorder();

	WriteFixedLineColor(0, 0, border, cyan);
	WriteFixedLineColor(0, 1, MakeUiLine(" Z-Wave Serial Monitor"), cyan);
	WriteFixedLineColor(0, 2, border, cyan);

	for (int y = FirstContentY; y < borderY; ++y)
		WriteFixedLine(0, y, MakeUiLine(""));

	WriteFixedLineColor(0, borderY, border, cyan);
	WriteFixedLine(0, commandsY, MakeUiLine(" Commands: EXIT, INTERVIEW, BATT <node>, ISDEAD <node>, REMOVE <node>, BIND <node> <group> <targetnodeid>, UNBIND <node> <group> <targetnodeid>, BIND? <node> <group>, CONFIG <node> <param> <value>, CONFIG? <node> <param>"));
	WriteFixedLine(0, promptY, "> ");
}

static void DrawStatus()
{
	SHORT consoleWidth = kConsoleWidth;
	int leftWidth = consoleWidth / 3 - 2;
	int rightWidth = consoleWidth - leftWidth - 5;

	int firstRow = FirstContentY;
	int lastRow = borderY - 1;
	int maxRows = lastRow - firstRow + 1;

	// -----------------------------
	// Build LEFT column (multi-line)
	// -----------------------------
	std::vector<std::string> leftLines;

	// Controller
	leftLines.push_back("=== Controller ===");
	{
		std::istringstream iss(ZW.HostToString());
		std::string line;
		while (std::getline(iss, line))
			leftLines.push_back(line);
	}
	// Nodes
	leftLines.push_back("=== Nodes ===");
	{
		std::istringstream iss(ZW.NodesToString());
		std::string line;
		while (std::getline(iss, line))
			leftLines.push_back(line);
	}
	// -----------------------------
	// Build RIGHT column (log)
	// -----------------------------
	auto log = Log.GetLog(maxRows);
	std::vector<std::string> rightLines;
	rightLines.reserve(log.size());
	for (const auto& s : log)
		rightLines.push_back(s);

	// -----------------------------
	// Render both columns
	// -----------------------------
	// Batch build full frame text
	std::string frame;
	frame.reserve(static_cast<size_t>(maxRows) * static_cast<size_t>(kConsoleWidth + 1));

	for (int i = 0; i < maxRows; i++)
	{
		std::string L = (i < (int)leftLines.size()) ? leftLines[i] : "";
		std::string Rraw = (i < (int)rightLines.size()) ? rightLines[i] : "";
		std::string R = Rraw;

		if (L.size() > (size_t)leftWidth)  L.resize(leftWidth);
		if (R.size() > (size_t)rightWidth) R.resize(rightWidth);

		L.append(leftWidth - L.size(), ' ');
		R.append(rightWidth - R.size(), ' ');

		frame.push_back('|');
		frame.append(L);
		frame.append(" | ");
		frame.append(R);
		frame.append("|\n");
	}

	// One cursor move + one write for all rows
	gotoxy(0, firstRow);
	DWORD written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), frame.c_str(), (DWORD)frame.size(), &written, nullptr);
}

static std::atomic<bool> typing{ false };
static void ClearCommandLine()
{
	int y = promptY; gotoxy(0, y); std::cout << std::string(kConsoleWidth, ' '); gotoxy(0, y); std::cout << "> ";
}

static std::string ReadCommand()
{
	std::string input; std::getline(std::cin, input); std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) { return std::toupper(c); }); return input;
}
int main()
{
	Log.SetLogType(eLogTypes::INFO);

	DrawUiFrame();

	ZW.OpenPort("COM3");

	ZW.StartInitialization();

	auto lastDraw = std::chrono::steady_clock::now();
	auto lastDraw1 = std::chrono::steady_clock::now();

	while (true)
	{
		if (!_kbhit())
		{
			auto now = std::chrono::steady_clock::now();
			if (now - lastDraw >= std::chrono::milliseconds(500))
			{
				if (!typing.load())
					DrawStatus();
				lastDraw = now;
			}
			if (NewConsoleSize())
			{
				if (!typing.load())
					DrawUiFrame();
				lastDraw1 = now;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		// User typed something
		ClearCommandLine();
		typing.store(true);
		auto cmd = ReadCommand();
		typing.store(false);

		// Commands:
		if (cmd == "EXIT")
		{
			ZW.ClosePort();
			return 0;
		}

        if (cmd == "INTERVIEW")
        {
            ZW.StartInterview();
            continue;
        }

        if (cmd.rfind("BATT", 0) == 0)
        {
            try
            {
                const int nodeId = std::stoi(cmd.substr(5));
                ZW.RequestBattery(static_cast<uint16_t>(nodeId));
            }
            catch (...)
            {
            }
            continue;
        }

        if (cmd.rfind("BIND?", 0) == 0)
        {
            try
            {
				const int nodeId = std::stoi(cmd.substr(5));
				ZW.AssociationInterview(static_cast<uint16_t>(nodeId));
            }
            catch (...)
            {
            }
            continue;
        }
	}

	ZW.ClosePort();
};
