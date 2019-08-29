#ifndef _PARSER_H_
#define _PARSER_H_
#include <string>
#include <vector>
#include <map>
#include <ctype.h>
#include <algorithm>
#include "Token.h"
using namespace std;
vector <Token> Tokens;
string parameters;
string varbuffer;
string bssbuffer;
string codbuffer;
string texbuffer;
string includes2;
int usedregister = 0;
int freeMemReg = 0;
string regbuffer;
int inLayer = 0;
int layerId = 0;
int returnLayer = 0;
int savedIfToElse = 0;
int savedElseToEnd = 0;
vector<string> FunctionNames;  //
vector<string> ifToElse;
vector<string> elseToEnd;
vector<string> jumpToEnd;
bool skippedRet = false;
vector<string> className;  //className . functionName:

bool hasFunctionStackFrame = false;
int framesAmount = 0;
bool isElse = false;

string returningDestName;
string paraAmount;

extern int getError(char, string&, string, int, string&);
extern int getWord(char, string&, string, int);
extern int getReversedIndex(char, string, int);
extern int getReversedWord(char, string&, string, int);
extern string readFile(string name);

string sx()
{
    string spaces = "";
    for (int i = 0; i < inLayer; i++)
    {
        spaces += " ";
    }
    return spaces;
}

int getIndex(string name)
{
    int secondPriority = 0;
    for (int i = 0; i < Tokens.size(); i++)
    {
        if (Tokens.at(i).Name == name)
        {
            if (Tokens.at(i).ifGlobal == true)
            {
                secondPriority = i;
            }
            else if (Tokens.at(i).FunctionLabelName != " " && FunctionNames.back() != " " && Tokens.at(i).owner == FunctionNames.back())
            {
                return i;
            }
        }
    }
    return secondPriority;
    cout << name + "doesnt exist!\n";
}

void disconnectFromRegister(string reg)
{
    for (auto i = Tokens.rbegin(); i != Tokens.rend(); )
    {
        if (i->Reg == reg)
        {
            i->eraseReg();
        }
        else
        {
            i++;
        }
    }
}

string autoValue(string normalSize, string multiplyer, int layer)
{
    if (multiplyer.size() > 0)
    {
        return normalSize + "*" + multiplyer + "*" + to_string(layer);
    }
    else
    {
        return normalSize;
    }
    
}

void getFreeReg()
{
    if (usedregister == 3)
    {
        regbuffer = "edx ";
        usedregister = 0;
    }
    else if (usedregister == 2)
    {
        regbuffer = "ecx ";
        usedregister = 3;
    }
    else if (usedregister == 1)
    {
        regbuffer = "ebx ";
        usedregister = 2;
    }
    else if (usedregister == 0)
    {
        regbuffer = "eax ";
        usedregister = 1;
    }
}

string getFreeMemReg()
{
    if (freeMemReg == 0)
    {
        freeMemReg++;
        return "esi ";
    }
    else
    {
        freeMemReg--;
        return "edi ";
    }
}

string autoName(string name, bool isString = false)
{   
    if (Tokens.size() == 0)
    {
        cout << "no Variables exist\n";
        return "";
    }
    else
    {
        return Tokens.at(getIndex(name)).getFullName();
    }
    
}

void makeVar(int &index)
{
    string name;
    string setting;
    string value;
    index = getWord(' ', name, parameters, index);  // name
    index = getWord(' ', setting, parameters, index);  // = or :
    index = getWord(' ', value, parameters, index);  // value or size
    Token Variable;
    Token Size;
    Variable.makeName(name);
    // check if it is a local var.
    if (FunctionNames.back() == " ")
    {
        //if it is global var.
        Variable.makePublic();
    }
    else
    {
        //if it is public.
        Variable.makePrivate(FunctionNames.back(), className.back());
        Variable.owner = FunctionNames.back();
    }

    if (setting == ":")
    {
        Variable.makeArray(value);
        Size.makeVar();
        Size.makeName(name + ".size");

        varbuffer += Size.getFullName() + " dd " + value + "\n";
        bssbuffer += Variable.getFullName() + " resd " + value + "\n";

        Tokens.push_back(Size);
    }
    else
    {
        Variable.makeVar();
        varbuffer += Variable.getFullName() + " dd " + value + "\n";
    }
    
    Tokens.push_back(Variable);
}

