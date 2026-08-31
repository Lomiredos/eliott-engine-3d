#pragma once

// ---------------------------------------------------------------------------
// Le contrat de REFLEXION : le pont entre une struct C++ (la verite d'un
// composant) et le monde data/editeur (catalogue JSON, inspecteur).
//
// Chaque composant SPECIALISE ce template pour se decrire :
//
//   template<> struct ee::reflection::Reflect<PlayerComponent> {
//       static constexpr const char *name = "PlayerComponent";
//       static void visit(PlayerComponent &_c, FieldVisitor &_v) {
//           _v.visit("speed", _c.speed);
//       }
//   };
//
// Le generateur parcourt les champs via visit() -> emet le catalogue. Un type
// que le FieldVisitor ne connait pas (voir FieldVisitor.hpp) n'est pas
// serialisable : soit c'est une struct elle-meme reflechie (recursion), soit
// il est ignore (runtime-only).
// ---------------------------------------------------------------------------

namespace ee::reflection
{
    template <typename T>
    struct Reflect;
}
