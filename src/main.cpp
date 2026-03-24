#include "app/EditorApplication.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc > 2)
        std::cout << "[Startup] Ignoring extra command-line arguments after the first structure path." << std::endl;

    const std::string startupStructurePath = (argc > 1) ? argv[1] : std::string();
    return runAtomsEditor(startupStructurePath);
}

