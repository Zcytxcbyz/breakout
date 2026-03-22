// Breakout - Brick Breaker Game Based on Box2D and SFML

// ---------- Header Files ----------
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <random>
#include <box2d/box2d.h>
#include <algorithm>

// ---------- Game Parameter Configuration ----------
namespace Config {
    // Window
    const int WINDOW_W = 960;               // Window width (pixels)
    const int WINDOW_H = 540;               // Window height (pixels)
    const int FRAME_LIMIT = 60;             // Frame rate limit

    // Physics unit conversion
    const float PPM = 30.0f;                // 1 meter = 30 pixels

    // Ball parameters (pixel units)
    const float BALL_RADIUS = 12.0f;        // Ball radius (pixels)
    const float BALL_DENSITY = 1.0f;        // Density (physics units)
    const float BALL_RESTITUTION = 1.0f;    // Restitution coefficient (physics units)
    const float BALL_INIT_SPEED = 200.0f;   // Pixels per second
    const float MAX_BALL_SPEED = 20.0f;     // Meters per second (physics units)
    const float SPEED_FACTOR = 1.05f;       // Speed multiplier after each collision

    // Paddle parameters (pixel units)
    const float PADDLE_WIDTH = 160.0f;      // Paddle width (pixels)
    const float PADDLE_HEIGHT = 30.0f;      // Paddle height (pixels)
    const float PADDLE_SPEED = 300.0f;      // Pixels per second
    const float PADDLE_DENSITY = 1.0f;      // Density (physics units)
    const float PADDLE_RESTITUTION = 1.0f;  // Restitution coefficient (physics units)

    // Boundary wall thickness (pixels)
    const float WALL_THICKNESS = 10.0f;     // Boundary wall thickness (pixels)

    // Brick parameters (pixel units)
    const int BRICK_ROWS = 5;               // Number of brick rows
    const int BRICK_COLS = 10;              // Number of brick columns
    const float BRICK_START_X = 80.0f;      // Brick starting X position (pixels)
    const float BRICK_START_Y = 60.0f;      // Brick starting Y position (pixels)  
    const float BRICK_WIDTH = 70.0f;        // Brick width (pixels)
    const float BRICK_HEIGHT = 25.0f;       // Brick height (pixels)
    const float BRICK_SPACING_X = 80.0f;    // Brick horizontal spacing (pixels)
    const float BRICK_SPACING_Y = 35.0f;    // Brick vertical spacing (pixels)
    const int BRICK_SPAWN_PROB = 70;        // Brick spawn probability (percentage)
    const int MIN_HEALTH = 1;               // Minimum brick health
    const int MAX_HEALTH = 3;               // Maximum brick health
    const int SCORE_PER_BRICK = 10;         // Score per brick

    // Initial position offsets (pixels)
    const float BALL_Y_OFFSET = 50.0f;      // Ball's Y offset relative to screen center
    const float PADDLE_Y_OFFSET = 50.0f;    // Paddle's distance from the bottom
}

// ---------- UI Text ----------
namespace Texts {
    const char* GameName = "Breakout";                              // Game name
    const char* StartGame = "Start Game";                           // Start game button text
    const char* Help = "Help";                                      // Help button text
    const char* Exit = "Exit";                                      // Exit button text
    const char* Controls = "Controls:";                             // Controls title
    const char* BrickHealth = "Brick Health (Color -> Hits):";      // Brick health explanation
    const char* MoveRight = "Move Right: D / Right Arrow";          // Move right explanation
    const char* MoveLeft = "Move Left: A / Left Arrow";             // Move left explanation
    const char* RedHit = "Red   -> 1 hit";                          // Red brick explanation
    const char* GreenHit = "Green -> 2 hits";                       // Green brick explanation
    const char* BlueHit = "Blue  -> 3 hits";                        // Blue brick explanation
    const char* DestroyAll = "Destroy all bricks to win!";          // Game objective explanation
    const char* Close = "Close";                                    // Close button text
    const char* ScoreFormat = "Score: %d";                          // Score display format
    const char* ScoreWindow = "Score";                              // Score window title
    const char* YouWin = "You Win!";                                // Victory text
    const char* GameOver = "Game Over";                             // Failure text
    const char* Restart = "Restart";                                // Restart button text
    const char* MainMenu = "Main Menu";                             // Main menu button text
}

