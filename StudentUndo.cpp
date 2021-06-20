#include "StudentUndo.h"
#include <stack>
#include <string>

using namespace std;
Undo* createUndo()
{
	return new StudentUndo;
}

void StudentUndo::submit(const Action action, int row, int col, char ch) {
	string s = string(1, ch);
	int colTemp = col;
	int rowTemp = row;
	if (action == Undo::Action::INSERT) { // submitting insert
		if (!undo.empty() && undo.top().act==INSERT) { // if checking if there is batching
			string temp = undo.top().m_batch; // store top stack string for batching
			s += temp;
			if (!undo.empty())
				undo.pop(); // pop first stack item if not empty
			while (!undo.empty() && undo.top().act == INSERT && undo.top().m_row == row) { // check if there is batching
				temp = undo.top().m_batch;
				s += temp; // add to string if there is batching
				undo.pop();
				colTemp = undo.top().m_col;
				rowTemp = undo.top().m_row;
			}
			undoInfo ins(INSERT, rowTemp, colTemp, s);
			undo.push(ins); // push string with correct rows and columns
		}
		else // submitting one character
		{
			undoInfo temp(INSERT, row, col, s); 
			undo.push(temp); // push the one character into stack
		}
	}
	if (action == Undo::Action::DELETE) {
		if(!undo.empty()){ // check for batching
			if (undo.top().m_col == col && undo.top().act == DELETE) { // see if top of stack if another delete action that is using delete key
				string temp = undo.top().m_batch;
				s.insert(0, temp);
				if (!undo.empty())
					undo.pop(); // pop to check other actions
				while (!undo.empty() && undo.top().m_col == col && undo.top().act == DELETE) { // repeatedly check for batching and add to string
					temp = undo.top().m_batch;
					s.insert(0, temp); // keep adding to string if there is batching
					undo.pop(); // pop to next stack item
				}
				undoInfo del(DELETE, row, col+1, s); 
				undo.push(del);// push string into stack once we find the batch
			}
			else {   // know that this action used the backspace key because not the same column
				string temp = undo.top().m_batch; 
				s += temp;
				if (!undo.empty())
					undo.pop(); // pop the stack if not empty
				while (!undo.empty() && undo.top().m_col == col + 1) { // check for batching
					temp = undo.top().m_batch;
					s += temp; // add to string if there is batching
					undo.pop();
				}
				undoInfo back(DELETE, row, col+1, s);
				undo.push(back); // push string into stack once we find the batch
			}
		}
		else {
			undoInfo te(DELETE, row, col, s);
			undo.push(te); // push single character
		}		
	}
	if (action == Undo::Action::JOIN) { // join action
		undoInfo temp(JOIN, row, col, s);
		undo.push(temp); // push join action with correct column and row into stack
	}
	if (action == Undo::Action::SPLIT) { // split action
		undoInfo temp(SPLIT, row, col, s);
		undo.push(temp); // push split action with correct column and row into stack
	}
}

StudentUndo::Action StudentUndo::get(int &row, int &col, int& count, std::string& text) {
	if (undo.empty())
		return Undo::Action::ERROR;
	Action act = undo.top().act;
	if (act == Undo::Action::INSERT) { //action is insert
		string s = undo.top().m_batch;
		count = s.size();
		row = undo.top().m_row; // change row to top stack's row
		col = undo.top().m_col - s.size(); // change column to original pos
		if (undo.empty())
			return Undo::Action::ERROR; // return error if stack is empty
		else
			undo.pop(); // pop stack if not empty
		return Undo::Action::DELETE; // return delete
	}
	if (act == Undo::Action::DELETE) {
		string s = undo.top().m_batch;
		row = undo.top().m_row; // change row to top stack's row
		col = undo.top().m_col;
		count = 1; // count is string size (for batching)
		text = s; // text for deleting
		if (undo.empty())
			return Undo::Action::ERROR; // return error if stack is empty
		else
			undo.pop(); // pop stack if not empty
		return Undo::Action::INSERT; // return action insert
	}
	if (act == Undo::Action::JOIN) {
		count = 1; // count is 1
		row = undo.top().m_row; // change row to top stack's row
		col = undo.top().m_col;
		if (undo.empty())
			return Undo::Action::ERROR; // return error if stack is empty
		else
			undo.pop(); // pop stack if not empty;
		return Undo::Action::SPLIT; // return action split
	}
	if (act == Undo::Action::SPLIT) {
		count = 1; // count is 1
		row = undo.top().m_row; // change row to top stack's row
		col = undo.top().m_col;
		if (undo.empty())
			return Undo::Action::ERROR; // return error if stack is empty
		else
			undo.pop(); // pop stack if not empty
		return Undo::Action::JOIN; // return action split
	}
	return Undo::Action::ERROR;
}

void StudentUndo::clear() {
	for (int i = 0; i < undo.size(); i++) {
		undo.pop(); // pop from stack repeatedly until empty
	}
}
