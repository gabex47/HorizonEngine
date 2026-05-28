#pragma once

#include "Instance.h"
#include "Part.h"

#include <memory>
#include <string>

namespace Horizon::EditorActions {

std::shared_ptr<Instance> CreateInstance(const std::string& className);
std::shared_ptr<Instance> InsertObject(const std::string& className, std::shared_ptr<Instance> parent);
std::shared_ptr<Instance> GetServerScriptsRoot();
void ResetScene();
bool EnterPlayMode();
bool ExitPlayMode();

} // namespace Horizon::EditorActions