// ---------- Global Variables ----------
// Game state enumeration
enum GameState {
    STATE_MENU,         // Main menu
    STATE_PLAYING,      // Game in progress
    STATE_GAMEOVER      // Game over
};

GameState gameState = STATE_MENU;           // Current game state
bool gameWin = false;                       // Whether the game has been won
int score = 0;                              // Current score
bool showHelp = false;                      // Whether to show the help window

b2World* world = nullptr;                   // Box2D physics world pointer
b2Body* ball = nullptr;                     // Ball physics body pointer
b2Body* paddle = nullptr;                   // Paddle physics body pointer
std::vector<b2Body*> bricks;                // List of brick physics body pointers
std::vector<int> brickHealth;               // List of brick health values
std::vector<sf::RectangleShape> sfBricks;   // List of brick SFML graphics

sf::CircleShape sfBall;                     // Ball SFML graphic
sf::RectangleShape sfPaddle;                // Paddle SFML graphic

std::random_device rd;  // Random number seed
std::mt19937 rng(rd()); // Random number generator

// ---------- Coordinate Conversion Functions ----------
inline b2Vec2 toBox2D(const sf::Vector2f& pos) {
    return { pos.x / Config::PPM, (Config::WINDOW_H - pos.y) / Config::PPM };
}

// Convert Box2D coordinates to SFML coordinates (note Y-axis flip)
inline sf::Vector2f toSFML(const b2Vec2& pos) {
    return { pos.x * Config::PPM, Config::WINDOW_H - pos.y * Config::PPM };
}

// ---------- Collision Listener ----------
class ContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override {
        b2Body* bodyA = contact->GetFixtureA()->GetBody();
        b2Body* bodyB = contact->GetFixtureB()->GetBody();

        // Check if it's a collision between the ball and another object
        bool ballHit = false;
        b2Body* other = nullptr;
        if (bodyA == ball) {
            ballHit = true;
            other = bodyB;
        }
        else if (bodyB == ball) {
            ballHit = true;
            other = bodyA;
        }

        if (ballHit) {
            // If the collided object is the paddle or a brick, increase the ball's speed
            if (other == paddle || std::find(bricks.begin(), bricks.end(), other) != bricks.end()) {
                b2Vec2 vel = ball->GetLinearVelocity();
                vel.x *= Config::SPEED_FACTOR;
                vel.y *= Config::SPEED_FACTOR;
                float speed = vel.Length();
                if (speed > Config::MAX_BALL_SPEED) {
                    vel *= Config::MAX_BALL_SPEED / speed;
                }
                ball->SetLinearVelocity(vel);
            }

            // If the collided object is a brick, reduce the brick's health and check if it needs to be destroyed
            if ((bodyA == ball && bodyB != paddle) || (bodyB == ball && bodyA != paddle)) {
                b2Body* brickBody = (bodyA == ball) ? bodyB : bodyA;
                auto it = std::find(bricks.begin(), bricks.end(), brickBody);
                if (it != bricks.end()) {
                    size_t idx = it - bricks.begin();
                    brickHealth[idx]--;
                    if (brickHealth[idx] == 1) sfBricks[idx].setFillColor(sf::Color::Red);
                    else if (brickHealth[idx] == 2) sfBricks[idx].setFillColor(sf::Color::Green);
                    if (brickHealth[idx] == 0) {
                        toDestroy.push_back(brickBody);
                    }
                }
            }

            // If the collided object is the ground, game over
            if ((bodyA == ball && bodyB == ground) || (bodyB == ball && bodyA == ground)) {
                gameState = STATE_GAMEOVER;
                gameWin = false;
            }
        }
    }

    // No processing needed on collision end
    std::vector<b2Body*> toDestroy;
    b2Body* ground = nullptr;
};

ContactListener contactListener;

