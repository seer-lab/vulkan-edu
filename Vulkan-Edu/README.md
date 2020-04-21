# vulkan-edu
to start, i put the `.obj` files  in the debug folder and placed my `vulkansdk` to get `external/vulkansdk-macos/macOS`

or you can change the working directory to `../Lab\ 1` and `../Lab\ 2` and such, just the ones containing the `main.cpp`

for u mac users, copy the environment path from the cmake output, it should look like
```commandline
environment path for u mac users:
VK_ICD_FILENAMES = /Users/blah/macOS/share/vulkan/icd.d/MoltenVK_icd.json;
VK_LAYER_PATH = /Users/blah/macOS/share/vulkan/explicit_layer.d
```
or perhaps run `setup-env.sh` as described in https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html

now i can use the cmake interface in `CMakeLists.txt` to choose which labs i want, or i have to uncomment and set the environment path for every lab

for cloning, try
```sh
git clone --depth 1 --recurse-submodules https://github.com/Ibrahimmushtaq98/Vulkan-Edu2
cd Vulkan-Edu2
git submodule add https://github.com/glfw/glfw external/glfw
git submodule add https://github.com/g-truc/glm external/glm
git submodule update --remote
```

theres also a one-liner on a newer version of git
```sh
git clone --depth 1 --recurse-submodules --remote-submodules https://github.com/Ibrahimmushtaq98/Vulkan-Edu2
```
then do the directory/file shuffle

## Linking FreeImage
First I downloaded the **Source distribution** on http://freeimage.sourceforge.net/download.html and followed the instructions on `README.osx`. I linked with the static library `FreeImage/Dist/libfreeimage.a`
After that I ran into a problem like on https://stackoverflow.com/q/22922585 , both the answers worked for me
