#ifndef HW1_SMASH_H
#define HW1_SMASH_H

#include "Commands.h"
#include "TimeOutCommand.h"
#include <map>

using std::map;
using std::string;
using std::stack;


const pid_t SMASH_PID = getpid();   // Man page promises it is always successful
const string DEFAULT_PROMPT = "smash";
const int JOB_NOT_FOUND = -1;

/*
 * ===================================================
 *                        JobsList
 *                  JobsList::JobEntry
 * ===================================================
 */
class JobsList {
public:
    class JobEntry {
    private:
        int jobID;
        pid_t pid;
        time_t init;
        bool isStopped;
        string originalCmd;

    public:
        JobEntry(int jobID, pid_t pid, string originalCmd, bool isStopped = false);
        JobEntry(const JobEntry&) = default;
        string toStringToJobCommand();
        string toStringForFGBG();
        string toStringForQuit();
        int getJobID();
        pid_t getPID();
        bool getIsStopped();
        void setIsStopped(bool value);
        bool isPipe();
        void updateInit();
    };

    JobsList() = default;
    ~JobsList() = default;
    void addJob(pid_t pid, string originalCmd, bool isStopped = false);
    void printJobsList();
    bool isEmpty();

    // Removing finished jobs from jobsMap
    void removeJobByPID(pid_t pid);
    void removeJobByJobID(int jobId);
    int getJobIdbyPID(pid_t pid);

    void killAllJobs();

    JobEntry* getJobByJobID(int jobID);
    JobEntry* getLastJob();
    JobEntry* getLastStoppedJob();

private:
    map<int, JobEntry> jobsMap;
};


/*
 * ===================================================
 *                      SmallShell
 * ===================================================
 */


class SmallShell {

private:
    JobsList jobList;
    TimeOutList toList;
    string prompt;
    stack<string> hist;
    pid_t currentFGpid;
    bool piping;
    string currentCmdStr;
    SmallShell();

public:
    /*
     * Core functions
     */
    ~SmallShell() = default;
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() {          // make SmallShell singleton
        static SmallShell instance;             // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }
    void quit(bool killFlag);                   // Kills smash process, and kills child procs if killFlag==True

    /*
     * Getters & Setters
     */
    string getPrompt();
    void setPrompt(string newPrompt);
    void executeCommand(const char* cmd_line);
    bool isHistEmpty();
    string getLastDir();
    void setLastDir(string str);
    pid_t getFGpid();
    void setFGpid(pid_t pid);
    bool isPiping() const;
    void setPiping(bool piping);
    const string &getCurrentCmdStr() const;
    void setCurrentCmdStr(const string &currentCmdStr);

    /*
     * Jobs handling
     */
    void addJob(pid_t pid, string originalCmd, bool isStopped);
    void clearFinishedJobs();       // Catches zombies and clear them from jobsList
    void killAllJobs();
    JobsList* getJobsList();

    /*
     * TO handling
     */
    TimeOutList* getTOList();
    /*
     * Handle TOA: if timeout set (positive), then add.
     * Call this function every execute!
     */
    void HandleTimeOutAlarm(string& cmdStr, time_t timeout, pid_t pid);

};

#endif //HW1_SMASH_H
