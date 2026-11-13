#include "TestUtils.hpp"

using namespace fsmtest;
using V = std::vector<std::string>;

TEST_CASE("premier tick : entree dans entryState"){
    auto def = mustLoad(makeFsm("[]", recStates({"A", "B"}), "B", "[]"));
    Runner r(def);
    journal().clear();
    CHECK(r.state() == "");
    r.tick();
    CHECK(r.state() == "B");
    CHECK(journal() == V{"B:enter"});
}

TEST_CASE("transitions simples et chainees"){
    auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B", "C"}), "A", R"([
        {"from":"A","to":"B","conditions":[{"param":"go"}]},
        {"from":"B","to":"C","conditions":[{"param":"go"}]},
        {"from":"C","to":"A","conditions":[{"param":"go"}]}])"));
    Runner r(def);
    r.tick();
    r.tick();
    CHECK(r.state() == "A");

    for (const char* expected : {"B", "C", "A", "B"}){
        ee::fsm::setTrigger(def, r.inst, "go");
        r.tick();
        CHECK(r.state() == expected);
    }
}

TEST_CASE("une seule transition par frame"){
    auto def = mustLoad(makeFsm("[]", recStates({"A", "B", "C"}), "A", R"([
        {"from":"A","to":"B"},
        {"from":"B","to":"C"}])"));
    Runner r(def);
    r.tick();
    CHECK(r.state() == "A");
    r.tick();
    CHECK(r.state() == "B");
    r.tick();
    CHECK(r.state() == "C");
}

TEST_CASE("ordre d'evaluation"){
    SUBCASE("Any passe avant l'etat courant"){
        auto def = mustLoad(makeFsm("[]", recStates({"A", "B", "C"}), "A", R"([
            {"from":"A","to":"B"},
            {"from":"Any","to":"C"}])"));
        Runner r(def);
        r.ticks(2);
        CHECK(r.state() == "C");
    }
    SUBCASE("deux transitions valides : la premiere du tableau gagne"){
        auto def = mustLoad(makeFsm("[]", recStates({"A", "B", "C"}), "A", R"([
            {"from":"A","to":"B"},
            {"from":"A","to":"C"}])"));
        Runner r(def);
        r.ticks(2);
        CHECK(r.state() == "B");
    }
    SUBCASE("deux Any valides : la premiere du tableau gagne"){
        auto def = mustLoad(makeFsm("[]", recStates({"A", "B", "C"}), "A", R"([
            {"from":"Any","to":"C"},
            {"from":"Any","to":"B"}])"));
        Runner r(def);
        r.ticks(2);
        CHECK(r.state() == "C");
    }
}

TEST_CASE("triggers"){
    SUBCASE("consomme une seule fois"){
        auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B", "C"}), "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"go"}]},
            {"from":"B","to":"C","conditions":[{"param":"go"}]}])"));
        Runner r(def);
        r.tick();
        ee::fsm::setTrigger(def, r.inst, "go");
        r.tick();
        CHECK(r.state() == "B");
        CHECK(ee::fsm::getTrigger(def, r.inst, "go") == false);
        r.ticks(5);
        CHECK(r.state() == "B");
    }
    SUBCASE("pas consomme par une transition qui echoue (input buffering)"){
        auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"},{"name":"ok","type":"bool"}])",
                                    recStates({"A", "B"}), "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"go"},{"param":"ok"}]}])"));
        Runner r(def);
        r.tick();
        ee::fsm::setTrigger(def, r.inst, "go");
        r.ticks(3);
        CHECK(r.state() == "A");
        CHECK(ee::fsm::getTrigger(def, r.inst, "go") == true);

        ee::fsm::setBool(def, r.inst, "ok", true);
        r.tick();
        CHECK(r.state() == "B");
        CHECK(ee::fsm::getTrigger(def, r.inst, "go") == false);
    }
    SUBCASE("seuls les triggers lus par la transition prise sont remis a false"){
        auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"},{"name":"other","type":"trigger"}])",
                                    recStates({"A", "B"}), "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"go"}]}])"));
        Runner r(def);
        r.tick();
        ee::fsm::setTrigger(def, r.inst, "go");
        ee::fsm::setTrigger(def, r.inst, "other");
        r.tick();
        CHECK(r.state() == "B");
        CHECK(ee::fsm::getTrigger(def, r.inst, "other") == true);
    }
}

