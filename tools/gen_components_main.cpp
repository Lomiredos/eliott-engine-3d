// Petit outil autonome, compile uniquement sur demande explicite (cible
// "gen_components", jamais part de ALL) : relit les composants CUSTOM du
// projet (via Components/RegisterComponents.hpp, genere par EE-Visu) et
// ecrit le catalogue assets/Components.json, pour que l'editeur connaisse
// leurs vrais champs (via Reflect<T>, jamais a la main).
#include "Components/RegisterComponents.hpp"
#include "visu/reflect/CatalogGen.hpp"

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    // Racine du projet deduite du chemin de l'executable (meme convention
    // que ee::runtime::detail::defaultProjectRoot() : <racine>/build/<exe>).
    std::filesystem::path projectRoot()
    {
#ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        std::filesystem::path exe(buffer);
#else
        std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe");
#endif
        return exe.parent_path().parent_path(); // .../build/exe -> ...
    }
}

int main()
{
    nlohmann::json components = nlohmann::json::array();
    buildComponentCatalog(components);
    ee::reflection::writeCatalog(components, projectRoot() / "Assets" / "Components.json");
    return 0;
}
