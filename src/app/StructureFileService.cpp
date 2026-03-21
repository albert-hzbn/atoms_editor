#include "app/StructureFileService.h"

#include "graphics/StructureInstanceBuilder.h"

bool loadStructureFromPath(
    const std::string& filePath,
    Structure& structure,
    std::string& errorMessage)
{
    return loadStructureFromFile(filePath, structure, errorMessage);
}

bool saveStructureWithOptionalSupercell(
    const Structure& structure,
    bool useTransformMatrix,
    const int (&transformMatrix)[3][3],
    const std::string& outputPath,
    const std::string& format,
    std::size_t& savedAtomCount)
{
    const bool shouldExpandToSupercell = useTransformMatrix && structure.hasUnitCell;
    const Structure structureToSave = shouldExpandToSupercell
        ? buildSupercell(structure, transformMatrix)
        : structure;

    savedAtomCount = structureToSave.atoms.size();
    return saveStructure(structureToSave, outputPath, format);
}
