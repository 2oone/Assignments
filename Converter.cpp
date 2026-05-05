#include<iostream>
#include<fstream>
#include<map>
#include<set>
#include<vector>
#include<string>
#include<queue>
#include <iomanip>
#include <sstream>
using namespace std;

//NFA 요소를 선언부
map<string, int> stateM;	//string으로 입력되는 state를 index로 저장하기 위한 map
map<string, int> terminalM; //string으로 입력되는 terminal symbol를 index로 저장하기 위한 map
set<int> finalStateM; //종결상태의 index를 저장하는 set (중복 제거 및 빠른 탐색 지원)
map<pair<int, int>, vector<int>> tableM; //state function을 저장하는 map, pair<state, symbol>에 대응되는 state 집합을 vector로 저장
map<int, vector<int>> epsilonM; //epsilon closure 연산을 위해 epsilon function 또한 저장
int startS; //statr state의 index 저장

vector<string> indexToState; //epsilon NFA 파일 출력을 위해 index에 해당하는 string값들 매핑
vector<string> indexToTerminal;

//DFA 요소 선언부
map<set<int>, int> dfaStateM; //DFA 상태들을 저장하는 map, NFA상태들의 부분집합(set<int>)에 대응되는 인덱스를 저장
map<pair<int, int>, int> dfaDeltaM;//state function을 저장하는 map, DFA는 일대응 대응이기에 val값을 int로 선언
set<int> dfaFinalS;//종결상태집합을 저장하는 set
//reduced DFA 요소 선언부
map<vector<int>, set<int>> reducedTableM; //DFA 집합들의 지시경로에 따라 상태들의 집합을 분류
vector<set<int>> partitions; //DFA를 분류한 집합들을 저장하는 vector
map<int, int>stateGroupM;//DFA의 각 상태들이 현재 속해있는 partions번호를 저장
set<int> reducedDFAfinalS; //종결상태저장
int reducedStartS;//시작상태저장

string getStr(string s) {
	if (s.find(",")==string::npos) {
		return s.substr(1, s.find("}") - 2);
		
	}
	else {
		return s.substr(1, s.find(",") - 1);
	}
}//epsilon-NFA 파일을 읽고 원소값들을 추출하기 위한 함수

string eraseStr(string s) { //epsilon-NFA 원소값들을 추출할 때 무의미한 string을 삭제하는 함수
	if (s.find(",") == string::npos) {
		return "";
	}
	else {
		s = s.erase(0, s.find(",") + 1);
		return s;
	}
}

set<int> epsilonClosure(set<int> iSet) {//상태 원소 집합을 입력하면 epsilon closure연산을 시행해주는 함수
	bool visit[100] = { false };
	set<int> s;
	queue<int> q;  

	for (auto it = iSet.begin();it != iSet.end();it++) {
		visit[*it] = true;
		q.push(*it);
		s.insert(*it);
	}
	//iSet의 모든 상태들에 대해 epsilon 연산이 가능한지 bfs한다. 
	while (!q.empty()) {
		int cur = q.front();
		q.pop();
		vector<int> v = epsilonM[cur];
		if (v.size()) {
			for (int i = 0;i < v.size();i++) {
				if (!visit[v[i]]) {
					visit[v[i]] = true;
					q.push(v[i]);
					s.insert(v[i]);
				}
			}
		}
	}	//q는 연산해야할 상태를 저장하고 s는 iSet에서 epsilon연산으로 접근가능한 상태들 저장
	return s;
}

string formatStateName(int id) {
	stringstream ss;
	ss << "q" << setfill('0') << setw(3) << id;
	return ss.str();
}//파일 출력시 state를 id에 따라 출력해주는 함수

