#include "Table.h"
#include <functional>
#include <cassert>
#include <iostream>
#include <algorithm>
using namespace std;


Table::Table(std::string keyColumn, const std::vector<std::string>& columns) {
	if (!validColumns(keyColumn, columns))
		valid = false; // keyColumn not in column vector
	m_bucket.resize(BUCKETS); // resize m_bucket vector to 997 (number of buckets)
	for (int i = 0; i < columns.size(); i++) {
		StringParser s(columns[i]); // string parser object for columns vector 
		string t;
		if (s.getNextField(t) && t == keyColumn) {
			keyCol = i; // find position in columns vector where the keyColumn is
			break;
		}
	}
	colSize = columns.size(); // keep track of column vector size
	for (int i = 0; i < colSize; i++) {
		colName.push_back(columns[i]); // push back columns into another vector for later
	}
}

bool Table::validColumns(std::string keyColumn, const std::vector<std::string>& columns) {
	if (columns.empty())
		return false; // columns vector is empty, so not valid
	bool t = false;
	for (int i = 0; i < columns.size(); i++) {
		if (columns[i] == keyColumn)
		{
			t = true; // found keyColumn in columns vector
			break; // break once found keyColumn
		}
	}
	if (!t)
		return false; // did not find keyColumn in columns vector
	
	for (int i = 0; i < columns.size(); i++) {
		int count = 0;
		string s = columns[i];
		for (int j = 0; j < columns.size(); j++) {
			string a = columns[j];
			if (s == a)
				count++; // count every duplicate
		}
		if (count > 1)
			return false; // not valid because there is a duplicate string in the vector
	}
	for (int i = 0; i < columns.size(); i++) {
		if (columns[i] == "")
			return false; // not valid if column name is empty string
	}
	return true; // column names are valid
}

bool stringToDouble(string s, double& d)
{
	// convert string to double
	char* end;
	d = std::strtof(s.c_str(), &end);
	return end == s.c_str() + s.size() && !s.empty();
}

Table::~Table() {

}

bool Table::good() const {
	if (valid)
		return true; // return true if table is valid
	else 
		return false; // table not valid
}
bool Table::insert(const std::string& recordString) {
	if (!good())
		return false; // not valid table

	// check for invalid number of columns
	StringParser t(recordString); // string parser object created
	string check;
	int numCol = 0;
	while (t.getNextField(check)) {
		numCol++; // increment number of columns if found
	}
	if (numCol != colSize)
		return false; // return false if incorrect number of columns

	// insert into table

	StringParser o(recordString); // string parser object for the string in parameter
	string s;
	int n = 0;
	while (n <= keyCol) {
		o.getNextField(s); // find key and store in string s
		n++;
	}

	// insert to bucket	
	int bucket = hashFunction(s);
	Entry x;
	x.m_data = recordString; // new entry object with the data that we will insert
	m_bucket[bucket].push_back(x); // push back record into linked list in designated bucket
	return true; // item inserted
}



void Table::find(std::string key, std::vector<std::vector<std::string>>& records) const {
	records.clear(); // clear records
	if (!good())
		return; // not valid table
	if (m_bucket.empty())
		return; // no records in hash table

	int bucket = hashFunction(key); // find bucket number for string key
	if (m_bucket[bucket].size() == 0)
		return; // no key found

	list<Entry> x = m_bucket[bucket];
	list<Entry>::iterator it = x.begin(); // iterator to traverse list
	while (it != x.end()) {
		Entry temp = *it; // variable to hold string data
		StringParser o(temp.m_data); // string parser object for the string in parameter
		string s;
		int n = 0;
		while (n <= keyCol) {
			o.getNextField(s); // find key and store in string s
			n++;
		}
		if (s == key) {
			string hold;
			string t;
			StringParser z(temp.m_data);
			if (records.size() == 0) {
				records.resize(1); // resize vector
				StringParser insert1(temp.m_data); // parse through string
				string ins;
				while (insert1.getNextField(ins)) {
					records[0].push_back(ins); // push back into records vector
				}
			}			
			else {
				records.resize(records.size() + 1); // resize records vector
				StringParser insert2(temp.m_data); // parse through string
				string ins;
				while (insert2.getNextField(ins)) {
					records[records.size() - 1].push_back(ins); // push back string data into records vector
				}
			}
		}
		it++; // imcrement iterator
	}
	return;
}

