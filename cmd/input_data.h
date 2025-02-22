#pragma once

#include "command.h"

class InputData : public Command{
public:
    InputData() {};
    ~InputData() override {};

    int execute() override;
};
