#!/bin/zsh

# shadercがインストールされているかチェック
if ! ( command -v glslc 2>&1 >/dev/null ); then
    # shadercがインストールされていない場合はインストールする
    brew install shaderc

    # memo:
    #  shadercはgoogle製のツールで、vulkan公式のものではない。
    #  今回は簡単にインストールすることができるshadercを利用する
fi

glslc -fshader-stage=vertex vert.glsl -o shader.vert.spv
glslc -fshader-stage=fragment frag.glsl -o shader.frag.spv

echo "done"