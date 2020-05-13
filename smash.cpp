#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Smash.h"
#include "signals.h"
#include <time.h>

using std::to_string;


/*
 * ===================================================
 *                  JobsList::JobEntry
 * ===================================================
 */
JobsList::JobEntry::JobEntry(int jobID, pid_t pid, string originalCmd, bool isStopped) {
    this->jobID = jobID;
    this->pid = pid;
    this->originalCmd = originalCmd;
    this->isStopped = isStopped;
    this->init = time(NULL);
}

string JobsList::JobEntry::toStringToJobCommand() {
    string str = '[' + to_string(this->jobID) + "] "
                + this->originalCmd + " : " + to_string(this->pid) + " "
                + to_string(int(difftime(time(NULL), this->init))) + " secs";
    if (this->isStopped) {
        str += " (stopped)";
    }
    return str;
}

string JobsList::JobEntry::toStringForFGBG() {
    return (this->originalCmd + " : " + to_string(this->pid)) ;
}

string JobsList::JobEntry::toStringForQuit() {
    return (to_string(this->pid) + ": " + this->originalCmd) ;
}

int JobsList::JobEntry::getJobID() {
    return this->jobID;
}

pid_t JobsList::JobEntry::getPID() {
    return this->pid;
}

bool JobsList::JobEntry::getIsStopped() {
    return this->isStopped;
}

void JobsList::JobEntry::setIsStopped(bool value) {
    this->isStopped = value;
}

bool JobsList::JobEntry::isPipe() {
    return this->originalCmd.find('|') != string::npos;     // Guaranteed to have only 1 pipe
}

void JobsList::JobEntry::updateInit() {
    time_t now = time(NULL);
    if (SyscallError(now, "time")) { return; }
    this->init = now;
}

/*
 * ===================================================
 *                      JobsList
 * ===================================================
 */
void JobsList::addJob(pid_t pid, string originalCmd, bool isStopped) {
    int newJobID = this->jobsMap.empty() ? 1 : this->jobsMap.rbegin()->first + 1 ;
    JobEntry je(newJobID,pid,originalCmd,isStopped);
    this->jobsMap.insert(std::pair<int,JobEntry>(newJobID,je));
}

void JobsList::printJobsList() {
    for (auto job : this->jobsMap) {
        cout << job.second.toStringToJobCommand() << endl;
    }
}

// Return nullptr if jobID does not exist
JobsList::JobEntry* JobsList::getJobByJobID(int jobID) {
    return (this->jobsMap.find(jobID) != this->jobsMap.end()) ? &(this->jobsMap.find(jobID))->second : nullptr;
}

// JobID = 0 is never assigned, therefor nullptr will be returned in case map is empty
JobsList::JobEntry* JobsList::getLastJob() {
    int lastJob = (this->jobsMap.rbegin() == this->jobsMap.rend()) ? 0 :  this->jobsMap.rbegin()->first;
    return getJobByJobID(lastJob);
}

// Return nullptr if no stopped jobs exists
JobsList::JobEntry* JobsList::getLastStoppedJob() {
    for (auto rit = jobsMap.rbegin(); rit != jobsMap.rend(); rit++) {
        if (rit->second.getIsStopped()) {
            return &(this->jobsMap.find(rit->first))->second;
        }
    }
    return nullptr;
}

bool JobsList::isEmpty() {
    return jobsMap.empty();
}

// Return JOB_NOT_FOUND if PID is not found
int JobsList::getJobIdbyPID(pid_t pid) {
    for (auto it : this->jobsMap) {
        if (it.second.getPID() == pid) {
            return it.first;
        }
    }
    return JOB_NOT_FOUND;
}

void JobsList::removeJobByJobID(int jobId){
    this->jobsMap.erase(jobId);
}

void JobsList::removeJobByPID(pid_t pid) {
    int jobId = getJobIdbyPID(pid);
    if (jobId != JOB_NOT_FOUND) {
        removeJobByJobID(jobId);
    }
}

void JobsList::killAllJobs() {
    cout << "smash: sending SIGKILL signal to " << jobsMap.size() << " jobs:" << endl;
    for (auto job: this->jobsMap) {
        cout << job.second.toStringForQuit() << endl;
        int kill_flag = job.second.isPipe() ? killpg(job.second.getPID(), SIGKILL) : kill(job.second.getPID(), SIGKILL);
        job.second.isPipe() ? SyscallError(kill_flag,"killpg") : SyscallError(kill_flag,"kill");
        removeJobByPID(job.second.getPID());
    }
}

/*
 * ===================================================
 *                      SmallShell
 * ===================================================
 */
SmallShell::SmallShell() {
    this->prompt = DEFAULT_PROMPT;
    this->piping = false;
    this->currentCmdStr = "";
    this->currentFGpid = SMASH_PID;
}

string SmallShell::getPrompt() {
    return this->prompt;
}

void SmallShell::setPrompt(string newPrompt) {
    this->prompt = newPrompt;
}

bool SmallShell::isHistEmpty() {
    return hist.empty();
}

// Checked before calling that this->hist is not empty
string SmallShell::getLastDir() {
    string str = this->hist.top();
    this->hist.pop();
    return str;
};


void SmallShell::setLastDir(string str) {
    this->hist.push(str);
};

void SmallShell::addJob(pid_t pid, string originalCmd, bool isStopped) {
    this->jobList.addJob(pid, originalCmd, isStopped);
}

JobsList * SmallShell::getJobsList() {
    return &(this->jobList);
}

pid_t SmallShell::getFGpid() {
    return this->currentFGpid;
}

void SmallShell::setFGpid(pid_t pid) {
    this->currentFGpid = pid;
}

bool SmallShell::isPiping() const {
    return this->piping;
}

void SmallShell::setPiping(bool piping) {
    this->piping = piping;
}

#include <sys/wait.h>
void SmallShell::clearFinishedJobs() {
    pid_t cpid = 1;
    while (cpid > 0) {  // cpid > 0 -> Caught something. cpid == 0 -> unchanged subproc. cpid < 0 -> no subprocs.
        int stat_loc;
        cpid = waitpid(-1, &stat_loc, WNOHANG | WUNTRACED);
        if (cpid > 0) {
            int sig = WSTOPSIG(stat_loc);
            if (sig == SIGSTOP || sig == SIGTSTP) { // In case SIGSTOP(19) or SIGTSTP(20) were sent via kill command, SHOULD NOT HAPPEN
                auto je = jobList.getJobByJobID(jobList.getJobIdbyPID(cpid));
                if (nullptr != je) je->setIsStopped(true);  // Should not be nullptr, but why not
            } else {
                this->jobList.removeJobByPID(cpid);
            }
            // cout << "Stat_loc of clearFinishedJobs: " << stat_loc << ", cause: " << sig << endl;
        }
    }
}

void SmallShell::quit(bool killFlag) {
    if (killFlag) {
        this->jobList.killAllJobs();
    }
    exit(0);
}

const string &SmallShell::getCurrentCmdStr() const {
    return currentCmdStr;
}

void SmallShell::setCurrentCmdStr(const string& currentCmdStr) {
    SmallShell::currentCmdStr = currentCmdStr;
}

TimeOutList* SmallShell::getTOList() {
    return &(this->toList);
}

void SmallShell::HandleTimeOutAlarm(string& cmdStr, time_t timeout, pid_t pid) {
    if(timeout > 0) {
        getTOList()->addTimeOutEntry(cmdStr,timeout,pid);
    }
}