// ---------- Reset Game ----------
void resetGame() {
    // Destroy
    if (world) {
        for (auto brick : bricks) world->DestroyBody(brick);
        if (ball) world->DestroyBody(ball);
        if (paddle) world->DestroyBody(paddle);
        delete world;
        world = nullptr;
        bricks.clear();
        brickHealth.clear();
        sfBricks.clear();
        contactListener.toDestroy.clear();
    }

    // Create a new physics world
    world = new b2World(b2Vec2(0, 0));
    world->SetContactListener(&contactListener);

    // ----- Boundary Walls -----
    b2BodyDef wallDef;
    wallDef.type = b2_staticBody;
    b2PolygonShape wallShape;

    // Left wall
    wallShape.SetAsBox(Config::WALL_THICKNESS / 2 / Config::PPM, Config::WINDOW_H / 2 / Config::PPM);
    b2Body* leftWall = world->CreateBody(&wallDef);
    leftWall->CreateFixture(&wallShape, 0);
    leftWall->SetTransform(b2Vec2(Config::WALL_THICKNESS / 2 / Config::PPM, Config::WINDOW_H / 2 / Config::PPM), 0);

    // Right wall
    wallShape.SetAsBox(Config::WALL_THICKNESS / 2 / Config::PPM, Config::WINDOW_H / 2 / Config::PPM);
    b2Body* rightWall = world->CreateBody(&wallDef);
    rightWall->CreateFixture(&wallShape, 0);
    rightWall->SetTransform(b2Vec2((Config::WINDOW_W - Config::WALL_THICKNESS) / Config::PPM, Config::WINDOW_H / 2 / Config::PPM), 0);

    // Ceiling (top)
    wallShape.SetAsBox(Config::WINDOW_W / 2 / Config::PPM, Config::WALL_THICKNESS / 2 / Config::PPM);
    b2Body* ceiling = world->CreateBody(&wallDef);
    ceiling->CreateFixture(&wallShape, 0);
    ceiling->SetTransform(b2Vec2(Config::WINDOW_W / 2 / Config::PPM, (Config::WINDOW_H - Config::WALL_THICKNESS / 2) / Config::PPM), 0);

    // Ground (bottom, used for loss detection)
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    b2PolygonShape groundShape;
    groundShape.SetAsBox(Config::WINDOW_W / 2 / Config::PPM, Config::WALL_THICKNESS / 2 / Config::PPM);
    b2Body* ground = world->CreateBody(&groundDef);
    ground->CreateFixture(&groundShape, 0);
    ground->SetTransform(b2Vec2(Config::WINDOW_W / 2 / Config::PPM, Config::WALL_THICKNESS / 2 / Config::PPM), 0);
    contactListener.ground = ground;

    // ----- Ball -----
    b2BodyDef ballDef;
    ballDef.type = b2_dynamicBody;
    ballDef.position = toBox2D(sf::Vector2f(Config::WINDOW_W / 2, Config::WINDOW_H / 2 + Config::BALL_Y_OFFSET));
    ball = world->CreateBody(&ballDef);
    b2CircleShape circle;
    circle.m_radius = Config::BALL_RADIUS / Config::PPM;
    b2FixtureDef ballFix;
    ballFix.shape = &circle;
    ballFix.density = Config::BALL_DENSITY;
    ballFix.restitution = Config::BALL_RESTITUTION;
    ball->CreateFixture(&ballFix);
    ball->SetLinearVelocity(b2Vec2(Config::BALL_INIT_SPEED / Config::PPM, -Config::BALL_INIT_SPEED / Config::PPM));
    ball->SetBullet(true);

    // ----- Paddle -----
    b2BodyDef paddleDef;
    paddleDef.type = b2_kinematicBody;
    paddleDef.position = toBox2D(sf::Vector2f(Config::WINDOW_W / 2, Config::WINDOW_H - Config::PADDLE_Y_OFFSET));
    paddle = world->CreateBody(&paddleDef);
    b2PolygonShape paddleShape;
    paddleShape.SetAsBox((Config::PADDLE_WIDTH / 2) / Config::PPM, (Config::PADDLE_HEIGHT / 2) / Config::PPM);
    b2FixtureDef paddleFix;
    paddleFix.shape = &paddleShape;
    paddleFix.density = Config::PADDLE_DENSITY;
    paddleFix.restitution = Config::PADDLE_RESTITUTION;
    paddle->CreateFixture(&paddleFix);

    // ----- Bricks -----
    std::uniform_int_distribution<int> healthDist(Config::MIN_HEALTH, Config::MAX_HEALTH);
    std::uniform_int_distribution<int> spawnDist(0, 99);

    // Generate bricks according to configuration, randomly decide whether to spawn at each position and the health value
    for (int r = 0; r < Config::BRICK_ROWS; ++r) {
        for (int c = 0; c < Config::BRICK_COLS; ++c) {
            // Decide whether to generate a brick based on spawn probability
            if (spawnDist(rng) >= Config::BRICK_SPAWN_PROB) continue;

            // Randomly generate brick health    
            int health = healthDist(rng);
            float x = Config::BRICK_START_X + c * Config::BRICK_SPACING_X;
            float y = Config::BRICK_START_Y + r * Config::BRICK_SPACING_Y;
            float centerX = x + Config::BRICK_WIDTH / 2;
            float centerY = y + Config::BRICK_HEIGHT / 2;

            // Create brick physics body
            b2BodyDef brickDef;
            brickDef.type = b2_staticBody;
            brickDef.position = toBox2D(sf::Vector2f(centerX, centerY));
            b2Body* brick = world->CreateBody(&brickDef);
            b2PolygonShape brickShape;
            brickShape.SetAsBox((Config::BRICK_WIDTH / 2) / Config::PPM, (Config::BRICK_HEIGHT / 2) / Config::PPM);
            brick->CreateFixture(&brickShape, 0);

            // Save brick information to global lists
            bricks.push_back(brick);
            brickHealth.push_back(health);

            // Set brick color based on health and create SFML graphic
            sf::RectangleShape rect(sf::Vector2f(Config::BRICK_WIDTH, Config::BRICK_HEIGHT));
            rect.setOrigin(sf::Vector2f(Config::BRICK_WIDTH / 2, Config::BRICK_HEIGHT / 2));
            rect.setPosition(sf::Vector2f(centerX, centerY));
            if (health == 1) rect.setFillColor(sf::Color::Red);
            else if (health == 2) rect.setFillColor(sf::Color::Green);
            else rect.setFillColor(sf::Color::Blue);
            sfBricks.push_back(rect);
        }
    }

    // Initialize ball and paddle graphics
    sfBall.setRadius(Config::BALL_RADIUS);
    sfBall.setFillColor(sf::Color::White);
    sfBall.setOrigin(sf::Vector2f(Config::BALL_RADIUS, Config::BALL_RADIUS));
    sfPaddle.setSize(sf::Vector2f(Config::PADDLE_WIDTH, Config::PADDLE_HEIGHT));
    sfPaddle.setFillColor(sf::Color::White);
    sfPaddle.setOrigin(sf::Vector2f(Config::PADDLE_WIDTH / 2, Config::PADDLE_HEIGHT / 2));

    // Reset score and game state
    score = 0;
    gameWin = false;
}

