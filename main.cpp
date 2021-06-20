#include "TextEditor.h"
#include "Undo.h"
#include "SpellCheck.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <initializer_list>
#include <algorithm>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include <cassert>
using namespace std;

const int NTE = 66;
const int NUN = 23;
const int NSP = 25;
const int BASETE = 0;
const int BASEUN = BASETE + NTE;
const int BASESP = BASEUN + NUN;

struct TesterUndo : public Undo
{
	struct Op
	{
		Op(Action a, int rr, int cc, string t) : act(a), r(rr), c(cc), text(t) {}
		Action act;
		int	r;
		int c;
		string text;
	};

	virtual void submit(Action action, int row, int col, char ch = 0)
	{
		undos.push(Op(action, row, col, string(1, ch)));
	}

	virtual Action get(int& row, int& col, int& count, std::string& text)
	{
		if (undos.empty())
			return Action::ERROR;
		Op op = undos.top();
		undos.pop();
		row = op.r;
		col = op.c;
		count = 1;
		text.clear();
		switch (op.act)
		{
		case Action::INSERT:
			col--;
			count = op.text.size();
			return Action::DELETE;
		case Action::DELETE:
			text = op.text;
			return Action::INSERT;
		case Action::JOIN:
			return Action::SPLIT;
		case Action::SPLIT:
			return Action::JOIN;
		case Action::ERROR:
			return Action::ERROR; // should never happen
		}
		return Action::ERROR; // should never happen
	}

	virtual void clear()
	{
		undos = stack<Op>();
	}

	void replace(int n, int r, int c, string t)
	{
		if (n <= 0)
			return;
		Op op = undos.top();
		if (op.act != Action::INSERT && op.act != Action::DELETE)
			return;
		for (; n > 0; n--)
		{
			op = undos.top();
			undos.pop();
		}
		undos.push(Op(op.act, r, c, t));

	}

	stack<Op> undos;
};

string makefilename()
{
	using namespace std::chrono;
	auto now = high_resolution_clock::now().time_since_epoch();
	auto us = duration_cast<microseconds>(now).count();
	return "p4test" + to_string(us % 1000000);
}

template<typename Ptr>
bool load(Ptr& p, string s)  // passed by ref since unique_ptr
{
	string filename = makefilename();
	{
		ofstream ofs(filename);
		if (!ofs)
		{
			cerr << "creating temp file failed!" << endl;
			return false;
		}
		ofs << s;
	}
	bool ret = p->load(filename.c_str());
	remove(filename.c_str());
	return ret;
}

bool goodsuggs(string w, vector<string> suggs, string dict)
{
	for (auto& s : suggs)
	{
		if (s.size() != w.size())
			return false;
		transform(s.begin(), s.end(), s.begin(), [](char ch) { return tolower(ch); });
		auto p = mismatch(s.begin(), s.end(), w.begin());
		if (p.first == s.end())
			return false;
		if (!islower(*p.first) && *p.first != '\'')
			return false;
		if (mismatch(++p.first, s.end(), ++p.second).first != s.end())
			return false;
		auto q = dict.begin();
		for (;;)
		{
			q = search(q, dict.end(), s.begin(), s.end());
			if (q == dict.end())
				return false;
			if (*(q + s.size()) == '\n' &&
				(q == dict.begin() || *(q - 1) == '\n'))
				break;
		}
	}
	sort(suggs.begin(), suggs.end());
	return adjacent_find(suggs.begin(), suggs.end()) == suggs.end();
}

bool isPerm(vector<SpellCheck::Position>& v, const initializer_list<SpellCheck::Position>& exp)
{
	if (v.size() != exp.size())
		return false;
	sort(v.begin(), v.end(), [](const auto& lhs, const auto& rhs) {
		if (lhs.start < rhs.start) return true;
		if (rhs.start < lhs.start) return false;
		return lhs.end < rhs.end;
		});
	return equal(v.begin(), v.end(), exp.begin(), [](const auto& lhs, const auto& rhs) {
		return lhs.start == rhs.start && lhs.end == rhs.end;
		});
}

