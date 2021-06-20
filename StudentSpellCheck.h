#ifndef STUDENTSPELLCHECK_H_
#define STUDENTSPELLCHECK_H_

#include "SpellCheck.h"

#include <string>
#include <vector>

class StudentSpellCheck : public SpellCheck {
public:
    StudentSpellCheck() { }
	virtual ~StudentSpellCheck();
	bool load(std::string dict_file);
	bool spellCheck(std::string word, int maxSuggestions, std::vector<std::string>& suggestions);
	void spellCheckLine(const std::string& line, std::vector<Position>& problems);

private:
	struct Trie {
	public:
		bool isEnd; // bool for each node to check if its the end of the trie
		Trie* c[27]; // array for 27 children
		Trie* root;
		Trie() { // constructor
			this->isEnd = false;
			for (int i = 0; i < 27; i++) {
				this->c[i] = nullptr; // all children are nullptr
			}
		}

		bool child(Trie* node) { // check if node has child
			for (int i = 0; i < 27; i++) {
				if (node->c[i] != nullptr)
					return true; // node has child,, then return true
			}
			return false; // no child
		}
		void insert(Trie* root, std::string word) { // insert node into trie
			Trie* currentNode = root;
			for (int g = 0; g < word.length(); g++) {
				int wordInd;
				if (word[g] == 39)
					wordInd = 26; // apostrophe case
				else {
					wordInd = word[g] - 'a'; // letter case
				}	 
				if (currentNode->c[wordInd] == nullptr)
					currentNode->c[wordInd] = new Trie(); // new node if there is not node there
				currentNode = currentNode->c[wordInd]; // iterator to next node
			}
			currentNode->isEnd = true; // new node is the end of the trie once we insert
		}

		~Trie() {
		}

		void free(Trie* node) { // delete trie
			for (int i = 0; i < 27; i++) {
				if (node != nullptr) {
					free(node->c[i]); // recursive function to reach all nodes below
				}
				else
					delete node; // delete the node
			}	
		}

		bool searchWord(Trie* node, std::string word) { // search for a word from a given root
			for (int i = 0; i < word.size(); i++) {
				int wordInd;
				if (word[i] == 39) {
					wordInd = 26; // apostrophe case
				}
				else {
					wordInd = word[i] - 'a'; // letter case
				}
				if (node->c[wordInd] == nullptr)
					return false; // no match
				else
					node = node->c[wordInd];
			}
			if (node != nullptr && node->isEnd)
				return true; // found the word 
			else
				return false; // did not find the word
		}
	};
	Trie* root = new Trie(); // root node
};

#endif  // STUDENTSPELLCHECK_H_
