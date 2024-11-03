# LzyUserThread

A user thread library run on Windows  

# Test environment

VS2022 x86/x64 with masn  

# How to compile

First entr the project root directory  

Then  

cd .\LzyUserThread  

md build  

cd build  

cmake ..

If you want to complie x86 version, run

cmake -A Win32 ..

After you run cmake, there are Visual Studio project files in build directory.  

Using them to compile static library file.  