int Table::select(std::string query, std::vector<std::vector<std::string>>& records) const {
	records.clear(); // clear records vector
	if (!good())
		return -1; // not valid table
	StringParser token(query);
	string temp;
	int count=0;
	while (token.getNextField(temp))
		count++; // count number of tokens
	if (count != 3)
		return -1; // badly formed query - more than 3 tokens
	string col;
	string op;
	string value;
	StringParser sel(query);
	sel.getNextField(col); // find column name in string
	sel.getNextField(op); // find operator value in string
	sel.getNextField(value); // find value in string
	bool t = false;
	if (op.length() == 0)
		return -1; // badly formed query;
	for (int i = 0; i < colName.size(); i++) {
		if (colName[i] == col) { // check if column Name is valid
			t = true;
			break;
		}
	}
	if (!t)
		return -1; // badly formed query - column name does not match
	if (isalpha(op[0]))
	{
		for (int i = 0; i < op.length(); i++) {
			op[i] = toupper(op[i]); // turn operator characters uppercase for evaluation
		}
	}
	if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "!=" || op == "==" || op == "=" || op == "LT" || op == "LE" || op == "GT" || op == "GE" || op == "NE" || op == "EQ")
	{
	} // badly formed query - invalid operator
	else
		return -1;
	bool num = false;
	bool alpha = false;
	double check;
	if (!stringToDouble(value, check)) {
		if (op == "LT" || op == "LE" || op == "GT" || op == "GE" || op == "NE" || op == "EQ")
			return -1; // invalid query formed
	}
	for (int i = 0; i < value.length(); i++) {
		if (isdigit(value[i])) // check for numbers in value string
			num = true;
		if (isalpha(value[i])) // check for letters in value string
			alpha = true;
	}
	if (num && alpha)
		return -1; // badly formed query - value  has digits and letters 
	if (value == "" && (op == "GT" || op == "LT" || op == "GE" || op == "LE" || op == "NE" || op == "EQ"))
		return -1; // badly formed query - empty string is not number
	if (alpha && (op == "GT" || op == "LT" || op == "GE" || op == "LE" || op == "NE" || op == "EQ"))
		return -1; // cannot use number comparators for string of letters
	if (num) {
		for (int i = 0; i < value.length(); i++) {
			char s = value[i];
			if (isdigit(s) || s == '.') {} // check for digits or decimals
			else
				return -1; // badly formed queury if there
		}
	}
	int colNumber = 0;
	for (int i = 0; i < colName.size(); i++) {
		if (col == colName[i])
		{
			colNumber = i; // find column index that we want
			break;
		}
	}
	int result = searchOperate(colNumber, value, records, op); // push select values into record
	return result; // return 0 or how many improper values there are
}

