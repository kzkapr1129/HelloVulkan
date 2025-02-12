# Hello Vulkan

Vulkanの練習用プロジェクト

## 環境構築手順

1. 本リポジトリを任意のディレクトリにcloneする
```
$ git clone --recursive <本リポジトリ>
```

2. MoltenVKをビルドする
```
$ cd MoltenVK
$  ./fetchDependencies --macos
$ make macos
```

3. Hello Vulkanをビルドする
```
$ cd <本リポジトリのプロジェクトフォルダ>
$ mkdir build && cd build
$ cmake ..
$ make
```

4. Hello Vulkanを実行する
```
$ ./app
```
