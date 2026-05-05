#pragma once

#include "io/StructureLoader.h"
#include "algorithms/AngularDistributionAnalysis.h"

#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <string>

struct AngularDistributionAnalysisDialog
{
    AngularDistributionAnalysisDialog()  = default;
    ~AngularDistributionAnalysisDialog();

    AngularDistributionAnalysisDialog(const AngularDistributionAnalysisDialog&)            = delete;
    AngularDistributionAnalysisDialog& operator=(const AngularDistributionAnalysisDialog&) = delete;

    void drawMenuItem(bool enabled);
    void drawDialog(const Structure& structure);

private:
    bool m_openRequested = false;

    // Parameters
    float        m_rCutoff      = 3.5f;
    int          m_binCount     = 180;
    bool         m_usePbc       = true;
    bool         m_normalize    = true;
    int          m_smoothPasses = 2;
    int          m_centreMode   = 0;   // 0=All 1=ByElement 2=ByPair
    char         m_centreSymbol[8]  = "";
    char         m_neighSym1[8]     = "";
    char         m_neighSym2[8]     = "";

    // Display options
    bool  m_showPeaks       = true;
    bool  m_showCoordStats  = true;
    bool  m_showRawCounts   = false;
    float m_xMin            = 0.0f;
    float m_xMax            = 180.0f;

    // UI state
    bool  m_paramsDirty     = false;   // settings changed since last compute

    // Background thread
    std::unique_ptr<std::thread> m_workerThread;
    std::atomic<bool>            m_isComputing{false};
    std::atomic<bool>            m_computeCompleted{false};
    Structure                    m_workerStructure;
    AdfResult                    m_workerResult;

    // Results (main-thread safe after swap)
    AdfResult m_result;
    bool      m_hasResult = false;

    // Unique elements found in last structure (for combos)
    std::vector<std::string> m_elements;

    void startCompute(const Structure& structure);
    void pollWorker();
    void updateElementList(const Structure& structure);
    void drawPlot();
    void drawPeakTable();
    void drawCoordStats();
    void drawSettings(const Structure& structure);
    void drawResultSummary();
};
