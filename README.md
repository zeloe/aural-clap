# aural-clap
Clap plugin with highpass and mid side clipping.
# Description
This is an old vst i ported from c++ into c more or less :)
# Further reading
from here i got filters: \
[filters](https://github.com/dimtass/DSP-Cpp-filters) \
some useful tutorials: \
[tutorials](https://www.youtube.com/watch?v=oko5xJDY39E) \
[tutorials2](https://nakst.gitlab.io/tutorial/clap-part-1.html) 

# How to build


```shell
git clone https://github.com/zeloe/aural-clap.git
cd aural-clap
git submodule update --init --recursive
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```
or 
```shell
git clone https://github.com/zeloe/aural-clap.git
cd aural-clap
git submodule update --init --recursive
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
# To Do
Add Gui
