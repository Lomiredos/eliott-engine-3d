#include "TestUtils.hpp"

using namespace fsmtest;
using V = std::vector<std::string>;

namespace {
    const std::string kSword = std::string(EE_SOURCE_DIR) + "/assets/fsm/sword_combo.fsm";

    ee::fsm::StateMachineDef loadSword(){
        std::vector<std::string> errors, warnings;
        auto def = ee::fsm::loadStateMachineFile(kSword, registries().get(), errors, &warnings);
        INFO("first error: ", errors.empty() ? "" : errors[0]);
        REQUIRE(def.has_value());
        return std::move(*def);
    }

    void press(Runner& _r, const char* _param){
        ee::fsm::setTrigger(_r.def, _r.inst, _param);
    }
}

TEST_CASE("sword_combo.fsm se charge"){
    auto def = loadSword();
    CHECK(def.name == "SwordCombo");
    CHECK(def.states.size() == 5);
    CHECK(def.states[def.entryState].name == "Idle");
    CHECK(def.anyTransition.size() == 1);
}

TEST_CASE("scenario 1 : trois clics en rythme -> Attack3 -> Idle"){
    auto def = loadSword();
    Runner r(def);
    r.tick();
    CHECK(r.state() == "Idle");
    journal().clear();

    press(r, "attackPressed");
    r.tick();
    CHECK(r.state() == "Attack1");

    press(r, "attackPressed");
    CHECK(r.tickUntil("Attack2", 5) > 0);

    press(r, "attackPressed");
    CHECK(r.tickUntil("Attack3", 5) > 0);

    CHECK(r.tickUntil("Idle", 10) > 0);
    CHECK(journal() == V{"Idle:exit", "Attack1:enter", "Attack1:exit", "Attack2:enter",
                         "Attack2:exit", "Attack3:enter", "Attack3:exit", "Idle:enter"});
}

TEST_CASE("scenario 1 bis : clic avant la fenetre garde en memoire (buffering)"){
    auto def = loadSword();
    Runner r(def);
    r.tick();
    press(r, "attackPressed");
    r.tick();
    REQUIRE(r.state() == "Attack1");

    press(r, "attackPressed");
    r.tick();
    CHECK(r.state() == "Attack1");
    CHECK(r.tickUntil("Attack2", 3) > 0);
    CHECK(r.inst.stateTime == doctest::Approx(0.1f));
}

TEST_CASE("scenario 2 : clic trop tard -> retour a Idle"){
    auto def = loadSword();
    Runner r(def);
    r.tick();
    press(r, "attackPressed");
    r.tick();
    REQUIRE(r.state() == "Attack1");

    CHECK(r.tickUntil("Idle", 10) > 0);
    journal().clear();

    press(r, "attackPressed");
    r.tick();
    CHECK(r.state() == "Attack1");
    CHECK(journal() == V{"Idle:exit", "Attack1:enter"});
}

TEST_CASE("scenario 3 : hit pendant Attack2 -> Hit -> Idle"){
    auto def = loadSword();
    Runner r(def);
    r.tick();
    press(r, "attackPressed");
    r.tick();
    press(r, "attackPressed");
    REQUIRE(r.tickUntil("Attack2", 5) > 0);

    press(r, "hit");
    r.tick();
    CHECK(r.state() == "Hit");
    CHECK(ee::fsm::getBool(def, r.inst, "canMove") == false);

    press(r, "hit");
    r.tick();
    CHECK(r.state() == "Hit");

    CHECK(r.tickUntil("Idle", 10) > 0);
    CHECK(ee::fsm::getBool(def, r.inst, "canMove") == true);
}
