#pragma once

#include "io/StructureLoader.h"

#include <functional>

struct CSLGrainBoundaryDialog
{
    void drawMenuItem(bool enabled);
    void drawDialog(Structure& structure,
                    const std::function<void(Structure&)>& updateBuffers);

private:
    bool m_openRequested = false;
};
