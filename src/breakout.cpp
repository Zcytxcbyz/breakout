// Breakout - Brick Breaker Game Based on Box2D and SFML

// ---------- Header Files ----------
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <random>
#include <box2d/box2d.h>
#include <algorithm>
#include <cmath>
#include <stdint.h>

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
    const float SPEED_FACTOR = 1.1f;        // Speed multiplier after each collision
    const float GRAVITY = 1.0f;             // Gravity strength (physics units, applied as downward force on the ball)

    // Paddle parameters (pixel units)
    const float PADDLE_WIDTH = 160.0f;      // Paddle width (pixels)
    const float PADDLE_HEIGHT = 30.0f;      // Paddle height (pixels)
    const float PADDLE_SPEED = 500.0f;      // Pixels per second
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
    const char* PauseHelp = "Pause: P / ESC";                       // Pause explanation
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
    const char* Paused = "Paused";                                  // Paused text
    const char* Resume = "Resume";                                  // Resume button text
    const char* GamePaused = "Game Paused";                         // Game paused text
}

// ---------- Global Variables ----------
// Game state enumeration
enum GameState {
    STATE_MENU,         // Main menu
    STATE_PLAYING,      // Game in progress
    STATE_GAMEOVER,     // Game over
    STATE_PAUSED        // Game paused
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

sf::SoundBuffer hitBrickBuffer;             // Sound buffer for hitting bricks
sf::SoundBuffer hitPaddleBuffer;            // Sound buffer for hitting paddle
sf::SoundBuffer clickBuffer;                // Sound buffer for button clicks
sf::SoundBuffer hoverBuffer;                // Sound buffer for button hover
sf::SoundBuffer wallBuffer;                 // Sound buffer for hitting walls
sf::SoundBuffer winBuffer;                  // Sound buffer for winning the game
sf::SoundBuffer loseBuffer;                 // Sound buffer for losing the game
sf::Sound* hitBrickSound = nullptr;         // Sound object for playing brick hit sound
sf::Sound* hitPaddleSound = nullptr;        // Sound object for playing paddle hit sound
sf::Sound* clickSound = nullptr;            // Sound object for playing button click sound
sf::Sound* hoverSound = nullptr;            // Sound object for playing button hover sound
sf::Sound* wallSound = nullptr;             // Sound object for playing wall hit sound
sf::Sound* winSound = nullptr;              // Sound object for playing win sound
sf::Sound* loseSound = nullptr;             // Sound object for playing lose sound

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

        // Check if the other object is a brick
        auto it = std::find(bricks.begin(), bricks.end(), other);
        bool isBrick = (it != bricks.end());

        // If the ball hit something, check what it hit and respond accordingly
        if (ballHit) {
            // If the collided object is the paddle or a brick, increase the ball's speed
            if (other == paddle || isBrick) {
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

            // Play the appropriate sound effect based on what was hit
            if (isBrick) {
                if(hitBrickSound) hitBrickSound->play(); // Play brick hit sound
            }
            else if (other == paddle) {
                if(hitPaddleSound) hitPaddleSound->play(); // Play paddle hit sound
            }
            else if (other != ground) {
                if(wallSound) wallSound->play();      // Play wall hit sound for hitting walls
            }

            // If the collided object is the ground, game over
            if ((bodyA == ball && bodyB == ground) || (bodyB == ball && bodyA == ground)) {
                gameState = STATE_GAMEOVER;
                gameWin = false;
                if(loseSound) loseSound->play(); // Play lose sound when hitting the ground
            }
        }
    }

    // No processing needed on collision end
    std::vector<b2Body*> toDestroy;
    b2Body* ground = nullptr;
};

ContactListener contactListener;

// ---------- Sound Generation Function ----------
sf::SoundBuffer generateHitSound(float frequency = 440.0f, float duration = 0.1f, float volume = 0.5f) {
    const int sampleRate = 44100;
    int sampleCount = static_cast<int>(sampleRate * duration);
    std::vector<std::int16_t> samples(sampleCount);

    // Generate a simple hit sound using a decaying sine wave
    for (int i = 0; i < sampleCount; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float envelope = std::exp(-t * 20.0f); // Exponential decay envelope
        float value = envelope * std::sin(2.0f * 3.14159265f * frequency * t);
        samples[i] = static_cast<std::int16_t>(value * volume * 32767);
    }

    // Map the samples to a mono sound channel
    std::vector<sf::SoundChannel> channelMap = { sf::SoundChannel::Mono };

    // Load the generated samples into an SFML sound buffer
    sf::SoundBuffer buffer;
    buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate, channelMap);
    return buffer;
}

