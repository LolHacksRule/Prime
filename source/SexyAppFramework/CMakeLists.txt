project(SexyAppFramework) #Prime uses multiple beyond C

set(SRC_PRIME #Prime includes
	Bezier.cpp
    BSpline.cpp
	Buffer.cpp #Header matches 1:1 with PL_D.dll
	ButtonWidget.cpp
	CfgMachine.cpp #Not yet fixed
	#Checkbox.cpp #Not in PL.dll but in Prime itself
	Color.cpp
	Common.cpp
	CompiledMap.cpp
	CritSect.cpp
	#CursorWidget.cpp #Part of Prime XNA? It's not even in mobile ports!
    CurvedVal.cpp
	Debug.cpp
	DescParser.cpp
	DeviceImage.cpp
	Dialog.cpp
	DialogButton.cpp
	#EditWidget.cpp #Not part of PL.dll
    EncodingParser.cpp
	Endian.cpp
	Flags.cpp #Not part of PL.dll. Part of BejClassic
	#FlashWidget.cpp #Not part of Prime
	Font.cpp
	Graphics.cpp
	#HyperlinkWidget.cpp #Not in PL.dll but in Prime itself, 1:1 to 1.30
	Image.cpp
	ImageFont.cpp
	Insets.cpp
	InternetManager.cpp
	#KeyCodes.cpp #CPP file not in Prime or BejBlitz PC.
	#ListWidget.cpp #Not in PL_D.dll or XNA
	MTRand.cpp
	MemoryImage.cpp
	Mesh.cpp
	ModVal.cpp
	MusicInterface.cpp
	NativeDisplay.cpp
	PerfTimer.cpp
	PIEffect.cpp
	PolyDivide.cpp
	PopAnim.cpp
	PopLoc.cpp
	PropertiesParser.cpp
	Quantize.cpp
	Ratio.cpp
	RenderEffect.cpp
	RenderStateManager.cpp
	ResStreamsManager.cpp
	ResourceManager.cpp
	#ScrollbarWidget.cpp #Not in PL_D.dll, doesn't match
	#ScrollbuttonWidget.cpp #Not in PL_D.dll
	SexyAppBase.cpp #Not yet fixed
	SexyCache.cpp
	SexyLocale.cpp
	SexyMatrix.cpp
    SexyThread.cpp
	SharedImage.cpp
	SharedRenderTarget.cpp
	#Slider.cpp #Not in PL_D.dll or BejBlitz
	SWTri.cpp
	SysFont.cpp
	#TextWidget.cpp #Not in PL_D.dll
	Widget.cpp
	WidgetContainer.cpp
	WidgetManager.cpp
	XMLParser.cpp
)

set(SRC_EXTERNAL #External libs
	ogg/bitwise.c
	ogg/block.c
	ogg/codebook.c
	ogg/floor0.c
	ogg/floor1.c
	ogg/framing.c
	ogg/info.c
	ogg/mapping0.c
	ogg/mdct.c
	ogg/registry.c
	ogg/res012.c
	ogg/sharedbook.c
	ogg/synthesis.c
	ogg/vorbisfile.c
	ogg/window.c
	#Crypt/md5.cpp #MD5 is in Crypt which is private, only that will be decompiled in it
)

