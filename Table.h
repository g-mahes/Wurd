#ifndef TABLE_INCLUDED
#define TABLE_INCLUDED
#include <string>
#include <vector>
#include <list>
#include <ctype.h>
#include <stdio.h>
#include <cstdlib>

const int BUCKETS = 997;

class Table
{
public:
	Table(std::string keyColumn, const std::vector<std::string>& columns);
	~Table();
	bool good() const;
	bool insert(const std::string& recordString);
	void find(std::string key, std::vector<std::vector<std::string>>& records) const;
	int select(std::string query, std::vector<std::vector<std::string>>& records) const;

	// We prevent a Table object from being copied or assigned by
	// making the copy constructor and assignment operator unavailable.
	Table(const Table&) = delete;
	Table& operator=(const Table&) = delete;
private:
	struct Entry {
		std::string m_data;
	};
	unsigned int hashFunction(std::string s) const;
	std::vector<std::list<Entry>> m_bucket;
	bool valid = true;
    int keyCol;
    int colSize;
    bool validColumns(std::string keyColumn, const std::vector<std::string>& columns);
    std::vector<std::string> colName;
    int searchOperate(int colNumber, std::string value, std::vector<std::vector<std::string>>& records, std::string op) const;
    void recordPush(std::string recordString, std::vector<std::vector<std::string>>& records) const;
};


class StringParser
{
public:
    StringParser(std::string text = "")
    {
        setString(text);
    }

    void setString(std::string text)
    {
        m_text = text;
        m_start = 0;
    }

    bool getNextField(std::string& field);

private:
    std::string m_text;
    size_t m_start;
};

bool StringParser::getNextField(std::string& fieldText)
{
    m_start = m_text.find_first_not_of(" \t\r\n", m_start);
    if (m_start == std::string::npos)
    {
        m_start = m_text.size();
        fieldText = "";
        return false;
    }
    if (m_text[m_start] != '\'')
    {
        size_t end = m_text.find_first_of(" \t\r\n", m_start + 1);
        fieldText = m_text.substr(m_start, end - m_start);
        m_start = end;
        return true;
    }
    fieldText = "";
    for (;;)
    {
        m_start++;
        size_t end = m_text.find('\'', m_start);
        fieldText += m_text.substr(m_start, end - m_start);
        m_start = (end != std::string::npos ? end + 1 : m_text.size());
        if (m_start == m_text.size() || m_text[m_start] != '\'')
            break;
        fieldText += '\'';
    }
    return true;
}



#endif