TEST_CASE("$stateTime"){
    const float dt = 0.25f;

    SUBCASE(">= : la transition part pile a la borne"){
        auto def = mustLoad(makeFsm("[]", recStates({"A", "B"}), "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"$stateTime","op":">=","value":0.5}]}])"));
        Runner r(def);
        r.tick(dt);
        CHECK(r.inst.stateTime == doctest::Approx(0.f));
        r.tick(dt);
        r.tick(dt);
        CHECK(r.inst.stateTime == doctest::Approx(0.5f));
        CHECK(r.state() == "A");
        r.tick(dt);
        CHECK(r.state() == "B");
    }
    SUBCASE("> : pas a la borne, la frame d'apres"){
        auto def = mustLoad(makeFsm("[]", recStates({"A", "B"}), "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"$stateTime","op":">","value":0.5}]}])"));
        Runner r(def);
        r.ticks(4, dt);
        CHECK(r.state() == "A");
        r.tick(dt);
        CHECK(r.state() == "B");
    }
    SUBCASE("remis a zero au changement d'etat"){
        auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B"}), "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"go"}]}])"));
        Runner r(def);
        r.ticks(9, dt);
        CHECK(r.inst.stateTime == doctest::Approx(2.f));
        ee::fsm::setTrigger(def, r.inst, "go");
        r.tick(dt);
        CHECK(r.state() == "B");
        CHECK(r.inst.stateTime == doctest::Approx(dt));
    }
    SUBCASE("fenetre <= et >="){
        auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B"}), "A", R"([
            {"from":"A","to":"B","conditions":[{"param":"go"},
                {"param":"$stateTime","op":">=","value":0.25},
                {"param":"$stateTime","op":"<=","value":0.5}]}])"));
        Runner tooLate(def);
        tooLate.ticks(4, dt);
        ee::fsm::setTrigger(def, tooLate.inst, "go");
        tooLate.tick(dt);
        CHECK(tooLate.state() == "A");

        Runner inWindow(def);
        inWindow.ticks(3, dt);
        ee::fsm::setTrigger(def, inWindow.inst, "go");
        inWindow.tick(dt);
        CHECK(inWindow.state() == "B");
    }
}

TEST_CASE("OnStart / OnEnd dans le bon ordre, etat 0 compris"){
    auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B"}), "A", R"([
        {"from":"A","to":"B","conditions":[{"param":"go"}]},
        {"from":"B","to":"A","conditions":[{"param":"go"}]}])"));
    REQUIRE(def.entryState == 0);
    Runner r(def);
    journal().clear();
    r.tick();
    ee::fsm::setTrigger(def, r.inst, "go");
    r.tick();
    ee::fsm::setTrigger(def, r.inst, "go");
    r.tick();
    CHECK(journal() == V{"A:enter", "A:exit", "B:enter", "B:exit", "A:enter"});
}

TEST_CASE("Any vers l'etat courant"){
    SUBCASE("ignoree sans allowSelf"){
        auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B"}), "A", R"([
            {"from":"Any","to":"A","conditions":[{"param":"go"}]}])"));
        Runner r(def);
        r.tick();
        journal().clear();
        ee::fsm::setTrigger(def, r.inst, "go");
        r.tick();
        CHECK(r.state() == "A");
        CHECK(journal().empty());
        CHECK(ee::fsm::getTrigger(def, r.inst, "go") == true);
    }
    SUBCASE("rejoue l'etat avec allowSelf"){
        auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B"}), "A", R"([
            {"from":"Any","to":"A","allowSelf":true,"conditions":[{"param":"go"}]}])"));
        Runner r(def);
        r.tick();
        journal().clear();
        ee::fsm::setTrigger(def, r.inst, "go");
        r.tick();
        CHECK(journal() == V{"A:exit", "A:enter"});
        CHECK(r.inst.stateTime == doctest::Approx(0.1f));
    }
}

TEST_CASE("conditions int / float / custom"){
    auto def = mustLoad(makeFsm(R"([{"name":"speed","type":"float"},{"name":"hp","type":"int","default":3}])",
                                recStates({"A", "Run", "Dead", "Air"}), "A", R"([
        {"from":"Any","to":"Dead","conditions":[{"param":"hp","op":"<=","value":0}]},
        {"from":"A","to":"Air","conditions":[{"custom":"IsGrounded","expected":false}]},
        {"from":"A","to":"Run","conditions":[{"param":"speed","op":">","value":1}]}])"));
    g_grounded = true;
    Runner r(def);
    r.ticks(2);
    CHECK(r.state() == "A");

    ee::fsm::setFloat(def, r.inst, "speed", 1.5f);
    r.tick();
    CHECK(r.state() == "Run");

    ee::fsm::setInt(def, r.inst, "hp", 0);
    r.tick();
    CHECK(r.state() == "Dead");

    Runner air(def);
    air.tick();
    g_grounded = false;
    air.tick();
    CHECK(air.state() == "Air");
    g_grounded = true;
}

