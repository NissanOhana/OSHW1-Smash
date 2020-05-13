#ifndef HW1_COMMAND_H_
#define HW1_COMMAND_H_

#include <vector>
#include <string>
#include <stack>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <climits>

using std::cout;
using std::cerr;
using std::endl;
using std::stack;
using std::vector;
using std::string;
using std::istringstream;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

typedef enum {CHPROMPT, SHOWPID, PWD, CD, JOBS, KILL, FG, BG, QUIT, EXTERNAL,
                                CP, PIPE, RED, OTHER, TIMEOUT} commands_e;
const time_t NO_TIMEOUT = 0;
const int NAN = INT_MIN+12374;   // Not A Number

class Command {

protected:
    vector<string> cmdVec;
    string originalCmd;

    /*
     * timeout can get positive value only if create command execute from timeout command.
     */
    time_t timeout;

public:
    Command(vector<string>& cmdVed, string& orgCmd, time_t timeout);
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual bool validCommand() { return true; };
    const string &getOriginalCmd() const;
    time_t getTimeout() const;

    //virtual void prepare();
    //virtual void cleanup();
 };

Command* _createCommand(vector<string>& cmdVec, string cmdStr, time_t timeout);
commands_e decodeSpecialCommand(vector<string>& cmdVec);
commands_e decodeCommand(string str);
bool SyscallError(int syscalRetValue, const char* syscall);
bool isBackgroundCommand(vector<string> cmd);
int _Not_ATOI_Stupid_Boy(string str);
bool _invalidArgs(const char* cmd);
string vecToStr(vector<string>& cmdVec);
vector<string> chompBG(vector<string>& cmdVec);


#endif //HW1_COMMAND_H_