void prepareFunction(int &index, string func)
{
    int funcIndex = getIndex(func);
    string parameter;
    vector<string> Params;
    for (int i = 0; i < Tokens.at(funcIndex).ParameterAmount; i++)
    {
        parameter = "";
        index = getWord(' ', parameter, parameters, index);
        Params.push_back(parameter);
    }
    for (int i = 0; 0 < Params.size(); i++)
    {
        parameter = Params.back();
        Params.pop_back();
        if (parameter.at(0) == '%')
        {
            int parIndex = getIndex(parameter);
            parameter.erase(parameter.begin());
            codbuffer += "push " + Tokens.at(parIndex).getFullName() + "\n";
        }
        else
        {
            int parIndex = getIndex(parameter);
            codbuffer += "push dword [" + Tokens.at(parIndex).getFullName() + "]\n";
        }
    }
    if (Tokens.at(funcIndex).ifFunction)
    {
        codbuffer += "call " + Tokens.at(funcIndex).getFullName() + "\n";
    }
    else if (Tokens.at(funcIndex).ifMacro)
    { 
        codbuffer += Tokens.at(funcIndex).getFullName() + "\n";
    }
    else
    {
        cout << "uknown Function type :c\n";
    }
    
}

void makeInitialDestiantion(int &index, string dest)
{
    int destIndex = getIndex(dest);
    if (dest.at(0) == '%')
    {
        dest.erase(dest.begin());
        int destIndex2 = getIndex(dest);
        codbuffer += "push dword [" + Tokens.at(destIndex2).getFullName() + "]\n";
    }
    else if (Tokens.at(destIndex).ifVar)
    {
        codbuffer += "push " + Tokens.at(destIndex).getFullName() + "\n";
    }
    else if (Tokens.at(destIndex).ifArray)
    {
        // get the ":" and the "index".
        string skip;
        string arrayIndex;
        index = getWord(' ', skip, parameters, index);
        index = getWord(' ', arrayIndex, parameters, index);
        //mov esi, [arrayIndex]
        //lea esi, dest[esi * 4]
        int indexIndex = getIndex(arrayIndex);
        string memReg = getFreeMemReg();
        codbuffer += "mov " + memReg + ", dword [" + Tokens.at(indexIndex).getFullName() + "]\n";
        codbuffer += "lea " + memReg + ", " + Tokens.at(destIndex).getFullName() + "[ " + memReg + "* 4]\n";
        codbuffer += "push " + memReg + "\n";
    }
    else if (Tokens.at(destIndex).ifFunction)
    {
        // get the parameter's for the function.
        prepareFunction(index, dest);
    }
    else
    {
        cout << "bad destination: " + dest + "\n";
    }
}

void getInitalDestination(int &index, string destReg)
{
    string memReg = getFreeMemReg();
    codbuffer += "pop " + memReg + "\n";
    codbuffer += "mov [" + memReg + "], " + destReg + "\n";
}

void callFunction(string function, int &index)
{
    vector<string> params;
    int funcIndex = getIndex(function);
    for (int i = 0; i < Tokens.at(funcIndex).ParameterAmount; i++)
    {
        string parameter;
        index = getWord(' ', parameter, parameters, index);
        params.push_back(parameter);
    }
    for (int i = 0; i < Tokens.at(funcIndex).ParameterAmount; i++)
    {
        makeInitialDestiantion(index, params.back());
        params.pop_back();
    }
    if (Tokens.at(funcIndex).ifFunction)
    {
        codbuffer += "call " + Tokens.at(funcIndex).getFullName() + "\n";
    }
    else if (Tokens.at(funcIndex).ifMacro)
    { 
        codbuffer += Tokens.at(funcIndex).getFullName() + "\n";
    }
    else
    {
        cout << "uknown Function type :c\n";
    }
}

string getReturn()
{
    int node = getIndex("return");
    if (Tokens.at(node).ifVar)
    {
        return Tokens.at(node).Name;
    }
    else
    {
        getFreeReg();
        string returnreg = regbuffer;
        codbuffer += sx() + "mov " + returnreg + ", dword [return]\n";
        Tokens.at(node).Reg = returnreg;
        return returnreg;
    }
}

void doReturn()
{
    if (isElse)
    {
        codbuffer += elseToEnd.back();
        elseToEnd.pop_back();
        inLayer--;
        isElse = false;
    }
    codbuffer += sx() + "mov esp, ebp\n" + sx() + "pop ebp\n";
    if (framesAmount == 1)
    {
        codbuffer += sx() +  "ret\n\n";
        FunctionNames.pop_back();
    }
    framesAmount--;
}

