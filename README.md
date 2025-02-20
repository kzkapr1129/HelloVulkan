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

4. GLFWのソース(glfw/src/vulkan.c)を一部修正する
```diff
GLFWbool _glfwInitVulkan(int mode)
{
    ...(省略)...
#if defined(_GLFW_VULKAN_LIBRARY)
        _glfw.vk.handle = _glfwPlatformLoadModule(_GLFW_VULKAN_LIBRARY);
#elif defined(_GLFW_WIN32)
        _glfw.vk.handle = _glfwPlatformLoadModule("vulkan-1.dll");
#elif defined(_GLFW_COCOA)
-        _glfw.vk.handle = _glfwPlatformLoadModule("libvulkan.1.dylib");
+        _glfw.vk.handle = _glfwPlatformLoadModule("libMoltenVK.dylib");
        if (!_glfw.vk.handle)
            _glfw.vk.handle = _glfwLoadLocalVulkanLoaderCocoa();
```

5. GLFWをビルドする
```
$ cd glfw
$ mkdir build && cd build
$ cmake -DBUILD_SHARED_LIBS=ON ..
$ make
```

6. Hello Vulkanをビルドする
```
$ cd <本リポジトリのプロジェクトフォルダ>
$ mkdir build && cd build
$ cmake ..
$ make
```

7. Hello Vulkanを実行する
```
$ ./app -s 1
```
