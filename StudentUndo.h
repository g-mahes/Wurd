#ifndef STUDENTUNDO_H_
#define STUDENTUNDO_H_

#include "Undo.h"
#include <string>
#include <stack>
class StudentUndo : public Undo {
public:

	void submit(Action action, int row, int col, char ch = 0);
	Action get(int& row, int& col, int& count, std::string& text);
	void clear();

private:
	struct undoInfo {
		undoInfo(Action action, int row, int col, std::string batch) : act(action), m_row(row),m_col(col),m_batch(batch){};
		Action act;
		int m_row;
		int m_col;
		std::string m_batch;
	};
	std::stack<undoInfo> undo;
};

#endif // STUDENTUNDO_H_
