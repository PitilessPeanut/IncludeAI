-- Invoke: ./premake5 --file=../premake5.lua --os=ios xcode4


workspace "Osgiliath"
        configurations { "Debug", "Release" }
        architecture "x86_64"
        project "TicTacToe"
                language "c++"
                kind "ConsoleApp" -- "StaticLib" "WindowedApp"
                cppdialect "C++20"
                rtti "Off"
                exceptionhandling "Off"
                warnings "extra"
                files { "../src/*.cpp",
                        "../src/*.hpp",
                        "../example/*.cpp",
                        "../example/*.hpp"
                      }
                removefiles { "../src/bridge.cpp", "../src/bridge.hpp" }
                targetdir "../"






                filter { "system:windows", "action:vs*" }
                        --systemversion(os.winSdkVersion() .. ".0") 
                        buildoptions { "/fp:fast" }
                        defines { "_CRT_SECURE_NO_WARNINGS", 
                                  "_CRT_NONSTDC_NO_WARNINGS",
                                }
                        location "../example"
                        
                filter {} -- "deactivate"






                gcc_buildoption_ignoreNaN = "-ffinite-math-only"
                gcc_buildoption_addAddressSanitize = "-fsanitize=address" -- dynamic bounds check "undefined reference"
                gcc_buildoption_utf8compiler = "-finput-charset=UTF-8 -fexec-charset=UTF-8 -fextended-identifiers"
                gcc_buildoption_fatal = "-Wfatal-errors" -- make gcc output bearable
                gcc_buildoption_shadow = "-Wshadow-compatible-local"
                gcc_buildoption_impl_fallthrough = "-Wimplicit-fallthrough" -- warn missing [[fallthrough]]
                gcc_buildoption_undef = "-Wundef" -- Macros must be defined

                filter { "system:ios" }
                        toolset "clang"
                        buildoptions { "-ffast-math"
                                     , "-pedantic"
                                     , gcc_buildoption_fatal
                                     , gcc_buildoption_shadow
                                     }
                        defines {
                                }
                        removefiles { "../src/.DS_Store",
                                      "../src/Assets.xcassets/.DS_Store"
                                    }
                        location "../example"
                        
                filter {} -- "deactivate"






                filter { "system:linux" }
                        toolset "gcc"
                        buildoptions { "-march=native"
                                     , "-pedantic"
                                     , "-ffast-math"
                                     , gcc_buildoption_utf8compiler
                                     , gcc_buildoption_shadow
                                     , gcc_buildoption_impl_fallthrough
                                     , gcc_buildoption_undef
                                     }
                        defines {
                                }
                        location "../example"
                        links {}
                        optimize "Debug" --"Size" --Debug" "Full"

                filter {} -- "deactivate"





                filter "configurations:Debug"
                        symbols "On"
                        defines "_DEBUG"
                        optimize "Debug"

                filter "configurations:Release"
                        symbols "Off"
                        defines "NDEBUG"
                        optimize "Speed"

                filter {}

print("done")
os.remove("Makefile")