void testTextEditor(int n)
{
	constexpr auto UP = TextEditor::Dir::UP;
	constexpr auto DOWN = TextEditor::Dir::DOWN;
	constexpr auto LEFT = TextEditor::Dir::LEFT;
	constexpr auto RIGHT = TextEditor::Dir::RIGHT;
	constexpr auto HOME = TextEditor::Dir::HOME;
	constexpr auto END = TextEditor::Dir::END;

	auto u = make_unique<TesterUndo>();
	auto t = unique_ptr<TextEditor>(createTextEditor(u.get()));

	int r = 999;
	int c = 999;
	vector<string> v;

	switch (n)
	{
	default: {
		cout << "Bad argument TE" << endl;
	} break; case BASETE + 1: {
		t->getPos(r, c);
		assert(r == 0 && c == 0);
	} break; case BASETE + 2: {
		int n = t->getLines(0, 10, v);
		assert((v.empty() || (v.size() == 1 && (v[0].empty() || v[0] == " "))));
	} break; case BASETE + 3: {
		assert(!t->load("/this/file/does/not/exist"));
		assert(load(t, "this is\na file\n"));
	} break; case BASETE + 4: {
		load(t, "this is\na file\n");
		t->move(RIGHT); t->move(RIGHT); t->move(DOWN);
		r = 0; c = 0;
		t->getPos(r, c);
		assert(r != 0 && c != 0);
		t->load("/this/file/does/not/exist");
		int rnew = 888, cnew = 888;
		t->getPos(rnew, cnew);
		assert(rnew == r && cnew == c);
	} break; case BASETE + 5: {
		load(t, "this is\na file\n");
		t->getLines(0, 10, v);
		assert(v.size() == 2);
	} break; case BASETE + 6: {
		load(t, "this is\na file\n");
		t->getLines(0, 10, v);
		assert(v.size() >= 2 && v[0] == "this is" && v[1] == "a file");
	} break; case BASETE + 7: {
		load(t, "this is\r\na file\r\n");
		t->getLines(0, 10, v);
		assert(v.size() >= 2 && v[0] == "this is" && v[1] == "a file");
	} break; case BASETE + 8: {
		load(t, "ttt\neee\nsss\nttt\neee\nrrr\n");
		t->getLines(0, 10, v);
		size_t vszold = v.size();
		load(t, "this is\na file\n");
		v.clear();
		t->getLines(0, 10, v);
		assert(v.size() >= 2 && v.size() < vszold);
	} break; case BASETE + 9: {
		load(t, "ttt\neee\nsss\nttt\neee\nrrr\n");
		t->move(RIGHT); t->move(RIGHT); t->move(DOWN);
		r = 0; c = 0;
		t->getPos(r, c);
		assert(r != 0 && c != 0);
		load(t, "this is\na file\n");
		t->getPos(r, c);
		assert(r == 0 && c == 0);
	} break; case BASETE + 10: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('?');
		t->move(UP);
		assert(!t->save("/this/file/does/not/exist"));
		string filename = makefilename();
		assert(t->save(filename.c_str()));
		remove(filename.c_str());
	} break; case BASETE + 11: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('?');
		t->move(UP);
		string filename = makefilename();
		t->save(filename.c_str());
		{
			ifstream ifs(filename);
			assert(ifs);
			string s;
			getline(ifs, s, '\xbb');
			assert(s == "XY\n?\n");
		}
		remove(filename.c_str());
	} break; case BASETE + 12: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('?');
		t->getPos(r, c);
		assert(r != 0 && c != 0);
		t->getLines(0, 2, v);
		assert(v.size() != 0);
		t->reset();
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 2, v);
		assert(v.size() == 0);
	} break; case BASETE + 13: {
		t->insert('X');
		t->backspace();
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->backspace();
		t->reset();
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->undo();
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 1, v);
		assert(v.size() == 0);
	} break; case BASETE + 14: {
		t->insert('X');
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->insert('Z');
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XZY");
	} break; case BASETE + 15: {
		t->insert('X');
		t->insert('\t');
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 0 && c == 6);
	} break; case BASETE + 16: {
		t->insert('X');
		t->insert('\t');
		t->insert('Y');
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "X    Y");
	} break; case BASETE + 17: {
		t->insert('X');
		t->move(LEFT);
		t->enter();
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 1 && c == 1);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "" && v[1] == "YX");
	} break; case BASETE + 18: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 1 && c == 1);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "X" && v[1] == "Y");
	} break; case BASETE + 19: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(LEFT);
		t->move(UP);
		t->enter();
		t->getPos(r, c);
		assert(r == 2 && c == 0);
		t->getLines(0, 4, v);
		assert(v.size() == 4 && v[0] == "X" && v[1] == "" && v[2] == "Y" && v[3] == "Z");
	} break; case BASETE + 20: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->enter();
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 1 && c == 1);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "X" && v[1] == "Y");
	} break; case BASETE + 21: {
		t->insert('X');
		t->insert('Y');
		t->insert('Z');
		t->move(LEFT);
		t->move(LEFT);
		t->del();
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XZ");
	} break; case BASETE + 22: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(UP);
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
		t->del();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		v.clear();
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZ");
	} break; case BASETE + 23: {
		t->insert('X');
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
		t->del();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		v.clear();
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 24: {
		t->insert('X');
		t->insert('Y');
		t->insert('Z');
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZ");
		t->backspace();
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		v.clear();
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XZ");
	} break; case BASETE + 25: {
		t->insert('X');
		t->insert('Y');
		t->insert('Z');
		t->getPos(r, c);
		assert(r == 0 && c == 3);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZ");
		t->backspace();
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		v.clear();
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 26: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
		t->backspace();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		v.clear();
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 27: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 1 && c == 0);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
		t->backspace();
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		v.clear();
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZ");
	} break; case BASETE + 28: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->getPos(r, c);
		assert(r == 1 && c == 0);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "");
		t->backspace();
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		v.clear();
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 29: {
		t->insert('X');
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 30: {
		t->insert('X');
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->move(LEFT);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->move(LEFT);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 31: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->getPos(r, c);
		assert(r == 1 && c == 1);
		t->move(LEFT);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
	} break; case BASETE + 32: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 33: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->move(RIGHT);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 34: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(UP);
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 1 && c == 0);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
	} break; case BASETE + 35: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->move(UP);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 36: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->getPos(r, c);
		assert(r == 1 && c == 1);
		t->move(UP);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
	} break; case BASETE + 37: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->insert('Z');
		t->insert('W');
		t->getPos(r, c);
		assert(r == 1 && c == 3);
		t->move(UP);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "X" && v[1] == "YZW");
	} break; case BASETE + 38: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->move(DOWN);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 39: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->insert('Z');
		t->move(LEFT);
		t->move(UP);
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->move(DOWN);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 1 && c == 1);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "X" && v[1] == "YZ");
	} break; case BASETE + 40: {
		t->insert('X');
		t->insert('Y');
		t->insert('Z');
		t->enter();
		t->insert('W');
		t->move(UP);
		t->move(RIGHT);
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 0 && c == 3);
		t->move(DOWN);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 1 && c == 1);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XYZ" && v[1] == "W");
	} break; case BASETE + 41: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->insert('Z');
		t->getPos(r, c);
		assert(r == 1 && c == 2);
		t->move(HOME);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 1 && c == 0);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "X" && v[1] == "YZ");
	} break; case BASETE + 42: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->insert('Z');
		t->move(LEFT);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 1 && c == 0);
		t->move(HOME);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 1 && c == 0);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "X" && v[1] == "YZ");
	} break; case BASETE + 43: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(UP);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->move(END);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
	} break; case BASETE + 44: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(UP);
		t->move(RIGHT);
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->move(END);
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
	} break; case BASETE + 45: {
		t->insert('X');
		assert(t->getLines(-1, 1, v) == -1);
	} break; case BASETE + 46: {
		t->insert('X');
		v.push_back("hello");
		t->getLines(-1, 1, v);
		assert(v.size() == 1 && v[0] == "hello");
	} break; case BASETE + 47: {
		t->insert('X');
		assert(t->getLines(0, -1, v) == -1);
	} break; case BASETE + 48: {
		t->insert('X');
		v.push_back("hello");
		t->getLines(0, -1, v);
		assert(v.size() == 1 && v[0] == "hello");
	} break; case BASETE + 49: {
		t->insert('X');
		assert(t->getLines(2, 1, v) == -1);
	} break; case BASETE + 50: {
		t->insert('X');
		v.push_back("hello");
		t->getLines(2, 1, v);
		assert(v.size() == 1 && v[0] == "hello");
	} break; case BASETE + 51: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->getLines(0, 10, v);
		assert(v.size() == 3);
	} break; case BASETE + 52: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->enter();
		t->insert('Z');
		int n = t->getLines(0, 10, v);
		assert(v.size() == n);
	} break; case BASETE + 53: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->enter();
		t->insert('Z');
		int n = t->getLines(0, 3, v);
		assert(v.size() == n);
	} break; case BASETE + 54: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->enter();
		t->insert('Z');
		v.push_back("hello");
		v.push_back("goodbye");
		int n = t->getLines(0, 3, v);
		assert(n == v.size() && n == 3);
		assert(v[0] == "X" && v[1] == "Y" && v[2] == "Z");
	} break; case BASETE + 55: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->enter();
		t->insert('Z');
		v.push_back("hello");
		t->getLines(3, 0, v);
		assert(v.empty());
	} break; case BASETE + 56: {
		t->insert('X');
		t->enter();
		t->insert('Y');
		t->enter();
		t->insert('Z');
		v.push_back("hello");
		t->getLines(3, 0, v);
		assert(v.empty());
	} break; case BASETE + 57: {
		load(t, "this is\na file\n");
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "this is" && v[1] == "a file");
		t->undo();
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "this is" && v[1] == "a file");
	} break; case BASETE + 58: {
		t->insert('X');
		t->insert('Y');
		t->backspace();
		t->insert('Y');
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
		t->undo();
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "X");
	} break; case BASETE + 59: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->del();
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "X");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 60: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(UP);
		t->move(RIGHT);
		t->del();
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZ");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
	} break; case BASETE + 61: {
		t->insert('X');
		t->insert('Y');
		t->move(LEFT);
		t->backspace();
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "Y");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
	} break; case BASETE + 62: {
		t->insert('X');
		t->insert('Y');
		t->enter();
		t->insert('Z');
		t->move(LEFT);
		t->backspace();
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZ");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 2);
		t->getLines(0, 2, v);
		assert(v.size() == 2 && v[0] == "XY" && v[1] == "Z");
	} break; case BASETE + 63: {
		t->insert('X');
		t->insert('Y');
		t->backspace();
		t->insert('Y');
		t->move(LEFT);
		t->move(LEFT);
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XY");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "X");
	} break; case BASETE + 64: {
		t->insert('W');
		t->move(LEFT);
		t->insert('X');
		t->insert('Y');
		t->insert('Z');
		u->replace(3, 0, 1, "XYZ");
		t->getPos(r, c);
		assert(r == 0 && c == 3);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZW");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 0);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "W");
	} break; case BASETE + 65: {
		t->insert('X');
		t->insert('Y');
		t->insert('Z');
		t->insert('W');
		t->move(LEFT);
		t->move(LEFT);
		t->move(LEFT);
		t->del();
		t->del();
		t->del();
		u->replace(3, 0, 1, "YZW");
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "X");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZW");
	} break; case BASETE + 66: {
		t->insert('X');
		t->insert('Y');
		t->insert('Z');
		t->insert('W');
		t->backspace();
		t->backspace();
		t->backspace();
		u->replace(3, 0, 1, "YZW");
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "X");
		t->undo();
		r = 999; c = 999;
		t->getPos(r, c);
		assert(r == 0 && c == 1);
		t->getLines(0, 1, v);
		assert(v.size() == 1 && v[0] == "XYZW");
	}
	}
}

