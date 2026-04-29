#pragma once

#include <string>
#include <vector>

// Returns true if argv contains a flag that should invoke CLI (headless) mode
// rather than launching the GUI.
bool isCLIMode(int argc, char* argv[]);

// Run the headless CLI build pipeline. Returns 0 on success, non-zero on error.
int runCLI(int argc, char* argv[]);
