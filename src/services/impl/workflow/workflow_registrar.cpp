#include "services/interfaces/workflow_registrar.hpp"

// SDL3 platform
#include "services/interfaces/workflow/workflow_sdl_init_step.hpp"
#include "services/interfaces/workflow/workflow_sdl_window_create_step.hpp"

// GPU initialization
#include "services/interfaces/workflow/graphics/workflow_graphics_init_viewport_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_init_renderer_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_gpu_init_step.hpp"

// Graphics pipeline
#include "services/interfaces/workflow/geometry/workflow_geometry_create_cube_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_gpu_shader_compile_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_gpu_pipeline_create_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_vertex_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_index_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_draw_submit_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_frame_begin_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_frame_end_step.hpp"
#include "services/interfaces/workflow/graphics/workflow_graphics_screenshot_request_step.hpp"

// Camera
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_setup_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_fps_update_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_look_at_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_set_fov_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_set_pose_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_teleport_step.hpp"

// Rendering
#include "services/interfaces/workflow/rendering/workflow_render_grid_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_render_grid_draw_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_begin_gpu_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_draw_bodies_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_end_gpu_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_textured_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_lighting_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_spotlight_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_spotlight_update_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_model_load_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_viewmodel_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_geometry_create_flashlight_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_map_load_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_map_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_taa_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_load_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_lightmap_atlas_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_parse_spawn_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_entity_update_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_portal_view_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_build_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_extract_textures_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_upload_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_bsp_build_collision_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_spawn_apply_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_draw_textured_box_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_shadow_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_shadow_pass_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_render_prepare_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_setup_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_begin_offscreen_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_frame_end_scene_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_fps_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_debug_screenshot_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_ssao_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_extract_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_blur_step.hpp"

// Texture
#include "services/interfaces/workflow/graphics/workflow_texture_load_step.hpp"

// Geometry
#include "services/interfaces/workflow/geometry/workflow_geometry_create_plane_step.hpp"

// Compute
#include "services/interfaces/workflow/compute/workflow_compute_tessellate_step.hpp"

// Physics
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_world_create_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_body_add_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_step_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_fps_move_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_sync_transforms_step.hpp"

// Input (logger-only)
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_poll_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_grab_step.hpp"
#include "services/interfaces/workflow/input/workflow_input_poll_all_step.hpp"

// Input (service-dependent, registered with nullptr)
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_key_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_gamepad_axis_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_gamepad_button_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_button_pressed_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_mouse_position_step.hpp"

// Scene (service-dependent, registered with nullptr)
#include "services/interfaces/workflow/scene/workflow_scene_create_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_load_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_update_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_clear_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_set_active_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_add_geometry_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_remove_geometry_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_get_bounds_step.hpp"

// Model
#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_despawn_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_set_transform_step.hpp"

// Camera (service-dependent)
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_build_view_state_step.hpp"

// Quake 3
#include "services/interfaces/workflow/quake3/workflow_q3_menu_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_weapon_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_hud_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_hud_head_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_crosshair_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_hitmarker_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_menu_frame_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_mapselect_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_begin_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pickups_draw_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_md3_load_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_md3_draw_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_spawn_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_draw_step.hpp"
// Q3 pmove
#include "services/interfaces/workflow/quake3/workflow_q3_pm_crouch_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_jump_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_slide_move_step.hpp"
// Q3 weapons + missiles
#include "services/interfaces/workflow/quake3/workflow_q3_weapon_select_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_weapon_fire_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_missiles_move_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_missiles_impact_step.hpp"
// Q3 damage + pickups + ammo
#include "services/interfaces/workflow/quake3/workflow_q3_ammo_init_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_damage_apply_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_bots_damage_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_player_death_check_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_player_respawn_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pickups_touch_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pickups_respawn_step.hpp"
// Q3 movers + triggers + nav
#include "services/interfaces/workflow/quake3/workflow_q3_movers_init_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_movers_update_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_triggers_check_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_triggers_apply_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_nav_build_step.hpp"

// Audio (service-dependent, registered with nullptr)
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_pause_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_play_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_resume_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_seek_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_set_looping_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_set_volume_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_audio_stop_step.hpp"

// Control structures
#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_while_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_for_each_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_if_else_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_switch_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_try_catch_step.hpp"

// Variables
#include "services/interfaces/workflow/workflow_generic_steps/workflow_variable_set_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_variable_get_step.hpp"

// Arrays
#include "services/interfaces/workflow/workflow_generic_steps/workflow_array_create_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_array_append_step.hpp"

// Bool
#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_and_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_not_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_bool_or_step.hpp"

