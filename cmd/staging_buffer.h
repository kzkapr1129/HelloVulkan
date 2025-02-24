#pragma once

#include "command.h"

// MEMO:
//  - デバイスから高速にアクセスできるメモリ(ホスト不可視、デバイス可視)
//    → デバイスローカル
//
//  - デバイスから高速にアクセスできないメモリ(ホスト可視、デバイス可視)
//    → ステージングバッファ
//
//  本サンプルでは一旦ステージングバッファに保存後、
//  デバイスが高速にアクセスできるデバイスローカルに移動させる

class StagingBuffer : public Command {
public:
    StagingBuffer() {};
    ~StagingBuffer() override {};

    int execute() override;
};