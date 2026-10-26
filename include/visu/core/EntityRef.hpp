#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// Reference vers une autre entite, reglable dans l'editeur en glissant une
// entite de la hierarchie sur le champ.
//
//   ee::EntityRef cible;                          // n'importe quelle entite
//   ee::EntityRefOf<CameraComponent> camera;      // seulement une entite qui
//                                                 // a un CameraComponent
//
// Dans la scene (.json) : index de l'entite, -1 = aucune. Dans le jeu : id
// est l'EntityID du World (converti au chargement), utilisable directement :
//   if (player.camera)
//       auto *tf = world.getComponent<TransformComponent>(player.camera.id);
// ---------------------------------------------------------------------------
namespace ee
{
    struct EntityRef
    {
        static constexpr uint32_t kNone = 0xFFFFFFFFu;

        uint32_t id = kNone;

        bool valid() const { return id != kNone; }
        explicit operator bool() const { return valid(); }
    };

    // Variante typee : l'editeur refuse les entites sans le composant C.
    template <typename C>
    struct EntityRefOf : EntityRef
    {
    };
}
