#include "TimeOutCommand.h"
#include "Smash.h"
#include <time.h>

/*
 * ===================================================
 *                      TIMEOUT COMMAND
 * ===================================================
 */
TimeOutCommand::TimeOutCommand(vector<string> cmdVec, string &cmdStr, time_t timeout) : Command(cmdVec,cmdStr, timeout) {
    if (TimeOutCommand::validCommand()) {
        this->timeout_cmd = _Not_ATOI_Stupid_Boy(cmdVec.at(1));
    } else {    // Invalid TimeOutCommand
        this->timeout_cmd = INVALID_TIMEOUT_COMMAND;
    }
}

bool TimeOutCommand::validCommand() {
    if (cmdVec.size() <= 1) {
        return false;
    }
    string number_str = this->cmdVec.at(1);
    return _Not_ATOI_Stupid_Boy(number_str) > 0 && cmdVec.size() > 2;    // In case it aint a num, NAN<0 is returned
}

void TimeOutCommand::execute() {
    if (this->timeout_cmd == INVALID_TIMEOUT_COMMAND) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }

    //SmallShell& smash = SmallShell::getInstance();
    //TimeOutList* tol = smash.getTOList();

    vector<string> exeCmd(this->cmdVec.begin()+2,this->cmdVec.end());
    Command* cmd = _createCommand(exeCmd,this->originalCmd,this->timeout_cmd);

    try {
        cmd->execute(); // Add to list: from execute for specific commnand.
    } catch (...) {
        //std::cerr << "Caught an exception" << endl; // TODO: excpection handling
    }

    delete(cmd);
}

/*
 * ===================================================
 *                      TIMEOUT LIST
 * ===================================================
 */
void TimeOutList::addTimeOutEntry(string originalCmd, time_t timeout, pid_t pid) {
    // Insert to list:
    TimeOutList::TimeOutListEntry toe(originalCmd, timeout, pid);
    time_t current_time_end = toe.getEndingTime();

    time_t early_time_end = (this->toMap.empty()) ? (current_time_end+1) : this->toMap.begin()->first;
    this->toMap.insert(std::pair<time_t,TimeOutListEntry>(current_time_end, toe));

    // Upadate Next Alarm:

    // set new alaram
    if (current_time_end < early_time_end) {
        alarm(timeout);
    }
}

void TimeOutList::removeTimeOutEntry() {
    if(this->toMap.empty()) {
        return;
    }
    this->toMap.erase(this->toMap.begin());
}

TimeOutList::TimeOutListEntry* TimeOutList::getNextTOC() {
    if (this->toMap.empty()) {
        return nullptr;
    }
    return &(this->toMap.begin()->second);
}

void TimeOutList::updateAlarm() {
    if (this->toMap.empty()) {
        return;
    }
    TimeOutListEntry toe = this->toMap.begin()->second;
    time_t current_time = time(NULL);

    // In case epsilon time between events
    time_t delta = toe.getEndingTime()-current_time;

    // make new alarm:
    if (delta>0) {
    alarm(delta);
    }
}

/*
 * ===================================================
 *                      TimeOutListEntry
 * ===================================================
 */

TimeOutList::TimeOutListEntry::TimeOutListEntry(string originalCmd, time_t t_o, pid_t pid) {
    this->originalCmd = originalCmd;
    this->ending_time = time(NULL) + t_o;
    this->pid = pid;
}

const string& TimeOutList::TimeOutListEntry::getOriginalCmd() const {
    return originalCmd;
}

time_t TimeOutList::TimeOutListEntry::getEndingTime() const {
    return ending_time;
}

pid_t TimeOutList::TimeOutListEntry::getPid() const {
    return pid;
}