// ---------- Game Update ----------
void updateGame() {
    // Control paddle movement based on keyboard input
    float paddle_speed = Config::PADDLE_SPEED / Config::PPM;
    b2Vec2 paddle_vel(0, 0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        paddle_vel.x = -paddle_speed;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        paddle_vel.x = paddle_speed;
    paddle->SetLinearVelocity(paddle_vel);

    // Physics step
    world->Step(1.0f / Config::FRAME_LIMIT, 8, 3);

    // Paddle boundary restriction
    float halfWidth = (Config::PADDLE_WIDTH / 2) / Config::PPM;
    float leftBound = Config::WALL_THICKNESS / Config::PPM;
    float rightBound = (Config::WINDOW_W - Config::WALL_THICKNESS) / Config::PPM;
    b2Vec2 paddlePos = paddle->GetPosition();
    if (paddlePos.x < leftBound + halfWidth) {
        paddlePos.x = leftBound + halfWidth;
        paddle->SetLinearVelocity(b2Vec2(0, 0));
    }
    else if (paddlePos.x > rightBound - halfWidth) {
        paddlePos.x = rightBound - halfWidth;
        paddle->SetLinearVelocity(b2Vec2(0, 0));
    }
    paddle->SetTransform(paddlePos, 0);

    // Destroy bricks with zero health and update score
    for (b2Body* brick : contactListener.toDestroy) {
        auto it = std::find(bricks.begin(), bricks.end(), brick);
        if (it != bricks.end()) {
            size_t idx = it - bricks.begin();
            world->DestroyBody(brick);
            bricks.erase(bricks.begin() + idx);
            brickHealth.erase(brickHealth.begin() + idx);
            sfBricks.erase(sfBricks.begin() + idx);
            score += Config::SCORE_PER_BRICK;
        }
    }
    contactListener.toDestroy.clear();

    // Check if all bricks have been destroyed, if so, game victory
    if (bricks.empty()) {
        gameState = STATE_GAMEOVER;
        gameWin = true;
        return;
    }

    // Update SFML graphic positions for ball and paddle
    sfBall.setPosition(toSFML(ball->GetPosition()));
    sfPaddle.setPosition(toSFML(paddle->GetPosition()));
    for (size_t i = 0; i < bricks.size(); ++i) {
        sfBricks[i].setPosition(toSFML(bricks[i]->GetPosition()));
    }
}

// ---------- Render Game ----------
void renderGame(sf::RenderWindow& window) {
    window.draw(sfBall);
    window.draw(sfPaddle);
    for (auto& brick : sfBricks) {
        window.draw(brick);
    }
}

// ---------- UI Window Functions ----------
void helpWindow(sf::RenderWindow& window, sf::Vector2u& winSize) {
    // Display help window at screen center, containing game control instructions and objective explanation
    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::Begin(Texts::Help, &showHelp, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text(Texts::Controls);
    ImGui::BulletText(Texts::MoveLeft);
    ImGui::BulletText(Texts::MoveRight);
    ImGui::Separator();
    ImGui::Text(Texts::BrickHealth);
    ImGui::BulletText(Texts::RedHit);
    ImGui::BulletText(Texts::GreenHit);
    ImGui::BulletText(Texts::BlueHit);
    ImGui::Separator();
    ImGui::Text(Texts::DestroyAll);
    ImGui::Spacing();
    if (ImGui::Button(Texts::Close, ImVec2(80, 30))) {
        showHelp = false;
    }
    ImGui::End();
}

// --------- Main Menu and Game Over Window ----------
void startMenu(sf::RenderWindow& window) {
    // Display main menu window at screen center, containing game title and option buttons
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Always);
    ImGui::Begin(Texts::MainMenu, nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text(Texts::GameName);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing(); ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button(Texts::StartGame, ImVec2(120, 40))) {
        gameState = STATE_PLAYING;
        resetGame();
    }
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button(Texts::Help, ImVec2(120, 40))) {
        showHelp = true;
    }
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button(Texts::Exit, ImVec2(120, 40))) {
        window.close();
    }
    ImGui::End();

    if (showHelp) {
        helpWindow(window, winSize);
    }
}