void doMath(int &index, string a, string math)
{
    string b;
    index = getWord(' ', b, parameters, index);
    //a +/*- b
    int aI = getIndex(a);
    int bI = getIndex(b);
    string opCode;
    if (math == "+")
    {
        opCode = "add ";
    }
    else if (math == "-")
    {
        opCode = "sub ";
    }
    else if (math == "/")
    {
        opCode = "idiv ";
    }
    else
    {
        opCode = "imul ";
    }
    if (Tokens.at(bI).ifFunction)
    {
        prepareFunction(index, b);
    }
    codbuffer += opCode + Tokens.at(aI).getReg(codbuffer) + ", " + Tokens.at(bI).getReg(codbuffer) + "\n\n";
    //check if there is more math to do.
    math = "";
    int offset = getWord(' ', math, parameters, index);
    if (math == "\n" || math == " " || math == "ret")
    {
        return;
    }
    else if (math == "+" || math == "-" || math == "/" || math == "*")
    {
        //this means that math exist on this same line of code :D.
        //so lets make it.
        index = offset;
        doMath(index, a, math);
    }
}

void useVar(int &index, string destination)
{
    //save the destination to stack.
    makeInitialDestiantion(index, destination);
    //skip the = mark.
    string skip;
    index = getWord(' ', skip, parameters, index);
    //start the math check.
    string bPart;
    index = getWord(' ', bPart, parameters, index);
    string math;
    int offset = getWord(' ', math, parameters, index);
    if (math == "+" || math == "-" || math == "/" || math == "*")
    {
        //this means that math exist on this same line of code :D.
        //so lets make it.
        index = offset;
        doMath(index, bPart, math);
    }

    // check if B part is a function
    int bIndex = getIndex(bPart);
    if (Tokens.at(bIndex).ifFunction)
    {
        prepareFunction(index, bPart);
    }

    //load the inital destination from stack and give it the inital sum.
    getInitalDestination(index, Tokens.at(bIndex).getReg(codbuffer));
}

void makeFunc(int &index)
{
    string para1;
    index = getWord(' ', para1, parameters, index);
    codbuffer += sx() + className.back() +  para1 + ":\n";
    Token func;
    func.makeFunc(para1);
    if (className.back() != " ")
    {
        func.makePrivate("", className.back());
    }
    else
    {
        func.makePublic();
    }
    
    returnLayer++;
    vector<string> paraOrder;
    string reg1 = regbuffer;
    int i = 0;
    while (true)
    {
        string para2;
        string error;
        int offset = getError(' ', para2, parameters, index, error);
        Token Parameter;
        Parameter.makeName(para2);
        Parameter.makePrivate(para1, className.back());
        if (para2.length() > 0) 
        {
            if (para2.find('%')!= string::npos)
            {
                Parameter.makePtr();
            }
            else
            {
                Parameter.makeVar();
            }
        }
        if (error == "\n")
        {
            index = offset;
            if (para2 != "(")
            {
                func.makeLink(Parameter);
            }
            break;
        }
        else
        {
            func.makeLink(Parameter);
            index = offset;
        }
        i++;
    }

    codbuffer += sx() + "push ebp\n" + sx() + "mov ebp, esp\n";
    if (func.ParameterAmount > 0)
    {
        codbuffer += sx() + "sub esp, " + to_string(func.ParameterAmount) + "\n";
    }
    hasFunctionStackFrame = true;
    FunctionNames.push_back(para1);
    Tokens.push_back(func);

    for (int i = 0; i < func.ParameterAmount; i++)
    {
        getFreeReg();
        string reg2 = regbuffer;
        string para3 = func.Links.at(i).Name;
        if (func.Links.at(i).ifPointer)
        {
            para3.erase(para3.begin());
        }

        Token child;
        child.makeName(para3);
        child.ifChild = true;
        child.owner = para1;
        if (func.Links.at(i).ifPointer)
        {
            string memreg = getFreeMemReg();
            codbuffer += ";" + para3 + " is now Pointer.\n";
            codbuffer += sx() + "mov " + memreg + ", [ebp + " + to_string(4 * i + 8) + "]\n";
            codbuffer += sx() + "lea " + memreg + ", [" + memreg + "]\n";
            child.makePtr();
        }
        else
        {
            codbuffer += ";" + para3 + " is now Variable.\n";
            codbuffer += sx() + "mov " + reg2 + ", [ebp+" + to_string(4 * i + 8) + "]\n";
            child.makeVar();
        }

        varbuffer += className.back() + para1 + "." + para3 + " dd 0\n";

        codbuffer += sx() + "mov [" + className.back() + para1 + "." + para3 + "], " + reg2 + "\n";
        child.makePrivate(para1, className.back());
        Tokens.push_back(child);
    }
    paraAmount = to_string(func.ParameterAmount * 4);
}

