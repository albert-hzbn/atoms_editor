#pragma once

#include "io/StructureLoader.h"

struct CommonNeighbourAnalysisDialog
{
    void drawMenuItem(bool enabled);
    void drawDialog(const Structure& structure);

private:
    bool m_openRequested = false;
};