#Get platform drivers
file(GLOB DRIVER_TYPES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/drivers ${CMAKE_CURRENT_SOURCE_DIR}/drivers/*)

foreach(DRIVER_TYPE ${DRIVER_TYPES})
	if(DRIVER_TYPE STREQUAL "misc")
        continue()  # Skip
    endif()
    set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/drivers/${DRIVER_TYPE}/${PLATFORM})
    
    if(EXISTS ${PLATFORM_DIR})
        file(GLOB PLATFORM_FILES ${PLATFORM_DIR}/*.cpp)
        list(APPEND SRC_PLATFORM_DRIVER ${PLATFORM_FILES})
    else()
        list(APPEND SRC_PLATFORM_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/drivers/${DRIVER_TYPE}/Null${DRIVER_TYPE}Driver.cpp)
    endif()
endforeach()

set(SRC_WINUTILS #Windows only utils, HTTPTransfer is present in PVZ Free but no actual code
	PixelTracer.cpp
	HTTPTransfer.cpp
	SEHCatcher.cpp #Windows only.
	WinInetHTTPTransfer.cpp #Windows only, no clue why it's here instead of the multiplat dir.
)

if(${PLATFORM} STREQUAL "Windows")
	file(GLOB BASS_FILES drivers/audio/bass/*.cpp)
    list(APPEND SRC_PLATFORM_DRIVER ${BASS_FILES})
	list(APPEND SRC_PRIME ${SRC_WINUTILS}) #For now
endif()

SET (SRC_FEAST FeastLexMain.cpp FeastLibMain.cpp FeastPrsMain.cpp) #FEAST 1.03 | C++ only, used by EAMT, PVZ2, MacOS, PC, not used on iOS and Transmension, only 1.03 and 1.04 were used to this day by EAMT (1.04), PVZ2 (1.04), MacOS (1.04), PC (1.04)
SET (SRC_XPRINTF xprintf.cpp)
SET (SRC_REFLECTION RefPdbReader.cpp RefPeFiles.cpp Reflection.cpp)

#if (PLATFORM_SUPPORTS_CONFIG_PARSING)
if(${FeastDLL})
	add_library(FEAST SHARED ${SRC_FEAST}) #Build DLL
elseif(${FeastLIB})
    add_library(FEAST STATIC ${SRC_FEAST}) #Build LIB
else() #PopCap standard
    list(APPEND SRC_PRIVATE_EXTERNAL ${SRC_FEAST})
endif()

#Reflection (Not on Mac or Steam, Transmension or EAMT)

if(${ReflectionDLL})
	add_library(Reflection SHARED ${SRC_REFLECTION})
elseif(${ReflectionLIB})
	add_library(Reflection STATIC ${SRC_REFLECTION})
else()
	list(APPEND SRC_PRIVATE_EXTERNAL ${SRC_REFLECTION})
endif()

#XPrintf

if(${XPrintfDLL})
	add_library(xprintf SHARED ${SRC_XPRINTF})
elseif(${XPrintfLIB})
	add_library(xprintf STATIC ${SRC_XPRINTF})
else()
	list(APPEND SRC_PRIVATE_EXTERNAL ${SRC_XPRINTF})
endif()

#if($WIN32) #On Win include D3D stuff and defs
#	target_include_directories(SexyAppFramework PRIVATE
#		${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/d3d8
#		#${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/d3d9
#	)
#	list (APPEND SRC_PLATFORM_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/Audio/Bass/*.cpp)
#	if (USE_FMOD_OVER_BASS)
#		list (APPEND SRC_PLATFORM_DRIVER ${CMAKE_CURRENT_SOURCE_DIR}/FMod*.cpp)
#	endif()
#	set(SRC_PCUTILS #Windows only utils, HTTPTransfer is present in PVZ Free but no actual code
#	PixelTracer.cpp
#	HTTPTransfer.cpp
#	SEHCatcher.cpp
#	)
#	list(APPEND SRC_PRIME ${SRC_PCUTILS})
#	add_compile_definitions(SexyAppFramework _LIB)
#	add_compile_definitions(SexyAppFramework WIN32)
#	add_compile_definitions(SexyAppFramework _VISUALC_)
#	add_compile_definitions(SexyAppFramework _JPEGLIB_)
#	add_compile_definitions(SexyAppFramework _CRT_SECURE_NO_DEPRECATE)
#	add_compile_definitions(SexyAppFramework _VC80_UPGRADE=0x0710)
#	add_compile_definitions(SexyAppFramework D3D_DEBUG_INFO)
#	#add_compile_options(SexyAppFramework -EHs)
#endif()

add_library(SexyAppFramework STATIC ${SRC_PRIME} ${SRC_EXTERNAL} ${SRC_PLATFORM_DRIVER} ${SRC_PRIVATE_EXTERNAL})
target_compile_definitions(SexyAppFramework PRIVATE _LIB D3D_DEBUG_INFO _VC80_UPGRADE=0x0710 _JPEGLIB_ _VISUALC_ _CRT_SECURE_NO_DEPRECATE _MBCS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_sources(SexyAppFramework PRIVATE SafeStrFntRcd.cpp) #Debug only, on Win?
endif()

#Platform includes
	#target_include_directories(SexyAppFramework PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/drivers/misc/${PLATFORM})

	target_include_directories(SexyAppFramework PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/d3d8
    #${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/d3d9
)

target_link_libraries(SexyAppFramework Reflection FEAST xprintf ImageLib PakLib ws2_32 winmm gdi32 ole32) #Do we need D3D8lib and 9lib and dsound
