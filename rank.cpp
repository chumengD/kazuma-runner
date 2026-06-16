#include "main.h"
#include <windows.h>

#define MAX 10

IMAGE rankStyle, rank_mask;

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
	GenerateMask(&rankStyle, &rank_mask);
}

void readRank(vector<tuple<int, double>>& t) {
	ifstream file;
	file.open("rank.txt", ios::in);
	if (!file) {
		std::cerr << "文件打开失败!" << std::endl;
		return;
	}

	string line;
	int score;
	double time;

	while (getline(file, line)) {
		stringstream string_stream(line);
		string_stream >> score >> time;
		t.push_back(tuple<int, double> {score, time});
	}

	file.close();
}


void writeRank(tuple<int, double>& element) {
	vector<tuple<int, double>> t = {};
	readRank(t);
	t.push_back(element);

	ofstream file;
	file.open("rank.txt", ios::out);
	if (!file) {
		std::cerr << "文件打开失败!" << std::endl;
		return;
	}

	for (auto& x : t) {
		file << std::get<0>(x) << " "
			 << std::get<1>(x) << endl;
	}

	file.close();
}

void drawRank() {
	initRank();
	cleardevice();
	putimage(0, 0, &bg1);

	vector<tuple<int, double>> rank;

	readRank(rank);
	int len = rank.size();

	int x_start = (WINDOW_WID - rankStyle.getwidth()) / 2;
	int y_start = 100;
	for (int i = 0; i < MAX; i++) {
		int x = x_start;
		int y = y_start + rankStyle.getheight() * 1.25 * i;
		// 透明贴图：遮罩 SRCAND 挖洞 + 原图 SRCPAINT 填充
		putimage(x, y, &rank_mask, SRCAND);
		putimage(x, y, &rankStyle, SRCPAINT);
		settextcolor(BLACK);
		
		if (i < len+1) {
			int id_x = x + 45;
			int score_x = x + 150;
			int time_x = x + rankStyle.getwidth() - 80;
			int text_y = y + 5;
			if (i == 0) {
				outtextxy(id_x-15, text_y, L"排名");
				outtextxy(score_x, text_y,L"得分");
				outtextxy(time_x, text_y, L"时间");
				continue;
			}
			// string → wstring，适配 outtextxy
			wstring id = to_wstring(i);
			wstring score = to_wstring(std::get<0>(rank[i-1]));
			TCHAR time_buf[32];
			_stprintf_s(time_buf, _T("%.2f"), std::get<1>(rank[i-1]));
			wstring time = time_buf;

			
			outtextxy(id_x, text_y, id.c_str());
			outtextxy(score_x,text_y, score.c_str());
			outtextxy(time_x, text_y, time.c_str());
		}
	}
	settextstyle(18, 0, _T("微软雅黑"));
	setcolor(RGB(180, 180, 180));
	RECT br = { 0, 650, WINDOW_WID, WINDOW_HEI };
	drawtext(_T("按 ESC 返回菜单"), &br, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	ExMessage ex;
	while (true) {
		if (peekmessage(&ex,EX_KEY)) {
			if (ex.vkcode == VK_ESCAPE) break;
		}
	}
}
