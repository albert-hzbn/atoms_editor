#pragma once

#include <string>

// Call once (per substitution request) to open the modal picker.
void openPeriodicTable();

// Draw the periodic-table picker modal.  Call every ImGui frame.
// Returns true exactly once, when the user clicks an element.
// On true: outSymbol and outAtomicNumber hold the chosen element.
bool drawPeriodicTable(std::string& outSymbol, int& outAtomicNumber);
