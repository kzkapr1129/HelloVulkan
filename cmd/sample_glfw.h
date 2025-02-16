#pragma once

#include "command.h"

class SampleGLFW : public Command {
public:
    SampleGLFW() {};
    ~SampleGLFW() override {};

    int execute() override;
};