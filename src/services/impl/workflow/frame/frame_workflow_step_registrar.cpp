#include "frame_workflow_step_registrar.hpp"

#include "workflow_frame_audio_step.hpp"
#include "workflow_frame_begin_step.hpp"
#include "workflow_frame_camera_step.hpp"
#include "workflow_frame_bullet_physics_step.hpp"
#include "workflow_frame_gui_step.hpp"
#include "workflow_frame_physics_step.hpp"
#include "workflow_frame_render_step.hpp"
#include "workflow_frame_scene_step.hpp"
#include "../workflow_generic_steps/workflow_audio_play_step.hpp"
#include "../workflow_generic_steps/workflow_audio_set_volume_step.hpp"
#include "../workflow_generic_steps/workflow_audio_stop_step.hpp"
#include "../workflow_generic_steps/workflow_bool_and_step.hpp"
#include "../workflow_generic_steps/workflow_bool_not_step.hpp"
#include "../workflow_generic_steps/workflow_bool_or_step.hpp"
#include "../workflow_generic_steps/workflow_camera_build_view_state_step.hpp"
#include "../workflow_generic_steps/workflow_camera_look_at_step.hpp"
#include "../workflow_generic_steps/workflow_camera_set_fov_step.hpp"
#include "../workflow_generic_steps/workflow_camera_set_pose_step.hpp"
#include "../workflow_generic_steps/workflow_camera_teleport_step.hpp"
#include "../workflow_generic_steps/workflow_compare_eq_step.hpp"
#include "../workflow_generic_steps/workflow_compare_gte_step.hpp"
#include "../workflow_generic_steps/workflow_compare_gt_step.hpp"
#include "../workflow_generic_steps/workflow_compare_lte_step.hpp"
#include "../workflow_generic_steps/workflow_compare_lt_step.hpp"
#include "../workflow_generic_steps/workflow_compare_ne_step.hpp"
#include "../workflow_generic_steps/workflow_list_append_step.hpp"
#include "../workflow_generic_steps/workflow_list_concat_step.hpp"
#include "../workflow_generic_steps/workflow_list_count_step.hpp"
#include "../workflow_generic_steps/workflow_list_filter_equals_step.hpp"
#include "../workflow_generic_steps/workflow_list_filter_gt_step.hpp"
#include "../workflow_generic_steps/workflow_list_literal_step.hpp"
#include "../workflow_generic_steps/workflow_list_map_add_step.hpp"
#include "../workflow_generic_steps/workflow_list_map_mul_step.hpp"
#include "../workflow_generic_steps/workflow_list_reduce_max_step.hpp"
#include "../workflow_generic_steps/workflow_list_reduce_min_step.hpp"
#include "../workflow_generic_steps/workflow_list_reduce_sum_step.hpp"
#include "../workflow_generic_steps/workflow_mesh_load_step.hpp"
#include "../workflow_generic_steps/workflow_model_despawn_step.hpp"
#include "../workflow_generic_steps/workflow_model_set_transform_step.hpp"
#include "../workflow_generic_steps/workflow_model_spawn_step.hpp"
#include "../workflow_generic_steps/workflow_number_abs_step.hpp"
#include "../workflow_generic_steps/workflow_number_add_step.hpp"
#include "../workflow_generic_steps/workflow_number_clamp_step.hpp"
#include "../workflow_generic_steps/workflow_number_div_step.hpp"
#include "../workflow_generic_steps/workflow_number_max_step.hpp"
#include "../workflow_generic_steps/workflow_number_min_step.hpp"
#include "../workflow_generic_steps/workflow_number_mul_step.hpp"
#include "../workflow_generic_steps/workflow_number_round_step.hpp"
#include "../workflow_generic_steps/workflow_number_sub_step.hpp"
#include "../workflow_generic_steps/workflow_scene_clear_step.hpp"
#include "../workflow_generic_steps/workflow_scene_load_step.hpp"
#include "../workflow_generic_steps/workflow_scene_set_active_step.hpp"
#include "../workflow_generic_steps/workflow_string_concat_step.hpp"
#include "../workflow_generic_steps/workflow_string_contains_step.hpp"
#include "../workflow_generic_steps/workflow_string_equals_step.hpp"
#include "../workflow_generic_steps/workflow_string_join_step.hpp"
#include "../workflow_generic_steps/workflow_string_lower_step.hpp"
#include "../workflow_generic_steps/workflow_string_replace_step.hpp"
#include "../workflow_generic_steps/workflow_string_split_step.hpp"
#include "../workflow_generic_steps/workflow_string_trim_step.hpp"
#include "../workflow_generic_steps/workflow_string_upper_step.hpp"
#include "../workflow_generic_steps/workflow_value_assert_exists_step.hpp"
#include "../workflow_generic_steps/workflow_value_assert_type_step.hpp"
#include "../workflow_generic_steps/workflow_value_clear_step.hpp"
#include "../workflow_soundboard_audio_step.hpp"
#include "../workflow_soundboard_catalog_scan_step.hpp"
#include "../workflow_soundboard_gui_step.hpp"
#include "../workflow_step_registry.hpp"
#include "../workflow_generic_steps/workflow_value_copy_step.hpp"
#include "../workflow_generic_steps/workflow_value_default_step.hpp"
#include "../workflow_generic_steps/workflow_value_literal_step.hpp"
#include "../workflow_validation_checkpoint_step.hpp"