int Table::searchOperate(int colNumber, std::string value, std::vector<std::vector<std::string>>& records, std::string op) const {
	int badRecord = 0; // variable to count number of records who value is not in proper form
	bool number = false;
	bool letter = false;
	for (int i = 0; i < value.length(); i++) {
		if (isdigit(value[i]))
			number = true; // check if value is number
		if (isalpha(value[i]))
			letter = true; // check if value composes of letters
	}
	for (int i = 0; i < m_bucket.size(); i++)
	{
		if (m_bucket[i].size() == 0)
			continue; // do not search empty buckets
		else {
			list<Entry> x = m_bucket[i];
			list<Entry>::iterator it = x.begin(); // iterator to traverse list
			while (it != x.end()) {
				Entry temp = *it; // get object by dereferencing
				string s = temp.m_data;
				string val;
				StringParser search(s); // string parser to search through iterator value in linked list
				int n = 0;
				while (n <= colNumber) {
					search.getNextField(val); // find column with value
					n++;
				}

				// perform comparisons
				bool numberS = false;
				bool letterS = false;
				bool other = false;
				for (int j = 0; j < val.length(); j++) {
					if (isdigit(val[j])) {
						numberS = true; // check if value is number
						continue;
					}

					if (isalpha(val[j])) {
						letterS = true; // check if value composes of letters
						continue;
					}

					else if (val[j] != '.' && val[0] != '-')
						other = true; // other character (not letter or number)
				}
				if (other) {
					if (op == "GT" || op == "LT" || op == "GE" || op == "LE" || op == "NE" || op == "EQ"){
					badRecord++; // bad record (using numeric comparator for non-numeric value
					it++; // iterate to next item in linked list
					continue; // continue from this loop
					}
				}
				if (letterS && numberS && (op == "GT" || op == "LT" || op == "GE" || op == "LE" || op == "NE" || op == "EQ")) { // value has both letters and numbers
					badRecord++; // increment badRecord count if there are letters and numbers in value
					it++; // next item in linked list
					continue;
				}
				if (number && val.length() == 0) {
					if (op == "GT" || op == "LT" || op == "GE" || op == "LE" || op == "NE" || op == "EQ") {
						badRecord++; // increment badRecord count if there are letters and numbers in value
						it++; // next item in linked list
						continue;
					}
				}
				if (val.length() == 0 && value.length() == 0) {
					if (op == "==" || op == "=" || op == "<=" || op == ">=") {
						recordPush(temp.m_data, records); // two empty strings are empty, so push onto records vector
						it++; // increment
						continue; // continue from loop
					}
				}
				if ((number && numberS) || (letterS && letter) || other || value == "" || val =="" || (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "!=" || op == "==" || op == "="))
				{
					if (op == "<" || op == "LT") {
						if (number && numberS && op == "LT") { // number comparisons
							double k ; // turn into double type
							double g ; // turn into double type
							if (stringToDouble(val, k)) {
								stringToDouble(value, g);
								if (k < g) // perform comparison
									recordPush(temp.m_data, records); // push into records vector using function
							}
							else
							{
								badRecord++; // invalid number
								it++; // next item in linked list
								continue;
							}
						}
						else if (op == "<" && val < value) { // string comparison
							recordPush(temp.m_data, records);
						}
					}
					if (op == ">" || op == "GT") {
						if (number && numberS && op == "GT") { // number comparisons
							double k; // turn into double type
							double g; // turn into double type
							if (stringToDouble(val, k)) {
								stringToDouble(value, g);
								if (k > g) // perform comparison
									recordPush(temp.m_data, records); // push into records vector using function
							}
							else
							{
								badRecord++; // invalid number
								it++; // next item in linked list
								continue;
							}
						}
						else if (op == ">" && val > value) { // string comparison
							recordPush(temp.m_data, records);
						}
					}
					if (op == "<=" || op == "LE") {
						if (number && numberS && op == "LE") { // number comparisons
							double k; // turn into double type
							double g; // turn into double type
							if (stringToDouble(val, k)) {
								stringToDouble(value, g);
								if (k <= g) // perform comparison
									recordPush(temp.m_data, records); // push into records vector using function
							}
							else
							{
								badRecord++; // invalid number
								it++; // next item in linked list
								continue;
							}
						}
						else if (op == "<=" && val <= value) { // string comparison
							recordPush(temp.m_data, records);
						}
					}
					if (op == ">=" || op == "GE") {
						if (number && numberS && op == "GE") { // number comparisons
							double k; // turn into double type
							double g; // turn into double type
							if (stringToDouble(val, k)) {
								stringToDouble(value, g);
								if (k >= g) // perform comparison
									recordPush(temp.m_data, records); // push into records vector using function
							}
							else
							{
								badRecord++; // invalid number
								it++; // next item in linked list
								continue;
							}
						}
						else if (op == ">=" && val >= value) { // string comparison
							recordPush(temp.m_data, records); // call function to push into records vector
						}
					}
					if (op == "!=" || op == "NE") {
						if (number && numberS && op == "NE") { // number comparisons
							double k; // turn into double type
							double g; // turn into double type
							if (stringToDouble(val, k)) {
								stringToDouble(value, g);
								if (k != g) // perform comparison
									recordPush(temp.m_data, records); // push into records vector using function
							}
							else
							{
								badRecord++; // invalid number
								it++; // next item in linked list
								continue;
							}
						}
						else if (op == "!=" && val != value) { // string comparison
							recordPush(temp.m_data, records);// call function to push into records vector
						}
					}
					if (op == "==" || op == "EQ" || op == "=") {
						if (number && numberS && op == "EQ") { // number comparisons
							double k; // turn into double type
							double g; // turn into double type
							if (stringToDouble(val, k)) {
								stringToDouble(value, g);
								if (k == g) // perform comparison
									recordPush(temp.m_data, records); // push into records vector using function
							}
							else
							{
								badRecord++; // invalid number
								it++; // next item in linked list
								continue;
							}
						}
						else if ((op == "==" || op == "=") && val == value) { // string comparison
							recordPush(temp.m_data, records); // call function to push into records vector
						}
					}
				}
				else
				{
					badRecord++; // increment number of values that are in improper form
				}
				it++; // increment
			}
		}
			
	}
	return badRecord; // return how many bad records there were (or 0 if there were none)
}

void Table::recordPush(std::string recordString, std::vector<std::vector<std::string>>& records) const {
	if (records.size() == 0) {
		records.resize(1); // resize vector if empty
		StringParser s(recordString); // string parser
		string temp;
		while (s.getNextField(temp)) {
			records[0].push_back(temp); // push back column values to the vector of records
		}
	}

	else {
		records.resize(records.size() + 1); // resize records vector
		StringParser s(recordString); // string parser
		string temp;
		while (s.getNextField(temp)) {
			records[records.size() - 1].push_back(temp); // push back string data into records vector
		}
	}
}

unsigned int Table::hashFunction(std::string s) const {
	hash<string> t; // hash function using library
	unsigned int value = t(s); // get the hash number based on the string 
	return value % BUCKETS;// return bucket number based on total number of buckets (997)
}

