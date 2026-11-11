#include "TestUtils.hpp"

using namespace fsmtest;

namespace {
    const std::string kParams = R"([{"name":"go","type":"trigger"},{"name":"speed","type":"float"}])";
    const std::string kStates = recStates({"A", "B"});

    Loaded loadBroken(const std::string& _text){
        Loaded l = load(_text);
        CHECK_FALSE(l.def.has_value());
        CHECK_FALSE(l.errors.empty());
        return l;
    }
}

TEST_CASE("fichier valide : noms convertis en index"){
    auto def = mustLoad(makeFsm(kParams, recStates({"A", "B", "C"}), "B", R"([
        {"from":"A","to":"C","conditions":[{"param":"speed","op":">=","value":2}]},
        {"from":"Any","to":"A","conditions":[{"param":"go"}]}])"));
    CHECK(def.name == "T");
    CHECK(def.entryState == 1);
    REQUIRE(def.states[0].transition.size() == 1);
    CHECK(def.states[0].transition[0].targetState == 2);
    REQUIRE(def.anyTransition.size() == 1);
    CHECK(def.anyTransition[0].targetState == 0);

    const auto* c = std::get_if<ee::fsm::ParamCondition>(&def.states[0].transition[0].conditions[0]);
    REQUIRE(c != nullptr);
    CHECK(c->param == 1);
    CHECK(c->op == ee::fsm::Op::Ge);
    REQUIRE(std::get_if<float>(&c->value) != nullptr);
    CHECK(std::get<float>(c->value) == doctest::Approx(2.f));
}

