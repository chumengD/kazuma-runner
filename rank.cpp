#include "main.h"
#include <windows.h>
#include <algorithm>

#define MAX_RANK 10  // 最多保存的记录数

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

// 从文件读取排行榜（只解析有效的 "score time" 行，跳过表头等）
void readRank(vector<tuple<int, double>>& t) {
	ifstream file("rank.txt");
	if (!file) return;  // 文件不存在 → 空列表

	string line;
	while (getline(file, line)) {
		if (line.empty()) continue;

		stringstream ss(line);
		int score = 0;
		double time = 0.0;

		// 尝试解析 "score time"，失败则跳过该行（表头 / 空行等）
		if (ss >> score >> time) {
			t.push_back({ score, time });
		}
	}
	file.close();
}

// 写入新纪录：读取 → 追加 → 按分数降序排序 → 保留前 MAX_RANK → 写回
void writeRank(tuple<int, double>& element) {
	vector<tuple<int, double>> t;
	readRank(t);
	t.push_back(element);

	// 按分数降序排序
	sort(t.begin(), t.end(), [](const tuple<int, double>& a, const tuple<int, double>& b) {
		return get<0>(a) > get<0>(b);
	});

	// 保留前 MAX_RANK 条
	if ((int)t.size() > MAX_RANK) {
		t.resize(MAX_RANK);
	}

	ofstream file("rank.txt");
	if (!file) {
		cerr << "排行榜文件写入失败!" << endl;
		return;
	}

	for (const auto& x : t) {
		file << get<0>(x) << " " << get<1>(x) << endl;
	}
	file.close();
}

// 绘制排行榜页面（阻塞直到 ESC）
void drawRank() {
	// 资源未加载时的保护
	if (rankStyle.getwidth() <= 0) return;

	cleardevice();
	putimage(0, 0, &bg1);

	vector<tuple<int, double>> rank;
	readRank(rank);
	int len = (int)rank.size();

	// 表头 + 记录行
	const int TOTAL_ROWS = MAX_RANK + 1;  // 1 表头 + 10 记录
	int x_start = (WINDOW_WID - rankStyle.getwidth()) / 2;
	int y_start = 100;

	for (int i = 0; i < TOTAL_ROWS; i++) {
		int x = x_start;
		int y = y_start + (int)(rankStyle.getheight() * 1.25 * i);

		// 透明贴图：遮罩 SRCAND 挖洞 + 原图 SRCPAINT 填充
		putimage(x, y, &rank_mask, SRCAND);
		putimage(x, y, &rankStyle, SRCPAINT);
		settextcolor(BLACK);

		int id_x = x + 45;
		int score_x = x + 150;
		int time_x = x + rankStyle.getwidth() - 80;
		int text_y = y + 5;

		if (i == 0) {
			// 表头行
			outtextxy(id_x - 15, text_y, L"排名");
			outtextxy(score_x, text_y, L"得分");
			outtextxy(time_x, text_y, L"时间");
			continue;
		}

		// 有记录才绘制
		if (i <= len) {
			const auto& rec = rank[i - 1];
			wstring id = to_wstring(i);
			wstring score = to_wstring(get<0>(rec));

			TCHAR time_buf[64];
			_stprintf_s(time_buf, _countof(time_buf), _T("%.2f"), get<1>(rec));
			wstring time = time_buf;

			outtextxy(id_x, text_y, id.c_str());
			outtextxy(score_x, text_y, score.c_str());
			outtextxy(time_x, text_y, time.c_str());
		}
	}

	settextstyle(18, 0, _T("微软雅黑"));
	setcolor(RGB(180, 180, 180));
	RECT br = { 0, 650, WINDOW_WID, WINDOW_HEI };
	drawtext(_T("按 ESC 返回菜单"), &br, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	ExMessage ex;
	while (true) {
		if (peekmessage(&ex, EX_KEY)) {
			if (ex.vkcode == VK_ESCAPE) break;
		}
	}
}
