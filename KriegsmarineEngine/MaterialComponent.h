#pragma once
#include "Core.Definition.h"
#include "Material.h"
#include "Core.Memory.h"

struct MaterialComponent
{
    SegmentedPointer<MaterialInstance> material{};
};
