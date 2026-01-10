#include "workflow_soundboard_catalog_scan_step.hpp"

#include "../config/json_config_document_parser.hpp"
#include "../soundboard/soundboard_path_resolver.hpp"
#include "workflow_step_io_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string PrettyClipName(const std::string& fileName) {
    std::string base = fileName;
    const auto dot = base.find_last_of('.');
    if (dot != std::string::npos) {
        base = base.substr(0, dot);
    }
    for (char& ch : base) {
        if (ch == '_' || ch == '-') {
            ch = ' ';
        }
    }
    bool capitalize = true;
    for (char& ch : base) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            capitalize = true;
        } else if (capitalize) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            capitalize = false;
        } else {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
    }
    return base;
}

std::vector<SoundboardClip> LoadClips(const std::filesystem::path& directory) {
    std::vector<SoundboardClip> clips;
    if (!std::filesystem::exists(directory)) {
        return clips;
    }
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto extension = ToLower(entry.path().extension().string());
        if (extension != ".ogg" && extension != ".wav") {
            continue;
        }
        const std::string fileName = entry.path().filename().string();
        SoundboardClip clip{};
        clip.id = fileName;
        clip.label = PrettyClipName(fileName);
        clip.path = entry.path();
        clips.push_back(std::move(clip));
    }
    std::sort(clips.begin(), clips.end(),
              [](const SoundboardClip& a, const SoundboardClip& b) {
                  return ToLower(a.id) < ToLower(b.id);
              });
    return clips;
}

SoundboardCatalog BuildCatalog(const rapidjson::Document& document,
                               const std::filesystem::path& packageRoot) {
    if (!document.HasMember("categories") || !document["categories"].IsArray()) {
        throw std::runtime_error("soundboard catalog requires a categories array");
    }
    SoundboardCatalog catalog{};
    catalog.packageRoot = packageRoot;
    const auto& categories = document["categories"];
    catalog.categories.reserve(categories.Size());
    for (const auto& categoryValue : categories.GetArray()) {
        if (!categoryValue.IsObject()) {
            throw std::runtime_error("soundboard catalog categories must be objects");
        }
        if (!categoryValue.HasMember("id") || !categoryValue["id"].IsString()) {
            throw std::runtime_error("soundboard catalog category requires string id");
        }
        if (!categoryValue.HasMember("name") || !categoryValue["name"].IsString()) {
            throw std::runtime_error("soundboard catalog category requires string name");
        }
        if (!categoryValue.HasMember("path") || !categoryValue["path"].IsString()) {
            throw std::runtime_error("soundboard catalog category requires string path");
        }
        SoundboardCategory category{};
        category.id = categoryValue["id"].GetString();
        category.name = categoryValue["name"].GetString();
        category.basePath = packageRoot / categoryValue["path"].GetString();
        category.clips = LoadClips(category.basePath);
        catalog.categories.push_back(std::move(category));
    }
    return catalog;
}

}  // namespace

WorkflowSoundboardCatalogScanStep::WorkflowSoundboardCatalogScanStep(
    std::shared_ptr<IConfigService> configService,
    std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)) {}

std::string WorkflowSoundboardCatalogScanStep::GetPluginId() const {
    return "soundboard.catalog.scan";
}

void WorkflowSoundboardCatalogScanStep::Execute(const WorkflowStepDefinition& step,
                                                WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "catalog");

    if (!cachedCatalog_) {
        cachedCatalog_ = LoadCatalog();
        if (logger_) {
            std::size_t clipCount = 0;
            for (const auto& category : cachedCatalog_->categories) {
                clipCount += category.clips.size();
            }
            logger_->Trace("WorkflowSoundboardCatalogScanStep", "Execute",
                           "categories=" + std::to_string(cachedCatalog_->categories.size()) +
                               ", clips=" + std::to_string(clipCount),
                           "Catalog scanned");
        }
    }

    context.Set(outputKey, *cachedCatalog_);
}

SoundboardCatalog WorkflowSoundboardCatalogScanStep::LoadCatalog() const {
    const std::filesystem::path packageRoot = ResolveSoundboardPackageRoot(configService_);
    const std::filesystem::path catalogPath = packageRoot / "assets" / "audio_catalog.json";
    json_config::JsonConfigDocumentParser parser;
    auto document = parser.Parse(catalogPath, "soundboard audio catalog");
    return BuildCatalog(document, packageRoot);
}

}  // namespace sdl3cpp::services::impl
