#include "main.h"

#define MAX 10

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

void rank() {
	vector<tuple<string, int, double>> rank;

	readRank(rank);


	writeRank(rank);
}
