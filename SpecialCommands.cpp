#include "SpecialCommands.h"
#include "Smash.h"
#include <algorithm>
#include <fcntl.h>
#include <string.h>

const int STDIN     = 0;
const int STDOUT    = 1;
const int STDERR    = 2;


/*
 * ===================================================
 *                      CP
 * ===================================================
 */
CopyCommand::CopyCommand(vector<string> cmdVec, string &cmdStr, time_t timeout)  : Command(cmdVec, cmdStr,timeout ) {
    if(cmdVec.size() < 3) {
        return; // Junk Values: execute will deal with it.
    }
    this->isBG = (cmdVec.size() >= 4 && cmdVec.at(3) == "&");
    this->src = this->cmdVec.at(1);
    this->dst = this->cmdVec.at(2);
}

copy_e CopyCommand::validArgs() {
    char* src_c;
    char* dst_c;
    int rp = 0;

    src_c = realpath(this->src.c_str(), nullptr);
    rp = (src_c == NULL) ? -1 : 1;
    if (SyscallError(rp,"realpath")) {
        return PATH_FAIL;
    }

    dst_c = realpath(this->dst.c_str(), nullptr);
    if (errno == ENOENT) {
        return VALID_PATH;                // file does not exist. make new file.
    }
    rp = (dst_c == NULL) ? -1 : 0;
    if (SyscallError(rp,"realpath")) {
        return PATH_FAIL;
    }

    return strcmp(src_c,dst_c) == 0 ? SAME_PATH : VALID_PATH;
}

copy_e CopyCommand::copyValidCommand() {
    // ignore if more the 3 args.
    if (cmdVec.size() > 2) {
        int src_fd = 0, dst_fd = 0;
        src_fd = open(this->src.c_str(), O_RDONLY);
        if (SyscallError(src_fd,"open")) {
            return COPY_FAIL_SYSCALL;
        }
        close(src_fd);
        dst_fd = open(this->dst.c_str(), O_RDONLY | O_CREAT, 0666 );
        if (SyscallError(dst_fd,"open")) {
            return COPY_FAIL_SYSCALL;
        }
        close(dst_fd);
        return COPY_SUCCESS;
    } else {
        return COPY_FAIL_ARGS;
    }
}

copy_e CopyCommand::copyFile() {
    copy_e copyFlag = copyValidCommand();
    if (copyFlag != COPY_SUCCESS) {
        return copyFlag;
    }
    int src_fd = 0, dst_fd = 0, close_flag1 = 0, close_flag2 = 0;
    ssize_t read_bs = 0, write_bs = 0;

    // Open file to copy from
    src_fd = open(this->src.c_str(), O_RDONLY);
    if(SyscallError(src_fd,"open")) {
        return COPY_FAIL_SYSCALL;
    }
    copy_e valid_flag = validArgs();
    if (valid_flag == SAME_PATH) {
        close_flag1 = close(src_fd);
        if (SyscallError(close_flag1, "close")) {
            return COPY_FAIL_SYSCALL;
        }
        // SAME_PATH file - "virtual" copy
        return COPY_SUCCESS;
    }

    // Create (or override) file
    dst_fd = open(this->dst.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0666 );
    if(SyscallError(dst_fd,"open")) {
        close_flag2 = close(dst_fd);
        if (SyscallError(close_flag2,"close")) {
            return COPY_FAIL_SYSCALL;
        }
        return COPY_FAIL_SYSCALL;               // FAIL resons: malloc/permission
    }

    // Copy file:
    bool copied = true;
    while ((read_bs = read(src_fd, buffer, buff_size)) > 0 ) {
        write_bs = write(dst_fd,buffer,read_bs);
        if (write_bs != read_bs) { // write fail
            SyscallError(-1,"write");
            copied =  false;
            break;
        }
    }
    if (SyscallError(read_bs, "read")) { // fail to read
        copied = false;
    }

    // Close src & dst:
    close_flag1 = close(src_fd);
    close_flag2 = close(dst_fd);
    if (SyscallError(close_flag1,"close") || SyscallError(close_flag2,"close")) {
        return COPY_FAIL_SYSCALL;
    }

    return copied ? COPY_SUCCESS : COPY_FAIL_SYSCALL;
}

void CopyCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    pid_t pid = fork();
    if (SyscallError(pid, "fork")) { return; }
    if (pid == 0) { // Child
        setpgrp();
        copy_e copyFlag = copyFile();
        if (copyFlag == COPY_SUCCESS) {
            cout << "smash: " << cmdVec.at(1) << " was copied to " << cmdVec.at(2) << endl;
            exit(0);
        } else if (copyFlag == COPY_FAIL_ARGS) {
            _invalidArgs("cp");
        }
        exit(0) ; // CopyFail - syscalls fails.

    } else {        // Parent
        smash.HandleTimeOutAlarm(this->originalCmd,this->timeout,pid);
        if (!this->isBG) {
            smash.setFGpid(pid);
            int status;
            waitpid(pid, &status, WUNTRACED);
            smash.setFGpid(SMASH_PID);
        } else {
            SmallShell& smash = SmallShell::getInstance();
            smash.addJob(pid, this->originalCmd, false);
        }
    }
}

/*
 * ===================================================
 *                      Redirection
 * ===================================================
 */
RedirectionCommand::RedirectionCommand(vector<string>& cmdVec, string& cmdStr, time_t timeout)
                                                            : Command(cmdVec, cmdStr,timeout) {
    this->append = find(cmdVec.begin(), cmdVec.end(), ">>") != cmdVec.end();
    auto index = append ? find(cmdVec.begin(), cmdVec.end(), ">>") : find(cmdVec.begin(), cmdVec.end(), ">");
    vector<string> left(cmdVec.begin(), index);
    vector<string> right(index+1, cmdVec.end());
    if(isBackgroundCommand(right)) {            // We assume we give this syntax only: " {cmd} {arg: > or >} {dst} {&} ".
        left.insert(left.end(),"&");
    }
    this->srcCmd = left;
    this->dest = "";
    this->dest = right.at(0);
}

