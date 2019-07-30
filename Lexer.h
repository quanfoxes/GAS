#ifndef _LEXER_H_
#define _LEXER_H_

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "parser.h"
using namespace std;
vector<string> keyWords;
string codbuffer1 = "global _start\n_start:\ncall main\nmov eax, 1\nmov ebx, 0\nint 80h\n\n[section .code]\n";
string varbuffer1 = "\n\n[section .data]\n";
string texbuffer1 = "\n\n[section .text]\n";

void initializeKeyWords()
{
    keyWords.push_back("var");
    keyWords.push_back("run");
    keyWords.push_back("func");
    keyWords.push_back("%");
    keyWords.push_back("ret");
    keyWords.push_back("+");
    keyWords.push_back("-");
    keyWords.push_back("/");
    keyWords.push_back("*");
    keyWords.push_back("if");
    keyWords.push_back("else");

}
string readFile(string name)
{
    string line;
    ifstream file(name, ios::ate | ios::binary);
    int length = file.tellg();
    file.seekg(0, file.beg);

    char* buffer = new char[length];
    memset(buffer, 0, length);

    if (file.is_open()) 
    {
        file.read(buffer, length);
    }
    file.close();
    return string(buffer);
}

int getWord(char end, string &destination, string source, int continu)
{
    for (int i = continu; i < source.size();i++)
    {
        if (source[i] != end && source[i] != '\n')
        {
            destination += source[i];
        }
        else
        {
            i++;
            return i;
        }
        
    }

    return source.size();
}

int getReversedIndex(char end, string source, int continu)
{
    int i = continu;
    while (i > 0)
    {
        if (source[i] == end || source[i] == '\n')
        {
            i--;
        }
        else
        {
            return i;
        }
        
    }
    return 0;
}
int getReversedWord(char end,string &destination, string source, int continu)
{
    int i = continu;
    for (;i > 0;i--)
    {
        if (source[i] != end && source[i] != '\n')
        {
            destination += source[i];
        }
        else
        {
            return i;
        }
        
    }
    return 0;
}

void writeFile(string name, string buffer)
{
    ofstream file(name);

    if (file.is_open()) {
        file.write(buffer.c_str(), buffer.length());
    }
    
    file.close();
}

void lexer(string file, string &outbuffer)
{
    string destination;
    int continu = 0;
    for (int i = 0; i < file.size(); i++)
    {
        destination = "";
        continu = getWord(' ', destination, file, continu);

        if (i != file.size()) 
        {
            parser(destination, file, continu, varbuffer1, codbuffer1, texbuffer1);
            destination = "";
        }
    }
    outbuffer = texbuffer1 + codbuffer1 + varbuffer1;
}

#endif