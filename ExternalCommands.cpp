#include <sstream>
#include <iterator>
#include <cstring>
#include "ExternalCommands.h"
#include "Smash.h"

const string BASH = "/bin/bash";
const string BASH_FLAG = "-c";

ExternalCommand::ExternalCommand(vector<string> &cmdVec, string strCmd, time_t timeout) : Command(cmdVec,strCmd,timeout) {
    this->isBG = (isBackgroundCommand(cmdVec));
    cmdVec = chompBG(cmdVec);
    string cmdStr = vecToStr(cmdVec);
    char* cmdCharStr = new char[cmdStr.size()+1];
    strcpy(cmdCharStr, cmdStr.c_str());
    char* bash = new char[BASH.size()+1];
    strcpy(bash, BASH.c_str());
    char* bash_flag = new char[BASH_FLAG.size()+1];
    strcpy(bash_flag, BASH_FLAG.c_str());
    this->argv[0] = bash;
    this->argv[1] = bash_flag;
    this->argv[2] = cmdCharStr;
    this->argv[3] = NULL;
}


void ExternalCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    pid_t pid = fork();
    if (SyscallError(pid, "fork")) { return; }
    if (pid == 0) { // Child
        if (!smash.isPiping()) {    // In pipe we want to preserve same grpid
            setpgrp();
        }
        execv(argv[0], argv);
    } else {        // Parent
        smash.HandleTimeOutAlarm(this->originalCmd,this->timeout,pid);
        if (smash.isPiping()) { // We need to wait anyway in case of piping
            waitpid(pid, nullptr, 0);
        }
        else if (!this->isBG) {
            smash.setFGpid(pid);
            int status;
            waitpid(pid, &status, WUNTRACED);
            smash.setFGpid(SMASH_PID);
        } else {
            smash.addJob(pid, this->originalCmd, false);
        }
    }
}

ExternalCommand::~ExternalCommand() {
    for(int i = 0; i < 3; i++) {    // We don't need to delete argv[3]
        delete(this->argv[i]);
    }
}