void saveNFA(const string& filename) { //입력받은 epsilon-NFA를 저장하고 결과 파일을 출력하는 함수
	ofstream fout(filename);

	fout << "StateSet = { ";
	for (int i = 0; i < indexToState.size(); ++i) {
		fout << indexToState[i];
		if (i < (int)indexToState.size() - 1) fout << ", ";
	}
	fout << " }" << endl;

	fout << "TerminalSet = { ";
	for (int i = 0; i < indexToTerminal.size(); ++i) {
		fout << indexToTerminal[i];
		if (i < (int)indexToTerminal.size() - 1) fout << ", ";
	}
	fout << " }" << endl;
	fout << "DeltaFunctions = {" << endl;

	for (auto iter = tableM.begin(); iter != tableM.end(); ++iter) {
		int stateId = iter->first.first;
		int termId = iter->first.second;
		vector<int>& dests = iter->second;

		fout << "(" << indexToState[stateId] << ", " << indexToTerminal[termId] << ") = { ";
		for (int i = 0; i < dests.size(); ++i) {
			fout << indexToState[dests[i]];
			if (i < (int)dests.size() - 1) fout << ", ";
		}
		fout << " }" << endl;
	}

	for (auto iter = epsilonM.begin(); iter != epsilonM.end(); ++iter) {
		int stateId = iter->first;
		vector<int>& dests = iter->second;

		fout << "(" << indexToState[stateId] << ", epsilon) = { ";
		for (int i = 0; i < dests.size(); ++i) {
			fout << indexToState[dests[i]];
			if (i < (int)dests.size() - 1) fout << ", ";
		}
		fout << " }" << endl;
	}
	fout << "}" << endl;

	fout << "StartState = " << indexToState[startS] << endl;

	fout << "FinalStateSet = { ";
	for (auto it = finalStateM.begin(); it != finalStateM.end(); ++it) {
		fout << indexToState[*it];
		if (next(it) != finalStateM.end()) fout << ", ";
	}
	fout << " }" << endl;

	fout.close();
}

void saveDFA(const string& filename) { //변환된 DFA를 결과 파일로 출력하는 함수
	ofstream fout(filename);
	fout << "StateSet = { ";
	for (int i = 0; i < dfaStateM.size(); ++i) {
		fout << formatStateName(i);
		if (i < (int)dfaStateM.size() - 1) fout << ", ";
	}
	fout << " }" << endl;

	fout << "TerminalSet = { ";
	for (int i = 0; i < indexToTerminal.size(); ++i) {
		fout << indexToTerminal[i];
		if (i < (int)indexToTerminal.size() - 1) fout << ", ";
	}
	fout << " }" << endl;

	fout << "DeltaFunctions = {" << endl;
	for (int i = 0; i < (int)dfaStateM.size(); ++i) {
		for (int t = 0; t < (int)indexToTerminal.size(); ++t) {
			auto it = dfaDeltaM.find(make_pair(i, t));
			if (it != dfaDeltaM.end()) {
				if (it->second == -1) {
					fout << "(" << formatStateName(i) << ", " << indexToTerminal[t] << ") = { }" << endl;
				}
				else {
					fout << "(" << formatStateName(i) << ", " << indexToTerminal[t] << ") = { "
						<< formatStateName(it->second) << " }" << endl;
				}
			}
		}
	}
	fout << "}" << endl;

	fout << "StartState = " << formatStateName(0) << endl;

	fout << "FinalStateSet = { ";
	for (auto it = dfaFinalS.begin(); it != dfaFinalS.end(); ++it) {
		fout << formatStateName(*it);
		if (next(it) != dfaFinalS.end()) fout << ", ";
	}
	fout << " }" << endl;

	fout.close();
}

void saveReducedDFA(const string& filename) { //reduced DFA를 결과 파일로 출력하는 함수
	ofstream fout(filename);

	fout << "StateSet = { ";
	for (int i = 0; i < partitions.size(); ++i) {
		fout << formatStateName(i);
		if (i < (int)partitions.size() - 1) fout << ", ";
	}
	fout << " }" << endl;


	fout << "TerminalSet = { ";
	for (int i = 0; i < indexToTerminal.size(); ++i) {
		fout << indexToTerminal[i];
		if (i < (int)indexToTerminal.size() - 1) fout << ", ";
	}
	fout << " }" << endl;

	fout << "DeltaFunctions = {" << endl;
	for (int i = 0; i < partitions.size(); ++i) {
		if (partitions[i].empty()) continue;

		int repState = *partitions[i].begin();

		for (int t = 0; t < indexToTerminal.size(); ++t) {

			int originalNextState = dfaDeltaM[make_pair(repState, t)];
			if (originalNextState == -1) {
				fout << "(" << formatStateName(i) << ", " << indexToTerminal[t] << ") = { }" << endl;
			}
			else {
				int nextGroup = stateGroupM[originalNextState];
				fout << "(" << formatStateName(i) << ", " << indexToTerminal[t] << ") = { "
					<< formatStateName(nextGroup) << " }" << endl;
			}
		}
	}
	fout << "}" << endl;

	fout << "StartState = " << formatStateName(reducedStartS) << endl;

	fout << "FinalStateSet = { ";
	for (auto it = reducedDFAfinalS.begin(); it != reducedDFAfinalS.end(); ++it) {
		fout << formatStateName(*it);
		if (next(it) != reducedDFAfinalS.end()) fout << ", ";
	}
	fout << " }" << endl;

	fout.close();
}


