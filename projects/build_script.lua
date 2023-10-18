-- Invoke: ./premake5 --file=../premake5.lua --os=ios xcode4


workspace "Osgiliath"
        configurations { "Debug", "Release" }
        architecture "x86_64"
        project "Osgiliath"
                language "c++"
                --kind "ConsoleApp"
                kind "WindowedApp"
                cppdialect "C++20"
                rtti "Off"
                warnings "extra"
                files { "../src/*.cpp",
                        "../src/*.hpp",
                      }
                targetdir "../"






                filter { "system:windows", "action:vs*" }
                        --systemversion(os.winSdkVersion() .. ".0") 
                        exceptionhandling "Off"
                        buildoptions { "/fp:fast" }
                        defines { "_CRT_SECURE_NO_WARNINGS", 
                                  "_CRT_NONSTDC_NO_WARNINGS",
                                  protect_strings,
                                  remove_strings
                                }
                        files { "../src_win/*.cpp",
                                "../src_win/*.hpp"
                              }
                        location "../build_win"
                        
                filter {} -- "deactivate"






                nix_buildoption_ignoreNaN = "-ffinite-math-only"
                nix_buildoption_addAddressSanitize = "-fsanitize=address" -- dynamic bounds check "undefined reference"
                nix_buildoption_utf8compiler = "-finput-charset=UTF-8 -fexec-charset=UTF-8 -fextended-identifiers"
                nix_buildoption_fatal = "-Wfatal-errors" -- make gcc output bearable
                nix_buildoption_shadow = "-Wshadow-compatible-local"                        

                filter { "system:ios" }
                        toolset "clang"
                        exceptionhandling "Off"
                        buildoptions { "-ffast-math"
                                     , "-pedantic"
                                     , nix_buildoption_fatal
                                     , nix_buildoption_shadow
                                     }
                        defines { protect_strings,
                                  remove_strings
                                }
                        files { "../src_ios/**" }
                        removefiles { "../src_ios/.DS_Store",
                                      "../src_ios/Assets.xcassets/.DS_Store"
                                    }
                        location "../build_ios"
                        
                filter {} -- "deactivate"






                filter { "system:linux" }
                        toolset "gcc"
                        buildoptions { "-march=native"
                                     , "-pedantic"
                                     , "-ffast-math"
                                     , nix_buildoption_utf8compiler
                                     , nix_buildoption_fatal
                                     , nix_buildoption_shadow
                                     }
                        defines { protect_strings,
                                  remove_strings
                                }
                        files { "../src_linux/*.cpp", -- ok if missing
                                "../src_linux/*.hpp",
                                "../src_server/*.cpp", -- ok if missing
                                "../src_server/*.hpp"
                              }
                        location "../build_linux"
                        links { "pthread",
                                "crypto",
                                "ssl"
                              }
                        optimize "Speed" --"Size" --Debug" -- Speed" "Full"


                filter {} -- "deactivate"





                filter "configurations:Debug"
                        symbols "On"
                        defines "_DEBUG"
                        optimize "Speed"


                filter "configurations:Release"
                        symbols "Off"
                        defines "NDEBUG"
                        optimize "Speed"


                filter {}

print("done")
os.remove("Makefile")