// Compare
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_eq_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_gt_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_gte_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_lt_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_lte_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_compare_ne_step.hpp"

// Debug
#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_log_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_debug_metrics_step.hpp"

// List
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_append_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_concat_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_count_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_filter_equals_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_filter_gt_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_literal_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_map_add_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_map_mul_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_max_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_min_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_list_reduce_sum_step.hpp"

// Number
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_abs_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_add_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_clamp_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_div_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_max_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_min_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_mul_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_round_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_number_sub_step.hpp"

// Particle
#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_emit_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_particle_update_step.hpp"

// String
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_concat_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_contains_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_equals_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_format_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_join_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_lower_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_replace_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_split_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_trim_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_string_upper_step.hpp"

// Value
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_assert_exists_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_set_if_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_assert_type_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_clear_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_copy_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_default_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_value_literal_step.hpp"


// VFX
#include "services/interfaces/workflow/workflow_generic_steps/workflow_vfx_spawn_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_vfx_destroy_step.hpp"

// Workflow composition
#include "services/interfaces/workflow/workflow_execute_step.hpp"

// System
#include "services/interfaces/workflow/workflow_exit_step.hpp"

// Cmdline / Data / Network / State (logger-only)
#include "services/interfaces/workflow/workflow_cmdline_args_step.hpp"
#include "services/interfaces/workflow/workflow_data_deserialize_step.hpp"
#include "services/interfaces/workflow/workflow_data_serialize_step.hpp"
#include "services/interfaces/workflow/workflow_network_connect_step.hpp"
#include "services/interfaces/workflow/workflow_network_receive_step.hpp"
#include "services/interfaces/workflow/workflow_network_send_step.hpp"
#include "services/interfaces/workflow/workflow_state_clear_step.hpp"
#include "services/interfaces/workflow/workflow_state_load_step.hpp"
#include "services/interfaces/workflow/workflow_state_save_step.hpp"

// Graphics device/swapchain (service-dependent)
#include "services/interfaces/workflow/workflow_graphics_init_device_step.hpp"
#include "services/interfaces/workflow/workflow_graphics_init_swapchain_step.hpp"

// Media (service-dependent)
#include "services/interfaces/workflow/workflow_media_catalog_scan_step.hpp"
#include "services/interfaces/workflow/workflow_media_item_select_step.hpp"

// Package shader loader
#include "services/interfaces/workflow/workflow_package_shader_loader_step.hpp"

// Shader system (service-dependent)
#include "services/interfaces/workflow/workflow_shader_builtin_constant_color_step.hpp"
#include "services/interfaces/workflow/workflow_shader_compile_step.hpp"
#include "services/interfaces/workflow/workflow_shader_system_initialize_step.hpp"
#include "services/interfaces/workflow/workflow_shader_system_set_step.hpp"

#include <memory>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRegistrar::WorkflowRegistrar(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

