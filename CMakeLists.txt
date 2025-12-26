cmake_minimum_required(VERSION 3.18)
project(SexyAppFramework)

if(CMAKE_SIZEOF_VOID_P EQUAL 8) #I haven't tested this
    set(BITNESS "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BITNESS "32")
endif()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules" )
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#For now
if(MSVC)
    message(STATUS "MSVC Version: ${MSVC_VERSION}")
    if (MSVC_VERSION EQUAL 1300)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GF /EHsc /GS /Gy /GR /W3 /c /Wp64 /Z7 /TP")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Og /Ob1 /Oi /Ot /Oy-")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    else()
#        add_definitions(-DNOMINMAX)
#
        if(CMAKE_CXX_FLAGS MATCHES "sanitize=address")
            message(STATUS "ASAN detected, switching to non-debug runtime library.")
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
        endif()
    endif()
#
#    add_definitions(-D_USE_MATH_DEFINES)
endif()

option(WideStrings "Enable Wide Strings" ON)
option(Use32BitWideString "Enable 32-Bit Wide Strings" OFF)
option(DeprecatedCode "Keep deprecated engine code that might've been useful, ignores revisional improvements" OFF)
option(UseLatestCode "Use latest engine logic based off Bejeweled Blitz Live for Windows 8 and mobile (VERY LIKELY TO FAIL)" OFF)
option(UseOptimizedCode "Use unofficial logic that is less bloated and better designed for current platforms (UNTESTED)" OFF)
option(DummyUnnecessaryCode "Dummy unnecessary code (ResGen, SexyCache), akin to XNA" OFF)
option(FeastDLL "Build FEAST as an external DLL" OFF)
option(FeastLIB "Build FEAST as an external LIB" OFF)
option(Feast106 "Use FEAST 1.06 over 1.04 (UNTESTED)" OFF)
option(ReflectionDLL "Build Reflection as an external DLL (UNTESTED)" OFF)
option(ReflectionLIB "Build Reflection as an external LIB (UNTESTED)" OFF)
option(XPrintfDLL "Build XPrintf as an external DLL (UNTESTED)" OFF)
option(XPrintfLIB "Build XPrintf as an external LIB (UNTESTED)" OFF)
option(SexyDLL "Build the engine as an external DLL (UNTESTED)" OFF)
option(EnablePerformanceProfiling "Enable performance profiling (UNTESTED)" OFF)
option(BuildDemos "Build framework demos" OFF)

#Recover missing defines
option(DisablePropertiesParser "Do not build the properties parser" OFF)
option(DumpLeakedMemory "Dump leaked memory" OFF)
option(EnableDebugTracing "Record memory traces" OFF)

if (${BuildDemos})
	add_subdirectory(source/demos)
endif()

if (${WideStrings})
	add_compile_definitions(_USE_WIDE_STRING)
endif()

if (${UseLatestCode})
	add_compile_definitions(_SEXYDECOMP_USE_LATEST_CODE)
	add_compile_definitions(_SEXYDECOMP_USE_32BIT_WIDESTRING)
	add_compile_definitions(_USE_WIDE_STRING)
endif()

if (${Use32BitWideString})
	add_compile_definitions(_SEXYDECOMP_USE_32BIT_WIDESTRING)
    add_compile_definitions(_USE_WIDE_STRING)
endif()

if (${DeprecatedCode})
	add_compile_definitions(_SEXYDECOMP_USE_DEPRECATED_CODE)
endif()

if (${UseOptimizedCode})
    add_compile_definitions(_SEXYDECOMP_USE_OPTIMIZED_CODE)
endif()

if (${DummyUnnecessaryCode})
    add_compile_definitions(_SEXYDECOMP_DUMMY_USELESS_CODE)
endif()

if (${EnablePerformanceProfiling})
    add_compile_definitions(SEXY_PERF_ENABLED)
endif()

if (${DisablePropertiesParser})
    add_compile_definitions(__SEXYAPPFRAMEWORK_NO_PROPERTIES_PARSER__) #Unused but recovering.
endif()

if (${DumpMemoryLeaks})
    add_compile_definitions(SEXY_MEMTRACE)
endif()

if (${TraceStuff})
    add_compile_definitions(SEXY_TRACE)
endif()

if (${FeastLIB})
    set(${FeastDLL} OFF)
endif()

if (${FeastDLL})
    set(${FeastLIB} OFF)
	add_compile_definitions(FEAST_DLL)
    add_compile_definitions(FEAST_DLL_EXPORTS) #DLL ATM only works with exports
endif()

if (${DumpLeakedMemory})
    add_compile_definitions(SEXY_DUMP_LEAKED_MEM)
endif()

if (${EnableDebugTracing}) #Unused but we can restore it!
    add_compile_definitions(SEXY_TRACING_ENABLED)
endif()

# Use BASS 2.0 on 64-bit platform
if(NOT WIN32 OR BITNESS STREQUAL "64")
    message(STATUS "64-bit platform detected, forcing BASS 2.x")
    set(USE_BASS2 ON)
endif()

if(NOT PLATFORM)
    if(WIN32 OR BITNESS STREQUAL "64") # THIS ASSUMES VCPKG OR SOURCES !!!!!!!
        set(PLATFORM "Windows" CACHE STRING "The platform to compile for.")
    #elseif(ANDROID)
    #    set(PLATFORM "Android" CACHE STRING "The platform to compile for.")
    #elseif(IOS)
    #    set(PLATFORM "iPhoneOS" CACHE STRING "The platform to compile for.")
    #elseif(OSX)
    #    set(PLATFORM "Macosx" CACHE STRING "The platform to compile for.")
    #elseif(WINDOWS_STORE)
    #    set(PLATFORM "WinRT" CACHE STRING "The platform to compile for.")
    #else()
    #    set(PLATFORM ${CMAKE_SYSTEM_NAME} CACHE STRING "The platform to compile for.")
    else()
        message(STATUS "The platform is unsupported.")
    endif()
endif()

add_subdirectory(source/SexyAppFramework)
add_subdirectory(source/SexyAppFramework/ImageLib)
add_subdirectory(source/SexyAppFramework/PakLib)