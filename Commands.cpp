#include "Commands.h"
#include "Smash.h"
#include "BuiltinCommands.h"
#include "ExternalCommands.h"
#include "SpecialCommands.h"
#include "TimeOutCommand.h"
#include <unordered_map>
#include <sstream>
#include <iterator>
#include <algorithm>

using std::unordered_map;
using std::istream_iterator;



const string WHITESPACE = " \n\r\t\f\v";


#if 1
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));


Command::Command(vector<string>& cmdVed, string& orgCmd, time_t timeout)
                                : cmdVec(cmdVed), originalCmd(orgCmd), timeout(timeout){}

const string &Command::getOriginalCmd() const {
    return originalCmd;
}

time_t Command::getTimeout() const {
    return timeout;
}

// Don't deal with negative numbers, cause NAN < 0 and its the same for us
int _Not_ATOI_Stupid_Boy(string str) {
    if (!str.empty() && str.find_first_not_of("-0123456789") == string::npos
        && ((str.substr(1,str.size())).find_first_not_of("0123456789") == string::npos)) {
        return std::atoi(str.c_str());
    }
    return NAN;
}

bool _invalidArgs(const char* cmd) {
    cerr << "smash error: " << cmd << ": invalid arguments" << endl;
    return false;
}

// Returns true if syscall failed (syscalRetValue == -1)
bool SyscallError(int syscalRetValue, const char *syscall) {
    string name = syscall;
    if (syscalRetValue == -1) {
        string str = "smash error: " + name + " failed";
        perror(str.c_str());
        return true;
    }
    return false;
}

bool isBackgroundCommand(vector<string> cmd) {
    return cmd[cmd.size() - 1] == "&";
}

// Returns command string without trailing '&'
string vecToStr(vector<string>& cmdVec) {
    const char* const delim = " ";
    std::ostringstream imp;

    //int shift = (isBackgroundCommand(cmdVec)) ? 1 : 0;
    std::copy(cmdVec.begin(), cmdVec.end(),
              std::ostream_iterator<string>(imp, delim));
    return imp.str();
}

vector<string> chompBG(vector<string>& cmdVec) {
    if (isBackgroundCommand(cmdVec)) {
        cmdVec.erase(cmdVec.cend()--);
    }
    return cmdVec;
}


// Commands length limit will not be tested @48
vector<string> parseCommandLine(const char* cmdLine) {

    string cmdStr = string(cmdLine);
    if (cmdStr.find(">>") != string::npos) {
        cmdStr.insert(cmdStr.find(">>")," ");
        cmdStr.insert(cmdStr.find(">>")+2," ");
    } else if (cmdStr.find(">") != string::npos) {
        cmdStr.insert(cmdStr.find(">")," ");
        cmdStr.insert(cmdStr.find(">")+1," ");
    } else if (cmdStr.find("|&") != string::npos) {
        cmdStr.insert(cmdStr.find("|&")," ");
        cmdStr.insert(cmdStr.find("|&")+2," ");
    } else if (cmdStr.find("|") != string::npos) {
        cmdStr.insert(cmdStr.find("|")," ");
        cmdStr.insert(cmdStr.find("|")+1," ");
    } else { // non-special command
        size_t index = cmdStr.find_first_of("&");
        if (index != string::npos) {  // & exists
            cmdStr = cmdStr.substr(0, index + 1);
            cmdStr.insert(index, " ");
        }

    }

    cmdStr += "   ";
    for (int i=0 ; cmdStr[i] != '\0' ; i++) {
        if (cmdStr[i] == '|' && cmdStr[i+1] == '&') {
            cmdStr.insert(i+2, " ");
            i++;
        } else if (cmdStr[i] == '&') {
            cmdStr.insert(i, " ");
            cmdStr.insert(i+1, " ");
            i+=2;
        }
    }

    istringstream iss(cmdStr);
    vector<string> cmd;
    copy(istream_iterator<string>(iss),
        istream_iterator<string>(),
        back_inserter(cmd));

    return cmd;
}


