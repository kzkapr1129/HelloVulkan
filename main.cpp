#include "command.h"
#include "simple_triangle.h"
#include "sample_glfw.h"
#include "input_data.h"
#include "index_buffer.h"
#include "staging_buffer.h"
#include <cxxopts.hpp>
#include <iostream>
#include <memory>

int main(int argc, char** argv) {
    cxxopts::Options options(argv[0], "Vulkan練習用アプリ");

    options.add_options()
        ("s,sample", "実行するサンプル番号を指定する", cxxopts::value<int>()->default_value("5"))
        ("h,help", "利用方法")
    ;

    cxxopts::ParseResult parseResult;
    try {
        // コマンドライン引数のパース
        parseResult = options.parse(argc, argv);
    } catch (cxxopts::exceptions::parsing& e) {
        // パースに失敗した場合
        std::cerr << e.what() << std::endl;
        return 1;
    }

    if (parseResult.count("help")) {
        // ヘルプの表示
        std::cout << options.help() << std::endl;
        return 0;
    }

    std::map<int, std::function<std::unique_ptr<Command>()>> classRegistry = {
        {1, []() -> std::unique_ptr<Command> { return std::unique_ptr<Command>(new SimpleTriangle()); }},
        {2, []() -> std::unique_ptr<Command> { return std::unique_ptr<Command>(new SampleGLFW()); }},
        {3, []() -> std::unique_ptr<Command> { return std::unique_ptr<Command>(new InputData()); }},
        {4, []() -> std::unique_ptr<Command> { return std::unique_ptr<Command>(new IndexBuffer()); }},
        {5, []() -> std::unique_ptr<Command> { return std::unique_ptr<Command>(new StagingBuffer()); }},
    };
    // サンプル番号から実行するコマンドクラスのインスタンスを取得
    auto res = classRegistry.find(parseResult["sample"].as<int>());
    if (res == classRegistry.end()) {
        std::cerr << "不明なサンプル番号が指定されました" << std::endl;
        return 1;
    }
    // コマンドを実行する
    return res->second()->execute();
}