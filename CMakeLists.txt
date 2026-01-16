cmake_minimum_required(VERSION 3.10)
# 设置CMake策略以消除警告
cmake_policy(SET CMP0074 NEW)
project(MyCProject C) # 声明为C项目
set(PROJECT_VERSION "1.0.0")

# 设置C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置链接类型：静态链接库
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

# ！！！关键：生成 compile_commands.json 文件供clangd使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 3. 查找SDL3库包[citation:4]
find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加可执行文件，链接所有源文件
add_executable(my_sdl_app src/main.c src/camera.c src/polygon.c src/batchingRender.c src/ui.c src/vector.c src/character.c src/frameController.c)

target_link_options(my_sdl_app PRIVATE -mwindows)

# 5. 将SDL3库链接到你的程序 - 动态链接SDL相关库
target_link_libraries(my_sdl_app PRIVATE SDL3::SDL3)
target_link_libraries(my_sdl_app PRIVATE SDL3_image::SDL3_image)
target_link_libraries(my_sdl_app PRIVATE SDL3_ttf::SDL3_ttf)

# 获取SDL3库的路径并复制必要的DLL文件（仅在找到库时）
if(TARGET SDL3::SDL3)
    get_target_property(SDL3_DLL SDL3::SDL3 IMPORTED_LOCATION)
    if(SDL3_DLL AND EXISTS ${SDL3_DLL})
        # 复制SDL3 DLL到输出目录
        add_custom_command(TARGET my_sdl_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL3_DLL} $<TARGET_FILE_DIR:my_sdl_app>
        )
    endif()
endif()

if(TARGET SDL3_image::SDL3_image)
    get_target_property(SDL3_IMAGE_DLL SDL3_image::SDL3_image IMPORTED_LOCATION)
    if(SDL3_IMAGE_DLL AND EXISTS ${SDL3_IMAGE_DLL})
        add_custom_command(TARGET my_sdl_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL3_IMAGE_DLL} $<TARGET_FILE_DIR:my_sdl_app>
        )
    endif()
endif()

if(TARGET SDL3_ttf::SDL3_ttf)
    get_target_property(SDL3_TTF_DLL SDL3_ttf::SDL3_ttf IMPORTED_LOCATION)
    if(SDL3_TTF_DLL AND EXISTS ${SDL3_TTF_DLL})
        add_custom_command(TARGET my_sdl_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL3_TTF_DLL} $<TARGET_FILE_DIR:my_sdl_app>
        )
    endif()
endif()

# 查找并添加SDL3依赖的其他DLLs，如zlib, libpng, libwebp, libfreetype, harfbuzz等
function(copy_sdl_dependencies target)
    # 查找SDL3_image的依赖
    if(TARGET SDL3_image::SDL3_image)
        get_target_property(SDL3_IMAGE_IMPORTED_IMPLIB SDL3_image::SDL3_image IMPORTED_IMPLIB)
        if(SDL3_IMAGE_IMPORTED_IMPLIB)
            get_filename_component(SDL3_DEP_DIR ${SDL3_IMAGE_IMPORTED_IMPLIB} DIRECTORY)
            file(GLOB SDL3_IMAGE_DEPS "${SDL3_DEP_DIR}/libpng*.dll" "${SDL3_DEP_DIR}/libwebp*.dll" "${SDL3_DEP_DIR}/libtiff*.dll" "${SDL3_DEP_DIR}/libjpeg*.dll" "${SDL3_DEP_DIR}/zlib*.dll")
            foreach(dll ${SDL3_IMAGE_DEPS})
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${dll} $<TARGET_FILE_DIR:${target}>
                )
            endforeach()
        endif()
    endif()

    # 查找SDL3_ttf的依赖
    if(TARGET SDL3_ttf::SDL3_ttf)
        get_target_property(SDL3_TTF_IMPORTED_IMPLIB SDL3_ttf::SDL3_ttf IMPORTED_IMPLIB)
        if(SDL3_TTF_IMPORTED_IMPLIB)
            get_filename_component(SDL3_DEP_DIR ${SDL3_TTF_IMPORTED_IMPLIB} DIRECTORY)
            file(GLOB SDL3_TTF_DEPS "${SDL3_DEP_DIR}/libfreetype*.dll" "${SDL3_DEP_DIR}/libharfbuzz*.dll" "${SDL3_DEP_DIR}/libbrotlidec*.dll" "${SDL3_DEP_DIR}/libbrotlicommon*.dll")
            foreach(dll ${SDL3_TTF_DEPS})
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${dll} $<TARGET_FILE_DIR:${target}>
                )
            endforeach()
        endif()
    endif()
endfunction()

copy_sdl_dependencies(my_sdl_app)