void WorkflowRegistrar::RegisterSteps(std::shared_ptr<IWorkflowStepRegistry> registry) {
    if (!registry) return;

    int count = 0;

    // ── SDL3 platform ──────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowSdlInitStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowSdlWindowCreateStep>(logger_));
    count += 2;

    // ── GPU initialization ─────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowGraphicsInitViewportStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsInitRendererStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsGpuInitStep>(logger_, nullptr));
    count += 3;

    // ── Graphics pipeline ──────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowGeometryCreateCubeStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGpuShaderCompileStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGpuPipelineCreateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsBufferCreateVertexStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsBufferCreateIndexStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsDrawSubmitStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsFrameBeginStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsFrameEndStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsScreenshotRequestStep>(logger_));
    count += 9;

    // ── Rendering ──────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowRenderGridSetupStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowRenderGridDrawStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowFrameBeginGpuStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowFrameDrawBodiesStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowFrameEndGpuStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDrawTexturedStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDrawTexturedBoxStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowLightingSetupStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowSpotlightSetupStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowSpotlightUpdateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowModelLoadStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDrawViewmodelStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGeometryCreateFlashlightStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowMapLoadStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDrawMapStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPostfxTaaStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspLoadStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspLightmapAtlasStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspParseSpawnStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspEntityUpdateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspPortalViewStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspBuildGeometryStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspExtractTexturesStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspUploadGeometryStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBspBuildCollisionStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowSpawnApplyStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowShadowSetupStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowShadowPassStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowRenderPrepareStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPostfxSetupStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowFrameBeginOffscreenStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowFrameEndSceneStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPostfxCompositeStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowOverlayFpsStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDebugScreenshotStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPostfxSsaoStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPostfxBloomExtractStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPostfxBloomBlurStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3MenuUpdateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3WeaponUpdateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PickupsDrawStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowOverlaySwBeginStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3HudStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3HudHeadStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3CrosshairStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3HitmarkerStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3MenuFrameStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3MapSelectStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowOverlaySwEndStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3Md3LoadStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3Md3DrawStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsSpawnStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsUpdateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsDrawStep>(logger_));
    // Q3 pmove
    registry->RegisterStep(std::make_shared<WorkflowQ3PmCrouchStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmGroundStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmFrictionStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmAccelerateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmJumpStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PmSlideMoveStep>(logger_));
    // Q3 weapons + missiles
    registry->RegisterStep(std::make_shared<WorkflowQ3WeaponSelectStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3WeaponFireStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3MissilesMoveStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3MissilesImpactStep>(logger_));
    // Q3 damage + pickups + ammo
    registry->RegisterStep(std::make_shared<WorkflowQ3AmmoInitStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3DamageApplyStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3BotsDamageStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PlayerDeathCheckStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PlayerRespawnStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PickupsTouchStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3PickupsRespawnStep>(logger_));
    // Q3 movers + triggers + nav
    registry->RegisterStep(std::make_shared<WorkflowQ3MoversInitStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3MoversUpdateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3TriggersCheckStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3TriggersApplyStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowQ3NavBuildStep>(logger_));
    count += 55;

    // ── Texture ───────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowTextureLoadStep>(logger_));
    count += 1;

    // ── Geometry (textured planes) ────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowGeometryCreatePlaneStep>(logger_));
    count += 1;

    // ── Compute (tessellation) ────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowComputeTessellateStep>(logger_));
    count += 1;

    // ── Camera ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowCameraSetupStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCameraFpsUpdateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCameraLookAtStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCameraSetFovStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCameraSetPoseStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCameraTeleportStep>(logger_));
    count += 6;

    // ── Physics ────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowPhysicsWorldCreateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPhysicsBodyAddStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPhysicsStepStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPhysicsFpsMoveStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowPhysicsSyncTransformsStep>(logger_));
    count += 5;

    // ── Input (logger-only) ────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowInputPollStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowInputMouseGrabStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowInputPollAllStep>(logger_));
    count += 3;

    // ── Input (service-dependent, nullptr until wired) ─────────
    std::shared_ptr<IInputService> inputSvc = nullptr;
    registry->RegisterStep(std::make_shared<WorkflowInputKeyPressedStep>(inputSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowInputGamepadAxisStep>(inputSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowInputGamepadButtonPressedStep>(inputSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowInputMouseButtonPressedStep>(inputSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowInputMousePositionStep>(inputSvc, logger_));
    count += 5;

    // ── Audio (service-dependent, nullptr until wired) ─────────
    std::shared_ptr<IAudioService> audioSvc = nullptr;
    registry->RegisterStep(std::make_shared<WorkflowAudioPauseStep>(audioSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowAudioPlayStep>(audioSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowAudioResumeStep>(audioSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowAudioSeekStep>(audioSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowAudioSetLoopingStep>(audioSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowAudioSetVolumeStep>(audioSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowAudioStopStep>(audioSvc, logger_));
    count += 7;

    // ── Control structures (need registry) ─────────────────────
    registry->RegisterStep(std::make_shared<WorkflowControlForEachStep>(logger_, registry));
    registry->RegisterStep(std::make_shared<WorkflowControlIfElseStep>(logger_, registry));
    registry->RegisterStep(std::make_shared<WorkflowControlSwitchStep>(logger_, registry));
    registry->RegisterStep(std::make_shared<WorkflowTryCatchStep>(logger_, registry));
    count += 4;

    // ── Variables ──────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowVariableSetStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowVariableGetStep>(logger_));
    count += 2;

    // ── Arrays ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowArrayCreateStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowArrayAppendStep>(logger_));
    count += 2;

    // ── Bool ───────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowBoolAndStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBoolNotStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowBoolOrStep>(logger_));
    count += 3;

    // ── Compare ────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowCompareEqStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCompareGtStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCompareGteStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCompareLtStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCompareLteStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowCompareNeStep>(logger_));
    count += 6;

    // ── Debug ──────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowDebugLogStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDebugMetricsStep>(logger_));
    count += 2;

    // ── List ───────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowListAppendStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListConcatStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListCountStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListFilterEqualsStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListFilterGtStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListLiteralStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListMapAddStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListMapMulStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListReduceMaxStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListReduceMinStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowListReduceSumStep>(logger_));
    count += 11;

    // ── Number ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowNumberAbsStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberAddStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberClampStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberDivStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberMaxStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberMinStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberMulStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberRoundStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNumberSubStep>(logger_));
    count += 9;

    // ── Particle ───────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowParticleEmitStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowParticleUpdateStep>(logger_));
    count += 2;

    // ── String ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowStringConcatStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringContainsStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringEqualsStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringFormatStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringJoinStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringLowerStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringReplaceStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringSplitStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringTrimStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStringUpperStep>(logger_));
    count += 10;

    // ── Value ──────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowValueAssertExistsStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowValueAssertTypeStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowValueClearStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowValueCopyStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowValueDefaultStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowValueLiteralStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowValueSetIfStep>(logger_));
    count += 7;

    // ── VFX ────────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowVfxSpawnStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowVfxDestroyStep>(logger_));
    count += 2;

    // ── Model ──────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowModelDespawnStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowModelSetTransformStep>(logger_));
    count += 2;

    // ── Scene (service-dependent, nullptr until wired) ─────────
    std::shared_ptr<ISceneService> sceneSvc = nullptr;
    registry->RegisterStep(std::make_shared<WorkflowSceneCreateStep>(sceneSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowSceneLoadStep>(sceneSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowSceneUpdateStep>(sceneSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowSceneClearStep>(sceneSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowSceneSetActiveStep>(sceneSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowSceneAddGeometryStep>(sceneSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowSceneRemoveGeometryStep>(sceneSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowSceneGetBoundsStep>(sceneSvc, logger_));
    count += 8;

    // ── Camera (service-dependent, nullptr until wired) ────────
    std::shared_ptr<IConfigService> configSvc = nullptr;
    registry->RegisterStep(std::make_shared<WorkflowCameraBuildViewStateStep>(configSvc, logger_));
    count += 1;

    // ── Cmdline / Data / Network / State (logger-only) ──────────
    registry->RegisterStep(std::make_shared<WorkflowCmdlineArgsStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDataDeserializeStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowDataSerializeStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNetworkConnectStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNetworkReceiveStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowNetworkSendStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStateClearStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStateLoadStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowStateSaveStep>(logger_));
    count += 9;

    // ── Graphics device/swapchain (logger-only constructor) ───
    registry->RegisterStep(std::make_shared<WorkflowGraphicsInitDeviceStep>(logger_));
    registry->RegisterStep(std::make_shared<WorkflowGraphicsInitSwapchainStep>(logger_));
    count += 2;

    // ── Media (service-dependent, nullptr until wired) ────────
    registry->RegisterStep(std::make_shared<WorkflowMediaCatalogScanStep>(configSvc, logger_));
    registry->RegisterStep(std::make_shared<WorkflowMediaItemSelectStep>(audioSvc, logger_));
    count += 2;

    // ── Package shader loader ─────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowPackageShaderLoaderStep>(logger_, "", std::filesystem::path{}));
    count += 1;

    // ── Shader system (service-dependent, nullptr until wired) ─
    std::shared_ptr<IGraphicsService> graphicsSvc = nullptr;
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry = nullptr;
    registry->RegisterStep(std::make_shared<WorkflowShaderBuiltinConstantColorStep>(logger_, graphicsSvc));
    registry->RegisterStep(std::make_shared<WorkflowShaderCompileStep>(logger_, shaderRegistry, graphicsSvc));
    registry->RegisterStep(std::make_shared<WorkflowShaderSystemInitializeStep>(logger_, shaderRegistry, graphicsSvc));
    registry->RegisterStep(std::make_shared<WorkflowShaderSystemSetStep>(logger_, shaderRegistry));
    count += 4;

    // ── System ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowExitStep>(logger_));
    count += 1;

    if (logger_) {
        logger_->Info("WorkflowRegistrar: " + std::to_string(count) + " base steps registered");
    }
}

void WorkflowRegistrar::RegisterExecutorSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<IWorkflowExecutor> executor) {
    if (!registry || !executor) return;

    registry->RegisterStep(std::make_shared<WorkflowControlWhileStep>(logger_, executor));
    registry->RegisterStep(std::make_shared<WorkflowExecuteStep>(logger_, executor));

    if (logger_) {
        logger_->Info("WorkflowRegistrar: 2 executor-dependent steps registered");
    }
}

}  // namespace sdl3cpp::services::impl
