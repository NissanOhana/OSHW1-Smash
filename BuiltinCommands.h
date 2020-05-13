#ifndef HW1_BUILTINCOMMANDS_H
#define HW1_BUILTINCOMMANDS_H

#include "Commands.h"
#include "Smash.h"
#include <string>
#include <vector>

using std::vector;
using std::string;

class BuiltInCommand : public Command {
public:
    BuiltInCommand(vector<string>& cmdVec, string& strCmd,time_t timeout);
    virtual ~BuiltInCommand() = default;
};

class ChangePrompt : public BuiltInCommand {
private:
    string newPrompt;
public:
    ChangePrompt(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~ChangePrompt() = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    ChangeDirCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~ChangeDirCommand() = default;
    void execute() override;
    bool validCommand() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~GetCurrDirCommand() = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~ShowPidCommand() = default;
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
private:
    bool killFlag;
public:
    QuitCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~QuitCommand() = default;
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~JobsCommand() = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
private:
    pid_t pid;
    int signal;
    int jobID;
    bool isPipe;

public:
    KillCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~KillCommand() = default;
    bool validCommand() override;
    void execute() override;

};

class ForegroundCommand : public BuiltInCommand {
private:
    JobsList::JobEntry* je;
    int jobID;
public:
    ForegroundCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~ForegroundCommand() = default;
    bool validCommand() override;
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
private:
    JobsList::JobEntry* jobToExecute;
    int jobID;

public:
    BackgroundCommand(vector<string>& cmdVec, string& strCmd, time_t timeout);
    virtual ~BackgroundCommand() = default;
    bool validCommand() override;
    void execute() override;
};



#endif