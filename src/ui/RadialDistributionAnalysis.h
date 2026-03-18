#pragma once

#include "io/StructureLoader.h"

struct RadialDistributionAnalysisDialog
{
    void drawMenuItem(bool enabled);
    void drawDialog(const Structure& structure);

private:
    bool m_openRequested = false;
};
