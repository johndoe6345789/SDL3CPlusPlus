
cmake_minimum_required(VERSION 3.24)
option(BUILD_SDL3_APP "Build the SDL3 GPU demo" ON)
option(ENABLE_CLANG_TIDY "Automatically run clang-tidy on every target" OFF)
set(SDL_VERSION "SDL3" CACHE STRING "SDL version to use")
set_property(CACHE SDL_VERSION PROPERTY STRINGS SDL3 sdl)


project(SDL3App LANGUAGES CXX)

# Build type configuration
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warning flags - enforce no macros culture
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wmacro-redefined
        -Wreserved-id-macro
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
        -Wbuiltin-macro-redefined
    )
endif()
if(MSVC)
    add_compile_options(/we4005)  # macro redefinition
endif()

# Conan toolchain
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

# Ninja generator fixes
if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
    if(DEFINED CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_GENERATOR_PLATFORM "" CACHE STRING "" FORCE)
    endif()
    if(DEFINED CMAKE_GENERATOR_TOOLSET)
        set(CMAKE_GENERATOR_TOOLSET "" CACHE STRING "" FORCE)
    endif()
endif()

# Windows Chocolatey paths
list(APPEND CMAKE_PROGRAM_PATH "C:/ProgramData/chocolatey/bin")

# Clang-tidy support
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy.exe HINTS "C:/Program Files/LLVM/bin" "C:/ProgramData/chocolatey/bin")
    if(CLANG_TIDY_EXE)
        if(WIN32)
            set(CLANG_TIDY_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/clang_tidy_wrapper.bat")
            file(WRITE ${CLANG_TIDY_WRAPPER}
                "@echo off\nsetlocal\nset \"CLANG_TIDY_EXE=${CLANG_TIDY_EXE}\"\necho [clang-tidy] starting %*\n\"%CLANG_TIDY_EXE%\" %*\nset RET=%ERRORLEVEL%\necho [clang-tidy] finished (exit %RET%)\nexit /b %RET%\n"
            )
        else()
            set(CLANG_TIDY_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/clang_tidy_wrapper.sh")
            file(WRITE ${CLANG_TIDY_WRAPPER}
                "#!/bin/sh\necho \"[clang-tidy] starting $@\"\n\"${CLANG_TIDY_EXE}\" \"$@\"\nret=$?\necho \"[clang-tidy] finished (exit $ret)\"\nexit $ret\n"
            )
            execute_process(COMMAND ${CMAKE_COMMAND} -E chmod +x ${CLANG_TIDY_WRAPPER})
        endif()
        set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CLANG_TIDY_WRAPPER})
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_WRAPPER}")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# SDL selection


if(SDL_VERSION STREQUAL "SDL3")
    find_package(SDL3 CONFIG REQUIRED)
    set(SDL_TARGET SDL3::SDL3)
elseif(SDL_VERSION STREQUAL "sdl")
    find_package(SDL CONFIG REQUIRED)
    set(SDL_TARGET SDL::SDL)
else()
    message(FATAL_ERROR "Invalid SDL_VERSION: ${SDL_VERSION}")
endif()


# Find all required packages