void testUndo(int n)
{
	constexpr auto ERR = Undo::Action::ERROR;
	constexpr auto INS = Undo::Action::INSERT;
	constexpr auto DEL = Undo::Action::DELETE;
	constexpr auto JOI = Undo::Action::JOIN;
	constexpr auto SPL = Undo::Action::SPLIT;

	auto u = unique_ptr<Undo>(createUndo());

	int r = 999;
	int c = 999;
	int ct = 999;
	string t = "ZZZZ";


	switch (n)
	{
	default: {
		cout << "Bad argument UN" << endl;
	} break; case BASEUN + 1: {
		assert(u->get(r, c, ct, t) == ERR);
	} break; case BASEUN + 2: {
		u->get(r, c, ct, t);
		assert(r == 999 && c == 999 && ct == 999 && t == "ZZZZ");
	} break; case BASEUN + 3: {
		u->clear();
		assert(u->get(r, c, ct, t) == ERR);
	} break; case BASEUN + 4: {
		u->submit(INS, 1, 1, 'k');
		u->submit(DEL, 2, 2, 'k');
		u->submit(JOI, 3, 3, 'k');
		assert(u->get(r, c, ct, t) != ERR);
		u->clear();
		assert(u->get(r, c, ct, t) == ERR);
	} break; case BASEUN + 5: {
		assert(u->get(r, c, ct, t) == ERR);
		u->submit(INS, 1, 1, 'k');
		u->submit(DEL, 2, 2, 'k');
		u->submit(INS, 3, 3, 'k');
		assert(u->get(r, c, ct, t) != ERR);
		assert(u->get(r, c, ct, t) != ERR);
		assert(u->get(r, c, ct, t) != ERR);
		assert(u->get(r, c, ct, t) == ERR);
	} break; case BASEUN + 6: {
		u->submit(INS, 10, 20, 'k');
		assert(u->get(r, c, ct, t) == DEL);
	} break; case BASEUN + 7: {
		u->submit(INS, 10, 20, 'k');
		u->get(r, c, ct, t);
		assert(r == 10 && c == 20 - 1 && ct == 1 && t.empty());
	} break; case BASEUN + 8: {
		u->submit(DEL, 10, 20, 'k');
		assert(u->get(r, c, ct, t) == INS);
	} break; case BASEUN + 9: {
		u->submit(DEL, 10, 20, 'k');
		u->get(r, c, ct, t);
		assert(r == 10 && c == 20 && ct == 1 && t == "k");
	} break; case BASEUN + 10: {
		u->submit(JOI, 10, 20, 'k');
		assert(u->get(r, c, ct, t) == SPL);
	} break; case BASEUN + 11: {
		u->submit(JOI, 10, 20, 'k');
		u->get(r, c, ct, t);
		assert(r == 10 && c == 20 && ct == 1 && t.empty());
	} break; case BASEUN + 12: {
		u->submit(SPL, 10, 20, 'k');
		assert(u->get(r, c, ct, t) == JOI);
	} break; case BASEUN + 13: {
		u->submit(SPL, 10, 20, 'k');
		u->get(r, c, ct, t);
		assert(r == 10 && c == 20 && ct == 1 && t.empty());
	} break; case BASEUN + 14: {
		u->submit(DEL, 10, 20, 'k');
		u->submit(INS, 10, 21, 'd');
		u->submit(INS, 10, 22, 'o');
		u->submit(INS, 10, 23, 'g');
		u->submit(DEL, 10, 24, 'm');
		u->get(r, c, ct, t);
		assert(u->get(r, c, ct, t) == DEL && u->get(r, c, ct, t) == INS);
	} break; case BASEUN + 15: {
		u->submit(DEL, 10, 20, 'k');
		u->submit(INS, 10, 21, 'd');
		u->submit(INS, 10, 22, 'o');
		u->submit(INS, 10, 23, 'g');
		u->submit(DEL, 10, 24, 'm');
		u->get(r, c, ct, t);
		r = 999; c = 999; ct = 999; t = "ZZZZ";
		u->get(r, c, ct, t);
		assert(r == 10 && c == 21 - 1 && ct == 3 && t.empty());
	} break; case BASEUN + 16: {
		u->submit(DEL, 10, 19, 'k');
		u->submit(INS, 9, 20, 'x');
		u->submit(INS, 10, 21, 'd');
		u->submit(INS, 10, 22, 'o');
		u->submit(INS, 10, 22, 'g');
		u->submit(INS, 10, 23, 'm');
		u->submit(INS, 10, 24, 'a');
		u->submit(DEL, 10, 25, 'n');
		u->get(r, c, ct, t);
		assert(u->get(r, c, ct, t) == DEL && ct == 3);
		ct = 999;
		assert(u->get(r, c, ct, t) == DEL && ct == 2);
	} break; case BASEUN + 17: {
		u->submit(INS, 10, 20, 'k');
		u->submit(DEL, 10, 20, 'd');
		u->submit(DEL, 10, 20, 'o');
		u->submit(DEL, 10, 20, 'g');
		u->submit(INS, 10, 20, 'm');
		u->get(r, c, ct, t);
		assert(u->get(r, c, ct, t) == INS && u->get(r, c, ct, t) == DEL);
	} break; case BASEUN + 18: {
		u->submit(INS, 10, 20, 'k');
		u->submit(DEL, 10, 20, 'd');
		u->submit(DEL, 10, 20, 'o');
		u->submit(DEL, 10, 20, 'g');
		u->submit(INS, 10, 20, 'm');
		u->get(r, c, ct, t);
		r = 999; c = 999; ct = 999; t = "ZZZZ";
		u->get(r, c, ct, t);
		assert(r == 10 && c == 20 && ct == 1 && t == "dog");
	} break; case BASEUN + 19: {
		u->submit(INS, 10, 20, 'k');
		u->submit(DEL, 9, 20, 'x');
		u->submit(DEL, 10, 20, 'd');
		u->submit(DEL, 10, 20, 'o');
		u->submit(DEL, 10, 21, 'g');
		u->submit(DEL, 10, 21, 'm');
		u->submit(DEL, 10, 21, 'a');
		u->submit(INS, 10, 21, 'n');
		u->get(r, c, ct, t);
		assert(u->get(r, c, ct, t) == INS && t == "gma");
		assert(u->get(r, c, ct, t) == INS && t == "do");
	} break; case BASEUN + 20: {
		u->submit(INS, 10, 24, 'm');
		u->submit(DEL, 10, 23, 'g');
		u->submit(DEL, 10, 22, 'o');
		u->submit(DEL, 10, 21, 'd');
		u->submit(INS, 10, 20, 'k');
		u->get(r, c, ct, t);
		assert(u->get(r, c, ct, t) == INS && u->get(r, c, ct, t) == DEL);
	} break; case BASEUN + 21: {
		u->submit(INS, 10, 24, 'm');
		u->submit(DEL, 10, 23, 'g');
		u->submit(DEL, 10, 22, 'o');
		u->submit(DEL, 10, 21, 'd');
		u->submit(INS, 10, 20, 'k');
		u->get(r, c, ct, t);
		r = 999; c = 999; ct = 999; t = "ZZZZ";
		u->get(r, c, ct, t);
		assert(r == 10 && c == 21 && ct == 1 && t == "dog");
	} break; case BASEUN + 22: {
		u->submit(INS, 10, 25, 'n');
		u->submit(DEL, 9, 24, 'x');
		u->submit(DEL, 10, 23, 'a');
		u->submit(DEL, 10, 22, 'm');
		u->submit(DEL, 10, 23, 'g');
		u->submit(DEL, 10, 22, 'o');
		u->submit(DEL, 10, 21, 'd');
		u->submit(INS, 10, 20, 'k');
		u->get(r, c, ct, t);
		assert(u->get(r, c, ct, t) == INS && t == "dog");
		assert(u->get(r, c, ct, t) == INS && t == "ma");
	} break; case BASEUN + 23: {
		u->submit(INS, 10, 22, 'g');
		u->submit(DEL, 10, 22, 'g');
		u->submit(DEL, 10, 22, 'm');
		u->submit(DEL, 10, 21, 'o');
		u->submit(DEL, 10, 20, 'd');
		u->submit(DEL, 10, 20, 'a');
		u->submit(DEL, 10, 20, 's');
		u->submit(INS, 10, 22, 'k');
		u->get(r, c, ct, t);
		assert(u->get(r, c, ct, t) == INS);
		assert(t == "dogmas" ||  // "dogmas" means batched mixed BS and DEL
			(t == "as" && u->get(r, c, ct, t) == INS && t == "do"));
	}
	}
}

