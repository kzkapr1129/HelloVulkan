# Hello Vulkan

Vulkanの練習用プロジェクト

## 環境構築手順

1. 本リポジトリを任意のディレクトリにcloneする
```
$ git clone --recursive <本リポジトリ>
```

2. glslをspir-vに変換する
```
$ cd shader
$ zsh conv.sh
```

3. MoltenVKをビルドする
```
$ cd MoltenVK
$  ./fetchDependencies --macos
$ make macos
```

4. GLFWをビルドする
```
$ cd glfw
$ mkdir build && cd build
$ cmake -DBUILD_SHARED_LIBS=ON ..
$ make
```

5. Hello Vulkanをビルドする
```
$ cd <本リポジトリのプロジェクトフォルダ>
$ mkdir build && cd build
$ cmake ..
$ make
```

6. Hello Vulkanを実行する
```
$ ./app -s 1
```
