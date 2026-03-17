#pragma once

#include "io/StructureLoader.h"

struct StructureInfoDialogState
{
    bool openRequested = false;
};

void drawStructureInfoDialog(StructureInfoDialogState& state,
                             bool requestOpen,
                             const Structure& structure);