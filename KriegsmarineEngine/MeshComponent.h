#pragma once
#include "Core.Definition.h"
#include "Mesh.h"
#include "Core.Memory.h"

struct MeshComponent
{
    SegmentedPointer<Mesh> mesh{};
};
