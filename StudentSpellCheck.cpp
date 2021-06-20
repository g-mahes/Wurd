#include "StudentSpellCheck.h"
#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <ctype.h>
#include <fstream>

using namespace std;

SpellCheck* createSpellCheck()
{
	return new StudentSpellCheck;
}

StudentSpellCheck::~StudentSpellCheck() {
	// TODO
}

bool StudentSpellCheck::load(std::string dictionaryFile) {
	root->free(root);
	ifstream infile(dictionaryFile);
	if (!infile)
		return false; // dictionaryFile not loaded
	string s;
	while (getline(infile, s)) {
		root->insert(root,s); // insert the line into trie data structure
	}
	return true;
}

bool StudentSpellCheck::spellCheck(std::string word, int max_suggestions, std::vector<std::string>& suggestions) {
	for (int i = 0; i < word.size(); i++) {
		word[i] = tolower(word[i]);
	}
	if (root->searchWord(root, word)) {
		return true;
	}
	else {
		vector<string>::iterator it = suggestions.begin();
		suggestions.clear(); // clear suggestions
		int suggest = 0;
		string sugg = word;
		string temp;
		for (int i = 0; i < word.size(); i++) {
			for (int g = 'a'; g < 'z'; g++) {
				temp = sugg;
				sugg[i] = g;
				if (root->searchWord(root, sugg) && suggest < max_suggestions) {
					suggest++;
					suggestions.push_back(sugg);
				}
				sugg = temp;
			}
		}
		return false;
	}
}

void StudentSpellCheck::spellCheckLine(const std::string& line, std::vector<SpellCheck::Position>& problems) {
	int pos1 = -1;
	int pos2 = -1;
	Position temp;
	string lineTemp = line;
	for (int i = 0; i < lineTemp.size(); i++) {
		if (isalpha(lineTemp[i]))
			lineTemp[i] = tolower(lineTemp[i]);
	}
	for (int i = 0; i < line.size(); i++) {
		if (isalpha(line[0])) { // case for first word
			pos1 = 0;
		}
		if (!isalpha(line[i]) && line[i] != '’' && pos1 == 0) { // case for first word
			pos2 = i;
			if (!root->searchWord(root, lineTemp.substr(pos1, pos2-pos1))) {
				temp.start = pos1;
				temp.end = pos2-1;
				problems.push_back(temp); // push back first word positions
			}
			if (pos2 + 2 >= line.size())
				return; // return if only one word
			break;
		}
	}
	for (int i = pos2+1; i < line.size(); i++) {
		if (ispunct(lineTemp[i])) {
			continue;
		}
		if (isalpha(line[i]) || line[i] == '’') { // case for first word
			pos1 = i;
			while (isalpha(line[i])) {
				i++;
			}
			pos2 = i;
			
			if (!root->searchWord(root, lineTemp.substr(pos1, pos2-pos1))) {
				temp.start = pos1;
				temp.end = pos2-1;
				problems.push_back(temp); // push back first word positions
				i--;
			}
		}
	}
}
