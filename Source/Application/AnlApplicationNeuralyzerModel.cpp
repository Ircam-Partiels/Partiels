#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::ModelInfo::ModelInfo(Accessor const& accessor)
: model(accessor.getAttr<AttrType::modelFile>())
, tplt(model.withFileExtension(".jinja").existsAsFile() ? model.withFileExtension(".jinja") : juce::File())
, contextSize(std::make_optional(accessor.getAttr<AttrType::contextSize>()))
, batchSize(std::make_optional(accessor.getAttr<AttrType::batchSize>()))
, minP(std::make_optional(accessor.getAttr<AttrType::minP>()))
, temperature(std::make_optional(accessor.getAttr<AttrType::temperature>()))
{
}

ANALYSE_FILE_END
