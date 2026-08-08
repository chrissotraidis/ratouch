add_custom_target(tests)
add_dependencies(tests test_miscasm test_face test_rect test_fading test_lcw test_xordelta test_irandom test_fatpixel test_tobuff test_drawline test_putpixel test_drawbuff)

add_executable(test_ios_modifier ios_modifier.cpp ../common/ios_modifier.cpp)
target_include_directories(test_ios_modifier PUBLIC ../common)
add_test(NAME ios_modifier COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_ios_modifier>)
add_dependencies(tests test_ios_modifier)

add_executable(test_wwtouch wwtouch.cpp ../common/wwtouch.cpp)
target_include_directories(test_wwtouch PUBLIC ../common)
add_test(NAME wwtouch COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_wwtouch>)
add_dependencies(tests test_wwtouch)

add_executable(test_cdfile_search cdfile_search.cpp)
target_include_directories(test_cdfile_search PUBLIC .. ../common)
target_compile_definitions(test_cdfile_search PUBLIC ${VANILLA_DEFS})
target_link_libraries(test_cdfile_search PUBLIC common ${STATIC_LIBS})
add_test(NAME cdfile_search COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_cdfile_search>)
add_dependencies(tests test_cdfile_search)

add_executable(test_mix_backing mix_backing.cpp)
target_include_directories(test_mix_backing PUBLIC .. ../common)
target_compile_definitions(test_mix_backing PUBLIC ${VANILLA_DEFS})
target_link_libraries(test_mix_backing PUBLIC common ${STATIC_LIBS})
add_test(NAME mix_backing COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_mix_backing>)
add_dependencies(tests test_mix_backing)

add_executable(test_video_geometry video_geometry.cpp ../common/video_geometry.cpp)
target_include_directories(test_video_geometry PUBLIC ../common)
add_test(NAME video_geometry COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_video_geometry>)
add_dependencies(tests test_video_geometry)

add_executable(test_wsa_sizing wsa_sizing.cpp)
target_include_directories(test_wsa_sizing PUBLIC .. ../common)
target_compile_definitions(test_wsa_sizing PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_wsa_sizing PUBLIC common ${STATIC_LIBS})
add_test(NAME wsa_sizing COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_wsa_sizing>)
add_dependencies(tests test_wsa_sizing)

add_executable(test_savegame_version savegame_version.cpp)
target_include_directories(test_savegame_version PUBLIC .. ../common ../redalert)
target_compile_definitions(test_savegame_version PUBLIC ${VANILLA_DEFS})
target_link_libraries(test_savegame_version PUBLIC common ${STATIC_LIBS})
add_test(NAME savegame_version COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_savegame_version>)
add_dependencies(tests test_savegame_version)

add_executable(test_ratouch_gameplay ratouch_gameplay.cpp ../common/ratouch_gameplay.cpp)
target_include_directories(test_ratouch_gameplay PUBLIC ../common)
add_test(NAME ratouch_gameplay COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_ratouch_gameplay>)
add_dependencies(tests test_ratouch_gameplay)

add_executable(test_ratouch_settings ratouch_settings.cpp ../common/ratouch_settings.cpp)
target_include_directories(test_ratouch_settings PUBLIC ../common)
add_test(NAME ratouch_settings COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_ratouch_settings>)
add_dependencies(tests test_ratouch_settings)

add_executable(test_iso9660 iso9660.cpp ../apple/shared/iso9660.cpp)
target_include_directories(test_iso9660 PUBLIC ../apple/shared)
add_test(NAME iso9660 COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_iso9660>)
add_dependencies(tests test_iso9660)

add_executable(test_mix_validator mix_validator.cpp ../apple/shared/mix_validator.cpp)
target_include_directories(test_mix_validator PUBLIC ../apple/shared)
add_test(NAME mix_validator COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_mix_validator>)
add_dependencies(tests test_mix_validator)

add_executable(test_asset_manifest asset_manifest.cpp ../apple/shared/asset_manifest.cpp)
target_include_directories(test_asset_manifest PUBLIC ../apple/shared)
add_test(NAME asset_manifest COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_asset_manifest>)
add_dependencies(tests test_asset_manifest)

if(APPLE AND SDL2)
    add_executable(test_ios_lifecycle ios_lifecycle.cpp ../common/ios_lifecycle.cpp)
    target_include_directories(test_ios_lifecycle PUBLIC ../common)
    target_link_libraries(test_ios_lifecycle PUBLIC ${SDL2_LIBRARY})
    add_test(NAME ios_lifecycle COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_ios_lifecycle>)
    add_dependencies(tests test_ios_lifecycle)
endif()