# 查找MinGW运行时库
if(MINGW)
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-prog-name=cc1
        OUTPUT_VARIABLE MINGW_CC1_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    get_filename_component(MINGW_BIN_DIR ${MINGW_CC1_PATH} DIRECTORY)
    
    find_file(LIBGCC_S_SEH_LIBRARY 
        NAMES libgcc_s_seh-1.dll 
        PATHS ${MINGW_BIN_DIR} ENV PATH
        NO_DEFAULT_PATH
    )
    find_file(LIBGCC_S_DW2_LIBRARY 
        NAMES libgcc_s_dw2-1.dll 
        PATHS ${MINGW_BIN_DIR} ENV PATH
        NO_DEFAULT_PATH
    )
    find_file(LIBWINPTHREAD_LIBRARY 
        NAMES libwinpthread-1.dll 
        PATHS ${MINGW_BIN_DIR} ENV PATH
        NO_DEFAULT_PATH
    )
    find_file(LIBSTDCPP_LIBRARY 
        NAMES libstdc++-6.dll 
        PATHS ${MINGW_BIN_DIR} ENV PATH
        NO_DEFAULT_PATH
    )
    
    # 复制MinGW运行时库到输出目录
    if(LIBGCC_S_SEH_LIBRARY)
        add_custom_command(TARGET my_sdl_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBGCC_S_SEH_LIBRARY} $<TARGET_FILE_DIR:my_sdl_app>
        )
    endif()
    if(LIBGCC_S_DW2_LIBRARY)
        add_custom_command(TARGET my_sdl_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBGCC_S_DW2_LIBRARY} $<TARGET_FILE_DIR:my_sdl_app>
        )
    endif()
    if(LIBWINPTHREAD_LIBRARY)
        add_custom_command(TARGET my_sdl_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBWINPTHREAD_LIBRARY} $<TARGET_FILE_DIR:my_sdl_app>
        )
    endif()
    if(LIBSTDCPP_LIBRARY)
        add_custom_command(TARGET my_sdl_app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${LIBSTDCPP_LIBRARY} $<TARGET_FILE_DIR:my_sdl_app>
        )
    endif()
endif()

# CPack打包配置
set(CPACK_GENERATOR "ZIP")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGE_NAME "MySDLApp")
set(CPACK_PACKAGE_VENDOR "MyCompany")
set(CPACK_PACKAGE_DESCRIPTION "A simple SDL3 application that can run without any programming environment")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_ZIP_STRIP_FILES OFF)

# 包含可执行文件和DLL文件
install(TARGETS my_sdl_app
    RUNTIME DESTINATION .
    CONFIGURATIONS Release RelWithDebInfo
)

# 安装DLL文件（如果存在）
if(TARGET SDL3::SDL3)
    get_target_property(SDL3_DLL SDL3::SDL3 IMPORTED_LOCATION)
    if(SDL3_DLL AND EXISTS ${SDL3_DLL})
        install(FILES ${SDL3_DLL} DESTINATION .)
    endif()
endif()

if(TARGET SDL3_image::SDL3_image)
    get_target_property(SDL3_IMAGE_DLL SDL3_image::SDL3_image IMPORTED_LOCATION)
    if(SDL3_IMAGE_DLL AND EXISTS ${SDL3_IMAGE_DLL})
        install(FILES ${SDL3_IMAGE_DLL} DESTINATION .)
    endif()
endif()

if(TARGET SDL3_ttf::SDL3_ttf)
    get_target_property(SDL3_TTF_DLL SDL3_ttf::SDL3_ttf IMPORTED_LOCATION)
    if(SDL3_TTF_DLL AND EXISTS ${SDL3_TTF_DLL})
        install(FILES ${SDL3_TTF_DLL} DESTINATION .)
    endif()
endif()

# 安装SDL3相关的其他DLL文件
function(install_sdl_dependencies target)
    # 安装SDL3_image的依赖
    if(TARGET SDL3_image::SDL3_image)
        get_target_property(SDL3_IMAGE_IMPORTED_IMPLIB SDL3_image::SDL3_image IMPORTED_IMPLIB)
        if(SDL3_IMAGE_IMPORTED_IMPLIB)
            get_filename_component(SDL3_DEP_DIR ${SDL3_IMAGE_IMPORTED_IMPLIB} DIRECTORY)
            file(GLOB SDL3_IMAGE_DEPS "${SDL3_DEP_DIR}/libpng*.dll" "${SDL3_DEP_DIR}/libwebp*.dll" "${SDL3_DEP_DIR}/libtiff*.dll" "${SDL3_DEP_DIR}/libjpeg*.dll" "${SDL3_DEP_DIR}/zlib*.dll")
            foreach(dll ${SDL3_IMAGE_DEPS})
                install(FILES ${dll} DESTINATION .)
            endforeach()
        endif()
    endif()

    # 安装SDL3_ttf的依赖
    if(TARGET SDL3_ttf::SDL3_ttf)
        get_target_property(SDL3_TTF_IMPORTED_IMPLIB SDL3_ttf::SDL3_ttf IMPORTED_IMPLIB)
        if(SDL3_TTF_IMPORTED_IMPLIB)
            get_filename_component(SDL3_DEP_DIR ${SDL3_TTF_IMPORTED_IMPLIB} DIRECTORY)
            file(GLOB SDL3_TTF_DEPS "${SDL3_DEP_DIR}/libfreetype*.dll" "${SDL3_DEP_DIR}/libharfbuzz*.dll" "${SDL3_DEP_DIR}/libbrotlidec*.dll" "${SDL3_DEP_DIR}/libbrotlicommon*.dll")
            foreach(dll ${SDL3_TTF_DEPS})
                install(FILES ${dll} DESTINATION .)
            endforeach()
        endif()
    endif()
endfunction()

install_sdl_dependencies(my_sdl_app)

# 安装MinGW运行时库
if(MINGW)
    if(LIBGCC_S_SEH_LIBRARY)
        install(FILES ${LIBGCC_S_SEH_LIBRARY} DESTINATION .)
    endif()
    if(LIBGCC_S_DW2_LIBRARY)
        install(FILES ${LIBGCC_S_DW2_LIBRARY} DESTINATION .)
    endif()
    if(LIBWINPTHREAD_LIBRARY)
        install(FILES ${LIBWINPTHREAD_LIBRARY} DESTINATION .)
    endif()
    if(LIBSTDCPP_LIBRARY)
        install(FILES ${LIBSTDCPP_LIBRARY} DESTINATION .)
    endif()
endif()

# 包含资源文件夹
if(EXISTS "${PROJECT_SOURCE_DIR}/data/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/data/" DESTINATION data)
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/font/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/font/" DESTINATION font)
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/image/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/image/" DESTINATION image)
endif()

include(CPack)