commands_e decodeCommand(string str) {
  using std::unordered_map;
  unordered_map<string,commands_e> map = {
          {"chprompt", CHPROMPT} , {"showpid", SHOWPID}, {"pwd", PWD}, {"cd", CD},
          {"jobs",     JOBS}, {"kill", KILL}, {"fg", FG}, {"bg", BG}, {"quit", QUIT},
          { "cp", CP}, {"timeout", TIMEOUT}
  };
  auto it = map.find(str);
  if ( it != map.end()) {
      return it->second;
  } else {
      return EXTERNAL;
  }

}

commands_e decodeSpecialCommand(vector<string>& cmdVec) {
    if (find(cmdVec.begin(),cmdVec.end(),"timeout") != cmdVec.end()) {
        return TIMEOUT;
    }
    if (find(cmdVec.begin(), cmdVec.end(), ">>") != cmdVec.end()
        || find(cmdVec.begin(), cmdVec.end(), ">") != cmdVec.end()) {
        return RED;
    } else if (find(cmdVec.begin(), cmdVec.end(), "|") != cmdVec.end()
        ||  find(cmdVec.begin(), cmdVec.end(), "|&") != cmdVec.end()) {
        return PIPE;
    }
    return OTHER;
}



/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 * Called from:
 * 1) execute command (main)
 * 2) timeout
 * 3) pipe and i/o red
*/
Command* _createCommand(vector<string>& cmdVec, string cmdStr, time_t timeout) {
    SmallShell& smash = SmallShell::getInstance();

    Command* cmd;
    commands_e cmdType = decodeSpecialCommand(cmdVec);
    if (cmdType == OTHER || cmdType == TIMEOUT) {
        cmdType = decodeCommand(cmdVec.at(0));
    }

    bool builitinCmd = true;

    switch(cmdType) {
        case TIMEOUT: {
            cmd = new TimeOutCommand(cmdVec,cmdStr, timeout);
            break;
        }
        case CHPROMPT: {
            cmd = new ChangePrompt(cmdVec, cmdStr, timeout);
            break;
        }
        case SHOWPID: {
            cmd = new ShowPidCommand(cmdVec, cmdStr, timeout);
            break;
        }
        case PWD: {
            cmd = new GetCurrDirCommand(cmdVec, cmdStr, timeout);
            break;
        }
        case CD: {
            cmd = new ChangeDirCommand(cmdVec, cmdStr, timeout);
            break;
        }
        case JOBS: {
            cmd = new JobsCommand(cmdVec, cmdStr, timeout);
            break;
        }
        case FG: {
            cmd = new ForegroundCommand(cmdVec, cmdStr, timeout);
            break;
        }
        case BG: {
            cmd = new BackgroundCommand(cmdVec, cmdStr ,timeout);
            break;
        }
        case KILL: {
            cmd = new KillCommand(cmdVec, cmdStr, timeout);
            break;
        }
        case QUIT: {
            cmd = new QuitCommand(cmdVec,cmdStr, timeout);
            break;
        }
        case CP: {
            cmd = new CopyCommand(cmdVec,cmdStr, timeout);
            builitinCmd = false;
            break;
        }
        case RED: {
            cmd = new RedirectionCommand(cmdVec, cmdStr, timeout);
            builitinCmd = false;
            break;
        }
        case PIPE: {
            cmd = new PipeCommand(cmdVec, cmdStr, timeout);
            builitinCmd = false;
            break;
        }
        default: {
            cmd = new ExternalCommand(cmdVec, cmdStr, timeout);
            builitinCmd = false;
            break;
        }
    }

    if(cmd != nullptr && builitinCmd) {
        smash.HandleTimeOutAlarm(cmdStr,cmd->getTimeout(),SMASH_PID);
    }
    return cmd;
}


void SmallShell::executeCommand(const char *cmd_line) {
    if (cmd_line[0] == '\0') {
        return;
    }
    vector<string> cmdVec = parseCommandLine(cmd_line);
    string cmdStr = string(cmd_line);

    this->currentCmdStr = cmdStr;
    Command* cmd = _createCommand(cmdVec, cmdStr,0); // default timeout: 0
    if (cmd != nullptr) {
      try { // TODO: How should we handle this exception?
          cmd->execute();
      } catch (...) {
          //cout << "Caught an exception :(" << endl;
      }
    }
    delete(cmd);
}
