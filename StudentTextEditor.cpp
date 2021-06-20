#include "StudentTextEditor.h"
#include "Undo.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

TextEditor* createTextEditor(Undo* un)
{
	return new StudentTextEditor(un);
}

StudentTextEditor::StudentTextEditor(Undo* undo)
 : TextEditor(undo) {
	rowEdit = 0;
	colEdit = 0;
	m_list.push_back("");
	it1 = m_list.begin();
}

StudentTextEditor::~StudentTextEditor()
{
	reset(); // reset doc
}

bool StudentTextEditor::load(std::string file) {
	ifstream infile(file);
	if (!infile)
		return false; // file cannot be loaded
	else {
		reset(); // old contents of text editor is reset
		string s;
		listSize = 0;
		while (getline(infile, s)) { // runs N times for new file being loaded (checks if last character needs to be removed)
			if (s.length() > 0) {
				if (s.back() == '\r') // check if last character is \r
					s.pop_back(); // remove \r is it is last character
			}
			m_list.push_back(s); // push back the line into new list
			listSize++; // increment size int
		}
		if (listSize == 0) {
			m_list.push_back("");
			listSize++;
		}			
		it1 = m_list.begin();
		rowEdit = 0;
		colEdit = 0;
		return true; // new file loaded
	}
}

bool StudentTextEditor::save(std::string file) {
	ofstream outfile(file);
	if (!outfile)
		return false; // false if cannot open file
	else {
		list<string>::iterator it = m_list.begin(); // iterator that starts at line 1
		while (it != m_list.end()) {
			outfile << *it << endl; // output line on the file along with new line character
			it++; // iterator iterator
		}
		return true; // return true if saved 
	}
}

void StudentTextEditor::reset() {
	rowEdit, colEdit = 0;
	list<string>::iterator it = m_list.begin();
	while (it != m_list.end()) {
		it = m_list.erase(it); // erase list contents 	
	}
	listSize = 0;
	getUndo()->clear();
	return;
}

void StudentTextEditor::move(Dir dir) {
	string currentLine = *it1;
	switch (dir)
	{
	case TextEditor::UP:
		if (rowEdit > 0) { // check if current line is not top line
			it1--; // iterate iterator to line above
			if (*it1 == "") {
				colEdit = 0;
				rowEdit--;
			}
			else {
				rowEdit--; // move up
			}
		}
		break;
	case TextEditor::DOWN:
		if (rowEdit < listSize - 1) { // check if current line is not bottom line
			it1++; // iterate iterator to line below
			if (*it1 == "") {
				colEdit = 0;
				rowEdit++;
			}
			else {
				rowEdit++; // move down
			}
		}
		break;
	case TextEditor::LEFT:
			if (colEdit == 0 && rowEdit == 0)
				break; // do nothing if current pos is first character of top line
			if (colEdit == 0) { // if position is on first character of the line
				it1--; // iterate iterator to line above
				if (*it1 == "") {
					rowEdit--;
					colEdit = 0;
				}
				else {
					rowEdit--; // move up a row
					currentLine = *it1;
					colEdit = currentLine.size(); // move current column after last character on previous line
				}
			}
			else {
				colEdit--; // move to the left
			}
			break;
	case TextEditor::RIGHT:
			currentLine = *it1;
			if (colEdit == currentLine.size() && rowEdit == listSize - 1)
				break; // do nothing if current pos is last character of bottom line
			if (colEdit == currentLine.size()) {
				it1++; // iterate iterator to line below
				if (*it1 == "")
				{
					colEdit = 0;
					rowEdit++;
				}
				else {
					colEdit = 0; // move to first column
					rowEdit++; // move to next row
				}
			}
			else {
				colEdit++; // move to the right
			}
			break;
	case TextEditor::HOME:
			colEdit = 0; // move to first character on line
			break;
	case TextEditor::END:
			int size = currentLine.size();
			colEdit = size; // move pos to just after character on line
			break;
	} 
}

