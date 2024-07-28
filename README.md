# About

This project is a simple implementation of big integers in C.
It also includes a simple Arena allocator, for fun!

## Note

Currently much of my focus is on the Arena allocator.
The C++ big int implementation is still there but it does not build.
This is because I've removed some of the project-specific C++ headers.

# Requirements

1. Windows 10
    * Visual Studio 2022
        * Desktop Development for C++
        * MSVC v143 - VS 2022 C++ x64/x86 build tools
        
        We need these 2 so we have access to the C/C++ compiler `CL.EXE` and linker `LINK.EXE`.
        
        Note that your chosen version MUST be able to support the C11 standard.

        * C++ Address Sanitizer

        We need the Address Sanitizer in order to pass the `-fsanitize=address` flag during compilation. This allows debug builds to report invalid memory accesses and immediately abort.

        * Windows 10 SDK 10.0.20348.0

        We need the particular Windows SDK version in order to include the `stdalign.h` header. SDK's earlier than 10.0.22000.0 do not have it.
        
    * GNU Make

        On Windows 10, this can be installed via `winget`.
        Alternatively, if you are using MSYS2, you can install via `pacman`.

1. Other Systems

    * Not supported yet!

## If you're using Visual Studio anyway, why not use NMake?

Microsoft NMake is absolutely braindead!

# Building

## Assumptions

* Installation: Visual Studio Build Tools 2022
* Architecture: AMD64

You can adapt the instructions in the following section depending on your requirements.

## Instructions

<!-- auto update lists: use all 1's, works for GitHub-flavored MD -->
1. Run your desired developer environment for Visual Studio. You have 2 options:
    1. Open the shortcut `Developer Command Prompt for Visual Studio 2022`.
    1. Run the following lines, one after the other, in a plain `CMD.EXE` instance.

        ```bat
        > cd "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
        > cd "VC\Auxiliary\Build"
        > call "vcvars64.bat"
        ```
        You should see something similar to the following output:
        ```bat
        Microsoft Windows [Version 10.0.19045.4651]
        (c) Microsoft Corporation. All rights reserved.
        **********************************************************************
        ** Visual Studio 2022 Developer Command Prompt v17.6.17
        ** Copyright (c) 2022 Microsoft Corporation
        **********************************************************************
        [vcvarsall.bat] Environment initialized for: 'x64'
        ```
        
    Of course you'll need to adjust for different Visual Studio years (e.g. 2019) or installations (Community, Professional or Build Tools)

1. Change the current working directory to the project directory.

    For example, the project on my laptop is found at `C:\Users\crimeraaa\repos\big_int`.

    ```bat
    > cd "C:\Users\crimeraaa\repos\big_int"
    ```
    
    Of course you'll need to change this depending on where you saved the repo.

1. Run `make`.

    ```bat
    > make
    ```

    You should something similar to the following output:

    ```bat
    cl -nologo -W4 -WX -permissive- -Zc:preprocessor -Fe:"bin/main.exe" -Fo:"obj/" -std:c11 -Zc:__STDC__ -Od -Zi -Fd:"bin/" -fsanitize=address -DDEBUG_USE_PRINT src/arena.c src/main.c
    arena.c
    main.c
    Generating Code...
       Creating library bin\main.lib and object bin\main.exp
    ```

    This is equivalent to `make debug`.
    You can run `make release` if you'd like optimizations and no debug information.
