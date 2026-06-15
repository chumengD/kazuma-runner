#include "main.h"
#include <windows.h>

#define MAX 10

IMAGE rankStyle;

// UTF-8 string → wstring（正确处理中文等多字节字符，跳过 BOM）
wstring UTF8ToWide(const string& str) {
	if (str.empty()) return wstring();
	// 跳过 UTF-8 BOM（如果存在）
	const char* src = str.c_str();
	if (str.size() >= 3 &&
		(unsigned char)str[0] == 0xEF &&
		(unsigned char)str[1] == 0xBB &&
		(unsigned char)str[2] == 0xBF) {
		src += 3;
	}
	int len = MultiByteToWideChar(CP_UTF8, 0, src, -1, nullptr, 0);
	if (len == 0) return wstring();
	wstring result(len - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, src, -1, &result[0], len);
	return result;
}

void initRank() {
	loadimage(&rankStyle, _T("public/rank.png"));
}

void readRank(vector<tuple<string, int, double>>& t) {
	ifstream file;
	file.open("rank.txt", ios::in);
	if (!file) {
		std::cerr << "文件打开失败!" << std::endl;
		return;
	}

	string line;
	string name;
	int score;
	double time;

	while (getline(file, line)) {
		stringstream string_stream(line);
		string_stream >> name >> score >> time;
		t.push_back(tuple<string, int, double> {name, score, time});
	}

	file.close();
}


void writeRank(vector<tuple<string, int, double>>& t) {
	ofstream file;
	file.open("rank.txt", ios::out);
	if (!file) {
		std::cerr << "文件打开失败!" << std::endl;
		return;
	}

	for (auto& x : t) {
		file << std::get<0>(x) << " "
			 << std::get<1>(x) << " "
			 << std::get<2>(x) << endl;
	}

	file.close();
}

void drawRank() {
	initRank();
	cleardevice();

	vector<tuple<string, int, double>> rank;

	readRank(rank);
	int len = rank.size();

	int x_start = (WINDOW_WID - rankStyle.getwidth()) / 2;
	int y_start = 50;
	for (int i = 0; i < MAX; i++) {
		int x = x_start;
		int y = y_start + rankStyle.getheight() * 1.5 * i;
		putimage(x, y, &rankStyle);
		settextcolor(BLACK);
		if (i < len) {
			int name_x = x + 20;

			int score_x = x + 150;
			int time_x = x + rankStyle.getwidth() - 50;

			// string → wstring，适配 outtextxy
			wstring name = UTF8ToWide(std::get<0>(rank[i]));
			wstring score = to_wstring(std::get<1>(rank[i]));
			wstring time = to_wstring(std::get<2>(rank[i]));

			outtextxy(name_x, y, name.c_str());
			outtextxy(score_x, y, score.c_str());
			outtextxy(time_x, y, time.c_str());
		}
	}
	ExMessage ex;
	while (true) {
		if (peekmessage(&ex,EX_KEY)) {
			if (ex.vkcode == VK_ESCAPE) break;
		}
	}
}
