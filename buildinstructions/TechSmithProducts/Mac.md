# Building with xcodebuild
 1. Clone this repository
 2. From a command line run the [xcodebuild command line tool](https://developer.apple.com/library/archive/technotes/tn2339/_index.html) with the following switches:
    - `-target libproxy`
    - `-configuration x86 | x64`
    - `SRCDIR [path to the root of the libproxy repo]
    - Examples:
        - 64bit build:
          ```
          xcodebuild -target libproxy -configuration x64 -SRCDIR "..\..\..\"
          ```
        - 32-bit build:
           ```
          xcodebuild -target libproxy -configuration x86 -SRCDIR "..\..\..\"
           ```
# Building with Visual Studio
1. Clone this repository
2. Open XCode
3. File -> Open -> [RepositoryDirectory]\buildinstructions\TechSmithProducts\Mac\libproxy.xcodeproj
4. Build -> Rebuild libproxy