void testSpellCheck(int n)
{
	auto s = unique_ptr<SpellCheck>(createSpellCheck());

	vector<string> v;
	vector<SpellCheck::Position> probs;

	switch (n)
	{
	default: {
		cout << "Bad argument SP" << endl;
	} break; case BASESP + 1: {
		assert(!s->load("/this/file/does/not/exist"));
		assert(load(s, "dog\ncat\n"));
	} break; case BASESP + 2: {
		load(s, "cat\n");
		assert(s->spellCheck("cat", 0, v) && !s->spellCheck("dog", 0, v));
	} break; case BASESP + 3: {
		load(s, "   c29^#%-a t@5,\n");
		assert(s->spellCheck("cat", 0, v) && !s->spellCheck("dog", 0, v));
	} break; case BASESP + 4: {
		load(s, "can' --t\n");
		assert(s->spellCheck("can't", 0, v) && !s->spellCheck("cant", 0, v));
	} break; case BASESP + 5: {
		load(s, "cant\n");
		assert(s->spellCheck("cant", 0, v) && !s->spellCheck("can't", 0, v));
	} break; case BASESP + 6: {
		load(s, "CaT\n");
		assert(s->spellCheck("cAt", 0, v) && !s->spellCheck("dog", 0, v));
	} break; case BASESP + 7: {
		load(s, "mouse\nfish\ncat\nhorse\n");
		assert(s->spellCheck("cat", 0, v) && !s->spellCheck("dog", 0, v));
	} break; case BASESP + 8: {
		load(s, "cat\n");
		assert(s->spellCheck("cat", 0, v) && !s->spellCheck("catch", 0, v));
	} break; case BASESP + 9: {
		load(s, "catch\n");
		assert(s->spellCheck("catch", 0, v) && !s->spellCheck("cat", 0, v));
	} break; case BASESP + 10: {
		load(s, "catch\ncat\ncatcher\n");
		assert(s->spellCheck("cat", 0, v) && s->spellCheck("catcher", 0, v) &&
			s->spellCheck("catch", 0, v) && !s->spellCheck("c", 0, v) &&
			!s->spellCheck("catc", 0, v) && !s->spellCheck("catchers", 0, v));
	} break; case BASESP + 11: {
		load(s, "'tis\n'enry\nzap\nzoo\nzounds\nzonk\na\n\ant\nape\nany\nain't\n\npo'\n");
		assert(s->spellCheck("zoo", 0, v) && s->spellCheck("a", 0, v) &&
			s->spellCheck("zonk", 0, v) && s->spellCheck("'enry", 0, v) &&
			s->spellCheck("ain't", 0, v) && s->spellCheck("po'", 0, v) &&
			!s->spellCheck("apt", 0, v) && !s->spellCheck("b", 0, v) &&
			!s->spellCheck("'til", 0, v));
	} break; case BASESP + 12: {
		load(s, "cat\n");
		assert(s->spellCheck("cat", 1, v) && v.empty());
	} break; case BASESP + 13: {
		string dict = "cat\n";
		load(s, dict);
		assert(!s->spellCheck("rat", 0, v) && v.empty());
	} break; case BASESP + 14: {
		string dict = "cat\n";
		load(s, dict);
		assert(!s->spellCheck("rat", 1, v) && v.size() == 1 &&
			goodsuggs("rat", v, dict));
	} break; case BASESP + 15: {
		string dict = "cat\nrate\nbrat\nrut\nrug\ncar\nset\ndog\nrag\n";
		load(s, dict);
		assert(!s->spellCheck("rat", 9, v) && v.size() == 3 &&
			goodsuggs("rat", v, dict));
	} break; case BASESP + 16: {
		string dict = "cat\nrate\nbrat\nrut\nrug\ncar\nset\ndog\nrag\n";
		load(s, dict);
		assert(!s->spellCheck("rat", 12, v) && v.size() == 3 &&
			goodsuggs("rat", v, dict));
	} break; case BASESP + 17: {
		string dict = "cat\nrate\nbrat\nrut\nrug\ncar\nset\ndog\nrag\n";
		load(s, dict);
		assert(!s->spellCheck("rat", 12, v) && v.size() == 3 &&
			goodsuggs("rat", v, dict));
	} break; case BASESP + 18: {
		string dict = "cat\nrate\nbrat\nrut\nrug\ncar\nset\ndog\nrag\n";
		load(s, dict);
		assert(!s->spellCheck("rat", 2, v) && v.size() == 2 &&
			goodsuggs("rat", v, dict));
	} break; case BASESP + 19: {
		string dict = "lease\nwe've\nwears\nleave\n";
		load(s, dict);
		assert(!s->spellCheck("weave", 2, v) && v.size() == 2 &&
			goodsuggs("weave", v, dict));
	} break; case BASESP + 20: {
		string dict = "lease\nweave\nwears\nwe're\n";
		load(s, dict);
		assert(!s->spellCheck("we've", 2, v) && v.size() == 2 &&
			goodsuggs("we've", v, dict));
	} break; case BASESP + 21: {
		load(s, "cat\ndog\n");
		s->spellCheckLine("cat dog dog cat cat", probs);
		assert(probs.empty());
	} break; case BASESP + 22: {
		load(s, "cat\ndog\n");
		s->spellCheckLine("cat dog rat dog bird cat cat caw", probs);
		assert(isPerm(probs, { { 8, 10 }, { 16, 19 }, { 29, 31 } }));
	} break; case BASESP + 23: {
		load(s, "cat\ndog\n");
		s->spellCheckLine(" cat@dog32%.~dog bird     cat:caw*", probs);
		assert(isPerm(probs, { { 17, 20 }, { 30, 32 } }));
	} break; case BASESP + 24: {
		load(s, "cat\ndog\n");
		s->spellCheckLine("CaT dOg rAt Dog BIrd CAT caT cAW", probs);
		assert(isPerm(probs, { { 8, 10 }, { 16, 19 }, { 29, 31 } }));
	} break; case BASESP + 25: {
		load(s, "can't\n'tis\n");
		s->spellCheckLine("I can't 'tis cant ", probs);
		assert(isPerm(probs, { { 0, 0 }, { 13, 16 } }));
	}
	}
}

int main()
{
	cout << "Enter test number: ";
	int n;
	cin >> n;
	if (n > BASESP)
		testSpellCheck(n);
	else if (n > BASEUN)
		testUndo(n);
	else
		testTextEditor(n);
	cout << "Passed" << endl;
}