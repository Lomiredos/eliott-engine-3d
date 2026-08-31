#pragma once

#include "math/Vector3.hpp"

#include <string>
#include <vector>
#include <filesystem>

// ---------------------------------------------------------------------------
// Magasin de meshes charges depuis des fichiers (.obj).
//
// Charge une fois, cache par chemin. Les donnees vivent cote CPU : elles
// servent au DRAW (envoyees au GPU via le renderer, handle memorise) ET au
// PICK (AABB testee contre le rayon). C'est le point de partage entre les deux.
// ---------------------------------------------------------------------------

namespace ee::core
{
    struct CpuMesh
    {
        std::vector<float> verts;       // interleaved x,y,z, nx,ny,nz
        std::vector<unsigned int> idx;  // indices (triangles)
        ee::math::Vector3<float> aabbMin;
        ee::math::Vector3<float> aabbMax;
        unsigned int gpu = 0; // handle renderer, 0 = pas encore envoye au GPU
        bool valid = false;   // false si le .obj est introuvable / vide / casse
    };

    // Dossier de base (racine du projet/jeu) pour resoudre les chemins .obj
    // RELATIFS. Les chemins absolus l'ignorent. A appeler a l'ouverture du projet.
    void setMeshBaseDir(const std::filesystem::path &_dir);

    // Renvoie le mesh pour ce chemin (charge + cache au 1er appel). Un chemin
    // relatif part du dossier de base ci-dessus. Toujours renvoyee : un .obj
    // introuvable donne un CpuMesh 'valid == false'.
    CpuMesh &getMesh(const std::string &_path);
}