// Game over window
void gameOver(sf::RenderWindow& window) {
    // Display game over or victory window at screen center, showing score and option buttons
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
    ImGui::Begin(Texts::GameOver, nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.3f);
    if (gameWin)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), Texts::YouWin);
    else
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), Texts::GameOver);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30);
    ImGui::Text(Texts::ScoreFormat, score);
    ImGui::Spacing(); ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button(Texts::Restart, ImVec2(120, 40))) {
        resetGame();
        gameState = STATE_PLAYING;
    }
    ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button(Texts::MainMenu, ImVec2(120, 40))) {
        gameState = STATE_MENU;
        resetGame();
    }
    ImGui::End();
}

// ---------- Display Score Window ----------
void showScore() {
    // Display current score at the top left corner of the screen, with semi-transparent background, non-interactive
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(100, 40), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin(Texts::ScoreWindow, nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text(Texts::ScoreFormat, score);
    ImGui::End();
}

// ---------- Main Function ----------
int main() {
    // Create SFML window
    sf::RenderWindow window(sf::VideoMode({ Config::WINDOW_W, Config::WINDOW_H }), Texts::GameName, sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(Config::FRAME_LIMIT);
    ImGui::SFML::Init(window);

    // Disable ImGui automatic window position and size saving
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Main loop
    sf::Clock deltaClock;
    while (window.isOpen()) {
        // Process events
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // Update ImGui state
        ImGui::SFML::Update(window, deltaClock.restart());

        // Display different UI based on game state
        if (gameState == STATE_MENU) {
            startMenu(window);
        }
        else if (gameState == STATE_PLAYING) {
            showScore();
            updateGame();
        }
        else if (gameState == STATE_GAMEOVER) {
            gameOver(window);
        }

        // Render game and UI
        window.clear();
        renderGame(window);
        ImGui::SFML::Render(window);
        window.display();
    }

    // Clean up resources
    ImGui::SFML::Shutdown();
    if (world) delete world;
    return 0;
}