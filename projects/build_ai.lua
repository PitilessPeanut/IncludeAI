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
                        "../example/tictac_test.cpp",
                        "../example/yavalath_test.cpp"
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






                local gcc_buildoption_ignoreNaN = "-ffinite-math-only"
                local gcc_buildoption_addAddressSanitize = "-fsanitize=address" -- dynamic bounds check "undefined reference"
                local gcc_buildoption_utf8compiler = "-finput-charset=UTF-8 -fexec-charset=UTF-8 -fextended-identifiers"
                local gcc_buildoption_fatal = "-Wfatal-errors" -- make gcc output bearable
                local gcc_buildoption_shadow = "-Wshadow-compatible-local" -- not working on clang
                local gcc_buildoption_impl_fallthrough = "-Wimplicit-fallthrough" -- warn missing [[fallthrough]]
                local gcc_buildoption_undef = "-Wundef" -- Macros must be defined

                filter { "system:ios" }
                        toolset "clang"
                        buildoptions { "-ffast-math"
                                     , "-pedantic"
                                     , "-Wno-unknown-warning-option"
                                     , gcc_buildoption_fatal
                                     , gcc_buildoption_shadow
                                     , gcc_buildoption_impl_fallthrough
                                     , gcc_buildoption_undef
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
                                     , "-Wno-unknown-warning-option"
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
