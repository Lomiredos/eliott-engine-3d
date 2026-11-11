#include "TestUtils.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

#include <nlohmann/json.hpp>

#include "visu/core/MeshStore.hpp"
#include "visu/fsm/EntityAPI.hpp"
#include "visu/systems/StateMachineSystem.hpp"

using namespace fsmtest;
namespace fs = std::filesystem;

namespace {
    const std::string kSword = std::string(EE_SOURCE_DIR) + "/assets/fsm/sword_combo.fsm";

    fs::path tempDir(const std::string& _name){
        fs::path dir = fs::temp_directory_path() / ("ee-fsm-tests-" + _name);
        fs::remove_all(dir);
        fs::create_directories(dir);
        return dir;
    }

    void writeFile(const fs::path& _file, const std::string& _text){
        std::ofstream f(_file, std::ios::binary | std::ios::trunc);
        f << _text;
    }

    void waitCacheCheck(){
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    }

    std::string hotFsm(const std::string& _entry){
        return makeFsm(R"([{"name":"go","type":"trigger"}])",
                       R"([{"name":"A","actions":[]},{"name":"B","actions":[]}])", _entry,
                       R"([{"from":"A","to":"B","conditions":[{"param":"go"}]}])");
    }
}

TEST_CASE("sauvegarde : aller-retour sans perte"){
    ee::fsm::ActionRegistry engineOnly;
    ee::fsm::registerEngineActions(engineOnly);
    ee::fsm::ConditionRegistry noConditions;
    std::vector<std::string> errors;
    auto def = ee::fsm::loadStateMachineFile(kSword, {engineOnly, noConditions}, errors);
    REQUIRE(def.has_value());

    const std::string text = ee::fsm::fsmToJsonText(*def);
    auto again = ee::fsm::fsmFromJsonText(text, {engineOnly, noConditions}, errors);
    REQUIRE(again.has_value());
    CHECK(ee::fsm::fsmToJsonText(*again) == text);

    nlohmann::json original = nlohmann::json::parse(std::ifstream(kSword));
    nlohmann::json saved = nlohmann::json::parse(text);
    CHECK(saved["parameters"] == original["parameters"]);
    CHECK(saved["entryState"] == original["entryState"]);
    CHECK(saved["transitions"].size() == original["transitions"].size());
    for (size_t i = 0; i < original["states"].size(); i++){
        CAPTURE(i);
        CHECK(saved["states"][i]["actions"] == original["states"][i]["actions"]);
        CHECK(saved["states"][i]["editor"] == original["states"][i]["editor"]);
    }
}

TEST_CASE("sauvegarde : action non enregistree reecrite telle quelle"){
    Loaded l = load(makeFsm("[]", R"([{"name":"A","actions":[{"type":"Fireball","power":3,"color":"#ff0000"}]}])", "A", "[]"));
    REQUIRE(l.def.has_value());
    nlohmann::json saved = ee::fsm::fsmToJson(*l.def);
    CHECK(saved["states"][0]["actions"][0] == nlohmann::json::parse(R"({"type":"Fireball","power":3,"color":"#ff0000"})"));
}

TEST_CASE("fichiers : erreurs prefixees par le chemin"){
    fs::path dir = tempDir("files");
    std::vector<std::string> errors;

    CHECK_FALSE(ee::fsm::loadStateMachineFile(dir / "absent.fsm", registries().get(), errors).has_value());
    CHECK(hasMessage(errors, {"absent.fsm", "cannot open"}));

    errors.clear();
    writeFile(dir / "bad.fsm", makeFsm("[]", recStates({"A"}), "Zed", "[]"));
    CHECK_FALSE(ee::fsm::loadStateMachineFile(dir / "bad.fsm", registries().get(), errors).has_value());
    CHECK(hasMessage(errors, {"bad.fsm: ", "Zed"}));

    auto def = mustLoad(hotFsm("A"));
    REQUIRE(ee::fsm::saveStateMachineFile(dir / "sub" / "saved.fsm", def));
    errors.clear();
    CHECK(ee::fsm::loadStateMachineFile(dir / "sub" / "saved.fsm", registries().get(), errors).has_value());
}

TEST_CASE("cache : une seule definition partagee et hot reload"){
    fs::path dir = tempDir("cache");
    ee::core::setMeshBaseDir(dir);
    writeFile(dir / "hot.fsm", hotFsm("A"));

    auto first = ee::fsm::getStateMachine("hot.fsm");
    REQUIRE(first != nullptr);
    CHECK(ee::fsm::getStateMachine("hot.fsm") == first);
    CHECK(ee::fsm::getStateMachine("") == nullptr);
    CHECK(ee::fsm::getStateMachine("absent.fsm") == nullptr);

    SUBCASE("fichier casse : l'ancienne version est gardee"){
        waitCacheCheck();
        writeFile(dir / "hot.fsm", hotFsm("A") + ",,,");
        CHECK(ee::fsm::getStateMachine("hot.fsm") == first);
    }
    SUBCASE("fichier modifie : nouvelle definition, l'ancienne reste valide"){
        waitCacheCheck();
        writeFile(dir / "hot.fsm", hotFsm("B"));
        auto second = ee::fsm::getStateMachine("hot.fsm");
        REQUIRE(second != nullptr);
        CHECK(second != first);
        CHECK(second->entryState == 1);
        CHECK(first->entryState == 0);
    }
    SUBCASE("fichier supprime : nullptr"){
        waitCacheCheck();
        fs::remove(dir / "hot.fsm");
        CHECK(ee::fsm::getStateMachine("hot.fsm") == nullptr);
    }
}

TEST_CASE("StateMachineSystem + helpers par entite"){
    fs::path dir = tempDir("system");
    ee::core::setMeshBaseDir(dir);
    writeFile(dir / "m.fsm", hotFsm("A"));

    ee::ecs::World world;
    world.registerSystem<StateMachineSystem>();
    ee::ecs::Signature sig;
    sig.set(ee::ecs::getComponentID<StateMachineComponent>());
    world.setSystemSignature<StateMachineSystem>(sig);

    ee::ecs::EntityID a = world.createEntity();
    ee::ecs::EntityID b = world.createEntity();
    ee::ecs::EntityID none = world.createEntity();
    StateMachineComponent comp;
    comp.path = "m.fsm";
    world.addComponent(a, comp);
    world.addComponent(b, comp);

    ee::fsm::setTrigger(world, a, "go");
    world.update(0.1f);
    CHECK(ee::fsm::currentStateName(world, a) == "A");
    CHECK(ee::fsm::currentStateName(world, b) == "A");
    CHECK(world.getComponent<StateMachineComponent>(a)->def == world.getComponent<StateMachineComponent>(b)->def);

    world.update(0.1f);
    CHECK(ee::fsm::currentStateName(world, a) == "B");
    CHECK(ee::fsm::currentStateName(world, b) == "A");

    ee::fsm::setTrigger(world, none, "go");
    CHECK(ee::fsm::getTrigger(world, none, "go") == false);
    CHECK(ee::fsm::currentStateName(world, none) == "");

    world.destroyEntity(a);
    world.update(0.1f);
    CHECK(ee::fsm::currentStateName(world, b) == "A");
}
