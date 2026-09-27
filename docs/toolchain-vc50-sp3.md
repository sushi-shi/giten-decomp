# Local VC5 toolchain

The repository contains source code and build tooling only. Supply a locally
obtained MSVC 5.0 SP3 toolchain and retail executable outside Git. Set
`GITEN_TOOLCHAIN` and `GITEN_RETAIL_EXE` before `nix develop`, or place them
at `build/local/toolchain` and `build/local/DDS.EXE`. Both paths are ignored.

The expected toolchain layout is:

```text
msvc/{bin,include,lib}
dx/{Include,Lib}
ninja/ninja.exe
```

The compiler and linker version evidence is in
[compiler identification](compiler-detection.md).
