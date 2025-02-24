#pragma once

#include "command.h"

class IndexBuffer : public Command {
public:
    IndexBuffer() {};
    ~IndexBuffer() override {};

    int execute() override;
};