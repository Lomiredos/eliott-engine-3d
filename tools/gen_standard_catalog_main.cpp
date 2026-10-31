#include "visu/components/Components.hpp"
#include "visu/reflect/CatalogGen.hpp"
#include <iostream>


int main(int argc, char ** argv){
    if (argc < 2) {std::cerr << "Usage : gen_standard_catalog <sortie.json>\n"; return 1;}
    nlohmann::json comps = nlohmann::json::array();
    ee::reflection::emitComponent<TransformComponent>(comps);
    ee::reflection::emitComponent<SphereComponent>(comps);
    ee::reflection::emitComponent<RectComponent>(comps);
    ee::reflection::emitComponent<CapsuleComponent>(comps);
    ee::reflection::emitComponent<MeshComponent>(comps);
    ee::reflection::emitComponent<RigidBodyComponent>(comps);
    ee::reflection::emitComponent<ColliderComponent>(comps);
    ee::reflection::emitComponent<CameraComponent>(comps);
    ee::reflection::emitComponent<MaterialComponent>(comps);
    ee::reflection::emitComponent<DirectionalLightComponent>(comps);
    ee::reflection::emitComponent<PointLightComponent>(comps);
    ee::reflection::emitComponent<AmbientLightComponent>(comps);
    ee::reflection::writeCatalog(comps, argv[1]);
}