if(SDL_AUDIO)
    add_executable(test_soundio_sdl2 soundio_sdl2.cpp)
    target_include_directories(test_soundio_sdl2 PUBLIC .. ../common)
    target_link_libraries(test_soundio_sdl2 PUBLIC commonv ${STATIC_LIBS})
    add_test(NAME soundio_sdl2 COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_soundio_sdl2>)
    set_tests_properties(soundio_sdl2 PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_dependencies(tests test_soundio_sdl2)
endif()

# Fast, asset-free gates for RAtouch's gameplay boundary. These protect input
# translation, presentation coordinates, lifecycle/save policy, data
# compatibility, audio streaming, and the score-screen animation regression.
# Red Alert's simulation remains engine-owned and is exercised by the live loop.
set_tests_properties(
    ios_modifier
    wwtouch
    cdfile_search
    mix_backing
    video_geometry
    wsa_sizing
    savegame_version
    ratouch_gameplay
    ratouch_settings
    iso9660
    mix_validator
    asset_manifest
    PROPERTIES LABELS "ratouch_gameplay_loop"
)

if(APPLE AND SDL2)
    set_tests_properties(ios_lifecycle PROPERTIES LABELS "ratouch_gameplay_loop")
endif()

if(SDL_AUDIO)
    set_tests_properties(soundio_sdl2 PROPERTIES LABELS "ratouch_gameplay_loop")
endif()

add_executable(test_miscasm miscasm.cpp)
target_include_directories(test_miscasm PUBLIC .. ../common)
target_compile_definitions(test_miscasm PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_miscasm PUBLIC common ${STATIC_LIBS})
add_test(NAME miscasm COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_miscasm>)

add_executable(test_face face.cpp)
target_include_directories(test_face PUBLIC .. ../common)
target_compile_definitions(test_face PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_face PUBLIC common ${STATIC_LIBS})
add_test(NAME face COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_face>)

add_executable(test_rect rect.cpp)
target_include_directories(test_rect PUBLIC .. ../common)
target_compile_definitions(test_rect PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_rect PUBLIC common ${STATIC_LIBS})
add_test(NAME rect COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_rect>)

add_executable(test_fading fading.cpp)
target_include_directories(test_fading PUBLIC .. ../common)
target_compile_definitions(test_fading PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_fading PUBLIC common ${STATIC_LIBS})
add_test(NAME fading COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_fading>)

add_executable(test_lcw lcw.cpp)
target_include_directories(test_lcw PUBLIC .. ../common)
target_compile_definitions(test_lcw PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_lcw PUBLIC common ${STATIC_LIBS})
add_test(NAME lcw COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_lcw>)

add_executable(test_xordelta xordelta.cpp)
target_include_directories(test_xordelta PUBLIC .. ../common)
target_compile_definitions(test_xordelta PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_xordelta PUBLIC common ${STATIC_LIBS})
add_test(NAME xordelta COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_xordelta>)

add_executable(test_irandom irandom.cpp)
target_include_directories(test_irandom PUBLIC .. ../common)
target_compile_definitions(test_irandom PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_irandom PUBLIC common ${STATIC_LIBS})
add_test(NAME irandom COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_irandom>)

add_executable(test_fatpixel fatpixel.cpp)
target_include_directories(test_fatpixel PUBLIC .. ../common)
target_compile_definitions(test_fatpixel PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_fatpixel PUBLIC commonv ${STATIC_LIBS})
add_test(NAME fatpixel COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_fatpixel>)

add_executable(test_tobuff tobuff.cpp)
target_include_directories(test_tobuff PUBLIC .. ../common)
target_compile_definitions(test_tobuff PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_tobuff PUBLIC commonv ${STATIC_LIBS})
add_test(NAME tobuff COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_tobuff>)

add_executable(test_drawline drawline.cpp)
target_include_directories(test_drawline PUBLIC .. ../common)
target_compile_definitions(test_drawline PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_drawline PUBLIC commonv ${STATIC_LIBS})
add_test(NAME drawline COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_drawline>)

add_executable(test_putpixel putpixel.cpp)
target_include_directories(test_putpixel PUBLIC .. ../common)
target_compile_definitions(test_putpixel PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_putpixel PUBLIC commonv ${STATIC_LIBS})
add_test(NAME putpixel COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_putpixel>)

add_executable(test_drawbuff drawbuff.cpp)
target_include_directories(test_drawbuff PUBLIC .. ../common)
target_compile_definitions(test_drawbuff PUBLIC TRUE_FALSE_DEFINED ENGLISH $<$<CONFIG:DEBUG>:_DEBUG> _WINDOWS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE WINSOCK_IPX)
target_link_libraries(test_drawbuff PUBLIC commonv ${STATIC_LIBS})
add_test(NAME drawbuff COMMAND ${TARGET_SYSTEM_EMULATOR} $<TARGET_FILE:test_drawbuff>)
