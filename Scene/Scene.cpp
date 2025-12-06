#include "Scene.hpp"
#include "../Utils/Logger.hpp"
#include "../Graphics/Renderer.hpp"

Scene::Scene() 
    : m_camera(glm::vec3(0.0f, 1.7f, 3.0f)) {
}

void Scene::initialize() {
    Logger::info("Scene initialized");
}

void Scene::loadModels() {
    Logger::info("Loading scene models...");
    
    // Create pyramid and ground FIRST
    createDefaultScene();
    
    // Then load OBJ models - use Renderer's TextureManager so textures persist
    GameObject* Seat = createGameObject("TestCube");
    if (Seat) {
        auto& renderer = Renderer::getInstance();
        try {
            Seat->loadModel("models/black_lether_chair.gltf", renderer.getTextureManager());
        } catch (const std::exception& e) {
            Logger::error(std::string("Exception while loading model for TestCube: ") + e.what());
        } catch (...) {
            Logger::error("Unknown exception while loading model for TestCube");
        }
        Seat->setPosition(glm::vec3(5.0f, 0.0f, 0.0f));
        Seat->setRotation(glm::vec3(0.0f, 45.0f, 0.0f));
        Seat->setScale(glm::vec3(3.0f, 3.0f, 3.0f));
    }
    Seat->setVisible(false);
    GameObject* SportsCar = createGameObject("SportsCar");
    auto& renderer = Renderer::getInstance();
    try {
        SportsCar->loadModel("models/classroom.glb", renderer.getTextureManager());
    } catch (const std::exception& e) {
        Logger::error(std::string("Exception while loading model for SportsCar: ") + e.what());
    } catch (...) {
        Logger::error("Unknown exception while loading model for SportsCar");
    }
    SportsCar->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    SportsCar->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    SportsCar->setScale(glm::vec3(1.0f, 1.0f, 1.0f));

    Logger::info("Scene models loaded");

    // Debug: optional stress-run to repeatedly load/unload a model to detect heap issues
    const char* stress = std::getenv("REPRO_LOAD_STRESS");
    if (stress && std::string(stress) == "1") {
        Logger::warn("REPRO_LOAD_STRESS enabled - running repeated model load/unload cycle for diagnostics");
        auto& renderer = Renderer::getInstance();
        for (int i = 0; i < 50; ++i) {
            Logger::info(std::string("Stress cycle: ") + std::to_string(i));
            try {
                auto tmp = std::make_unique<Model>("models/black_lether_chair.gltf", renderer.getTextureManager());
            } catch (const std::exception& e) {
                Logger::error(std::string("Exception during stress load: ") + e.what());
                break;
            } catch (...) {
                Logger::error("Unknown exception during stress load");
                break;
            }
        }
        Logger::warn("REPRO_LOAD_STRESS diagnostics complete");
    }
}

void Scene::update(float deltaTime) {
    for (auto& [name, obj] : m_gameObjects) {
    }
}

void Scene::render(unsigned int shaderProgram) {
    Logger::info("Scene::render called - objects: " + std::to_string(m_gameObjects.size()));
    int idx = 0;
    for (auto& [name, obj] : m_gameObjects) {
        Logger::info(std::string("  [") + std::to_string(idx) + "] Drawing GameObject: " + name + " visible=" + std::to_string(obj->isVisible()));
        obj->draw(shaderProgram);
        idx++;
    }
}

void Scene::renderForShadow(unsigned int shadowShader) {
    for (auto& [name, obj] : m_gameObjects) {
        obj->drawForShadow(shadowShader);
    }
}

void Scene::computeSceneBounds(glm::vec3& outCenter, float& outRadius) const {
    outCenter = glm::vec3(0.0f);
    outRadius = 0.0f;
    if (m_gameObjects.empty()) return;

    // Compute average position as center
    for (const auto& kv : m_gameObjects) {
        outCenter += kv.second->getPosition();
    }
    outCenter /= static_cast<float>(m_gameObjects.size());

    // Compute max distance from center
    float maxDistSq = 0.0f;
    for (const auto& kv : m_gameObjects) {
        float d = glm::length(kv.second->getPosition() - outCenter);
        if (d * d > maxDistSq) maxDistSq = d * d;
    }
    outRadius = std::sqrt(maxDistSq);

    // Add a small margin
    outRadius *= 1.2f;
    if (outRadius < 10.0f) outRadius = 10.0f;
}

