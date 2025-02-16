#pragma once

class Command {
public:
    virtual ~Command() {}

    virtual int execute() = 0;
};