TEST_CASE(".fsm invalides rejetes proprement"){
    SUBCASE("JSON mal forme"){
        Loaded l = loadBroken(R"({"version":2, "states": [)");
        CHECK(hasMessage(l.errors, {"invalid JSON"}));
    }
    SUBCASE("racine qui n'est pas un objet"){
        Loaded l = loadBroken("[1, 2]");
        CHECK(hasMessage(l.errors, {"root"}));
    }
    SUBCASE("version absente"){
        Loaded l = loadBroken(R"({"states":)" + kStates + R"(,"entryState":"A","transitions":[]})");
        CHECK(hasMessage(l.errors, {"version"}));
    }
    SUBCASE("version non supportee"){
        Loaded l = loadBroken(R"({"version":99,"states":)" + kStates + R"(,"entryState":"A","transitions":[]})");
        CHECK(hasMessage(l.errors, {"99"}));
    }
    SUBCASE("states absent ou vide"){
        Loaded l = loadBroken(makeFsm("[]", "[]", "A", "[]"));
        CHECK(hasMessage(l.errors, {"states"}));
    }
    SUBCASE("etat d'entree manquant"){
        Loaded l = loadBroken(R"({"version":2,"states":)" + kStates + R"(,"transitions":[]})");
        CHECK(hasMessage(l.errors, {"entryState"}));
    }
    SUBCASE("etat d'entree inconnu"){
        Loaded l = loadBroken(makeFsm("[]", kStates, "Nowhere", "[]"));
        CHECK(hasMessage(l.errors, {"entryState", "Nowhere"}));
    }
    SUBCASE("etat d'entree Any"){
        Loaded l = loadBroken(makeFsm("[]", kStates, "Any", "[]"));
        CHECK(hasMessage(l.errors, {"entryState", "Any"}));
    }
    SUBCASE("etat inconnu dans from"){
        Loaded l = loadBroken(makeFsm("[]", kStates, "A", R"([{"from":"Atack","to":"B"}])"));
        CHECK(hasMessage(l.errors, {"transitions[0]", "Atack"}));
    }
    SUBCASE("etat inconnu dans to"){
        Loaded l = loadBroken(makeFsm("[]", kStates, "A", R"([{"from":"A","to":"A"},{"from":"A","to":"Bb"}])"));
        CHECK(hasMessage(l.errors, {"transitions[1]", "Bb"}));
    }
    SUBCASE("transition vers Any"){
        Loaded l = loadBroken(makeFsm("[]", kStates, "A", R"([{"from":"A","to":"Any"}])"));
        CHECK(hasMessage(l.errors, {"transitions[0]", "Any"}));
    }
    SUBCASE("allowSelf hors Any"){
        Loaded l = loadBroken(makeFsm("[]", kStates, "A", R"([{"from":"A","to":"B","allowSelf":true}])"));
        CHECK(hasMessage(l.errors, {"transitions[0]", "allowSelf"}));
    }
    SUBCASE("parametre inconnu dans une condition"){
        Loaded l = loadBroken(makeFsm(kParams, kStates, "A", R"([{"from":"A","to":"B","conditions":[{"param":"attak"}]}])"));
        CHECK(hasMessage(l.errors, {"transitions[0]", "conditions[0]", "attak"}));
    }
    SUBCASE("valeur integree inconnue"){
        Loaded l = loadBroken(makeFsm(kParams, kStates, "A", R"([{"from":"A","to":"B","conditions":[{"param":"$time","op":">","value":1}]}])"));
        CHECK(hasMessage(l.errors, {"conditions[0]", "$time"}));
    }
    SUBCASE("operateur inconnu"){
        Loaded l = loadBroken(makeFsm(kParams, kStates, "A", R"([{"from":"A","to":"B","conditions":[{"param":"speed","op":"=>","value":1}]}])"));
        CHECK(hasMessage(l.errors, {"conditions[0]", "=>"}));
    }
    SUBCASE("operateur d'ordre sur un trigger"){
        Loaded l = loadBroken(makeFsm(kParams, kStates, "A", R"([{"from":"A","to":"B","conditions":[{"param":"go","op":"<","value":true}]}])"));
        CHECK(hasMessage(l.errors, {"conditions[0]", "go"}));
    }
    SUBCASE("op ou value manquant sur un float"){
        Loaded l = loadBroken(makeFsm(kParams, kStates, "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"speed","value":1}]},
            {"from":"A","to":"B","conditions":[{"param":"speed","op":">"}]}])"));
        CHECK(hasMessage(l.errors, {"transitions[0]", "op", "speed"}));
        CHECK(hasMessage(l.errors, {"transitions[1]", "value", "speed"}));
    }
    SUBCASE("$stateTime sans op ni valeur numerique"){
        Loaded l = loadBroken(makeFsm(kParams, kStates, "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"$stateTime","value":1}]},
            {"from":"A","to":"B","conditions":[{"param":"$stateTime","op":">","value":"x"}]}])"));
        CHECK(hasMessage(l.errors, {"transitions[0]", "$stateTime"}));
        CHECK(hasMessage(l.errors, {"transitions[1]", "$stateTime"}));
    }
    SUBCASE("value d'une condition du mauvais type"){
        Loaded l = loadBroken(makeFsm(kParams, kStates, "A", R"([{"from":"A","to":"B","conditions":[{"param":"speed","op":">","value":"vite"}]}])"));
        CHECK(hasMessage(l.errors, {"conditions[0]", "speed"}));
    }
    SUBCASE("default du mauvais type"){
        Loaded l = loadBroken(makeFsm(R"([{"name":"hp","type":"int","default":1.5}])", kStates, "A", "[]"));
        CHECK(hasMessage(l.errors, {"parameters[0]", "hp"}));
    }
    SUBCASE("type de parametre inconnu"){
        Loaded l = loadBroken(makeFsm(R"([{"name":"hp","type":"double"}])", kStates, "A", "[]"));
        CHECK(hasMessage(l.errors, {"parameters[0]", "double"}));
    }
    SUBCASE("etat en double"){
        Loaded l = loadBroken(makeFsm("[]", recStates({"A", "B", "A"}), "A", "[]"));
        CHECK(hasMessage(l.errors, {"states[2]", "A"}));
    }
    SUBCASE("parametre en double"){
        Loaded l = loadBroken(makeFsm(R"([{"name":"go","type":"trigger"},{"name":"go","type":"bool"}])", kStates, "A", "[]"));
        CHECK(hasMessage(l.errors, {"parameters[1]", "go"}));
    }
    SUBCASE("etat nomme Any"){
        Loaded l = loadBroken(makeFsm("[]", recStates({"A", "Any"}), "A", "[]"));
        CHECK(hasMessage(l.errors, {"states[1]", "Any"}));
    }
    SUBCASE("parametre qui commence par $"){
        Loaded l = loadBroken(makeFsm(R"([{"name":"$hp","type":"int"}])", kStates, "A", "[]"));
        CHECK(hasMessage(l.errors, {"parameters[0]", "$hp"}));
    }
    SUBCASE("etat sans tableau actions"){
        Loaded l = loadBroken(makeFsm("[]", R"([{"name":"A"}])", "A", "[]"));
        CHECK(hasMessage(l.errors, {"states[0]", "actions"}));
    }
    SUBCASE("transitions absent"){
        Loaded l = loadBroken(R"({"version":2,"states":)" + kStates + R"(,"entryState":"A"})");
        CHECK(hasMessage(l.errors, {"transitions"}));
    }
    SUBCASE("toutes les erreurs sont collectees d'un coup"){
        Loaded l = loadBroken(makeFsm(R"([{"name":"hp","type":"int","default":true}])", recStates({"A", "A"}), "Zed", R"([
            {"from":"Q","to":"A"},
            {"from":"A","to":"A","conditions":[{"param":"mana","op":">","value":1}]}])"));
        CHECK(l.errors.size() >= 5);
        CHECK(hasMessage(l.errors, {"hp"}));
        CHECK(hasMessage(l.errors, {"states[1]"}));
        CHECK(hasMessage(l.errors, {"Zed"}));
        CHECK(hasMessage(l.errors, {"\"Q\""}));
        CHECK(hasMessage(l.errors, {"mana"}));
    }
}

TEST_CASE("action ou condition custom non enregistree : warning seulement"){
    Loaded l = load(makeFsm("[]", R"([{"name":"A","actions":[{"type":"Fireball","power":3}]},{"name":"B","actions":[]}])", "A", R"([
        {"from":"A","to":"B","conditions":[{"custom":"IsFlying"}]}])"));
    REQUIRE(l.def.has_value());
    CHECK(l.errors.empty());
    CHECK(hasMessage(l.warnings, {"Fireball"}));
    CHECK(hasMessage(l.warnings, {"IsFlying"}));

    REQUIRE(l.def->states[0].actions.size() == 1);
    CHECK(l.def->states[0].actions[0].action == nullptr);

    Runner r(*l.def);
    r.ticks(5);
    CHECK(r.state() == "A");
}

TEST_CASE("1 ecrit pour un float est converti"){
    auto def = mustLoad(makeFsm(R"([{"name":"speed","type":"float","default":1}])", kStates, "A", R"([
        {"from":"A","to":"B","conditions":[{"param":"speed","op":">","value":2}]}])"));
    REQUIRE(std::get_if<float>(&def.params[0].defaultValue) != nullptr);
    CHECK(std::get<float>(def.params[0].defaultValue) == doctest::Approx(1.f));
}