#include <stdexcept>
#include <unordered_set>

namespace sdl3cpp::services::impl {

FrameWorkflowStepRegistrar::FrameWorkflowStepRegistrar(std::shared_ptr<ILogger> logger,
                                                       std::shared_ptr<IConfigService> configService,
                                                       std::shared_ptr<IAudioService> audioService,
                                                       std::shared_ptr<IInputService> inputService,
                                                       std::shared_ptr<IMeshService> meshService,
                                                       std::shared_ptr<IPhysicsService> physicsService,
                                                       std::shared_ptr<ISceneService> sceneService,
                                                       std::shared_ptr<IRenderCoordinatorService> renderService,
                                                       std::shared_ptr<IValidationTourService> validationTourService,
                                                       std::shared_ptr<ISoundboardStateService> soundboardStateService)
    : logger_(std::move(logger)),
      configService_(std::move(configService)),
      audioService_(std::move(audioService)),
      inputService_(std::move(inputService)),
      meshService_(std::move(meshService)),
      physicsService_(std::move(physicsService)),
      sceneService_(std::move(sceneService)),
      renderService_(std::move(renderService)),
      validationTourService_(std::move(validationTourService)),
      soundboardStateService_(std::move(soundboardStateService)) {}

void FrameWorkflowStepRegistrar::RegisterUsedSteps(
    const WorkflowDefinition& workflow,
    const std::shared_ptr<IWorkflowStepRegistry>& registry) const {
    if (!registry) {
        throw std::runtime_error("FrameWorkflowStepRegistrar: registry is null");
    }
    std::unordered_set<std::string> plugins;
    for (const auto& step : workflow.steps) {
        plugins.insert(step.plugin);
    }

    if (plugins.contains("frame.begin")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameBeginStep>(logger_));
    }
    if (plugins.contains("frame.physics")) {
        registry->RegisterStep(std::make_shared<WorkflowFramePhysicsStep>(physicsService_, logger_));
    }
    if (plugins.contains("frame.bullet_physics")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameBulletPhysicsStep>(physicsService_, logger_));
    }
    if (plugins.contains("frame.camera")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameCameraStep>(configService_, logger_));
    }
    if (plugins.contains("frame.scene")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameSceneStep>(sceneService_, logger_));
    }
    if (plugins.contains("frame.render")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameRenderStep>(renderService_, logger_));
    }
    if (plugins.contains("frame.audio")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameAudioStep>(audioService_, logger_));
    }
    if (plugins.contains("frame.gui")) {
        registry->RegisterStep(std::make_shared<WorkflowFrameGuiStep>(inputService_, logger_));
    }
    if (plugins.contains("validation.tour.checkpoint")) {
        registry->RegisterStep(std::make_shared<WorkflowValidationCheckpointStep>(
            validationTourService_, logger_));
    }
    if (plugins.contains("soundboard.catalog.scan")) {
        registry->RegisterStep(std::make_shared<WorkflowSoundboardCatalogScanStep>(configService_, logger_));
    }
    if (plugins.contains("soundboard.gui")) {
        registry->RegisterStep(std::make_shared<WorkflowSoundboardGuiStep>(inputService_,
                                                                          configService_,
                                                                          soundboardStateService_,
                                                                          logger_));
    }
    if (plugins.contains("soundboard.audio")) {
        registry->RegisterStep(std::make_shared<WorkflowSoundboardAudioStep>(audioService_,
                                                                             soundboardStateService_,
                                                                             logger_));
    }
    if (plugins.contains("camera.set_pose")) {
        registry->RegisterStep(std::make_shared<WorkflowCameraSetPoseStep>(logger_));
    }
    if (plugins.contains("camera.look_at")) {
        registry->RegisterStep(std::make_shared<WorkflowCameraLookAtStep>(logger_));
    }
    if (plugins.contains("camera.teleport")) {
        registry->RegisterStep(std::make_shared<WorkflowCameraTeleportStep>(logger_));
    }
    if (plugins.contains("camera.set_fov")) {
        registry->RegisterStep(std::make_shared<WorkflowCameraSetFovStep>(logger_));
    }
    if (plugins.contains("camera.build_view_state")) {
        registry->RegisterStep(std::make_shared<WorkflowCameraBuildViewStateStep>(configService_, logger_));
    }
    if (plugins.contains("audio.play")) {
        registry->RegisterStep(std::make_shared<WorkflowAudioPlayStep>(audioService_, logger_));
    }
    if (plugins.contains("audio.stop")) {
        registry->RegisterStep(std::make_shared<WorkflowAudioStopStep>(audioService_, logger_));
    }
    if (plugins.contains("audio.set_volume")) {
        registry->RegisterStep(std::make_shared<WorkflowAudioSetVolumeStep>(audioService_, logger_));
    }
    if (plugins.contains("mesh.load")) {
        registry->RegisterStep(std::make_shared<WorkflowMeshLoadStep>(meshService_, logger_));
    }
    if (plugins.contains("model.spawn")) {
        registry->RegisterStep(std::make_shared<WorkflowModelSpawnStep>(logger_));
    }
    if (plugins.contains("model.despawn")) {
        registry->RegisterStep(std::make_shared<WorkflowModelDespawnStep>(logger_));
    }
    if (plugins.contains("model.set_transform")) {
        registry->RegisterStep(std::make_shared<WorkflowModelSetTransformStep>(logger_));
    }
    if (plugins.contains("scene.load")) {
        registry->RegisterStep(std::make_shared<WorkflowSceneLoadStep>(sceneService_, logger_));
    }
    if (plugins.contains("scene.clear")) {
        registry->RegisterStep(std::make_shared<WorkflowSceneClearStep>(sceneService_, logger_));
    }
    if (plugins.contains("scene.set_active")) {
        registry->RegisterStep(std::make_shared<WorkflowSceneSetActiveStep>(sceneService_, logger_));
    }
    if (plugins.contains("value.copy")) {
        registry->RegisterStep(std::make_shared<WorkflowValueCopyStep>(logger_));
    }
    if (plugins.contains("value.default")) {
        registry->RegisterStep(std::make_shared<WorkflowValueDefaultStep>(logger_));
    }
    if (plugins.contains("value.literal")) {
        registry->RegisterStep(std::make_shared<WorkflowValueLiteralStep>(logger_));
    }
    if (plugins.contains("value.clear")) {
        registry->RegisterStep(std::make_shared<WorkflowValueClearStep>(logger_));
    }
    if (plugins.contains("value.assert.exists")) {
        registry->RegisterStep(std::make_shared<WorkflowValueAssertExistsStep>(logger_));
    }
    if (plugins.contains("value.assert.type")) {
        registry->RegisterStep(std::make_shared<WorkflowValueAssertTypeStep>(logger_));
    }
    if (plugins.contains("number.add")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberAddStep>(logger_));
    }
    if (plugins.contains("number.sub")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberSubStep>(logger_));
    }
    if (plugins.contains("number.mul")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberMulStep>(logger_));
    }
    if (plugins.contains("number.div")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberDivStep>(logger_));
    }
    if (plugins.contains("number.min")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberMinStep>(logger_));
    }
    if (plugins.contains("number.max")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberMaxStep>(logger_));
    }
    if (plugins.contains("number.clamp")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberClampStep>(logger_));
    }
    if (plugins.contains("number.abs")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberAbsStep>(logger_));
    }
    if (plugins.contains("number.round")) {
        registry->RegisterStep(std::make_shared<WorkflowNumberRoundStep>(logger_));
    }
    if (plugins.contains("compare.eq")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareEqStep>(logger_));
    }
    if (plugins.contains("compare.ne")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareNeStep>(logger_));
    }
    if (plugins.contains("compare.gt")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareGtStep>(logger_));
    }
    if (plugins.contains("compare.gte")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareGteStep>(logger_));
    }
    if (plugins.contains("compare.lt")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareLtStep>(logger_));
    }
    if (plugins.contains("compare.lte")) {
        registry->RegisterStep(std::make_shared<WorkflowCompareLteStep>(logger_));
    }
    if (plugins.contains("bool.and")) {
        registry->RegisterStep(std::make_shared<WorkflowBoolAndStep>(logger_));
    }
    if (plugins.contains("bool.or")) {
        registry->RegisterStep(std::make_shared<WorkflowBoolOrStep>(logger_));
    }
    if (plugins.contains("bool.not")) {
        registry->RegisterStep(std::make_shared<WorkflowBoolNotStep>(logger_));
    }
    if (plugins.contains("string.concat")) {
        registry->RegisterStep(std::make_shared<WorkflowStringConcatStep>(logger_));
    }
    if (plugins.contains("string.trim")) {
        registry->RegisterStep(std::make_shared<WorkflowStringTrimStep>(logger_));
    }
    if (plugins.contains("string.lower")) {
        registry->RegisterStep(std::make_shared<WorkflowStringLowerStep>(logger_));
    }
    if (plugins.contains("string.upper")) {
        registry->RegisterStep(std::make_shared<WorkflowStringUpperStep>(logger_));
    }
    if (plugins.contains("string.replace")) {
        registry->RegisterStep(std::make_shared<WorkflowStringReplaceStep>(logger_));
    }
    if (plugins.contains("string.contains")) {
        registry->RegisterStep(std::make_shared<WorkflowStringContainsStep>(logger_));
    }
    if (plugins.contains("string.equals")) {
        registry->RegisterStep(std::make_shared<WorkflowStringEqualsStep>(logger_));
    }
    if (plugins.contains("string.split")) {
        registry->RegisterStep(std::make_shared<WorkflowStringSplitStep>(logger_));
    }
    if (plugins.contains("string.join")) {
        registry->RegisterStep(std::make_shared<WorkflowStringJoinStep>(logger_));
    }
    if (plugins.contains("list.literal")) {
        registry->RegisterStep(std::make_shared<WorkflowListLiteralStep>(logger_));
    }
    if (plugins.contains("list.append")) {
        registry->RegisterStep(std::make_shared<WorkflowListAppendStep>(logger_));
    }
    if (plugins.contains("list.concat")) {
        registry->RegisterStep(std::make_shared<WorkflowListConcatStep>(logger_));
    }
    if (plugins.contains("list.filter.equals")) {
        registry->RegisterStep(std::make_shared<WorkflowListFilterEqualsStep>(logger_));
    }
    if (plugins.contains("list.filter.gt")) {
        registry->RegisterStep(std::make_shared<WorkflowListFilterGtStep>(logger_));
    }
    if (plugins.contains("list.map.add")) {
        registry->RegisterStep(std::make_shared<WorkflowListMapAddStep>(logger_));
    }
    if (plugins.contains("list.map.mul")) {
        registry->RegisterStep(std::make_shared<WorkflowListMapMulStep>(logger_));
    }
    if (plugins.contains("list.reduce.sum")) {
        registry->RegisterStep(std::make_shared<WorkflowListReduceSumStep>(logger_));
    }
    if (plugins.contains("list.reduce.min")) {
        registry->RegisterStep(std::make_shared<WorkflowListReduceMinStep>(logger_));
    }
    if (plugins.contains("list.reduce.max")) {
        registry->RegisterStep(std::make_shared<WorkflowListReduceMaxStep>(logger_));
    }
    if (plugins.contains("list.count")) {
        registry->RegisterStep(std::make_shared<WorkflowListCountStep>(logger_));
    }
}

}  // namespace sdl3cpp::services::impl