bool RedirectionCommand::validCommand() {
    return true;
}

void RedirectionCommand::execute() {
    if(!validCommand()) {
        return;
    }
    //SmallShell& smash = SmallShell::getInstance();
    Command* cmd = _createCommand(this->srcCmd, this->getOriginalCmd(),this->timeout);

    int std_out = dup(1);
    if(SyscallError(std_out,"dup")) {
        return;
    }

    int fd_open = 0, fd_close = 0, dup_res = 0;
    fd_close = close(1);
    if (SyscallError(fd_close,"close")) {
        return;
    }

    if (this->append) {
        fd_open = open(this->dest.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666 );
    } else {
        fd_open = open(this->dest.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0666 );
    }
    if (SyscallError(fd_open,"open")) {
        // if open fail, make std_out again.
        dup_res = dup(std_out);
        if (SyscallError(dup_res,"dup")) {
            return;
        }
        return;
    }


    try {
        cmd->execute();
    } catch (...) { // TODO: dealing with exceptions
        //cout << "DAM WE CATCH SOMETHING" << endl;
    }

    fd_close = close(1);
    if (SyscallError(fd_close,"close")) {
        return;
    }

    dup_res = dup(std_out);
    if (SyscallError(dup_res,"dup")) {
        return;
    }

    fd_close = close(std_out);
    if (SyscallError(fd_close,"close")) {
        return;
    }

    delete(cmd);
}

/*
 * ===================================================
 *                      PIPE
 * ===================================================
 */
PipeCommand::PipeCommand(vector<string> &cmdVec, string &cmdStr, time_t timeout) : Command(cmdVec, cmdStr, timeout) {
    bool firstCmd = true;
    this->isBG = cmdVec.back() == "&";
    for (auto it : cmdVec) {
        if (it.at(0) == '|') {
            this->isRegPipe = it.size() == 1;   // It is '|' and not '|&'
            firstCmd = false;
        } else {
            if (it == "&") {
                break;
            } else if (firstCmd) {
                this->cmd1.push_back(it);
            } else {
                this->cmd2.push_back(it);
            }
        }
    }
}

void PipeCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.setPiping(true);
    pid_t pipeManagerPID = fork();
    if (!SyscallError(pipeManagerPID, "fork")) {
        if (pipeManagerPID == 0) { // Child - pipeManager process
            setpgrp();
            execSubCommands();
        } else {    // Parent
            smash.HandleTimeOutAlarm(this->originalCmd,this->timeout,pipeManagerPID);
            if (!this->isBG) {
                smash.setFGpid(pipeManagerPID);
                waitpid(pipeManagerPID, NULL, WUNTRACED);
                smash.setFGpid(SMASH_PID);
            } else {
                smash.addJob(pipeManagerPID, this->originalCmd, false);
            }
        }
    } // else: first fork failed, we no don't continue
    smash.setPiping(false);
}

void PipeCommand::execSubCommands() {
    int myPipe[2];
    int pipeRet = pipe(myPipe);
    if (SyscallError(pipeRet, "pipe")) { exit(0); };
    int pid2 = fork();
    if (SyscallError(pid2, "fork")) {exit(0);};
    if (pid2 != 0) {
        int pid1 = fork();
        if (SyscallError(pid1, "fork")) {
            waitpid(pid1, nullptr, 0);   // Wait for cmd2 to finish
            exit(0);    // pipeManager exit
        }
        if (pid1 != 0) {    // Parent - pipeManager
            close(myPipe[0]);
            close(myPipe[1]);
            int stat1, stat2;
            waitpid(pid2, &stat2, 0);   // Wait for dest
            waitpid(pid1, &stat1, 0);   // Wait for src
             exit(0);    // pipeManager exit
        } else {            // cmd2 proc
            int closed = close(myPipe[1]);
            if (SyscallError(closed, "close")) exit(0);

            closed = dup2(myPipe[0], STDIN);     // STDIN is the pipe
            if (SyscallError(closed, "dup2")) exit(0);
            closed = close(myPipe[0]);
            if (SyscallError(closed, "close")) exit(0);

            try {
                Command *cmd2 = _createCommand(this->cmd2, "Dest command of pipe",INVALID_TIMEOUT_COMMAND); // Assuming does not end with '&'
                cmd2->execute();
                delete(cmd2);
            }
            catch (...) {   // TODO: How should we handle this exception?
                //cout << "Caught exception of cmd2 in pipe :(" << endl;
            }

            exit(0);    // cmd2 exit
        }
    } else {                // cmd1 proc
        int closed = close(myPipe[0]);
        if (SyscallError(closed, "close")) exit(0);

        int dstOut = this->isRegPipe ? STDOUT : STDERR;
        closed = dup2(myPipe[1], dstOut);     // dstOut is the pipe
        if (SyscallError(closed, "dup2")) exit(0);
        closed = close(myPipe[1]);
        if (SyscallError(closed, "close")) exit(0);

        try {
            Command* cmd1 = _createCommand(this->cmd1, "Src command of pipe",INVALID_TIMEOUT_COMMAND);  // Assuming does not end with '&'
            cmd1->execute();
            delete(cmd1);
        }
        catch (...) {   // TODO: How should we handle this exception?
            //cout << "Caught exception of cmd1 in pipe :(" << endl;
        }
        exit(0);    // cmd1 exit
    }

    // Should never get here
}