// ---------- Button with Sound Effect ----------
bool ButtonWithSound(const char* label, const ImVec2& size, std::function<void()> onClick) {
    // Record hover state (using button label as the only identifier)
    static std::unordered_map<std::string, bool> hoverStateMap;
    bool& wasHovered = hoverStateMap[label];  // Default false

    // Check if the button is clicked
    bool clicked = false;
    if (ImGui::Button(label, size)) {
        if (clickSound) clickSound->play();
        if (onClick) onClick(); // Call the provided click handler  
        clicked = true;
    }

    // Check hover state and play hover sound on transition from not hovered to hovered
    bool isHovered = ImGui::IsItemHovered();
    if (isHovered && !wasHovered) {
        if (hoverSound) hoverSound->play();
    }
    wasHovered = isHovered;

    return clicked;
}

// ---------- Sound Initialization ----------
void InitSound() {
    // Generate a higher-pitched, shorter sound for hitting bricks to differentiate it from hitting the paddle
    hitBrickBuffer = generateHitSound(600.0f, 0.08f, 0.6f);
    hitBrickSound = new sf::Sound(hitBrickBuffer);

    // Generate a lower-pitched, longer sound for hitting the paddle to differentiate it from hitting bricks
    hitPaddleBuffer = generateHitSound(300.0f, 0.1f, 0.5f);
    hitPaddleSound = new sf::Sound(hitPaddleBuffer);

    // Generate distinct click and hover sounds for UI interactions to enhance feedback
    clickBuffer = generateHitSound(880.0f, 0.05f, 0.4f);
    clickSound = new sf::Sound(clickBuffer);

    // Generate a softer, higher-pitched sound for button hover to differentiate it from clicks and in-game hits
    hoverBuffer = generateHitSound(440.0f, 0.04f, 0.3f);
    hoverSound = new sf::Sound(hoverBuffer);

    // Generate a solid, mid-frequency sound for hitting walls to differentiate it from hitting bricks and the paddle
    wallBuffer = generateHitSound(400.0f, 0.06f, 0.5f);
    wallSound = new sf::Sound(wallBuffer);

    // Generate a bright, celebratory sound for winning the game to provide positive feedback
    winBuffer = generateHitSound(523.0f, 0.2f, 0.6f);
    winSound = new sf::Sound(winBuffer);

    // Generate a somber, lower-pitched sound for losing the game to provide appropriate feedback
    loseBuffer = generateHitSound(220.0f, 0.15f, 0.5f);
    loseSound = new sf::Sound(loseBuffer);

}

