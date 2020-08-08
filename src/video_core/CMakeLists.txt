add_library(video_core STATIC
    command_processor.cpp
    command_processor.h
    debug_utils/debug_utils.cpp
    debug_utils/debug_utils.h
    geometry_pipeline.cpp
    geometry_pipeline.h
    gpu_debugger.h
    pica.cpp
    pica.h
    pica_state.h
    pica_types.h
    primitive_assembly.cpp
    primitive_assembly.h
    rasterizer_interface.h
    regs.cpp
    regs.h
    regs_framebuffer.h
    regs_lighting.h
    regs_pipeline.h
    regs_rasterizer.h
    regs_shader.h
    regs_texturing.h
    renderer_base.cpp
    renderer_base.h
    renderer_opengl/frame_dumper_opengl.cpp
    renderer_opengl/frame_dumper_opengl.h
    renderer_opengl/gl_rasterizer.cpp
    renderer_opengl/gl_rasterizer.h
    renderer_opengl/gl_rasterizer_cache.cpp
    renderer_opengl/gl_rasterizer_cache.h
    renderer_opengl/gl_resource_manager.cpp
    renderer_opengl/gl_resource_manager.h
    renderer_opengl/gl_shader_decompiler.cpp
    renderer_opengl/gl_shader_decompiler.h
    renderer_opengl/gl_shader_disk_cache.cpp
    renderer_opengl/gl_shader_disk_cache.h
    renderer_opengl/gl_shader_gen.cpp
    renderer_opengl/gl_shader_gen.h
    renderer_opengl/gl_shader_manager.cpp
    renderer_opengl/gl_shader_manager.h
    renderer_opengl/gl_shader_util.cpp
    renderer_opengl/gl_shader_util.h
    renderer_opengl/gl_state.cpp
    renderer_opengl/gl_state.h
    renderer_opengl/gl_stream_buffer.cpp
    renderer_opengl/gl_stream_buffer.h
    renderer_opengl/gl_surface_params.cpp
    renderer_opengl/gl_surface_params.h
    renderer_opengl/gl_vars.cpp
    renderer_opengl/gl_vars.h
    renderer_opengl/pica_to_gl.h
    renderer_opengl/post_processing_opengl.cpp
    renderer_opengl/post_processing_opengl.h
    renderer_opengl/renderer_opengl.cpp
    renderer_opengl/renderer_opengl.h
    renderer_opengl/texture_filters/anime4k/anime4k_ultrafast.cpp
    renderer_opengl/texture_filters/anime4k/anime4k_ultrafast.h
    renderer_opengl/texture_filters/bicubic/bicubic.cpp
    renderer_opengl/texture_filters/bicubic/bicubic.h
    renderer_opengl/texture_filters/scale_force/scale_force.cpp
    renderer_opengl/texture_filters/scale_force/scale_force.h
    renderer_opengl/texture_filters/texture_filter_base.h
    renderer_opengl/texture_filters/texture_filterer.cpp
    renderer_opengl/texture_filters/texture_filterer.h
    renderer_opengl/texture_filters/xbrz/xbrz_freescale.cpp
    renderer_opengl/texture_filters/xbrz/xbrz_freescale.h
    #temporary, move these back in alphabetical order before merging
    renderer_opengl/gl_format_reinterpreter.cpp
    renderer_opengl/gl_format_reinterpreter.h

    renderer_vulkan/vk_instance.h
    renderer_vulkan/vk_rasterizer_cache.cpp
    renderer_vulkan/vk_rasterizer_cache.h
    renderer_vulkan/vk_convert_and_transfer.cpp
    renderer_vulkan/vk_convert_and_transfer.h
    renderer_vulkan/conversion_shaders/buffer_to_image_conversions.h
    renderer_vulkan/conversion_shaders/image_to_buffer_conversions.h
    renderer_vulkan/conversion_shaders/utils.comp
    renderer_vulkan/conversion_shaders/etc_utils.comp
    renderer_vulkan/vk_shader_manager.cpp
    renderer_vulkan/vk_shader_manager.h
    renderer_vulkan/vk_shader_decompiler.cpp
    renderer_vulkan/vk_shader_decompiler.h
    renderer_vulkan/vk_shader_gen.cpp
    renderer_vulkan/vk_shader_gen.h

    shader/debug_data.h
    shader/shader.cpp
    shader/shader.h
    shader/shader_interpreter.cpp
    shader/shader_interpreter.h
    swrasterizer/clipper.cpp
    swrasterizer/clipper.h
    swrasterizer/framebuffer.cpp
    swrasterizer/framebuffer.h
    swrasterizer/lighting.cpp
    swrasterizer/lighting.h
    swrasterizer/proctex.cpp
    swrasterizer/proctex.h
    swrasterizer/rasterizer.cpp
    swrasterizer/rasterizer.h
    swrasterizer/swrasterizer.cpp
    swrasterizer/swrasterizer.h
    swrasterizer/texturing.cpp
    swrasterizer/texturing.h
    texture/etc1.cpp
    texture/etc1.h
    texture/texture_decode.cpp
    texture/texture_decode.h
    utils.h
    vertex_loader.cpp
    vertex_loader.h
    video_core.cpp
    video_core.h
)

