
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/config/"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/workflow/workflow_config_load_step.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/workflow/workflow_config_migration_step.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/workflow/workflow_config_schema_step.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/workflow/workflow_config_version_step.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/workflow/workflow_config_pipeline.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/workflow/graphics/workflow_graphics_shader_load_step.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/services/impl/workflow/rendering/workflow_render_cube_grid_step.cpp"
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
        src/services/impl/diagnostics/logger_service.cpp
        src/services/impl/workflow/workflow_executor.cpp
        src/services/impl/workflow/workflow_step_registry.cpp
        src/services/impl/workflow/workflow_exit_step.cpp
        src/services/impl/workflow/workflow_parameter_reader.cpp
        src/services/impl/workflow/workflow_step_io_resolver.cpp
        src/services/impl/workflow/workflow_step_parameter_resolver.cpp
        src/services/impl/workflow/workflow_registrar.cpp
        src/services/impl/workflow/workflow_sdl_init_step.cpp
        src/services/impl/workflow/workflow_sdl_window_create_step.cpp
        src/services/impl/workflow/workflow_app_init_step.cpp
        src/services/impl/workflow/workflow_load_workflow_step.cpp
        src/services/impl/workflow/workflow_definition_parser.cpp
        src/services/impl/workflow/workflow_definition_parser_nodes.cpp
        src/services/impl/workflow/workflow_definition_parser_variables.cpp
        src/services/impl/workflow/workflow_definition_parser_includes.cpp
        src/services/impl/workflow/workflow_connection_resolver.cpp
        src/services/impl/workflow/workflow_execute_step.cpp
        src/stb_image.cpp
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
        src/services/impl/workflow/workflow_generic_steps/workflow_model_despawn_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_model_set_transform_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_abs_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_add_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_clamp_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_div_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_max_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_min_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_mul_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_round_step.cpp
        src/services/impl/workflow/workflow_generic_steps/workflow_number_sub_step.cpp
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
        src/services/impl/workflow/geometry/workflow_geometry_create_cube_step.cpp
        src/services/impl/workflow/geometry/workflow_geometry_create_plane_step.cpp
        src/services/impl/workflow/geometry/workflow_geometry_cube_generate_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_menu_update_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_begin_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_sw_end_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_hud_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_hud_head_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_crosshair_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_hitmarker_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_menu_frame_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_mapselect_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pickups_draw_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_weapon_update_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_md3_load_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_md3_draw_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_spawn_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_update_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_draw_step.cpp
        # Q3 pmove
        src/services/impl/workflow/quake3/workflow_q3_pm_crouch_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_ground_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_friction_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_accelerate_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_jump_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pm_slide_move_step.cpp
        # Q3 weapons + missiles
        src/services/impl/workflow/quake3/workflow_q3_weapon_select_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_weapon_fire_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_missiles_move_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_missiles_impact_step.cpp
        # Q3 damage + pickups + ammo
        src/services/impl/workflow/quake3/workflow_q3_ammo_init_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_damage_apply_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_bots_damage_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_player_death_check_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_player_respawn_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pickups_touch_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_pickups_respawn_step.cpp
        # Q3 movers + triggers + nav
        src/services/impl/workflow/quake3/workflow_q3_movers_init_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_movers_update_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_triggers_check_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_triggers_apply_step.cpp
        src/services/impl/workflow/quake3/workflow_q3_nav_build_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_build_collision_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_build_geometry_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_entity_update_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_extract_textures_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_lightmap_atlas_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_load_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_parse_spawn_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_portal_view_step.cpp
        src/services/impl/workflow/rendering/workflow_bsp_upload_geometry_step.cpp
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
        src/services/impl/workflow/rendering/workflow_postfx_bloom_blur_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_bloom_extract_step.cpp
        src/services/impl/workflow/rendering/workflow_debug_screenshot_step.cpp
        src/services/impl/workflow/rendering/workflow_overlay_fps_step.cpp
        src/services/impl/workflow/rendering/workflow_postfx_composite_step.cpp
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
        src/services/impl/workflow/graphics/workflow_gpu_pipeline_create_step.cpp
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
        src/services/impl/workflow/input/workflow_input_axis_combine_step.cpp
        src/services/impl/workflow/input/workflow_input_button_combine_step.cpp
        src/services/impl/workflow/input/workflow_input_gamepad_poll_step.cpp
        src/services/impl/workflow/input/workflow_input_keyboard_poll_step.cpp
        src/services/impl/workflow/input/workflow_input_mouse_poll_step.cpp
        src/services/impl/workflow/input/workflow_input_poll_all_step.cpp
        src/services/impl/workflow/scene/workflow_scene_add_geometry_step.cpp
        src/services/impl/workflow/scene/workflow_scene_clear_step.cpp
        src/services/impl/workflow/scene/workflow_scene_create_step.cpp
        src/services/impl/workflow/scene/workflow_scene_get_bounds_step.cpp
        src/services/impl/workflow/scene/workflow_scene_load_step.cpp
        src/services/impl/workflow/scene/workflow_scene_remove_geometry_step.cpp
        src/services/impl/workflow/scene/workflow_scene_set_active_step.cpp
        src/services/impl/workflow/scene/workflow_scene_update_step.cpp
        src/services/impl/workflow/compute/workflow_compute_pipeline_create_step.cpp
        src/services/impl/workflow/compute/workflow_compute_tessellate_dispatch_step.cpp
        src/services/impl/workflow/compute/workflow_compute_tessellate_step.cpp
        src/services/impl/workflow/workflow_camera_view_state_builder.cpp
        src/services/impl/workflow/workflow_cmdline_args_step.cpp
        src/services/impl/workflow/workflow_data_deserialize_step.cpp
        src/services/impl/workflow/workflow_data_serialize_step.cpp
        src/services/impl/workflow/workflow_graphics_init_device_step.cpp
        src/services/impl/workflow/workflow_graphics_init_swapchain_step.cpp
        src/services/impl/workflow/workflow_media_catalog_scan_step.cpp
        src/services/impl/workflow/workflow_media_item_select_step.cpp
        src/services/impl/workflow/workflow_network_connect_step.cpp
        src/services/impl/workflow/workflow_network_receive_step.cpp
        src/services/impl/workflow/workflow_network_send_step.cpp
        src/services/impl/workflow/workflow_package_shader_loader_step.cpp
        src/services/impl/workflow/workflow_shader_builtin_constant_color_step.cpp
        src/services/impl/workflow/workflow_shader_compile_step.cpp
        src/services/impl/workflow/workflow_shader_system_initialize_step.cpp
        src/services/impl/workflow/workflow_shader_system_set_step.cpp
        src/services/impl/workflow/workflow_state_clear_step.cpp
        src/services/impl/workflow/workflow_state_load_step.cpp
        src/services/impl/workflow/workflow_state_save_step.cpp
        src/services/impl/workflow/workflow_template_resolver.cpp
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
    )
endif()

# ============================================================================
# FILE COPYING
# ============================================================================
if(BUILD_SDL3_APP)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packages")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/packages" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endif()