// ---------- Sound Cleanup ----------
void cleanupSound() {
    if(hitBrickSound) delete hitBrickSound;
    if(hitPaddleSound) delete hitPaddleSound;
    if(clickSound) delete clickSound;
    if(hoverSound) delete hoverSound;
    if(wallSound) delete wallSound;
    if(winSound) delete winSound;
    if(loseSound) delete loseSound;
}

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
    world = new b2World(b2Vec2(0, -Config::GRAVITY));
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
    sfBall.setPosition(toSFML(ball->GetPosition()));
    sfPaddle.setSize(sf::Vector2f(Config::PADDLE_WIDTH, Config::PADDLE_HEIGHT));
    sfPaddle.setFillColor(sf::Color::White);
    sfPaddle.setOrigin(sf::Vector2f(Config::PADDLE_WIDTH / 2, Config::PADDLE_HEIGHT / 2));
    sfPaddle.setPosition(toSFML(paddle->GetPosition()));

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
        if(winSound) winSound->play(); // Play win sound effect on victory
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
    ImGui::SetNextWindowSize(ImVec2(280, 260), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    ImGui::Begin(Texts::Help, &showHelp,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar);

    // Help window title with close button at the top right corner
    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGui::Text(Texts::Help);
    ImGui::SameLine(availWidth - 10);
    ButtonWithSound("X", ImVec2(20, 20), [&] { showHelp = false; });
    ImGui::Separator();
    ImGui::Spacing(); ImGui::Spacing();

    // Controls explanation with bullet points for each control action
    ImGui::Text(Texts::Controls);
    ImGui::BulletText(Texts::MoveLeft);
    ImGui::BulletText(Texts::MoveRight);
    ImGui::BulletText(Texts::PauseHelp);
    ImGui::Separator();

    // Brick health explanation
    ImGui::Text(Texts::BrickHealth);
    ImGui::BulletText(Texts::RedHit);
    ImGui::BulletText(Texts::GreenHit);
    ImGui::BulletText(Texts::BlueHit);

    // Game objective explanation at the bottom of the help window
    ImGui::Separator();
    ImGui::Text(Texts::DestroyAll);
    ImGui::Spacing(); ImGui::Spacing();

    // Close button at the bottom center of the window
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
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
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    ImGui::Begin(Texts::MainMenu, nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    // Display game title at the top center of the menu
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text(Texts::GameName);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing(); ImGui::Spacing();

    // Start Game button resets the game state and starts a new game
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::StartGame, ImVec2(120, 40), [&] {
        gameState = STATE_PLAYING;
        resetGame();
        });
    ImGui::Spacing();

    // Help button shows the help window, which contains game control instructions and objective explanation
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Help, ImVec2(120, 40), [&] {
        showHelp = true;
        });
    ImGui::Spacing();

    // Exit button closes the game
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Exit, ImVec2(120, 40), [&] {
        window.close();
        });
    ImGui::End();

    // If the help window is triggered, display it on top of the main menu
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

    // Display game over or victory text at the top center of the window, colored green for victory and red for defeat
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.3f);
    if (gameWin)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), Texts::YouWin);
    else
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), Texts::GameOver);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    // Display final score below the game over/victory text, centered horizontally
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30);
    ImGui::Text(Texts::ScoreFormat, score);
    ImGui::Spacing(); ImGui::Spacing();

    // Restart button resets the game state and starts a new game
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Restart, ImVec2(120, 40), [&] {
        resetGame();
        gameState = STATE_PLAYING;
        });
    ImGui::Spacing();

    // Main Menu button returns to the main menu and resets the game state
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::MainMenu, ImVec2(120, 40), [&] {
        gameState = STATE_MENU;
        resetGame();
        });
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

// ---------- Pause Menu ----------
void pauseMenu(sf::RenderWindow& window) {
    // Display pause menu at screen center, containing pause text and option buttons to resume or return to main menu
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
    ImGui::Begin(Texts::Paused, nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    // Display "Game Paused" text at the top center of the window
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 56);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text(Texts::GamePaused);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing(); ImGui::Spacing();

    // Resume button resumes the game and returns to playing state
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Resume, ImVec2(120, 40), [&] {
        gameState = STATE_PLAYING;
        });
    ImGui::Spacing();

    // Main Menu button returns to the main menu and resets the game state
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::MainMenu, ImVec2(120, 40), [&] {
        gameState = STATE_MENU;
        resetGame(); // Reset game state when returning to main menu
        });
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

    resetGame(); // Create physical world and game objects in advance
    InitSound(); // Initialize sound effects in advance to avoid delay on first play

    // Main loop
    sf::Clock deltaClock;
    while (window.isOpen()) {
        // Process events
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            // Handle window close event and pause/unpause game with Escape or P key
            if (event->is<sf::Event::Closed>())
                window.close();

            // Toggle pause state when Escape or P key is pressed
            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Escape
                    || keyEvent->code == sf::Keyboard::Key::P) {
                    if (gameState == STATE_PLAYING) gameState = STATE_PAUSED;
                    else if (gameState == STATE_PAUSED) gameState = STATE_PLAYING;
                }
            }
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
        else if (gameState == STATE_PAUSED) {
            showScore();
            pauseMenu(window);
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
    cleanupSound();
    return 0;
}