set(SHADER_FILES
    renderer_opengl/texture_filters/anime4k/refine.frag
    renderer_opengl/texture_filters/anime4k/refine.vert
    renderer_opengl/texture_filters/anime4k/x_gradient.frag
    renderer_opengl/texture_filters/anime4k/y_gradient.frag
    renderer_opengl/texture_filters/anime4k/y_gradient.vert
    renderer_opengl/texture_filters/bicubic/bicubic.frag
    renderer_opengl/texture_filters/scale_force/scale_force.frag
    renderer_opengl/texture_filters/tex_coord.vert
    renderer_opengl/texture_filters/xbrz/xbrz_freescale.frag
    renderer_opengl/texture_filters/xbrz/xbrz_freescale.vert
)

include(${CMAKE_CURRENT_SOURCE_DIR}/generate_shaders.cmake)

foreach(shader_file ${SHADER_FILES})
    get_filename_component(shader_file_name ${shader_file} NAME)
    GetShaderHeaderFile(${shader_file_name})
    list(APPEND SHADER_HEADERS ${shader_header_file})
endforeach()

add_custom_target(shaders
    BYPRODUCTS ${SHADER_HEADERS}
    COMMAND cmake -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_shaders.cmake
    SOURCES ${SHADER_FILES}
)
add_dependencies(video_core shaders)

target_include_directories(video_core PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(ARCHITECTURE_x86_64)
    target_sources(video_core
        PRIVATE
            shader/shader_jit_x64.cpp
            shader/shader_jit_x64_compiler.cpp

            shader/shader_jit_x64.h
            shader/shader_jit_x64_compiler.h
    )
endif()

create_target_directory_groups(video_core)

target_link_libraries(video_core PUBLIC common core)
target_link_libraries(video_core PRIVATE glad nihstro-headers Boost::serialization)

if (ARCHITECTURE_x86_64)
    target_link_libraries(video_core PUBLIC xbyak)
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(video_core PRIVATE Vulkan::Vulkan shaderc)
target_include_directories(video_core PRIVATE Vulkan::Vulkan)

set(buffer_to_image_shaders
    rgba8_to_rgba8
    rgb8_to_rgba8
    rgb5a1_to_rgba8
    rgb565_to_rgba8
    rgba4_to_rgba8
    ia8_to_rg8
    rg8_to_rg8
    i8_to_r8
    a8_to_r8
    ia4_to_rg8
    i4_to_r8
    a4_to_r8
    etc1_to_rgba8
    etc1a4_to_rgba8
    d24s8_to_s8
    d24_to_x8d24
    d16_to_d16
)

set(image_to_buffer_shaders
    rgba8_to_rgba8
    rgba8_to_rgb8
    rgba8_to_rgb5a1
    rgba8_to_rgb565
    rgba8_to_rgba4
    s8_to_d24s8
    x8d24_to_d24
    d16_to_d16
)

function(CompileShader shader_name dir)
string(REPLACE "." "_" shader_name ${shader})
set(shader_path ${CMAKE_CURRENT_SOURCE_DIR}/renderer_vulkan/conversion_shaders/${dir}/${shader}.comp)
set(shader_header ${CMAKE_CURRENT_SOURCE_DIR}/renderer_vulkan/conversion_shaders/${dir}/spirv/${shader_name}.h)
add_custom_command(
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    MAIN_DEPENDENCY ${shader_path}
    OUTPUT ${shader_header}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/renderer_vulkan/conversion_shaders/utils.comp ${CMAKE_CURRENT_SOURCE_DIR}/renderer_vulkan/conversion_shaders/etc_utils.comp
    COMMAND glslc
        ${shader_path}
        -o ${shader_header}
        --target-spv=spv1.3
        --target-env=vulkan1.1
        -O
        -mfmt=c
)
set_source_files_properties(${shader_header} PROPERTIES GENERATED TRUE)
target_sources(video_core PRIVATE ${shader_path})
target_sources(video_core PRIVATE ${shader_header})
endfunction()

foreach(shader ${buffer_to_image_shaders})
    CompileShader(shader buffer_to_image)
endforeach()

foreach(shader ${image_to_buffer_shaders})
    CompileShader(shader image_to_buffer)
endforeach()

#add_executable(conversion_test
#    renderer_vulkan/conversion_test/main.cpp
#)

#target_link_libraries(conversion_test PRIVATE video_core Vulkan::Vulkan lodepng glad)
#target_include_directories(conversion_test PRIVATE Vulkan::Vulkan)