void Scene::createDefaultScene() {
    Logger::info("Creating default scene with manual meshes and lights");
    
    auto& renderer = Renderer::getInstance();
    
    // Clear any default lights first
    renderer.clearLights();
    
    // Ground
    GameObject* ground = createGameObject("Ground");
    if (ground) {
        std::vector<Vertex> groundVertices;
        groundVertices.push_back({glm::vec3(-20.0f, 0.0f, -20.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)});
        groundVertices.push_back({glm::vec3(20.0f, 0.0f, -20.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f)});
        groundVertices.push_back({glm::vec3(20.0f, 0.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)});
        groundVertices.push_back({glm::vec3(-20.0f, 0.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)});

        std::vector<unsigned int> groundIndices = {0, 1, 2, 0, 2, 3};
        PBRMaterial groundMaterial(glm::vec3(0.3f, 0.8f, 0.4f), 0.9f, 0.1f, 1.0f);

        ground->setManualMesh(groundVertices, groundIndices, groundMaterial);
        ground->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        ground->setVisible(false);
    }
    
    // Pyramid
    GameObject* pyramid = createGameObject("Pyramid");
    if (pyramid) {
        std::vector<Vertex> pyramidVertices;
        pyramidVertices.push_back({glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)});
        pyramidVertices.push_back({glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f)});
        pyramidVertices.push_back({glm::vec3(-1.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f)});
        pyramidVertices.push_back({glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f)});
        pyramidVertices.push_back({glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.5f, 0.5f)});
        
        std::vector<unsigned int> pyramidIndices = {
            0, 2, 1, 1, 2, 3,
            0, 1, 4, 1, 3, 4, 3, 2, 4, 2, 0, 4
        };
        
        PBRMaterial pyramidMaterial(glm::vec3(0.9f, 0.3f, 0.2f), 0.1f, 0.3f, 1.0f);
        
        pyramid->setManualMesh(pyramidVertices, pyramidIndices, pyramidMaterial);
        pyramid->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        pyramid->setVisible(false);
    }
    
    // Test different light configurations
    /*
    // Test 1: Directional light (sun)
    Logger::info("=== TEST 1: Directional Light ===");
    DirectionalLight* sun = renderer.createDirectionalLight(
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, 1.0f, 0.95f),
        1.0f
    );
    if (sun) {
        sun->castShadows = true;
        Logger::info("Directional sun created. castShadows: " + std::to_string(sun->castShadows));
    }

    */
    
    
    // Test 2: Spot light (casts shadows)

    float Lumin = 50.0f;
   
    SpotLight* spotlight1 = renderer.createSpotLight(
        glm::vec3(2.3f, 3.0f, 0.9f),//towards outer window/up down /away from board
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Lumin
    );
    // Use reasonable inner/outer angles for the spot light and set a larger radius
    spotlight1->setAngles(15.0f, 120.0f);
    spotlight1->radius = 30.0f;
    spotlight1->castShadows = true;
    


    SpotLight* spotlight2 = renderer.createSpotLight(
        glm::vec3(2.3f, 3.0f, -1.8f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Lumin
    );
    // Use reasonable inner/outer angles for the spot light and set a larger radius
    spotlight2->setAngles(15.0f, 120.0f);
    spotlight2->radius = 30.0f;
    spotlight2->castShadows = true;


    SpotLight* spotlight3 = renderer.createSpotLight(
        glm::vec3(-1.0f, 3.0f, 0.9f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Lumin
    );
    // Use reasonable inner/outer angles for the spot light and set a larger radius
    spotlight3->setAngles(15.0f, 120.0f);
    spotlight3->radius = 30.0f;
    spotlight3->castShadows = true;



    SpotLight* spotlight4 = renderer.createSpotLight(
        glm::vec3(-1.0f, 3.0f, -1.8f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Lumin
    );
    // Use reasonable inner/outer angles for the spot light and set a larger radius
    spotlight4->setAngles(15.0f, 120.0f);
    spotlight4->radius = 30.0f;
    spotlight4->castShadows = true;



    SpotLight* spotlight5 = renderer.createSpotLight(
        glm::vec3(-3.2f, 3.0f, 0.9f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Lumin
    );
    // Use reasonable inner/outer angles for the spot light and set a larger radius
    spotlight5->setAngles(15.0f, 120.0f);
    spotlight5->radius = 30.0f;
    spotlight5->castShadows = true;



    SpotLight* spotlight6 = renderer.createSpotLight(
        glm::vec3(-3.2f, 3.0f, -1.8f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Lumin
    );
    // Use reasonable inner/outer angles for the spot light and set a larger radius
    spotlight6->setAngles(15.0f, 120.0f);
    spotlight6->radius = 30.0f;
    spotlight6->castShadows = true;



    // Test 3: Point light (keep shadows off for now until point-shadow implementation is complete)
  /*
    Logger::info("=== TEST 3: Point Light ===");
    PointLight* pointlight = renderer.createPointLight(
        glm::vec3(3.0f, 3.0f, 0.0f),
        glm::vec3(1.0f, 0.5f, 0.2f),
        100.0f,
        10.0f
    );
    pointlight->castShadows = true;
 
    Logger::info("Point light created. castShadows: " + std::to_string(pointlight->castShadows));
    */
    Logger::info("=== Total lights created: " + std::to_string(renderer.getLights().size()));
    
    Logger::info("Created default scene objects and lights");
}

GameObject* Scene::createGameObject(const std::string& name) {
    if (m_gameObjects.find(name) != m_gameObjects.end()) {
        Logger::warn("GameObject with name '" + name + "' already exists!");
        return nullptr;
    }
    auto gameObject = std::make_unique<GameObject>(name);
    GameObject* ptr = gameObject.get();
    m_gameObjects[name] = std::move(gameObject);
    Logger::info("Created GameObject: " + name);
    return ptr;
}

GameObject* Scene::getGameObject(const std::string& name) {
    auto it = m_gameObjects.find(name);
    return (it != m_gameObjects.end()) ? it->second.get() : nullptr;
}

void Scene::removeGameObject(const std::string& name) {
    auto it = m_gameObjects.find(name);
    if (it != m_gameObjects.end()) {
        m_gameObjects.erase(it);
        Logger::info("Removed GameObject: " + name);
    }
}