TEST_CASE("plusieurs instances sur la meme definition"){
    auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"},{"name":"n","type":"int"}])",
                                recStates({"A", "B"}), "A", R"([
        {"from":"A","to":"B","conditions":[{"param":"go"}]}])"));
    Runner r1(def), r2(def);
    r1.tick();
    r2.tick();
    ee::fsm::setTrigger(def, r1.inst, "go");
    ee::fsm::setInt(def, r2.inst, "n", 42);
    r1.tick();
    r2.tick();
    CHECK(r1.state() == "B");
    CHECK(r2.state() == "A");
    CHECK(ee::fsm::getInt(def, r1.inst, "n") == 0);
    CHECK(ee::fsm::getInt(def, r2.inst, "n") == 42);
    r2.ticks(3);
    CHECK(r1.inst.stateTime == doctest::Approx(0.1f));
    CHECK(r2.inst.stateTime == doctest::Approx(0.4f));
}

TEST_CASE("parametre inconnu ou mauvais type : pas de crash"){
    auto def = mustLoad(makeFsm(R"([{"name":"flag","type":"bool"}])", recStates({"A"}), "A", "[]"));
    Runner r(def);
    r.tick();

    ee::fsm::setBool(def, r.inst, "nope", true);
    ee::fsm::setTrigger(def, r.inst, "nope");
    ee::fsm::setInt(def, r.inst, "flag", 3);
    ee::fsm::setBool(def, r.inst, 99, true);
    ee::fsm::setBool(def, r.inst, -1, true);
    CHECK(ee::fsm::getBool(def, r.inst, "nope") == false);
    CHECK(ee::fsm::getFloat(def, r.inst, "flag") == doctest::Approx(0.f));
    CHECK(ee::fsm::getBool(def, r.inst, "flag") == false);

    ee::fsm::StateMachineInstance uninit;
    ee::fsm::setBool(def, uninit, "flag", true);
    CHECK(ee::fsm::getBool(def, uninit, "flag") == false);
}

TEST_CASE("SetParam : action du moteur"){
    auto def = mustLoad(makeFsm(R"([{"name":"canMove","type":"bool","default":true}])", R"([
        {"name":"A","actions":[{"type":"SetParam","param":"canMove","value":{"type":"bool","value":false}}]}])",
        "A", "[]"));
    Runner r(def);
    CHECK(ee::fsm::getBool(def, r.inst, "canMove") == true);
    r.tick();
    CHECK(ee::fsm::getBool(def, r.inst, "canMove") == false);
}

TEST_CASE("changement de definition : l'instance repart de entryState"){
    auto def1 = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"}])", recStates({"A", "B"}), "A", R"([
        {"from":"A","to":"B","conditions":[{"param":"go"}]}])"));
    auto def2 = mustLoad(makeFsm("[]", recStates({"X"}), "X", "[]"));
    Runner r(def1);
    r.tick();
    ee::fsm::setTrigger(def1, r.inst, "go");
    r.tick();
    REQUIRE(r.inst.currentState == 1);

    ee::ecs::World w;
    ee::fsm::Ctx ctx{w, 0, def2, r.inst};
    ee::fsm::update(def2, ctx, r.inst, 0.1f);
    CHECK(r.inst.initializedWith == &def2);
    CHECK(r.inst.currentState == 0);
    CHECK(r.inst.params.empty());
}

TEST_CASE("SetParam sur un trigger : true l'arme, false le vide"){
    auto def = mustLoad(makeFsm(R"([{"name":"go","type":"trigger"},{"name":"fire","type":"trigger"}])", R"([
        {"name":"A","actions":[{"type":"SetParam","param":"go","value":{"type":"bool","value":false}},
                               {"type":"SetParam","param":"fire","value":{"type":"bool","value":true}}]}])",
        "A", "[]"));
    Runner r(def);
    ee::fsm::setTrigger(def, r.inst, "go");
    r.tick();
    CHECK(ee::fsm::getTrigger(def, r.inst, "go") == false);
    CHECK(ee::fsm::getTrigger(def, r.inst, "fire") == true);
}
