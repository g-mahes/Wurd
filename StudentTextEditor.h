#ifndef STUDENTTEXTEDITOR_H_
#define STUDENTTEXTEDITOR_H_

#include "TextEditor.h"
#include <list>
class Undo;

class StudentTextEditor : public TextEditor {
public:

	StudentTextEditor(Undo* undo);
	~StudentTextEditor();
	bool load(std::string file);
	bool save(std::string file);
	void reset();
	void move(Dir dir);
	void del();
	void backspace();
	void insert(char ch);
	void enter();
	void getPos(int& row, int& col) const;
	int getLines(int startRow, int numRows, std::vector<std::string>& lines) const;
	void undo();

private:
	std::list<std::string> m_list;
	std::list<std::string>::iterator it1 = m_list.begin();
	int rowEdit =0;
	int colEdit =0;
	int listSize;
};

#endif // STUDENTTEXTEDITOR_H_
