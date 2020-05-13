//
// Created by student on 4/28/20.
//

#ifndef HW1_TIMEOUTCOMMAND_H
#define HW1_TIMEOUTCOMMAND_H

#include "Commands.h"
#include <map>

const int INVALID_TIMEOUT_COMMAND = 0;
using std::map;


class TimeOutCommand : public Command {
private:
    time_t timeout_cmd;
public:
    TimeOutCommand(vector<string> cmdVec, string& cmdStr, time_t timeout);
    virtual ~TimeOutCommand() = default;
    void execute() override;
    bool validCommand() override;
};

/*
 * ===================================================
 *                        TimeOutList
 *                  TimeOutList::TimeOutLstEntry
 * ===================================================
 */

/***
 * TimeOutList storage all timeout entry: process that get execute with timeout command.
 * The list make sure and save the priorty of every timeout command.
 *
 * >> Upadate and set alarm signal: in AddEntry function.
 */
 class TimeOutList {
 public:
     class TimeOutListEntry;

    class TimeOutListEntry {
    private:
        /*
         * ending_time : the absolute timeout_cmd to send signal
         */
        time_t ending_time;

        string originalCmd;
        pid_t pid;
    public:
        TimeOutListEntry(string originalCmd, time_t time, pid_t pid);
        TimeOutListEntry(const TimeOutListEntry&) = default;
        const string& getOriginalCmd() const;
        time_t getEndingTime() const;
        pid_t getPid() const;
    };

public:
    TimeOutList() = default;
    ~TimeOutList() = default;

    /*
     * Add new TO Entry & UPDATE ALARM SIGNAL
     */
    void addTimeOutEntry(string originalCmd, time_t timeout, pid_t pid);

    /*
     * Remove Entry. we always remove the early alarm, and its happened after signal.
     */
    void removeTimeOutEntry();

    /*
     * Return the earliest Entry. when we get alarm, its for this timeout_cmd entry.
     */
    TimeOutListEntry* getNextTOC();

    /*
     * Update alaram syscall (use after alarmHandler)
     */
    void updateAlarm();

private:
    map<time_t,TimeOutListEntry> toMap;
};


#endif //HW1_TIMEOUTCOMMAND_H
