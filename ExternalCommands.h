#ifndef HW1_EXTERNALCOMMANDS_H
#define HW1_EXTERNALCOMMANDS_H

#include "Commands.h"

const int NUM_OF_ARGV = 4;


class ExternalCommand : public Command {
private:
    char* argv [NUM_OF_ARGV];
    bool isBG;

public:
    ExternalCommand(vector<string>& cmdVec,string strCmd, time_t timeout);
    virtual ~ExternalCommand();
    void execute() override;
};

#endif //HW1_EXTERNALCOMMANDS_H