void StudentTextEditor::del() {
	list<string>::iterator it = m_list.end();
	it--;
	string temp = *it;
	int rowBefore = rowEdit;
	int colBefore = colEdit;
	if (rowEdit == listSize-1 && colEdit == temp.size())
		return; // do nothing if current pos is at last character on last line
	string s = *it1; // keep track of current line as a string
	if (colEdit == s.size()) {
		it1++;
		string s1 = *it1; // temp string for next line
		s = s + s1; // combine both lines
		it1 = m_list.erase(it1); // erase the line from the list
		listSize--; // decrement size
		it1--; // decrement current line
		*it1 = s; // store new line in list
		getUndo()->submit(Undo::Action::JOIN, rowBefore, colBefore, ' ');
		return;
	}
	else {
		if (colEdit == 0) { // if editing first pos in line
			char ch = s[0];
			s = s.substr(1, s.size()); // take substring from pos 1 to size
			*it1 = s; // store new line
			getUndo()->submit(Undo::Action::DELETE, rowEdit, colEdit+1, ch);
			return;
		}
		else if (colEdit == s.size() - 1) { // if deleting last character on line
			char ch = s[s.size() - 1];
			s = s.substr(0, s.size() - 2); // take substring from pos 0 to characer before character we want to delete
			*it1 = s; // store new line 
			getUndo()->submit(Undo::Action::DELETE, rowEdit, colEdit+1, ch);
			return;
		}
		else {
			char ch = s[colEdit];
			s = s.substr(0, colEdit) + s.substr(colEdit + 1, s.size()); // take substring from pos 0 to size excluding the current column
			*it1 = s; // store new line
			getUndo()->submit(Undo::Action::DELETE, rowEdit, colEdit+1, ch);
			return;
		}
	}
}

void StudentTextEditor::backspace() {
	string s = *it1;
	if (colEdit == 0 && rowEdit == 0)
		return;
	if (colEdit > 0) // case for anything after first character
	{
		char ch;
		if (colEdit == 1) // if second character
		{
			ch = s[0];
			s = s.substr(1, s.size()); // substring from first character to end of string
		}
		else if(colEdit > 1 && colEdit <= s.size()-1) {
			ch = s[colEdit-1];
			int colDelete = colEdit;
			s = s.substr(0, colDelete-1) + s.substr(colDelete, s.size()); // substring excluding current column -1
		}
		else if (colEdit == s.size()) { // if backspacing at end of line
			ch = s[s.size()-1];
			s = s.substr(0, s.size() - 1); // substring from beginning to end of line -1
		}
		*it1 = s;
		colEdit--;
		getUndo()->submit(Undo::Action::DELETE, rowEdit, colEdit+1, ch); // submit undo 
		return;
	}
	if (s == "" || colEdit == 0) { // if deleting empty line or first column
		it1--; // get line above
		string s1 = *it1; // store first line
		it1++; // get current line
		string s2 = *it1; // store second line
		rowEdit--;
		colEdit = s1.size();
		s1 = s1 + s2; // combine both lines
		it1 = m_list.erase(it1); // erase second line from list
		listSize--;
		it1--;
		*it1 = s1;
		getUndo()->submit(Undo::Action::JOIN, rowEdit, colEdit, s[0]); // submit undo
		return;
	}
}

void StudentTextEditor::insert(char ch) {
	string s = *it1;
	if (colEdit == 0) {
		if (ch == '\t') { // if tab character
			s = "    " + s;
			colEdit = colEdit + 4; // iterate current pos
			getUndo()->submit(Undo::Action::INSERT, rowEdit, colEdit, '\t'); // submit undo
		}
		else {
			s = ch + s;
			colEdit++;  // iterate current pos
			getUndo()->submit(Undo::Action::INSERT, rowEdit, colEdit, ch);// submit undo
		} 
		*it1 = s;
	}
	else {
		if (ch == '\t') { // if tab character
			s = s.substr(0, colEdit) + "    " + s.substr(colEdit, s.size()); // substring that includes new character
			colEdit = colEdit + 4;  // iterate current pos
			getUndo()->submit(Undo::Action::INSERT, rowEdit, colEdit, '\t');// submit undo
		}
		else {
			s = s.substr(0, colEdit) + ch + s.substr(colEdit, s.size()); // substring that includes new character
			colEdit++;  // iterate current pos
			getUndo()->submit(Undo::Action::INSERT, rowEdit, colEdit, ch);// submit undo
		}
		*it1 = s;
	}
	return;
}

void StudentTextEditor::enter() {
	string s = *it1;
	if (colEdit == s.size()) { // last pos
		int colBefore = colEdit;
		int rowBefore = rowEdit;
		rowEdit++; // increment row pos
		colEdit = 0; // current column pos is 0
		m_list.insert(next(it1), ""); // insert empty line
		it1++; // increment iterator to next line
		listSize++; // increment size
		getUndo()->submit(Undo::Action::SPLIT, rowBefore, colBefore, ' ');
		return;
	}
	if (colEdit >= 0 && colEdit < s.size()) {
		int colBefore = colEdit;
		int rowBefore = rowEdit;
		string s1 = s.substr(0, colEdit); // substring from pos 0 to colEdit-1
		string s2 = s.substr(colEdit, s.size()); // substring from colEdit to end of the line
		*it1 = s1; // store s1 into line
		rowEdit++; // increment row to next line
		colEdit = 0; // column is set to 0 
		m_list.insert(next(it1), s2); // insert new line (s2)
		it1++; // increment iterator
		listSize++; // increment size
		getUndo()->submit(Undo::Action::SPLIT, rowBefore, colBefore, ' ');
		return;
	}
}

