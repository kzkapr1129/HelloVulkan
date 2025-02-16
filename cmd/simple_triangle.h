#pragma once

#include "command.h"

class SimpleTriangle : public Command {
public:
    SimpleTriangle() {};
    ~SimpleTriangle() override {};

    int execute() override;
};