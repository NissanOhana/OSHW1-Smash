#include <expat.h>
#include "BuiltinCommands.h"
#include <algorithm>

using std::to_string;

const int MIN_SIGNAL = 1;
const int MAX_SIGNAL = 31;

BuiltInCommand::BuiltInCommand(vector<string>& cmdVec, string& strCmd, time_t timeout) : Command(cmdVec, strCmd, timeout) { }

/*
 * ===================================================
 *                      CHPROMPT
 * ===================================================
 */
ChangePrompt::ChangePrompt(vector<string>& cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {
    this->newPrompt = (cmdVec.size() == 1 || (cmdVec.size() > 1 && cmdVec.at(1) == "&"))
            ? DEFAULT_PROMPT : this->cmdVec.at(1); // According to @62 on piazza
}

void ChangePrompt::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.setPrompt(this->newPrompt);
}

/*
 * ===================================================
 *                      SHOWPID
 * ===================================================
 */
ShowPidCommand::ShowPidCommand(vector<string>& cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << SMASH_PID << endl;
}

/*
 * ===================================================
 *                      PWD
 * ===================================================
 */
GetCurrDirCommand::GetCurrDirCommand(vector<string>& cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {}

void GetCurrDirCommand::execute() {
    char* current = getcwd(nullptr, 0);
    cout << current << endl;
    free(current);
}

/*
 * ===================================================
 *                      CD
 * ===================================================
 */
ChangeDirCommand::ChangeDirCommand(vector<string>& cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {}

bool ChangeDirCommand::validCommand() {
    SmallShell& smash = SmallShell::getInstance();
    if (cmdVec.size() == 1 || (cmdVec.size() > 1 && cmdVec.at(1) == "&")) {
        return false;   // Print no error, @62
    } else if (cmdVec.size() > 2 && cmdVec.at(2) != "&") {
        cerr << "smash error: cd: too many arguments" << endl;
        return false;
    } else if (cmdVec.at(1) == "-" && smash.isHistEmpty()) {
        cerr << "smash error: cd: OLDPWD not set" << endl;
        return false;
    }
    return true;
}

void ChangeDirCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if (validCommand()) {
         string newDirStr;
         char* curDir = getcwd(nullptr, 0);

        if(cmdVec.at(1) == "-") {
            newDirStr = smash.getLastDir();
        } else {
            newDirStr = cmdVec.at(1);
        }
        const char* newDir = newDirStr.c_str();

        int result = chdir(newDir);
        if (SyscallError(result,"chdir")) { }
        else { // if (cmdVec.at(1) != "-") {  - According to @49, lastDir should be updated anytime
            smash.setLastDir(string(curDir));
        }
        free(curDir);
    }
}

/*
 * ===================================================
 *                      JOBS
 * ===================================================
 */
JobsCommand::JobsCommand(vector<string>& cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {}

void JobsCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.getJobsList()->printJobsList();
}

/*
 * ===================================================
 *                      FG
 * ===================================================
 */
bool ForegroundCommand::validCommand() {
    SmallShell& smash = SmallShell::getInstance();

    if ((cmdVec.size() == 1 || (cmdVec.size() > 1 && cmdVec.at(1) == "&")) && smash.getJobsList()->isEmpty()) {
        cerr << "smash error: fg: jobs list is empty" << endl;
        return false;
    }

    if ((cmdVec.size() == 2 || (cmdVec.size() > 2 && cmdVec.at(2) == "&")) && je == nullptr) {
        if (jobID == NAN) {
            cerr << "smash error: fg: invalid arguments" << endl;
        } else {  // Job does not exist
            cerr << "smash error: fg: job-id " << cmdVec.at(1) << " does not exist" << endl;
        }
        return false;
    }

    if (cmdVec.size() > 2 && cmdVec.at(2) != "&") { return _invalidArgs("fg"); }
    if (cmdVec.size() > 2 && nullptr == this->je) {   // bg X & <balbla>
        cerr << "smash error: fg: job-id " << cmdVec.at(1) << " does not exist" << endl;
        return false;
    }

    return true;
}

ForegroundCommand::ForegroundCommand(vector<string> &cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {
    SmallShell& smash = SmallShell::getInstance();

    if (cmdVec.size() == 1) {
        // last job
        this->je = smash.getJobsList()->getLastJob();
        this->jobID = NAN;
    } else {
        this->jobID = _Not_ATOI_Stupid_Boy(cmdVec.at(1));
        this->je = smash.getJobsList()->getJobByJobID(jobID);
    }
}

void ForegroundCommand::execute() {
    if (validCommand()) {
        cout << je->toStringForFGBG() << endl;
        this->je->setIsStopped(false);
        SmallShell& smash = SmallShell::getInstance();
        int cpid = je->getPID();

        smash.setPiping(je->isPipe());
        smash.setFGpid(cpid);

        int result = je->isPipe() ? killpg(this->je->getPID(),SIGCONT) : kill(this->je->getPID(),SIGCONT);
        if (!je->isPipe() && SyscallError(result, "kill")) { return; }
        if (je->isPipe() && SyscallError(result, "killpg")) { return; }

        int stat_loc;
        waitpid(cpid, &stat_loc, WUNTRACED);
        /*cout << "Waitpid done, pid: " << cpid
            << ", stat_loc: " << stat_loc
            << ", WIFCONTINUED: " << WIFCONTINUED(stat_loc)
            << ", WIFSTOPPED: " << WIFSTOPPED(stat_loc)
            << endl;*/
        if (WIFSTOPPED(stat_loc) == 0) {    // Remove from JobsList only if finished "naturally". @83 promises kill -SIGCONT X will NOT be tested
            smash.getJobsList()->removeJobByPID(cpid);
        }
        smash.setFGpid(SMASH_PID);
    }
}

/*
 * ===================================================
 *                      BG
 * ===================================================
 */
BackgroundCommand::BackgroundCommand(vector<string>& cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {
    JobsList* jl = SmallShell::getInstance().getJobsList();
    if (cmdVec.size() == 1 || (cmdVec.size() > 1 && cmdVec.at(1) == "&")) {
        this->jobToExecute = jl->getLastStoppedJob();
    } else {
        this->jobID = _Not_ATOI_Stupid_Boy(cmdVec.at(1));
        this->jobToExecute = jl->getJobByJobID(jobID);
    }
}

bool BackgroundCommand::validCommand() {
    if ((cmdVec.size() == 1 || (cmdVec.size() > 1 && cmdVec.at(1) == "&")) && nullptr == this->jobToExecute) {
        cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        return false;
    }
    if (cmdVec.size() >= 3 && cmdVec.at(2) != "&") { return _invalidArgs("bg"); }
    if (nullptr == this->jobToExecute) {
        if (jobID == NAN) {
            return _invalidArgs("bg");
        } else {  // Job does not exist
            cerr << "smash error: bg: job-id " << cmdVec.at(1) << " does not exist" << endl;
            return false;
        }
    }
    if (!this->jobToExecute->getIsStopped()) {    // Promised not to be nullptr - checked previously
        cerr << "smash error: bg: job-id " << cmdVec.at(1) << " is already running in the background" << endl;
        return false;
    }

    return true;
}

void BackgroundCommand::execute() {
    if (validCommand()) {   // If true than jobToExecuted is not nullptr
        cout << this->jobToExecute->toStringForFGBG() << endl;
        this->jobToExecute->setIsStopped(false);
        //jobToExecute->setInit(timeout_cmd(NULL));
        //SmallShell& smash = SmallShell::getInstance();
        int result = jobToExecute->isPipe() ?
                killpg(jobToExecute->getPID(), SIGCONT) : kill(jobToExecute->getPID(), SIGCONT);
        //if (SyscallError(result,"kill")) { }
        if (!jobToExecute->isPipe() && SyscallError(result, "kill")) {}
        if (jobToExecute->isPipe() && SyscallError(result, "killpg")) {}
    }
}

/*
 * ===================================================
 *                      KILL
 * ===================================================
 */
KillCommand::KillCommand(vector<string>& cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout){
    this->pid = -1;
    this->signal = -1;
    this->jobID = -1;
    this->isPipe = false;
}

bool KillCommand::validCommand() {
    SmallShell& smash = SmallShell::getInstance();
    JobsList* jl = smash.getJobsList();
    int size = cmdVec.size();

    if (size < 3) { return _invalidArgs("kill"); }   // Not enough arguments
    if (size >= 2 && cmdVec.at(1).at(0) != '-') { return _invalidArgs("kill"); }    // Not in right format
    if (size > 3 && cmdVec.at(3) != "&") { return _invalidArgs("kill"); }

    this->jobID = _Not_ATOI_Stupid_Boy(cmdVec.at(2));
    JobsList::JobEntry* je = jl->getJobByJobID(this->jobID);
    string signalStr = cmdVec.at(1).substr(1, signalStr.size()-1);
    int signal = _Not_ATOI_Stupid_Boy(signalStr);          // = NAN if no conversion available or negative

    if (signal == NAN) { return _invalidArgs("kill"); }

    if (nullptr == je) {    // In case job-id is negative - print it doesnot exist
        if (jobID == NAN) {
            return _invalidArgs("kill");
        } else {
            cerr << "smash error: kill: job-id " << jobID << " does not exist" << endl;
            return false;
        }
    }

    //if (signal < MIN_SIGNAL || signal > MAX_SIGNAL) { return _invalidArgs("kill"); }    // Invalid signal

    pid_t pid = je->getPID();
    this->pid = pid;
    this->signal = signal;
    this->isPipe = je->isPipe();
    return true;
}

void KillCommand::execute() {
    //SmallShell& smash = SmallShell::getInstance();
    //JobsList* jl = smash.getJobsList();
    if (validCommand()) {   // validCommand() initiate pid & signal at success

        int syscall = this->isPipe ? killpg(this->pid, this->signal) : kill(this->pid, this->signal);

        if ((this->isPipe && SyscallError(syscall,"killpg")) || (!this->isPipe && SyscallError(syscall,"kill"))) {
            return;
        } else {
            cout << "signal number " << to_string(this->signal) << " was sent to pid " << to_string(pid) << endl;
        }
    }
}

/*
 * ===================================================
 *                      QUIT
 * ===================================================
 */
QuitCommand::QuitCommand(vector<string> &cmdVec, string& strCmd, time_t timeout) : BuiltInCommand(cmdVec, strCmd,timeout) {
    this->killFlag = std::find(cmdVec.begin(), cmdVec.end(), "kill") != cmdVec.end();   // Any place is allowed according to @101
}

void QuitCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.quit(this->killFlag);
}