void StudentTextEditor::getPos(int& row, int& col) const {
	row = rowEdit;
	col = colEdit;
}

int StudentTextEditor::getLines(int startRow, int numRows, std::vector<std::string>& lines) const {
	if (startRow < 0 || numRows < 0)
		return -1; // return -1 if startRow or numrows is negative
	if (startRow > listSize) {
		return -1; // return -1 if startRow is greater than number of lines in the text
	}
	if (startRow == listSize)
		return 0; // nothing happens if startRow is equal to number of lines in text
	list<string>::const_iterator it = m_list.begin();
	int count = 0;
	int countLines = 0;
	while (it != m_list.end() && countLines <= numRows) {
		if (count >= startRow ) {
			lines.push_back((*it));
			countLines++;
		}
		it++; // iterate iterator
		count++; // iterate variable that tracks number of lines
	}
	return countLines; // returns number of lines in the lines parameter

}

void StudentTextEditor::undo() {
	int row1;
	int col1;
	int count1;
	string undo;
	switch (getUndo()->get(row1,col1,count1,undo))
	{
	case Undo::Action::INSERT: { // inserting back into doc
		while (rowEdit > row1) { // ensure iterator is correct
			rowEdit--;
			it1--;
		}
		while (rowEdit < row1) { // ensure iterator is correct
			rowEdit++;
			it1++;
		}
		rowEdit = row1;
		colEdit = col1;
		string currLine = *it1;
		if (colEdit == 0) { // if inserting to first column
			currLine = undo + currLine; //combine character into line
			*it1 = currLine;
			break;
		}
		if (colEdit > 0 && colEdit <= currLine.size() - 1) {
			currLine = currLine.substr(0, colEdit-undo.size()) + undo + currLine.substr(colEdit-undo.size(), currLine.size()); //combine character into line
			*it1 = currLine;
			break;
		}
		if (colEdit == currLine.size()) {
			currLine = currLine + undo; //combine character into line
			*it1 = currLine;
			break;
		}
		break; }
	case Undo::Action::DELETE: {
		while (rowEdit > row1) { // ensure iterator is correct
			rowEdit--;
			it1--;
		}
		while (rowEdit < row1) { // ensure iterator is correct
			rowEdit++;
			it1++;
		}
		rowEdit = row1; // assign correct row
		colEdit = col1; // assign correct col
		string s = *it1;
		if (colEdit == 0) { // if deleting first column
			s = s.substr(count1, s.size()); // substring excluding first characters that we want to delete
		}
		else {
			s = s.substr(0, colEdit) + s.substr(colEdit + count1, s.size()); // substring that excludes characters we awnt to delete
		}
		*it1 = s;
		
		break;
	}	
	case Undo::Action::ERROR: {
		break;
	}			
	case Undo::Action::JOIN: {
		while (rowEdit > row1) { // ensure iterator is correct
			rowEdit--;
			it1--;
		}
		while (rowEdit < row1) { // ensure iterator is correct
			rowEdit++;
			it1++;
		}
		rowEdit = row1;
		colEdit = col1;
		string s = *it1;
		if (colEdit == s.size()) {
			it1++;
			string s1 = *it1; // temp string for next line
			s = s + s1; // combine both lines
			it1 = m_list.erase(it1); // erase the line from the list
			listSize--; // decrement size
			it1--; // decrement current line
			*it1 = s; // store new line in list
			return;
		}
		break;
	}		
	case Undo::Action::SPLIT:
		while (rowEdit > row1) { // ensure iterator is correct
			rowEdit--;
			it1--;
		}
		while (rowEdit < row1) { // ensure iterator is correct
			rowEdit++;
			it1++;
		}
		rowEdit = row1;
		colEdit = col1;
		string s = *it1;
		string temp = s.substr(0, colEdit); // store first line
		string temp2 = s.substr(colEdit, s.size()); // store second lline
		m_list.insert(next(it1), temp2); // split both lines
		*it1 = temp;
		break;
	}
}