int main() {
	ifstream fin;
	fin.open("Input_Epsilon-NFA.txt");
	cout << "Epsilon-NFA 입력\n";

	string iStr; //State read
	int idx;
	getline(fin, iStr);
	idx = iStr.find("{");
	iStr.erase(0, idx + 1);
	int cnt = 0;
	while (true) {
		if (iStr == "" || iStr.find_first_not_of(" \t\r\n") == string::npos)
			break;
		stateM[getStr(iStr)] = cnt++;
		indexToState.push_back(getStr(iStr));
		iStr = eraseStr(iStr);
	}//입력파일의 state 집합에서 원소들을 추출하고 index에 따라 저장

	getline(fin, iStr); //Terminal symbol read
	idx = iStr.find("{");
	iStr.erase(0, idx + 1);
	cnt = 0;
	while (true) {
		if (iStr == "" || iStr.find_first_not_of(" \t\r\n") == string::npos)
			break;
		terminalM[getStr(iStr)] = cnt++;
		indexToTerminal.push_back(getStr(iStr));
		iStr = eraseStr(iStr);
	}//terminal symbol 집합에서 심벌들을 index에 따라 저장

	getline(fin, iStr); //Delta Function read
	while (true) {
		getline(fin, iStr);
		if (iStr == "}")
			break;
		string state, Vt;
		idx = iStr.find(",");
		state = iStr.substr(1, idx - 1);
		iStr.erase(0, idx + 2);
		idx = iStr.find(")");
		Vt = iStr.substr(0, idx);
		iStr.erase(0, iStr.find("{") + 1);
		if (Vt == "epsilon") {
			while (true) {//epsilon 연산의 결과를 저장하는 반복문
				string nextS;
				if (iStr == "" || iStr.find_first_not_of(" \t\r\n") == string::npos)
					break;
				epsilonM[stateM[state]].push_back(stateM[getStr(iStr)]);
				iStr = eraseStr(iStr);
			}
		}
		else {
			while (true) {
				string nextS;
				if (iStr == "" || iStr.find_first_not_of(" \t\r\n") == string::npos)
					break;
				tableM[make_pair(stateM[state], terminalM[Vt])].push_back(stateM[getStr(iStr)]);
				iStr = eraseStr(iStr);
			}//심볼에 따른 결과 상태도가 1개 이상인 state를 모두 저장하기 위한 while문
		}
	}

	getline(fin, iStr); //StartState read
	iStr.erase(0, iStr.find("=") + 2);
	startS = stateM[iStr];

	getline(fin, iStr); //FinalState read
	iStr.erase(0, iStr.find("{") + 1);
	while (true) {
		if (iStr == "")
			break;
		finalStateM.insert(stateM[getStr(iStr)]);
		iStr = eraseStr(iStr);
	}
	fin.close();

	saveNFA("Epsilon-NFA.txt"); //epsilon-NFA out

	cout << "Epsilon-NFA 출력\n";
	cout << "Epsilon-NFA --> DFA 변환\n";
	
	//epsilon NFA ---> dfa
	queue<set<int>> stateQ;
	cnt = 0;
	set<int> startSet;
	startSet.insert(startS);
	startSet = epsilonClosure(startSet);
	stateQ.push(startSet);//bfs를 위해queue에 시작 상태에 epsilonClosure연산을 시행한 상태 집합을 저장 
	dfaStateM[startSet] = cnt++;
	while (!stateQ.empty()) {
		set<int> curSet = stateQ.front();
		stateQ.pop();
		for (auto it = curSet.begin();it != curSet.end();it++) {
			if (finalStateM.find(*it) != finalStateM.end()) {
				dfaFinalS.insert(dfaStateM[curSet]);
				break;
			}
		}//queue의 상태집합을 꺼내고 종결상태라면 dfa종결상태집합을 저장하는 set에 저장
		for (int t = 0;t < indexToTerminal.size();t++) {
			set<int> newSet;
			for (set<int>::iterator it = curSet.begin();it != curSet.end();it++) {
				vector<int> v = tableM[make_pair(*it, t)];
				for (int i = 0;i < v.size();i++) {
					newSet.insert(v[i]);
				}
			}//심벌에 따라 상태집합을 순회하는 이중for문으로 이동가능한 모든 상태 저장
			newSet = epsilonClosure(newSet);//특정 심벌에 대해 이동가능한 상태집합을 추출하였으면 epsilonCloure연산
			if (newSet.empty()) {
				dfaDeltaM[make_pair(dfaStateM[curSet], t)] = -1;
				continue; 
			}

			if (dfaStateM.find(newSet) == dfaStateM.end()) {
				dfaStateM[newSet] = cnt++;
				stateQ.push(newSet);
			}//새로운 상태집합으면 삽입 및 인덱스화
			dfaDeltaM[make_pair(dfaStateM[curSet], t)] = dfaStateM[newSet];//DFA의 deltadunction 저장
		}
	}

	//DFA OUT
	saveDFA("DFA.txt");
	cout << "DFA 출력\n";
	cout << "DFA --> reduced DFA\n";

	
	//reduce DFA

	vector<set<int>> tempPartitions(2);

	for (int j = 0; j < dfaStateM.size(); j++) {
		if (dfaFinalS.find(j) != dfaFinalS.end()) {
			tempPartitions[0].insert(j);
		}
		else {
			tempPartitions[1].insert(j);
		}
	}//DFA의 종결상태그룹과 비종결상태그룹 분류

	for (int i = 0; i < 2; i++) {
		if (!tempPartitions[i].empty()) {
			partitions.push_back(tempPartitions[i]);
			int realGroupIndex = partitions.size() - 1;

			for (int state : tempPartitions[i]) {
				stateGroupM[state] = realGroupIndex;
			}
		}
	}//분류 종류 수가 1개일 경우 예외처리

	while (true) {
		int partitionsCnt = partitions.size();
		for (int k=0;k < partitionsCnt;k++) {
			map<vector<int>, set<int>> patternMap;//symbol과의 연산에 따라 그룹들을 분류하기위한 map
			//예시로 심벌 a,b에 대한 이동 그룹이 1,2일경우 key={1,2}, value= 그룹 인덱스
			for (auto it = partitions[k].begin();it != partitions[k].end();it++) {
				vector<int>v;
				for (int l = 0;l < terminalM.size();l++) {
					int nextDfaState = dfaDeltaM[make_pair(*it, l)];
					if (nextDfaState == -1) {
						v.push_back(-1);
					}
					else {
						v.push_back(stateGroupM[nextDfaState]);
					}
				}//patternMap의 key를 구성하기 위한 반복분
				patternMap[v].insert(*it);
			}
			partitions[k].clear();
			partitions[k] = patternMap.begin()->second;//현재 분류하고 있는 그룹을 재정립
			auto it2 = patternMap.begin();
			it2++;
			for (;it2 != patternMap.end();it2++) {
				partitions.push_back(it2->second);
				for (auto it3 = it2->second.begin();it3 != it2->second.end();it3++) {
					stateGroupM[*it3] = partitions.size() - 1;
				}
			}//추가로 분류된 그룹 인덱싱 및 그룹의 상태들의 이동 맵핑
		}
		if (partitionsCnt == partitions.size())//맨 처음 추출한 분류집합의 수가 일치하면 더이상 분류할 수 없다는 것이므로 종료
			break;
	}
	
	reducedStartS = stateGroupM[0];//시작상태는 DFA의 시작상태가 속해있는 집합
	for (auto fIter = dfaFinalS.begin();fIter != dfaFinalS.end();fIter++) {
		reducedDFAfinalS.insert(stateGroupM[*fIter]);
	}//종결상태는 DFA의 종결상태가 포함된 모든 집합들

	//reduced DFA out
	saveReducedDFA("reducedDFA.txt");
	cout << "reduced DFA 출력\n";

}