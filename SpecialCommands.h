#ifndef HW1_SPECIALCOMMANDS_H
#define HW1_SPECIALCOMMANDS_H

#include "Commands.h"

const int buff_size = 4096;
typedef enum {VALID_PATH, SAME_PATH, PATH_FAIL,
                        COPY_SUCCESS, COPY_FAIL_ARGS, COPY_FAIL_SYSCALL} copy_e;

class CopyCommand : public Command {
private:
    bool isBG;
    char buffer[buff_size];
    string src;
    string dst;
public:
    CopyCommand(vector<string> cmdVec, string& cmdStr, time_t timeout);
    virtual ~CopyCommand() = default;
    void execute() override;
    copy_e copyFile();
    copy_e validArgs();
    copy_e copyValidCommand() ;
};


class RedirectionCommand : public Command {
    bool append;
    vector<string> srcCmd;
    string dest;
public:
    RedirectionCommand(vector<string>& cmdVec, string& cmdStr,time_t timeout);
    virtual ~RedirectionCommand() = default;
    void execute() override;
    bool validCommand();
    //void prepare() override;
    //void cleanup() override;
};

class PipeCommand : public Command {
    bool isBG;
    vector<string> cmd1;
    vector<string> cmd2;
    bool isRegPipe;         // True if 'cmd1 | cmd2', false if 'cmd1 |& cmd2'
public:
    PipeCommand(vector<string>& cmdVec, string& cmdStr,time_t timeout);
    void execSubCommands();
    virtual ~PipeCommand() = default;
    void execute() override;
};

#endif //HW1_SPECIALCOMMANDS_H