string getJump(string condition)
{
    if (condition == "==")
    {
        return "jne ";
    }
    if (condition == "!=")
    {
        return "je ";
    }
    if (condition == ">")
    {
        return "jle ";
    }
    if (condition == "<")
    {
        return "jge ";
    }
    if (condition == "!>" || condition == ">!")
    {
        return "jg ";
    }
    if (condition == "!<" || condition == "<!")
    {
        return "jl ";
    }
    if (condition == "=>" || condition == ">=")
    {
        return "jl ";
    }
    if (condition == "=<" || condition == "<=")
    {
        return "jg ";
    }
    else
    {
        return "jmp ";
    }
}

void doComparing(int &i)
{
    string a;  //a
    string condition;  // ==
    string b;  //b
    i = getWord(' ', a, parameters, i);
    i = getWord(' ', condition, parameters, i);
    i = getWord(' ', b, parameters, i);
    condition = getJump(condition);
    int aI = getIndex(a);
    int bI = getIndex(b);

    codbuffer += sx() +   "cmp " + Tokens.at(aI).getReg(codbuffer) + ", " + Tokens.at(bI).getReg(codbuffer) + "\n";
    
    inLayer++;
    layerId++;
    codbuffer += sx() + condition + "else" + to_string(inLayer) + to_string(layerId) + "\n";
    ifToElse.push_back(sx() + "else" + to_string(inLayer) + to_string(layerId) + ": \n");
    elseToEnd.push_back(sx() + "end" + to_string(inLayer) + to_string(layerId) + ": \n");
    jumpToEnd.push_back(sx() + "jmp end" + to_string(inLayer) + to_string(layerId) + "\n");
    savedIfToElse++;
    savedElseToEnd++;
}

void doElse()
{
    savedIfToElse--;
    savedElseToEnd--;
    codbuffer += jumpToEnd.back();
    codbuffer += ifToElse.back();
    jumpToEnd.pop_back();
    ifToElse.pop_back();
    isElse = true;
}

