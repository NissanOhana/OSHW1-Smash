#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include "Smash.h"


void ctrlZHandler(int sig_num) {    // For CtrlZ -> SIGTSTP = kill -20
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-Z" << std::endl;
    int stop_flag = 0, jobID = 0;
    pid_t current_pid = smash.getFGpid();
    if (current_pid == SMASH_PID) {
        return;             // ignore cntrl_Z if smash if in the fg.
    }

    // STOP:
    if (smash.isPiping()) { // IF PIFE
        stop_flag = killpg(current_pid,SIGSTOP);
        if (SyscallError(stop_flag, "killpg")) {
            smash.setPiping(false);
            return;
        }
        smash.setFGpid(SMASH_PID);
    } else {
        stop_flag = kill(current_pid,SIGSTOP);
        if (SyscallError(stop_flag,"kill")) {
            return;
        }
        smash.setFGpid(SMASH_PID);
    }

    // Add process to job list. if its already there, than, stop it!
    jobID = smash.getJobsList()->getJobIdbyPID(current_pid);
    if (jobID == JOB_NOT_FOUND) {
        smash.getJobsList()->addJob(current_pid,smash.getCurrentCmdStr(),true); // Add new STOPPED job
    } else { // Job alreardy here!
        smash.getJobsList()->getJobByJobID(jobID)->setIsStopped(true);
        smash.getJobsList()->getJobByJobID(jobID)->updateInit();
    }
    std::cout << "smash: process "  << current_pid << " was stopped" << std::endl;
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash: got ctrl-C" << std::endl;
    int kill_flag = 0;
    pid_t current_pid = smash.getFGpid();
    if (current_pid == SMASH_PID) {
        return;             // ignore cntrl_C if smash if in the fg.
    }

    if (smash.isPiping()) { // IF PIFE
        kill_flag = killpg(current_pid,SIGKILL);
        if (SyscallError(kill_flag,"killpg")) {
            smash.setPiping(false);
            return;
        }
        smash.setFGpid(SMASH_PID);
    } else {                // NO PIPE
        kill_flag = kill(current_pid,SIGKILL);
        if (SyscallError(kill_flag,"kill")) {
            return;
        }
        smash.setFGpid(SMASH_PID);
    }

    std::cout << "smash: process "  << current_pid << " was killed" << std::endl;
    smash.getJobsList()->removeJobByPID(current_pid);
}

void alarmHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    smash.clearFinishedJobs();

    TimeOutList* tol = smash.getTOList();
    TimeOutList::TimeOutListEntry* toe = tol->getNextTOC(); // If we get SIGALRM, its the first in the list.
    if (toe == nullptr) {
        return;
    }

    bool youKilledMe = true;
    if (toe->getPid() != SMASH_PID) { //kill it, but if not builtin cmd - no fork - no sigkill.
        int kill_flag = kill(toe->getPid(),SIGKILL);
        if (kill_flag == -1 || errno == ESRCH) { // ot maybe use: if (errno == ESRCH) // the pid is already dead
            // SyscallError(kill_flag,"kill");
            youKilledMe = false; // I was already dead bitch
        }
    } else {
        youKilledMe = false; // BuiltinCommand > no fork > no need to print kill message.
    }
    cout << "smash: got an alarm" << endl;
    if (youKilledMe) {
        cout << "smash: " << toe->getOriginalCmd() << " timed out!" << endl;
    }

    tol->removeTimeOutEntry();
    tol->updateAlarm();
}