find_package(RapidJSON CONFIG REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

find_package(Bullet CONFIG REQUIRED)

find_package(glm CONFIG REQUIRED)

find_package(stb CONFIG REQUIRED)

find_package(EnTT CONFIG REQUIRED)

find_package(assimp CONFIG REQUIRED)

find_package(libzip CONFIG REQUIRED)

find_package(spdlog CONFIG REQUIRED)

find_package(GTest CONFIG REQUIRED)

find_package(ZLIB CONFIG REQUIRED)


# Build render stack library group
set(SDL3CPP_RENDER_STACK_LIBS EnTT::EnTT)

# Freetype library group
set(SDL3CPP_FREETYPE_LIBS)
if(TARGET Freetype::Freetype)
    list(APPEND SDL3CPP_FREETYPE_LIBS Freetype::Freetype)
elseif(TARGET freetype::freetype)
    list(APPEND SDL3CPP_FREETYPE_LIBS freetype::freetype)
endif()

# libzip library group
set(SDL3CPP_ZIP_LIBS)
if(TARGET libzip::zip)
    list(APPEND SDL3CPP_ZIP_LIBS libzip::zip)
elseif(TARGET libzip::libzip)
    list(APPEND SDL3CPP_ZIP_LIBS libzip::libzip)
endif()

# stb library group
set(SDL3CPP_STB_LIBS)
if(TARGET stb::stb_image)
    list(APPEND SDL3CPP_STB_LIBS stb::stb_image)
elseif(TARGET stb::stb)
    list(APPEND SDL3CPP_STB_LIBS stb::stb)
elseif(TARGET stb)
    list(APPEND SDL3CPP_STB_LIBS stb)
endif()

# ============================================================================
# SOURCE FILE GROUPINGS
# ============================================================================
# Auto-discover source files from key directories using file(GLOB)

# JSON config service sources
file(GLOB JSON_CONFIG_SOURCES src/config/json_config*.cpp)

# Workflow service sources (all workflow steps in main workflow directory)
file(GLOB WORKFLOW_SOURCES src/services/impl/workflow/*.cpp)
# Remove problematic files with missing dependencies or incomplete implementations
list(REMOVE_ITEM WORKFLOW_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/"
)

# Frame-specific workflow steps
file(GLOB FRAME_WORKFLOW_SOURCES src/services/impl/workflow/frame/*.cpp)
# Apply same exclusions to frame sources
list(REMOVE_ITEM FRAME_WORKFLOW_SOURCES
)

# Generic workflow steps (atomic, reusable steps)
file(GLOB GENERIC_WORKFLOW_SOURCES src/services/impl/workflow/generic_steps/*.cpp)

# ============================================================================
# EXECUTABLE TARGETS
# ============================================================================
if(BUILD_SDL3_APP)
    add_executable(sdl3_app
        src/main.cpp
        src/services/impl/diagnostics/logger_service_levels.cpp
        src/services/impl/diagnostics/logger_service_construct.cpp
        src/services/impl/diagnostics/logger_service_config.cpp
        src/services/impl/diagnostics/logger_service_log.cpp
        src/services/impl/diagnostics/logger_service_trace_variable.cpp
        src/services/impl/app/cli_env_override.cpp
        src/services/impl/app/app_cli_args.cpp
        src/services/impl/app/app_require_package.cpp
        src/services/impl/app/app_logger_init.cpp
        src/services/impl/app/app_workflow_runtime.cpp
        src/services/impl/app/app_package_config.cpp
        src/services/impl/app/app_context_populate.cpp
        src/services/impl/workflow/workflow_executor.cpp
        src/services/impl/workflow/workflow_step_registry.cpp
        src/services/impl/workflow/workflow_exit_step.cpp
        src/services/impl/workflow/workflow_parameter_reader.cpp
        src/services/impl/workflow/workflow_parameter_node_reader.cpp
        src/services/impl/workflow/workflow_parameter_value_parser.cpp
        src/services/impl/workflow/workflow_parameter_array_parser.cpp
        src/services/impl/workflow/workflow_step_io_resolver.cpp
        src/services/impl/workflow/workflow_step_parameter_resolver.cpp
        src/services/impl/workflow/workflow_step_parameter_resolver_scalars.cpp
        src/services/impl/workflow/workflow_step_parameter_resolver_lists.cpp
        src/services/impl/workflow/workflow_registrar.cpp
        src/services/impl/workflow/workflow_registrar_graphics_init.cpp
        src/services/impl/workflow/workflow_registrar_rendering_core.cpp
        src/services/impl/workflow/workflow_registrar_rendering_bsp.cpp
        src/services/impl/workflow/workflow_registrar_rendering_postfx.cpp
        src/services/impl/workflow/workflow_registrar_rendering_overlay.cpp
        src/services/impl/workflow/workflow_registrar_rendering_q3hud.cpp
        src/services/impl/workflow/workflow_registrar_rendering_q3model.cpp
        src/services/impl/workflow/workflow_registrar_rendering_q3pmove.cpp
        src/services/impl/workflow/workflow_registrar_rendering_q3combat.cpp
        src/services/impl/workflow/workflow_registrar_graphics_misc.cpp
        src/services/impl/workflow/workflow_registrar_camera.cpp
        src/services/impl/workflow/workflow_registrar_physics.cpp
        src/services/impl/workflow/workflow_registrar_input.cpp
        src/services/impl/workflow/workflow_registrar_audio.cpp
        src/services/impl/workflow/workflow_registrar_control.cpp
        src/services/impl/workflow/workflow_registrar_dataops_core.cpp
        src/services/impl/workflow/workflow_registrar_list_number.cpp
        src/services/impl/workflow/workflow_registrar_particle_string_value_vfx.cpp
        src/services/impl/workflow/workflow_registrar_model.cpp
        src/services/impl/workflow/workflow_registrar_scene.cpp
        src/services/impl/workflow/workflow_registrar_camera_service.cpp
        src/services/impl/workflow/workflow_registrar_system_data.cpp
        src/services/impl/workflow/workflow_registrar_media.cpp
        src/services/impl/workflow/workflow_registrar_shader.cpp
        src/services/impl/workflow/workflow_registrar_exit.cpp
        src/services/impl/workflow/workflow_sdl_init_step.cpp
        src/services/impl/workflow/workflow_sdl_window_create_step.cpp
        src/services/impl/workflow/workflow_app_init_step.cpp
        src/services/impl/workflow/workflow_load_workflow_step.cpp
        src/services/impl/workflow/workflow_definition_parser.cpp
        src/services/impl/workflow/workflow_definition_parser_steps.cpp
        src/services/impl/workflow/workflow_definition_parser_nodes.cpp
        src/services/impl/workflow/workflow_definition_parser_node_parse.cpp
        src/services/impl/workflow/workflow_definition_parser_node_sort.cpp
        src/services/impl/workflow/workflow_definition_parser_variables.cpp
        src/services/impl/workflow/workflow_connection_reader.cpp
        src/services/impl/workflow/workflow_connection_main_parser.cpp
        src/services/impl/workflow/workflow_node_topo_sorter.cpp
        src/services/impl/workflow/workflow_connection_graph_builder.cpp
        src/services/impl/workflow/workflow_kahn_topo_sort.cpp
        src/services/impl/workflow/workflow_execute_step.cpp
        src/services/impl/workflow/workflow_execute_helpers.cpp
        src/stb_image.cpp
        src/services/impl/workflow/workflow_generic_steps/audio_play_input_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/camera_fps_eye_position.cpp
        src/services/impl/workflow/workflow_generic_steps/camera_fps_params.cpp
        src/services/impl/workflow/workflow_generic_steps/camera_fps_update_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/camera_fps_yaw_pitch.cpp
        src/services/impl/workflow/workflow_generic_steps/camera_matrix_builder.cpp
        src/services/impl/workflow/workflow_generic_steps/camera_set_pose_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/control_if_else_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/debug_log_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/input_event_drain.cpp
        src/services/impl/workflow/workflow_generic_steps/input_keyboard_snapshot.cpp
        src/services/impl/workflow/workflow_generic_steps/metric_parse.cpp
        src/services/impl/workflow/workflow_generic_steps/metric_store.cpp
        src/services/impl/workflow/workflow_generic_steps/model_input_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/model_spawn_input_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/particle_update_helpers.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_body_builder.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_body_params.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_fps_input.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_fps_recovery.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_fps_step_probe.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_fps_step_up.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_fps_velocity_integration.cpp
        src/services/impl/workflow/workflow_generic_steps/physics_fps_wish_velocity.cpp
        src/services/impl/workflow/workflow_generic_steps/string_template.cpp
        src/services/impl/workflow/workflow_generic_steps/switch_value_matcher.cpp
        src/services/impl/workflow/workflow_generic_steps/try_catch_dispatch.cpp
        src/services/impl/workflow/workflow_generic_steps/vfx_destroy_all_or_target.cpp
        src/services/impl/workflow/workflow_generic_steps/vfx_destroy_by_id.cpp
        src/services/impl/workflow/workflow_generic_steps/vfx_destroy_selection.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_array_append_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_array_create_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_audio_pause_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_audio_play_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_audio_resume_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_audio_seek_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_audio_set_looping_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_audio_set_volume_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_audio_stop_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_bool_and_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_bool_not_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_bool_or_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_camera_build_view_state_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_camera_fps_update_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_camera_look_at_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_camera_set_fov_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_camera_set_pose_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_camera_setup_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_camera_teleport_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_compare_eq_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_compare_gt_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_compare_gte_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_compare_lt_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_compare_lte_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_compare_ne_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_control_for_each_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_control_if_else_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_control_switch_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_control_while_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_debug_log_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_debug_metrics_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_input_gamepad_axis_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_input_gamepad_button_pressed_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_input_key_pressed_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_input_mouse_button_pressed_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_input_mouse_grab_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_input_mouse_position_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_input_poll_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_append_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_concat_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_count_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_filter_equals_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_filter_gt_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_literal_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_map_add_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_map_mul_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_reduce_max_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_reduce_min_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_list_reduce_sum_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_mesh_load_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_model_despawn_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_model_set_transform_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_model_spawn_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_abs_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_add_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_clamp_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_div_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_max_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_min_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_mul_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_round_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_sub_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_package_loader.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_particle_emit_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_particle_update_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_physics_body_add_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_physics_fps_move_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_physics_step_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_physics_sync_transforms_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_physics_world_create_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_concat_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_contains_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_equals_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_format_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_join_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_lower_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_replace_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_split_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_trim_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_string_upper_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_try_catch_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_value_assert_exists_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_value_assert_type_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_value_clear_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_value_copy_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_value_default_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_value_literal_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_value_set_if_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_variable_get_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_variable_set_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_vfx_destroy_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_vfx_spawn_step.cpp
        src/services/impl/workflow/geometry/geometry_create_cube_helpers.cpp
        src/services/impl/workflow/geometry/geometry_cube_generate_helpers.cpp
        src/services/impl/workflow/geometry/geometry_plane_buffer_create.cpp
        src/services/impl/workflow/geometry/geometry_plane_mesh_build.cpp
        src/services/impl/workflow/geometry/geometry_plane_params.cpp
        src/services/impl/workflow/geometry/geometry_plane_upload.cpp
        src/services/impl/workflow/geometry/workflow_geometry_create_cube_step.cpp
        src/services/impl/workflow/geometry/workflow_geometry_create_plane_step.cpp
        src/services/impl/workflow/geometry/workflow_geometry_cube_generate_step.cpp
        src/services/impl/workflow/rendering/bloom_ping_pong.cpp
        src/services/impl/workflow/rendering/bloom_ping_pong_textures.cpp
        src/services/impl/workflow/rendering/box_face_builder.cpp
        src/services/impl/workflow/rendering/box_face_context.cpp
        src/services/impl/workflow/rendering/box_face_draw.cpp
        src/services/impl/workflow/rendering/box_face_geometry.cpp
        src/services/impl/workflow/rendering/box_face_resources.cpp
        src/services/impl/workflow/rendering/bsp_brush_classify.cpp
        src/services/impl/workflow/rendering/bsp_brush_collision_shapes.cpp
        src/services/impl/workflow/rendering/bsp_brush_convex_shape.cpp
        src/services/impl/workflow/rendering/bsp_brush_lump_view.cpp
        src/services/impl/workflow/rendering/bsp_brush_vertex_hull.cpp
        src/services/impl/workflow/rendering/bsp_collision_body.cpp
        src/services/impl/workflow/rendering/bsp_entity_classification.cpp
        src/services/impl/workflow/rendering/bsp_entity_enrich.cpp
        src/services/impl/workflow/rendering/bsp_entity_json_builder.cpp
        src/services/impl/workflow/rendering/bsp_entity_lexer.cpp
        src/services/impl/workflow/rendering/bsp_entity_lump_parser.cpp
        src/services/impl/workflow/rendering/bsp_entity_origin.cpp
        src/services/impl/workflow/rendering/bsp_entity_pickup.cpp
        src/services/impl/workflow/rendering/bsp_entity_trigger.cpp
        src/services/impl/workflow/rendering/bsp_entity_trigger_geometry.cpp
        src/services/impl/workflow/rendering/bsp_face_visibility.cpp
        src/services/impl/workflow/rendering/bsp_geometry_lumps.cpp
        src/services/impl/workflow/rendering/bsp_geometry_upload.cpp
        src/services/impl/workflow/rendering/bsp_lightmap_atlas_build.cpp
        src/services/impl/workflow/rendering/bsp_lightmap_atlas_upload.cpp
        src/services/impl/workflow/rendering/bsp_map_geometry_textures.cpp
        src/services/impl/workflow/rendering/bsp_model_lump.cpp
        src/services/impl/workflow/rendering/bsp_patch_bezier_basis.cpp
        src/services/impl/workflow/rendering/bsp_patch_face_builder.cpp
        src/services/impl/workflow/rendering/bsp_patch_lightmap_uv.cpp
        src/services/impl/workflow/rendering/bsp_patch_sample.cpp
        src/services/impl/workflow/rendering/bsp_patch_tessellator.cpp
        src/services/impl/workflow/rendering/bsp_pmove_brush_model.cpp
        src/services/impl/workflow/rendering/bsp_polygon_face_builder.cpp
        src/services/impl/workflow/rendering/bsp_portal_destination.cpp
        src/services/impl/workflow/rendering/bsp_portal_view_draw.cpp
        src/services/impl/workflow/rendering/bsp_portal_view_targets.cpp
        src/services/impl/workflow/rendering/bsp_portal_view_uniforms.cpp
        src/services/impl/workflow/rendering/bsp_q3_coordinates.cpp
        src/services/impl/workflow/rendering/bsp_shader_script.cpp
        src/services/impl/workflow/rendering/bsp_shader_script_parse.cpp
        src/services/impl/workflow/rendering/bsp_texture_batch_loader.cpp
        src/services/impl/workflow/rendering/bsp_texture_group_flattener.cpp
        src/services/impl/workflow/rendering/bsp_texture_groups_context.cpp
        src/services/impl/workflow/rendering/bsp_texture_mip_upload.cpp
        src/services/impl/workflow/rendering/bsp_texture_pk3_load.cpp
        src/services/impl/workflow/rendering/bsp_texture_white.cpp
        src/services/impl/workflow/rendering/draw_bsp_map_geometry.cpp
        src/services/impl/workflow/rendering/draw_legacy_map_geometry.cpp
        src/services/impl/workflow/rendering/draw_map_texture_config.cpp
        src/services/impl/workflow/rendering/draw_map_vertex_uniforms.cpp
        src/services/impl/workflow/rendering/draw_textured_facing_transform.cpp
        src/services/impl/workflow/rendering/draw_textured_free_transform.cpp
        src/services/impl/workflow/rendering/draw_textured_params.cpp
        src/services/impl/workflow/rendering/draw_textured_resources.cpp
        src/services/impl/workflow/rendering/draw_textured_transform.cpp
        src/services/impl/workflow/rendering/draw_textured_uniforms.cpp
        src/services/impl/workflow/rendering/flashlight_mesh_build.cpp
        src/services/impl/workflow/rendering/flashlight_mesh_primitives.cpp
        src/services/impl/workflow/rendering/flashlight_mesh_upload.cpp
        src/services/impl/workflow/rendering/fps_meter.cpp
        src/services/impl/workflow/rendering/frame_begin_shared_helpers.cpp
        src/services/impl/workflow/rendering/frame_clear_params.cpp
        src/services/impl/workflow/rendering/frame_draw_bodies_helpers.cpp
        src/services/impl/workflow/rendering/frame_render_scale.cpp
        src/services/impl/workflow/rendering/frame_swapchain_acquire.cpp
        src/services/impl/workflow/rendering/gpu_overlay_quad.cpp
        src/services/impl/workflow/rendering/gpu_text_overlay_buffer_factory.cpp
        src/services/impl/workflow/rendering/gpu_text_overlay_pipeline_factory.cpp
        src/services/impl/workflow/rendering/gpu_text_overlay_resources.cpp
        src/services/impl/workflow/rendering/gpu_text_overlay_resources_destroy.cpp
        src/services/impl/workflow/rendering/gpu_text_overlay_shader_loader.cpp
        src/services/impl/workflow/rendering/gpu_text_overlay_surface_factory.cpp
        src/services/impl/workflow/rendering/gpu_text_overlay_upload.cpp
        src/services/impl/workflow/rendering/grid_draw_config.cpp
        src/services/impl/workflow/rendering/grid_draw_cubes.cpp
        src/services/impl/workflow/rendering/grid_gpu_resources.cpp
        src/services/impl/workflow/rendering/grid_render_pass.cpp
        src/services/impl/workflow/rendering/grid_setup_gpu_resources.cpp
        src/services/impl/workflow/rendering/grid_setup_params.cpp
        src/services/impl/workflow/rendering/legacy_mesh_normal.cpp
        src/services/impl/workflow/rendering/map_load_params.cpp
        src/services/impl/workflow/rendering/map_mesh_geometry.cpp
        src/services/impl/workflow/rendering/map_mesh_gpu_upload.cpp
        src/services/impl/workflow/rendering/map_mesh_physics.cpp
        src/services/impl/workflow/rendering/map_scene_node_processor.cpp
        src/services/impl/workflow/rendering/model_load_helpers.cpp
        src/services/impl/workflow/rendering/model_load_params.cpp
        src/services/impl/workflow/rendering/overlay_arenas_txt_parser.cpp
        src/services/impl/workflow/rendering/overlay_head_portrait.cpp
        src/services/impl/workflow/rendering/overlay_head_quad_upload.cpp
        src/services/impl/workflow/rendering/overlay_head_sampler.cpp
        src/services/impl/workflow/rendering/overlay_sw_begin_destroy.cpp
        src/services/impl/workflow/rendering/overlay_sw_begin_load.cpp
        src/services/impl/workflow/rendering/overlay_sw_begin_publish.cpp
        src/services/impl/workflow/rendering/overlay_sw_end_binary_load.cpp
        src/services/impl/workflow/rendering/overlay_sw_end_gpu_buffers.cpp
        src/services/impl/workflow/rendering/overlay_sw_end_pipeline.cpp
        src/services/impl/workflow/rendering/overlay_sw_end_resources.cpp
        src/services/impl/workflow/rendering/overlay_sw_end_shader_paths.cpp
        src/services/impl/workflow/rendering/overlay_sw_end_shader_stage.cpp
        src/services/impl/workflow/rendering/pk3_bsp_loader.cpp
        src/services/impl/workflow/rendering/postfx_bloom_blur_helpers.cpp
        src/services/impl/workflow/rendering/postfx_composite_pass.cpp
        src/services/impl/workflow/rendering/postfx_samplers.cpp
        src/services/impl/workflow/rendering/postfx_ssao_resources.cpp
        src/services/impl/workflow/rendering/postfx_taa_history.cpp
        src/services/impl/workflow/rendering/postfx_taa_jitter.cpp
        src/services/impl/workflow/rendering/postfx_taa_pipeline.cpp
        src/services/impl/workflow/rendering/postfx_taa_resolve.cpp
        src/services/impl/workflow/rendering/render_prepare_camera.cpp
        src/services/impl/workflow/rendering/render_prepare_helpers.cpp
        src/services/impl/workflow/rendering/render_prepare_shadow.cpp
        src/services/impl/workflow/rendering/shadow_caster_draw.cpp
        src/services/impl/workflow/rendering/shadow_face_rotations.cpp
        src/services/impl/workflow/rendering/shadow_map_resources.cpp
        src/services/impl/workflow/rendering/spotlight_pose_camera.cpp
        src/services/impl/workflow/rendering/spotlight_pose_viewmodel.cpp
        src/services/impl/workflow/rendering/spotlight_pose_world.cpp
        src/services/impl/workflow/rendering/spotlight_setup_helpers.cpp
        src/services/impl/workflow/rendering/spotlight_update_pose.cpp
        src/services/impl/workflow/rendering/viewmodel_draw_mesh.cpp
        src/services/impl/workflow/rendering/viewmodel_draw_params.cpp
        src/services/impl/workflow/rendering/viewmodel_draw_texture.cpp
        src/services/impl/workflow/rendering/viewmodel_draw_uniforms.cpp
        src/services/impl/workflow/rendering/viewmodel_transform.cpp
        src/services/impl/workflow/rendering/workflow_bsp_build_collision_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_build_polygons_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_entity_update_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_extract_textures_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_flatten_geometry_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_lightmap_atlas_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_load_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_parse_spawn_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_portal_view_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_tessellate_patches_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_upload_geometry_step.cpp
        src/services/impl/workflow/rendering/workflow_debug_screenshot_step.cpp
        src/services/impl/workflow/rendering/workflow_draw_map_step.cpp
        src/services/impl/workflow/rendering/workflow_draw_textured_box_step.cpp
        src/services/impl/workflow/rendering/workflow_draw_textured_step.cpp
        src/services/impl/workflow/rendering/workflow_draw_viewmodel_step.cpp
        src/services/impl/workflow/rendering/workflow_frame_begin_gpu_step.cpp
        src/services/impl/workflow/rendering/workflow_frame_begin_offscreen_step.cpp
        src/services/impl/workflow/rendering/workflow_frame_draw_bodies_step.cpp
        src/services/impl/workflow/rendering/workflow_frame_end_gpu_step.cpp
        src/services/impl/workflow/rendering/workflow_frame_end_scene_step.cpp
        src/services/impl/workflow/rendering/workflow_geometry_create_flashlight_step.cpp
        src/services/impl/workflow/rendering/workflow_lighting_setup_step.cpp
        src/services/impl/workflow/rendering/workflow_map_load_step.cpp
        src/services/impl/workflow/rendering/workflow_model_load_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_fps_draw_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_fps_init_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_fps_upload_quad_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_fps_upload_text_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_begin_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_end_blit_head_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_end_draw_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_end_init_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_end_screenshot_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_end_upload_quad_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_end_upload_surface_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_bloom_blur_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_bloom_extract_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_composite_draw_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_composite_finish_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_overlay_fps_draw_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_overlay_fps_init_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_overlay_fps_upload_quad_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_overlay_fps_upload_text_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_setup_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_ssao_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_taa_step.cpp
        src/services/impl/workflow/rendering/workflow_render_grid_draw_step.cpp
        src/services/impl/workflow/rendering/workflow_render_grid_setup_step.cpp
        src/services/impl/workflow/rendering/workflow_render_prepare_step.cpp
        src/services/impl/workflow/rendering/workflow_shadow_pass_step.cpp
        src/services/impl/workflow/rendering/workflow_shadow_setup_step.cpp
        src/services/impl/workflow/rendering/workflow_spawn_apply_step.cpp
        src/services/impl/workflow/rendering/workflow_spotlight_setup_step.cpp
        src/services/impl/workflow/rendering/workflow_spotlight_update_step.cpp
        src/services/impl/workflow/graphics/frame_begin_describe.cpp
        src/services/impl/workflow/graphics/frame_clear_color.cpp
        src/services/impl/workflow/graphics/frame_depth_texture.cpp
        src/services/impl/workflow/graphics/frame_render_pass.cpp
        src/services/impl/workflow/graphics/frame_swapchain.cpp
        src/services/impl/workflow/graphics/gpu_device_create.cpp
        src/services/impl/workflow/graphics/gpu_device_window.cpp
        src/services/impl/workflow/graphics/gpu_framebuffer_readback.cpp
        src/services/impl/workflow/graphics/gpu_pipeline_create_info.cpp
        src/services/impl/workflow/graphics/gpu_pipeline_formats.cpp
        src/services/impl/workflow/graphics/gpu_pipeline_params.cpp
        src/services/impl/workflow/graphics/gpu_pipeline_shaders.cpp
        src/services/impl/workflow/graphics/gpu_pipeline_vertex_layout.cpp
        src/services/impl/workflow/graphics/gpu_present_mode.cpp
        src/services/impl/workflow/graphics/gpu_readback_blit.cpp
        src/services/impl/workflow/graphics/gpu_readback_keys.cpp
        src/services/impl/workflow/graphics/gpu_swapchain_capture.cpp
        src/services/impl/workflow/graphics/graphics_buffer_create_vertex_helpers.cpp
        src/services/impl/workflow/graphics/graphics_buffer_json_readers.cpp
        src/services/impl/workflow/graphics/graphics_buffer_upload_params.cpp
        src/services/impl/workflow/graphics/graphics_gpu_buffer_create.cpp
        src/services/impl/workflow/graphics/graphics_gpu_buffer_transfer.cpp
        src/services/impl/workflow/graphics/graphics_gpu_buffer_upload.cpp
        src/services/impl/workflow/graphics/graphics_index_buffer_upload.cpp
        src/services/impl/workflow/graphics/graphics_screenshot_path.cpp
        src/services/impl/workflow/graphics/graphics_screenshot_request_helpers.cpp
        src/services/impl/workflow/graphics/shader_binary_io.cpp
        src/services/impl/workflow/graphics/shader_compile_params.cpp
        src/services/impl/workflow/graphics/shader_format_detection.cpp
        src/services/impl/workflow/graphics/shader_gpu_compile.cpp
        src/services/impl/workflow/graphics/texture_gpu_transfer.cpp
        src/services/impl/workflow/graphics/texture_gpu_upload.cpp
        src/services/impl/workflow/graphics/texture_image_io.cpp
        src/services/impl/workflow/graphics/texture_load_sampler.cpp
        src/services/impl/workflow/graphics/workflow_gpu_command_buffer_submit_step.cpp
        src/services/impl/workflow/graphics/workflow_gpu_pipeline_create_step.cpp
        src/services/impl/workflow/graphics/workflow_gpu_screenshot_capture_step.cpp
        src/services/impl/workflow/graphics/workflow_gpu_shader_compile_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_buffer_create_index_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_buffer_create_vertex_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_buffer_upload_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_draw_submit_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_frame_begin_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_frame_end_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_framebuffer_readback_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_gpu_init_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_init_renderer_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_init_viewport_step.cpp
        src/services/impl/workflow/graphics/workflow_graphics_screenshot_request_step.cpp
        src/services/impl/workflow/graphics/workflow_texture_load_step.cpp
        src/services/impl/workflow/input/gamepad_state_read.cpp
        src/services/impl/workflow/input/input_aggregation_config.cpp
        src/services/impl/workflow/input/input_axis_combine_write.cpp
        src/services/impl/workflow/input/input_axis_deadzone.cpp
        src/services/impl/workflow/input/input_axis_source_value.cpp
        src/services/impl/workflow/input/input_button_combine_config.cpp
        src/services/impl/workflow/input/input_button_combine_helpers.cpp
        src/services/impl/workflow/input/input_button_combine_sources.cpp
        src/services/impl/workflow/input/input_button_gamepad_source.cpp
        src/services/impl/workflow/input/workflow_input_axis_combine_step.cpp
        src/services/impl/workflow/input/workflow_input_button_combine_step.cpp
        src/services/impl/workflow/input/workflow_input_gamepad_poll_step.cpp
        src/services/impl/workflow/input/workflow_input_keyboard_poll_step.cpp
        src/services/impl/workflow/input/workflow_input_mouse_poll_step.cpp
        src/services/impl/workflow/scene/scene_uuid.cpp
        src/services/impl/workflow/scene/workflow_scene_add_geometry_step.cpp
        src/services/impl/workflow/scene/workflow_scene_clear_step.cpp
        src/services/impl/workflow/scene/workflow_scene_create_step.cpp
        src/services/impl/workflow/scene/workflow_scene_get_bounds_step.cpp
        src/services/impl/workflow/scene/workflow_scene_load_step.cpp
        src/services/impl/workflow/scene/workflow_scene_remove_geometry_step.cpp
        src/services/impl/workflow/scene/workflow_scene_set_active_step.cpp
        src/services/impl/workflow/scene/workflow_scene_update_step.cpp
        src/services/impl/workflow/compute/compute_pipeline_create.cpp
        src/services/impl/workflow/compute/compute_pipeline_params.cpp
        src/services/impl/workflow/compute/compute_shader_binary.cpp
        src/services/impl/workflow/compute/compute_shader_format.cpp
        src/services/impl/workflow/compute/compute_tessellate_grid_alloc.cpp
        src/services/impl/workflow/compute/compute_tessellate_grid_create.cpp
        src/services/impl/workflow/compute/compute_tessellate_grid_dispatch.cpp
        src/services/impl/workflow/compute/compute_tessellate_grid_params.cpp
        src/services/impl/workflow/compute/compute_tessellate_grid_publish.cpp
        src/services/impl/workflow/compute/compute_tessellate_grid_upload.cpp
        src/services/impl/workflow/compute/workflow_compute_pipeline_create_step.cpp
        src/services/impl/workflow/compute/workflow_compute_tessellate_dispatch_step.cpp
        src/services/impl/workflow/compute/workflow_compute_tessellate_step.cpp
        src/services/impl/workflow/quake3/q3_anim_cfg.cpp
        src/services/impl/workflow/quake3/q3_axes.cpp
        src/services/impl/workflow/quake3/q3_axes_model.cpp
        src/services/impl/workflow/quake3/q3_bot_model_part_draw.cpp
        src/services/impl/workflow/quake3/q3_bot_model_render.cpp
        src/services/impl/workflow/quake3/q3_bot_model_tag_lookup.cpp
        src/services/impl/workflow/quake3/q3_bot_mover.cpp
        src/services/impl/workflow/quake3/q3_bot_update_animation.cpp
        src/services/impl/workflow/quake3/q3_bot_update_kinematics.cpp
        src/services/impl/workflow/quake3/q3_bot_update_movement.cpp
        src/services/impl/workflow/quake3/q3_bot_update_params.cpp
        src/services/impl/workflow/quake3/q3_bot_update_sensing.cpp
        src/services/impl/workflow/quake3/q3_bot_usercmd.cpp
        src/services/impl/workflow/quake3/q3_brush_collision.cpp
        src/services/impl/workflow/quake3/q3_brush_trace.cpp
        src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp
        src/services/impl/workflow/quake3/q3_entity_field_parsers.cpp
        src/services/impl/workflow/quake3/q3_hud_draw.cpp
        src/services/impl/workflow/quake3/q3_hud_head_draw.cpp
        src/services/impl/workflow/quake3/q3_hud_head_render.cpp
        src/services/impl/workflow/quake3/q3_hud_head_sway.cpp
        src/services/impl/workflow/quake3/q3_hud_head_targets.cpp
        src/services/impl/workflow/quake3/q3_item_models.cpp
        src/services/impl/workflow/quake3/q3_item_transform.cpp
        src/services/impl/workflow/quake3/q3_mapselect_draw.cpp
        src/services/impl/workflow/quake3/q3_mapselect_info.cpp
        src/services/impl/workflow/quake3/q3_mapselect_levelshot.cpp
        src/services/impl/workflow/quake3/q3_mapselect_nav.cpp
        src/services/impl/workflow/quake3/q3_md3_anim_frame.cpp
        src/services/impl/workflow/quake3/q3_md3_buffer_upload.cpp
        src/services/impl/workflow/quake3/q3_md3_draw_params.cpp
        src/services/impl/workflow/quake3/q3_md3_draw_surfaces.cpp
        src/services/impl/workflow/quake3/q3_md3_gpu_upload.cpp
        src/services/impl/workflow/quake3/q3_md3_model_matrix.cpp
        src/services/impl/workflow/quake3/q3_md3_shader_images.cpp
        src/services/impl/workflow/quake3/q3_md3_source.cpp
        src/services/impl/workflow/quake3/q3_md3_surface_geometry.cpp
        src/services/impl/workflow/quake3/q3_md3_surface_texture.cpp
        src/services/impl/workflow/quake3/q3_md3_surface_uploader.cpp
        src/services/impl/workflow/quake3/q3_md3_tag_matrix.cpp
        src/services/impl/workflow/quake3/q3_md3_tags.cpp
        src/services/impl/workflow/quake3/q3_md3_texture_candidates.cpp
        src/services/impl/workflow/quake3/q3_md3_texture_upload.cpp
        src/services/impl/workflow/quake3/q3_menu_config.cpp
        src/services/impl/workflow/quake3/q3_menu_navigation.cpp
        src/services/impl/workflow/quake3/q3_menu_toggle.cpp
        src/services/impl/workflow/quake3/q3_missiles_impact_apply.cpp
        src/services/impl/workflow/quake3/q3_missiles_impact_detect.cpp
        src/services/impl/workflow/quake3/q3_mover_init.cpp
        src/services/impl/workflow/quake3/q3_mover_kinematics.cpp
        src/services/impl/workflow/quake3/q3_music_helpers.cpp
        src/services/impl/workflow/quake3/q3_nav_aabb.cpp
        src/services/impl/workflow/quake3/q3_nav_connect.cpp
        src/services/impl/workflow/quake3/q3_nav_nearest.cpp
        src/services/impl/workflow/quake3/q3_nav_pathfinding.cpp
        src/services/impl/workflow/quake3/q3_nav_sample.cpp
        src/services/impl/workflow/quake3/q3_overlay_scale.cpp
        src/services/impl/workflow/quake3/q3_pickup_apply.cpp
        src/services/impl/workflow/quake3/q3_pickup_can_grab.cpp
        src/services/impl/workflow/quake3/q3_pickup_classify.cpp
        src/services/impl/workflow/quake3/q3_pickup_effects.cpp
        src/services/impl/workflow/quake3/q3_pickup_entities_draw.cpp
        src/services/impl/workflow/quake3/q3_pickup_entity_draw.cpp
        src/services/impl/workflow/quake3/q3_pickup_lookup_tables.cpp
        src/services/impl/workflow/quake3/q3_pickup_position.cpp
        src/services/impl/workflow/quake3/q3_pk3_reader.cpp
        src/services/impl/workflow/quake3/q3_player_position.cpp
        src/services/impl/workflow/quake3/q3_pm_slide_move_helpers.cpp
        src/services/impl/workflow/quake3/q3_pm_tuning.cpp
        src/services/impl/workflow/quake3/q3_pmove.cpp
        src/services/impl/workflow/quake3/q3_pmove_accel.cpp
        src/services/impl/workflow/quake3/q3_pmove_ground.cpp
        src/services/impl/workflow/quake3/q3_skin_file.cpp
        src/services/impl/workflow/quake3/q3_sky_cloud_uv.cpp
        src/services/impl/workflow/quake3/q3_sky_dome.cpp
        src/services/impl/workflow/quake3/q3_sky_dome_upload.cpp
        src/services/impl/workflow/quake3/q3_sky_resources.cpp
        src/services/impl/workflow/quake3/q3_sky_scroll.cpp
        src/services/impl/workflow/quake3/q3_sky_texture_name.cpp
        src/services/impl/workflow/quake3/q3_sky_uniforms.cpp
        src/services/impl/workflow/quake3/q3_slide_move.cpp
        src/services/impl/workflow/quake3/q3_slide_planes.cpp
        src/services/impl/workflow/quake3/q3_sound_bank.cpp
        src/services/impl/workflow/quake3/q3_sound_playback.cpp
        src/services/impl/workflow/quake3/q3_step_slide.cpp
        src/services/impl/workflow/quake3/q3_step_slide_trace.cpp
        src/services/impl/workflow/quake3/q3_trigger_load.cpp
        src/services/impl/workflow/quake3/q3_trigger_overlap.cpp
        src/services/impl/workflow/quake3/q3_weapon_cycle.cpp
        src/services/impl/workflow/quake3/q3_weapon_fire_context.cpp
        src/services/impl/workflow/quake3/q3_weapon_hitscan.cpp
        src/services/impl/workflow/quake3/q3_weapon_hitscan_common.cpp
        src/services/impl/workflow/quake3/q3_weapon_hitscan_patterns.cpp
        src/services/impl/workflow/quake3/q3_weapon_projectile.cpp
        src/services/impl/workflow/quake3/q3_weapon_stats.cpp
        src/services/impl/workflow/quake3/q3_wish_dir.cpp
        src/services/impl/workflow/quake3/workflow_q3_ammo_init_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_damage_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_draw_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_spawn_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_update_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_crosshair_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_damage_apply_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_frame_time_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_hitmarker_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_hud_head_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_hud_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_mapselect_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_md3_draw_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_md3_parse_anim_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_md3_parse_tags_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_md3_read_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_md3_upload_surfaces_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_menu_frame_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_menu_update_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_missiles_impact_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_missiles_move_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_movers_init_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_movers_update_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_music_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_nav_build_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pickups_draw_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pickups_respawn_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pickups_touch_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_player_commit_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_player_death_check_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_player_respawn_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_player_sync_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_crouch_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_friction_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_ground_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_jump_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_slide_move_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_step_slide_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_sky_draw_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_sound_init_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_sound_load_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_sound_play_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_stats_init_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_triggers_apply_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_triggers_check_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_weapon_fire_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_weapon_select_step.cpp
        src/services/impl/workflow/gta5/gta5_asset_index_build.cpp
        src/services/impl/workflow/gta5/gta5_asset_scan.cpp
        src/services/impl/workflow/gta5/gta5_asset_scan_slice.cpp
        src/services/impl/workflow/gta5/gta5_asset_scan_textures.cpp
        src/services/impl/workflow/gta5/gta5_assets_index_step.cpp
        src/services/impl/workflow/gta5/gta5_assets_ready.cpp
        src/services/impl/workflow/gta5/gta5_bone_pose.cpp
        src/services/impl/workflow/gta5/gta5_collision_body.cpp
        src/services/impl/workflow/gta5/gta5_collision_shape.cpp
        src/services/impl/workflow/gta5/gta5_daylight.cpp
        src/services/impl/workflow/gta5/gta5_decal_stick.cpp
        src/services/impl/workflow/gta5/gta5_draw_bind.cpp
        src/services/impl/workflow/gta5/gta5_draw_context_build.cpp
        src/services/impl/workflow/gta5/gta5_draw_instances.cpp
        src/services/impl/workflow/gta5/gta5_draw_probe.cpp
        src/services/impl/workflow/gta5/gta5_drawable_geometry.cpp
        src/services/impl/workflow/gta5/gta5_drawable_locate.cpp
        src/services/impl/workflow/gta5/gta5_drawable_mesh.cpp
        src/services/impl/workflow/gta5/gta5_effects_art.cpp
        src/services/impl/workflow/gta5/gta5_effects_blast.cpp
        src/services/impl/workflow/gta5/gta5_effects_decal.cpp
        src/services/impl/workflow/gta5/gta5_effects_impact.cpp
        src/services/impl/workflow/gta5/gta5_effects_mesh.cpp
        src/services/impl/workflow/gta5/gta5_effects_prepare_step.cpp
        src/services/impl/workflow/gta5/gta5_effects_quad.cpp
        src/services/impl/workflow/gta5/gta5_effects_setup.cpp
        src/services/impl/workflow/gta5/gta5_effects_spawn.cpp
        src/services/impl/workflow/gta5/gta5_effects_step.cpp
        src/services/impl/workflow/gta5/gta5_effects_update.cpp
        src/services/impl/workflow/gta5/gta5_engine_bank_load.cpp
        src/services/impl/workflow/gta5/gta5_engine_grain.cpp
        src/services/impl/workflow/gta5/gta5_engine_voice.cpp
        src/services/impl/workflow/gta5/gta5_evict_plan.cpp
        src/services/impl/workflow/gta5/gta5_fall_guard.cpp
        src/services/impl/workflow/gta5/gta5_finite_guard.cpp
        src/services/impl/workflow/gta5/gta5_fragment_axles.cpp
        src/services/impl/workflow/gta5/gta5_frame_mark_step.cpp
        src/services/impl/workflow/gta5/gta5_frame_stats_step.cpp
        src/services/impl/workflow/gta5/gta5_frustum.cpp
        src/services/impl/workflow/gta5/gta5_gamepad_keys.cpp
        src/services/impl/workflow/gta5/gta5_gamepad_publish.cpp
        src/services/impl/workflow/gta5/gta5_gamepad_read.cpp
        src/services/impl/workflow/gta5/gta5_gamepad_step.cpp
        src/services/impl/workflow/gta5/gta5_garage_load.cpp
        src/services/impl/workflow/gta5/gta5_geometry_arena.cpp
        src/services/impl/workflow/gta5/gta5_geometry_arena_upload.cpp
        src/services/impl/workflow/gta5/gta5_geometry_cache.cpp
        src/services/impl/workflow/gta5/gta5_geometry_request.cpp
        src/services/impl/workflow/gta5/gta5_geometry_upload.cpp
        src/services/impl/workflow/gta5/gta5_grid.cpp
        src/services/impl/workflow/gta5/gta5_ground_probe.cpp
        src/services/impl/workflow/gta5/gta5_held_weapon.cpp
        src/services/impl/workflow/gta5/gta5_hitch_report.cpp
        src/services/impl/workflow/gta5/gta5_hud_art.cpp
        src/services/impl/workflow/gta5/gta5_hud_cross.cpp
        src/services/impl/workflow/gta5/gta5_hud_frame.cpp
        src/services/impl/workflow/gta5/gta5_hud_gauge.cpp
        src/services/impl/workflow/gta5/gta5_hud_load.cpp
        src/services/impl/workflow/gta5/gta5_hud_menu.cpp
        src/services/impl/workflow/gta5/gta5_hud_needle.cpp
        src/services/impl/workflow/gta5/gta5_hud_step.cpp
        src/services/impl/workflow/gta5/gta5_hud_wheel.cpp
        src/services/impl/workflow/gta5/gta5_indexed_texture.cpp
        src/services/impl/workflow/gta5/gta5_instance_batch_build.cpp
        src/services/impl/workflow/gta5/gta5_instance_batch_upload.cpp
        src/services/impl/workflow/gta5/gta5_json_file.cpp
        src/services/impl/workflow/gta5/gta5_lights_load.cpp
        src/services/impl/workflow/gta5/gta5_lights_mesh.cpp
        src/services/impl/workflow/gta5/gta5_lights_step.cpp
        src/services/impl/workflow/gta5/gta5_load_budget.cpp
        src/services/impl/workflow/gta5/gta5_load_pool.cpp
        src/services/impl/workflow/gta5/gta5_load_progress.cpp
        src/services/impl/workflow/gta5/gta5_loading_text_step.cpp
        src/services/impl/workflow/gta5/gta5_lod.cpp
        src/services/impl/workflow/gta5/gta5_lod_select_step.cpp
        src/services/impl/workflow/gta5/gta5_look.cpp
        src/services/impl/workflow/gta5/gta5_map_atlas.cpp
        src/services/impl/workflow/gta5/gta5_map_font.cpp
        src/services/impl/workflow/gta5/gta5_map_frame.cpp
        src/services/impl/workflow/gta5/gta5_map_legend.cpp
        src/services/impl/workflow/gta5/gta5_map_look_step.cpp
        src/services/impl/workflow/gta5/gta5_map_markers.cpp
        src/services/impl/workflow/gta5/gta5_map_overlay_draw.cpp
        src/services/impl/workflow/gta5/gta5_map_overlay_gpu.cpp
        src/services/impl/workflow/gta5/gta5_map_overlay_load.cpp
        src/services/impl/workflow/gta5/gta5_map_poi.cpp
        src/services/impl/workflow/gta5/gta5_map_quads.cpp
        src/services/impl/workflow/gta5/gta5_map_step.cpp
        src/services/impl/workflow/gta5/gta5_map_travel.cpp
        src/services/impl/workflow/gta5/gta5_material_surface.cpp
        src/services/impl/workflow/gta5/gta5_mesh_extract.cpp
        src/services/impl/workflow/gta5/gta5_mesh_geometry_upload.cpp
        src/services/impl/workflow/gta5/gta5_mesh_transform.cpp
        src/services/impl/workflow/gta5/gta5_mesh_vertex.cpp
        src/services/impl/workflow/gta5/gta5_model_matrix.cpp
        src/services/impl/workflow/gta5/gta5_ped_arm_dir.cpp
        src/services/impl/workflow/gta5/gta5_ped_arms.cpp
        src/services/impl/workflow/gta5/gta5_ped_axes.cpp
        src/services/impl/workflow/gta5/gta5_ped_blend.cpp
        src/services/impl/workflow/gta5/gta5_ped_gait.cpp
        src/services/impl/workflow/gta5/gta5_ped_legs.cpp
        src/services/impl/workflow/gta5/gta5_ped_load.cpp
        src/services/impl/workflow/gta5/gta5_ped_mesh.cpp
        src/services/impl/workflow/gta5/gta5_ped_plant.cpp
        src/services/impl/workflow/gta5/gta5_ped_pose.cpp
        src/services/impl/workflow/gta5/gta5_ped_skin.cpp
        src/services/impl/workflow/gta5/gta5_ped_stance.cpp
        src/services/impl/workflow/gta5/gta5_ped_torso.cpp
        src/services/impl/workflow/gta5/gta5_ped_upload.cpp
        src/services/impl/workflow/gta5/gta5_ped_walk.cpp
        src/services/impl/workflow/gta5/gta5_pick.cpp
        src/services/impl/workflow/gta5/gta5_placement_parse.cpp
        src/services/impl/workflow/gta5/gta5_player_camera_step.cpp
        src/services/impl/workflow/gta5/gta5_player_character_load.cpp
        src/services/impl/workflow/gta5/gta5_player_character_step.cpp
        src/services/impl/workflow/gta5/gta5_player_fly_step.cpp
        src/services/impl/workflow/gta5/gta5_player_hold_step.cpp
        src/services/impl/workflow/gta5/gta5_player_pin.cpp
        src/services/impl/workflow/gta5/gta5_player_swim_step.cpp
        src/services/impl/workflow/gta5/gta5_player_travel.cpp
        src/services/impl/workflow/gta5/gta5_prepared_finish.cpp
        src/services/impl/workflow/gta5/gta5_prepared_geometry.cpp
        src/services/impl/workflow/gta5/gta5_proxy.cpp
        src/services/impl/workflow/gta5/gta5_radio_list.cpp
        src/services/impl/workflow/gta5/gta5_radio_open.cpp
        src/services/impl/workflow/gta5/gta5_radio_play.cpp
        src/services/impl/workflow/gta5/gta5_radio_step.cpp
        src/services/impl/workflow/gta5/gta5_radio_synth.cpp
        src/services/impl/workflow/gta5/gta5_range_allocator.cpp
        src/services/impl/workflow/gta5/gta5_reflection_pass.cpp
        src/services/impl/workflow/gta5/gta5_reflection_plane.cpp
        src/services/impl/workflow/gta5/gta5_reflection_resources.cpp
        src/services/impl/workflow/gta5/gta5_reflection_step.cpp
        src/services/impl/workflow/gta5/gta5_resource.cpp
        src/services/impl/workflow/gta5/gta5_resource_cache.cpp
        src/services/impl/workflow/gta5/gta5_resource_load.cpp
        src/services/impl/workflow/gta5/gta5_resource_text.cpp
        src/services/impl/workflow/gta5/gta5_roads_load.cpp
        src/services/impl/workflow/gta5/gta5_roads_near.cpp
        src/services/impl/workflow/gta5/gta5_settings.cpp
        src/services/impl/workflow/gta5/gta5_settings_keep.cpp
        src/services/impl/workflow/gta5/gta5_shader_surface.cpp
        src/services/impl/workflow/gta5/gta5_shader_textures.cpp
        src/services/impl/workflow/gta5/gta5_shadow_draw.cpp
        src/services/impl/workflow/gta5/gta5_shadow_resources.cpp
        src/services/impl/workflow/gta5/gta5_shadow_step.cpp
        src/services/impl/workflow/gta5/gta5_shadow_view.cpp
        src/services/impl/workflow/gta5/gta5_shop_buy.cpp
        src/services/impl/workflow/gta5/gta5_shop_keep.cpp
        src/services/impl/workflow/gta5/gta5_shop_load.cpp
        src/services/impl/workflow/gta5/gta5_shop_pages.cpp
        src/services/impl/workflow/gta5/gta5_shop_step.cpp
        src/services/impl/workflow/gta5/gta5_shown_transform.cpp
        src/services/impl/workflow/gta5/gta5_skeleton.cpp
        src/services/impl/workflow/gta5/gta5_sky_draw_step.cpp
        src/services/impl/workflow/gta5/gta5_sky_uniforms.cpp
        src/services/impl/workflow/gta5/gta5_sound_engine.cpp
        src/services/impl/workflow/gta5/gta5_sound_feet.cpp
        src/services/impl/workflow/gta5/gta5_sound_load.cpp
        src/services/impl/workflow/gta5/gta5_sound_loop.cpp
        src/services/impl/workflow/gta5/gta5_sound_shots.cpp
        src/services/impl/workflow/gta5/gta5_sound_step.cpp
        src/services/impl/workflow/gta5/gta5_sound_synth.cpp
        src/services/impl/workflow/gta5/gta5_spawn_tile.cpp
        src/services/impl/workflow/gta5/gta5_step_params.cpp
        src/services/impl/workflow/gta5/gta5_stream_lead.cpp
        src/services/impl/workflow/gta5/gta5_streaming_config_load.cpp
        src/services/impl/workflow/gta5/gta5_submesh_upload.cpp
        src/services/impl/workflow/gta5/gta5_texture_blob_upload.cpp
        src/services/impl/workflow/gta5/gta5_texture_cache.cpp
        src/services/impl/workflow/gta5/gta5_texture_format.cpp
        src/services/impl/workflow/gta5/gta5_texture_upload.cpp
        src/services/impl/workflow/gta5/gta5_tile_coord.cpp
        src/services/impl/workflow/gta5/gta5_tile_io.cpp
        src/services/impl/workflow/gta5/gta5_tile_order.cpp
        src/services/impl/workflow/gta5/gta5_tile_source.cpp
        src/services/impl/workflow/gta5/gta5_tiles_cull_step.cpp
        src/services/impl/workflow/gta5/gta5_tiles_draw_step.cpp
        src/services/impl/workflow/gta5/gta5_tiles_evict_step.cpp
        src/services/impl/workflow/gta5/gta5_tiles_load_step.cpp
        src/services/impl/workflow/gta5/gta5_tiles_resolve_step.cpp
        src/services/impl/workflow/gta5/gta5_time_step.cpp
        src/services/impl/workflow/gta5/gta5_traffic_drive.cpp
        src/services/impl/workflow/gta5/gta5_traffic_junctions.cpp
        src/services/impl/workflow/gta5/gta5_traffic_lamps.cpp
        src/services/impl/workflow/gta5/gta5_traffic_lights.cpp
        src/services/impl/workflow/gta5/gta5_traffic_load.cpp
        src/services/impl/workflow/gta5/gta5_traffic_path.cpp
        src/services/impl/workflow/gta5/gta5_traffic_show.cpp
        src/services/impl/workflow/gta5/gta5_traffic_spawn.cpp
        src/services/impl/workflow/gta5/gta5_traffic_step.cpp
        src/services/impl/workflow/gta5/gta5_upload_batch.cpp
        src/services/impl/workflow/gta5/gta5_upload_batch_flush.cpp
        src/services/impl/workflow/gta5/gta5_vantage.cpp
        src/services/impl/workflow/gta5/gta5_vehicle.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_body.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_camera_step.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_control_step.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_drive.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_hold.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_input.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_load.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_move.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_pedals.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_remove.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_report.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_seat.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_spawn_saved.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_spawn_step.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_wheel_load.cpp
        src/services/impl/workflow/gta5/gta5_vehicle_wheels.cpp
        src/services/impl/workflow/gta5/gta5_vehicles_sync_step.cpp
        src/services/impl/workflow/gta5/gta5_vertex_layout.cpp
        src/services/impl/workflow/gta5/gta5_vertex_terrain.cpp
        src/services/impl/workflow/gta5/gta5_wanted_tiles.cpp
        src/services/impl/workflow/gta5/gta5_wasd.cpp
        src/services/impl/workflow/gta5/gta5_water_load.cpp
        src/services/impl/workflow/gta5/gta5_water_map_step.cpp
        src/services/impl/workflow/gta5/gta5_water_mesh.cpp
        src/services/impl/workflow/gta5/gta5_water_query.cpp
        src/services/impl/workflow/gta5/gta5_water_step.cpp
        src/services/impl/workflow/gta5/gta5_water_uniforms.cpp
        src/services/impl/workflow/gta5/gta5_water_upload.cpp
        src/services/impl/workflow/gta5/gta5_weapon_blast.cpp
        src/services/impl/workflow/gta5/gta5_weapon_dress.cpp
        src/services/impl/workflow/gta5/gta5_weapon_fire.cpp
        src/services/impl/workflow/gta5/gta5_weapon_grip.cpp
        src/services/impl/workflow/gta5/gta5_weapon_hold.cpp
        src/services/impl/workflow/gta5/gta5_weapon_keep.cpp
        src/services/impl/workflow/gta5/gta5_weapon_load.cpp
        src/services/impl/workflow/gta5/gta5_weapon_muzzle.cpp
        src/services/impl/workflow/gta5/gta5_weapon_projectile.cpp
        src/services/impl/workflow/gta5/gta5_weapon_ray.cpp
        src/services/impl/workflow/gta5/gta5_weapon_step.cpp
        src/services/impl/workflow/gta5/gta5_weapon_wheel.cpp
        src/services/impl/workflow/gta5/gta5_world_config_load.cpp
        src/services/impl/workflow/gta5/gta5_ymap.cpp
        src/services/impl/workflow/gta5/gta5_ymap_placement.cpp
        src/services/impl/workflow/gta5/gta5_ymap_tile.cpp
        src/services/impl/workflow/workflow_definition_parser_includes.cpp
        src/services/impl/workflow/workflow_definition_parser_include_path.cpp
        src/services/impl/workflow/workflow_definition_parser_expand_include.cpp
        src/services/impl/workflow/workflow_mesh_payload_converter.cpp
        src/services/impl/workflow/workflow_camera_view_state_builder.cpp
        src/services/impl/workflow/workflow_cmdline_args_step.cpp
        src/services/impl/workflow/workflow_data_deserialize_step.cpp
        src/services/impl/workflow/workflow_data_serialize_step.cpp
        src/services/impl/workflow/graphics_init_device_helpers.cpp
        src/services/impl/workflow/workflow_graphics_init_device_step.cpp
        src/services/impl/workflow/workflow_graphics_init_swapchain_step.cpp
        src/services/impl/workflow/media_catalog_items.cpp
        src/services/impl/workflow/media_catalog_builder.cpp
        src/services/impl/workflow/media_catalog_count.cpp
        src/services/impl/workflow/media_item_dispatch.cpp
        src/services/impl/workflow/workflow_media_catalog_scan_step.cpp
        src/services/impl/workflow/workflow_media_item_select_step.cpp
        src/services/impl/workflow/network_connect_helpers.cpp
        src/services/impl/workflow/network_receive_helpers.cpp
        src/services/impl/workflow/network_receive_params.cpp
        src/services/impl/workflow/network_receive_output_keys.cpp
        src/services/impl/workflow/network_send_simulation.cpp
        src/services/impl/workflow/workflow_network_connect_step.cpp
        src/services/impl/workflow/workflow_network_receive_step.cpp
        src/services/impl/workflow/workflow_network_send_step.cpp
        src/services/impl/workflow/package_root_resolver.cpp
        src/services/impl/workflow/package_shader_load.cpp
        src/services/impl/workflow/workflow_package_shader_loader_step.cpp
        src/services/impl/workflow/workflow_shader_builtin_constant_color_step.cpp
        src/services/impl/workflow/workflow_shader_compile_step.cpp
        src/services/impl/workflow/shader_compile_step_helpers.cpp
        src/services/impl/workflow/shader_compile_debug_marker.cpp
        src/services/impl/workflow/shader_compile_build_map.cpp
        src/services/impl/workflow/shader_compile_gpu_load.cpp
        src/services/impl/workflow/workflow_shader_system_initialize_step.cpp
        src/services/impl/workflow/workflow_shader_system_gltf_load_step.cpp
        src/services/impl/workflow/workflow_shader_system_compile_step.cpp
        src/services/impl/workflow/workflow_shader_system_set_step.cpp
        src/services/impl/workflow/workflow_state_clear_step.cpp
        src/services/impl/workflow/workflow_state_load_step.cpp
        src/services/impl/workflow/workflow_state_save_step.cpp
        src/services/impl/workflow/workflow_template_resolver.cpp
        src/services/impl/workflow/workflow_registrar_gta5.cpp
        src/services/impl/workflow/workflow_registrar_gta5_player.cpp
    )
    target_include_directories(sdl3_app PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )
    target_link_libraries(sdl3_app PRIVATE
        ${SDL_TARGET}
        rapidjson
        nlohmann_json::nlohmann_json
        Bullet::Bullet
        glm::glm
        stb::stb
        EnTT::EnTT
        libzip::zip
        assimp::assimp
        spdlog::spdlog
        ZLIB::ZLIB
    )
endif()

# ============================================================================
# TEST TARGETS
# ============================================================================
enable_testing()

add_executable(q3_slide_planes_test)
target_sources(q3_slide_planes_test PRIVATE tests/q3_slide_planes_test.cpp)
target_sources(q3_slide_planes_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_include_directories(q3_slide_planes_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_slide_planes_test PRIVATE
    GTest::gtest_main
    glm::glm
)
add_test(NAME q3_slide_planes_test COMMAND q3_slide_planes_test)

add_executable(q3_overlay_scale_test)
target_sources(q3_overlay_scale_test PRIVATE tests/q3_overlay_scale_test.cpp)
target_sources(q3_overlay_scale_test PRIVATE src/services/impl/workflow/quake3/q3_overlay_scale.cpp)
target_include_directories(q3_overlay_scale_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_overlay_scale_test PRIVATE
    GTest::gtest_main
)
add_test(NAME q3_overlay_scale_test COMMAND q3_overlay_scale_test)

add_executable(q3_pm_constants_test)
target_sources(q3_pm_constants_test PRIVATE tests/q3_pm_constants_test.cpp)
target_include_directories(q3_pm_constants_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_pm_constants_test PRIVATE
    GTest::gtest_main
)
add_test(NAME q3_pm_constants_test COMMAND q3_pm_constants_test)

add_executable(q3_frame_time_test)
target_sources(q3_frame_time_test PRIVATE tests/q3_frame_time_test.cpp)
target_include_directories(q3_frame_time_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_frame_time_test PRIVATE
    GTest::gtest_main
)
add_test(NAME q3_frame_time_test COMMAND q3_frame_time_test)

add_executable(q3_prop_font_test)
target_sources(q3_prop_font_test PRIVATE tests/q3_prop_font_test.cpp)
target_include_directories(q3_prop_font_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_prop_font_test PRIVATE
    GTest::gtest_main
    ${SDL_TARGET}
    stb::stb
    libzip::zip
)
add_test(NAME q3_prop_font_test COMMAND q3_prop_font_test)

add_executable(q3_sound_bank_test)
target_sources(q3_sound_bank_test PRIVATE tests/q3_sound_bank_test.cpp)
target_sources(q3_sound_bank_test PRIVATE src/services/impl/workflow/quake3/q3_sound_bank.cpp)
target_include_directories(q3_sound_bank_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_sound_bank_test PRIVATE
    GTest::gtest_main
    ${SDL_TARGET}
)
add_test(NAME q3_sound_bank_test COMMAND q3_sound_bank_test)

add_executable(q3_skin_file_test)
target_sources(q3_skin_file_test PRIVATE tests/q3_skin_file_test.cpp)
target_sources(q3_skin_file_test PRIVATE src/services/impl/workflow/quake3/q3_skin_file.cpp)
target_include_directories(q3_skin_file_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_skin_file_test PRIVATE
    GTest::gtest_main
)
add_test(NAME q3_skin_file_test COMMAND q3_skin_file_test)

add_executable(q3_anim_cfg_test)
target_sources(q3_anim_cfg_test PRIVATE tests/q3_anim_cfg_test.cpp)
target_sources(q3_anim_cfg_test PRIVATE src/services/impl/workflow/quake3/q3_anim_cfg.cpp)
target_include_directories(q3_anim_cfg_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_anim_cfg_test PRIVATE
    GTest::gtest_main
)
add_test(NAME q3_anim_cfg_test COMMAND q3_anim_cfg_test)

add_executable(q3_wish_dir_test)
target_sources(q3_wish_dir_test PRIVATE tests/q3_wish_dir_test.cpp)
target_sources(q3_wish_dir_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_include_directories(q3_wish_dir_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_wish_dir_test PRIVATE
    GTest::gtest_main
    glm::glm
)
add_test(NAME q3_wish_dir_test COMMAND q3_wish_dir_test)

add_executable(q3_movement_integration_test)
target_sources(q3_movement_integration_test PRIVATE tests/q3_movement_integration_test.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_friction_step.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_ground_step.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_step_slide_step.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide_trace.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_slide_move.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_accel.cpp)
target_sources(q3_movement_integration_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_ground.cpp)
target_include_directories(q3_movement_integration_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_movement_integration_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
)
add_test(NAME q3_movement_integration_test COMMAND q3_movement_integration_test)

add_executable(q3_input_axes_test)
target_sources(q3_input_axes_test PRIVATE tests/q3_input_axes_test.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/workflow_input_axis_combine_step.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_aggregation_config.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_axis_combine_write.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_axis_deadzone.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_axis_source_value.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/workflow_input_button_combine_step.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_button_combine_helpers.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_button_combine_config.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_button_gamepad_source.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/input/input_button_combine_sources.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver_scalars.cpp)
target_sources(q3_input_axes_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver_lists.cpp)
target_include_directories(q3_input_axes_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_input_axes_test PRIVATE
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    ${SDL_TARGET}
)
add_test(NAME q3_input_axes_test COMMAND q3_input_axes_test)

add_executable(q3_map_movement_test)
target_sources(q3_map_movement_test PRIVATE tests/q3_map_movement_test.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_friction_step.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_ground_step.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_step_slide_step.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide_trace.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_slide_move.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/workflow_bsp_load_step.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/pk3_bsp_loader.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/workflow_bsp_build_collision_step.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_brush_classify.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_brush_collision_shapes.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_brush_convex_shape.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_brush_lump_view.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_brush_vertex_hull.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_collision_body.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/workflow_bsp_parse_spawn_step.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_entity_lump_parser.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_entity_lexer.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_model_lump.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_entity_json_builder.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_entity_enrich.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_entity_origin.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_entity_classification.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_q3_coordinates.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver_scalars.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver_lists.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_brush_collision.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/rendering/bsp_pmove_brush_model.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_accel.cpp)
target_sources(q3_map_movement_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_ground.cpp)
target_include_directories(q3_map_movement_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_map_movement_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
    nlohmann_json::nlohmann_json
    libzip::zip
)
add_test(NAME q3_map_movement_test COMMAND q3_map_movement_test)

add_executable(q3_input_to_motion_test)
target_sources(q3_input_to_motion_test PRIVATE tests/q3_input_to_motion_test.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/input/workflow_input_axis_combine_step.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/input/input_aggregation_config.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/input/input_axis_combine_write.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/input/input_axis_deadzone.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/input/input_axis_source_value.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver_scalars.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/workflow_step_parameter_resolver_lists.cpp)
target_sources(q3_input_to_motion_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_accel.cpp)
target_include_directories(q3_input_to_motion_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_input_to_motion_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
    nlohmann_json::nlohmann_json
    ${SDL_TARGET}
)
add_test(NAME q3_input_to_motion_test COMMAND q3_input_to_motion_test)

add_executable(q3_step_up_test)
target_sources(q3_step_up_test PRIVATE tests/q3_step_up_test.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_friction_step.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_ground_step.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_step_slide_step.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide_trace.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_slide_move.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_accel.cpp)
target_sources(q3_step_up_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_ground.cpp)
target_include_directories(q3_step_up_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_step_up_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
)
add_test(NAME q3_step_up_test COMMAND q3_step_up_test)

add_executable(q3_ramp_walk_test)
target_sources(q3_ramp_walk_test PRIVATE tests/q3_ramp_walk_test.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_friction_step.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_ground_step.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_step_slide_step.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide_trace.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_slide_move.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_accel.cpp)
target_sources(q3_ramp_walk_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_ground.cpp)
target_include_directories(q3_ramp_walk_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_ramp_walk_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
)
add_test(NAME q3_ramp_walk_test COMMAND q3_ramp_walk_test)

add_executable(q3_stairs_test)
target_sources(q3_stairs_test PRIVATE tests/q3_stairs_test.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_friction_step.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_ground_step.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/workflow_q3_pm_step_slide_step.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide_trace.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_slide_move.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_accel.cpp)
target_sources(q3_stairs_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_ground.cpp)
target_include_directories(q3_stairs_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_stairs_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
)
add_test(NAME q3_stairs_test COMMAND q3_stairs_test)

add_executable(q3_brush_trace_test)
target_sources(q3_brush_trace_test PRIVATE tests/q3_brush_trace_test.cpp)
target_sources(q3_brush_trace_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace.cpp)
target_sources(q3_brush_trace_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp)
target_include_directories(q3_brush_trace_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_brush_trace_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
)
add_test(NAME q3_brush_trace_test COMMAND q3_brush_trace_test)

add_executable(q3_weapon_cycle_test)
target_sources(q3_weapon_cycle_test PRIVATE tests/q3_weapon_cycle_test.cpp)
target_sources(q3_weapon_cycle_test PRIVATE src/services/impl/workflow/quake3/q3_weapon_cycle.cpp)
target_include_directories(q3_weapon_cycle_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_weapon_cycle_test PRIVATE
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)
add_test(NAME q3_weapon_cycle_test COMMAND q3_weapon_cycle_test)

add_executable(q3_pickup_rules_test)
target_sources(q3_pickup_rules_test PRIVATE tests/q3_pickup_rules_test.cpp)
target_sources(q3_pickup_rules_test PRIVATE src/services/impl/workflow/quake3/q3_pickup_apply.cpp)
target_sources(q3_pickup_rules_test PRIVATE src/services/impl/workflow/quake3/q3_pickup_can_grab.cpp)
target_sources(q3_pickup_rules_test PRIVATE src/services/impl/workflow/quake3/q3_pickup_lookup_tables.cpp)
target_include_directories(q3_pickup_rules_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_pickup_rules_test PRIVATE
    GTest::gtest_main
    glm::glm
    nlohmann_json::nlohmann_json
)
add_test(NAME q3_pickup_rules_test COMMAND q3_pickup_rules_test)

add_executable(q3_axes_test)
target_sources(q3_axes_test PRIVATE tests/q3_axes_test.cpp)
target_sources(q3_axes_test PRIVATE src/services/impl/workflow/quake3/q3_axes.cpp)
target_sources(q3_axes_test PRIVATE src/services/impl/workflow/quake3/q3_axes_model.cpp)
target_include_directories(q3_axes_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(q3_axes_test PRIVATE
    GTest::gtest_main
    glm::glm
)
add_test(NAME q3_axes_test COMMAND q3_axes_test)

add_executable(q3_bot_move_test)
target_sources(q3_bot_move_test PRIVATE tests/q3_bot_move_test.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_bot_mover.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_bot_usercmd.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_pmove.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_ground.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_pmove_accel.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_axes.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_step_slide_trace.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_slide_move.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_slide_planes.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_wish_dir.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace.cpp)
target_sources(q3_bot_move_test PRIVATE src/services/impl/workflow/quake3/q3_brush_trace_clip.cpp)
target_include_directories(q3_bot_move_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)
target_link_libraries(q3_bot_move_test PRIVATE
    GTest::gtest_main
    glm::glm
    Bullet::Bullet
    nlohmann_json::nlohmann_json
)
add_test(NAME q3_bot_move_test COMMAND q3_bot_move_test)

# ============================================================================
# FILE COPYING
# ============================================================================
if(BUILD_SDL3_APP)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packages")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/packages" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endif()