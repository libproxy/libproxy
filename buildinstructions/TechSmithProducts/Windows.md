# Building with MSBuild
 1. Clone this repository
 2. From a command line run the [msbuild command line tool](https://docs.microsoft.com/en-us/cpp/build/msbuild-visual-cpp) with the following switches:
    - `-p:Configuration=Release`
    - `-p:Platform=[ Win32 | x64 ]`
    - `-p:libproxyRepoPath=[path to the root of the libproxy repo]`
    - `-t:[ Build | Rebuild | Clean ]`
    - Examples:
        - 64bit build:
          ```
          msbuild.exe -p:Configuration=Release -p:Platform=x64 -p:libproxyRepoPath="../../" libproxy_msbuild_release.vcxproj
          ```
        - 32-bit build:
           ```b
           msbuild.exe -p:Configuration=Release -p:Platform=Win32 -p:libproxyRepoPath="../../" libproxy_msbuild_release.vcxproj
           ```
# Building with Visual Studio
1. Clone this repository
2. Open Visual Studio
3. File -> Open -> [RepositoryDirectory]\buildinstructions\TechSmithProducts\libproxy_msbuild_release.vcxproj
4. Build -> Rebuild libproxy