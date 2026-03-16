#pragma once

#include <functional>

struct TransformAtomsDialog
{
    bool isEnabled() const { return useTransformMatrix; }
    const int (&getMatrix() const)[3][3] { return transformMatrix; }

    void drawMenuItem(bool hasUnitCell);
    void drawDialog(const std::function<void()>& onApply);

private:
    bool showDialog = false;
    bool useTransformMatrix = false;

    int transformMatrix[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };

    int pendingMatrix[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
};