bool replace(string& str, const string& from, const string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void doInclude(int &i)
{
    int offset;
    string including;
    //string name;
    i -= 4;
    i = getWord('\n', including, parameters, i);
    replace(parameters, including, "");
    replace(including, "use ", "");
    includes2 = readFile(including);
    i = 0;
}

void makeNewType(int &index)
{
    string typeName;
    index = getWord(' ', typeName, parameters, index);
    Token type;
    type.makeType(typeName);
    className.push_back(typeName + ".");
    varbuffer += "\n" + typeName + ":\n";
}

void endType()
{
    className.pop_back();
    varbuffer += "\n\n";
}

void makeNew(int &index)
{
  //
}

void doInterruption(int &index)
{
    string eax;
    string ebx;
    string ecx;
    string edx;
    string callingnumber;
    string carry;
    index = getWord(' ', eax, parameters, index);
    index = getWord(' ', ebx, parameters, index);
    index = getWord(' ', ecx, parameters, index);
    index = getWord(' ', edx, parameters, index);
    index = getWord(' ', callingnumber, parameters, index);
    index = getWord(' ', carry, parameters, index);
    int aI = getIndex(eax);
    int bI = getIndex(ebx);
    int cI = getIndex(ecx);
    int dI = getIndex(edx);
    codbuffer += "push eax\n";
    if (isdigit(eax.at(0)) || eax.at(0) == '-')
    {
        codbuffer += "mov eax, " + eax + "\n";
    }
    else
    {
        codbuffer += "mov eax, " + Tokens.at(aI).getFullName() + "\n";
    }
    if (isdigit(ebx.at(0)) || ebx.at(0) == '-')
    {
        codbuffer += "mov ebx, " + ebx + "\n";
    }
    else
    {
        codbuffer += "mov ebx, " + Tokens.at(bI).getFullName() + "\n";
    }
    if (isdigit(ecx.at(0)) || ecx.at(0) == '-')
    {
        codbuffer += "mov ecx, " + ecx + "\n";
    }
    else
    {
        codbuffer += "mov ecx, " + Tokens.at(cI).getFullName() + "\n";
    }
    if (isdigit(edx.at(0)) || edx.at(0) == '-')
    {
        codbuffer += "mov edx, " + edx + "\n";
    }
    else
    {
        codbuffer += "mov edx, " + Tokens.at(dI).getFullName() + "\n";
    }
    
    codbuffer += "int " + callingnumber + "\n";
    codbuffer += "mov [" + carry + "], eax\n";
    codbuffer += "pop eax\n";
}

void makeNewString(int &index)
{
    string name;
    string is;
    string str;
    index = getWord(' ', name, parameters, index);
    index = getWord(' ', is, parameters, index);
    index = getWord('"', str, parameters, index);
    index = getWord('"', str, parameters, index);
}

void useStr(int &index, string destination)
{
    string command;
    string firstStr;

    index = getWord(' ', command, parameters, index);
    index = getWord(' ', firstStr, parameters, index);

    if (command == "=")
    {
        codbuffer += "lea edi, [" + autoName(destination, true) + "]\n";
        codbuffer += "lea esi, [" + autoName(firstStr, true) + "]\n";

        codbuffer += "push ecx\n";
        codbuffer += "mov ecx, [" + autoName(firstStr, true) + ".size]\n";
        codbuffer += "repz movsb \n";
    }
    else
    {
        // if this occurs it mean it's time for some functions baby!
        
    }
}

void makeNewMem(int &index)
{
    // mem abc : 123
    string skip;
    //get the name.
    string newAllocatedMemName;
    index = getWord(' ', newAllocatedMemName, parameters, index);
    //skip the : mark.
    index = getWord(' ', skip, parameters, index);
    //get the memory size to allocate on.
    string newAllocatedSize;
    index = getWord(' ', newAllocatedSize, parameters, index);
    codbuffer += newAllocatedMemName + ":\n";
    for (int i = 0; i < atoi(newAllocatedSize.c_str()); i++)
    {
        codbuffer += "nop\n";
    }
    Token newMem;
    newMem.makeName(newAllocatedMemName);
    newMem.makeArray(newAllocatedSize);
    newMem.makePublic();
    Tokens.push_back(newMem);
}

void returnValue(int &index)
{
    string dest;
    index = getWord(' ', dest, parameters, index);
    int destIndex = getIndex(dest);
    codbuffer += sx() + "mov esp, ebp\n" + sx() + "pop ebp\n";
    codbuffer += "pop eax\n";
    codbuffer += "add esp, " + paraAmount + "\n";
    codbuffer += "push dword [" + Tokens.at(destIndex).getFullName() + "]\n";
    codbuffer += "jmp eax\n\n";
}

void parser(string destination, string &file, int &continu, string &varbuffer1, string &codbuffer1, string &texbuffer1, string &includes1, string &bssbuffer1)
{
    codbuffer = "";
    texbuffer = "";
    varbuffer = "";
    bssbuffer = "";
    parameters = file;
    int dest = getIndex(destination);
    if (destination == "return")
    {
        returnValue(continu);
    }
    else if (destination == "var")
    {
        makeVar(continu);
    }
    else if (destination == "type")
    {
        makeNewType(continu);
    }
    else if (destination == ";")
    {
        endType();
    }
    else if (destination == "func")
    {
        makeFunc(continu);
    }
    else if (destination == "*")
    {
        disconnectFromRegister("eax ");
    }
    else if (destination == "/")
    {
        disconnectFromRegister("eax ");
    }
    else if (destination == ")")
    {
        doReturn();
    }
    else if (destination == "(")
    {
        if (hasFunctionStackFrame)
        {
            hasFunctionStackFrame = false;
        }
        else
        {
            codbuffer += sx() + "push ebp\n" + sx() + "mov ebp, esp\n";
        }
        framesAmount++;
    }
    else if (destination == "if")
    {
        doComparing(continu);
    }
    else if (destination == "else")
    {
        doElse();
    }
    else if (destination == "use")
    {
        doInclude(continu);
    }
    else if (destination == "new")
    {
        makeNew(continu);
    }
    else if (destination == "sys")
    {
        doInterruption(continu);
    }
    else if (destination == "str")
    {
        makeNewString(continu);
    }
    else if (destination == "mem")
    {
        makeNewMem(continu);
    }
    else if (Tokens.at(dest).ifString)
    {
        useStr(continu, destination);
    }
    else if (Tokens.at(dest).ifVar || Tokens.at(dest).ifArray) 
    {
        useVar(continu, destination);
    }
    else if (Tokens.at(dest).ifFunction)
    {
        callFunction(destination, continu);
    }

    file = parameters;
    varbuffer1 += varbuffer;
    bssbuffer1 += bssbuffer;
    codbuffer1 += codbuffer;
    texbuffer1 += texbuffer;
    includes1 += includes2;
    includes2 = "";
}

#endif