#include "AnlApplicationNeuralyzerModel.h"

ANALYSE_FILE_BEGIN

Application::Neuralyzer::ModelInfo::ModelInfo(Accessor const& accessor)
: model(accessor.getAttr<AttrType::modelFile>())
, tplt(model.withFileExtension(".jinja"))
, contextSize(accessor.getAttr<AttrType::contextSize>())
, batchSize(accessor.getAttr<AttrType::batchSize>())
, minP(accessor.getAttr<AttrType::minP>())
, temperature(accessor.getAttr<AttrType::temperature>())
{
}

ANALYSE_FILE_END
