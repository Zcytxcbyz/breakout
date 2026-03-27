/*
=================================================================================================================
Breakout - Brick Breaker Game Based on Box2D and SFML
=================================================================================================================
This is a simple implementation of the classic Breakout game using C++,
Box2D for physics simulation, and SFML for graphics and audio.
The game features a ball that bounces around the screen,
a paddle controlled by the player, and rows of bricks that can be destroyed by hitting them with the ball.
The player wins by destroying all the bricks and loses if the ball falls below the paddle.
=================================================================================================================
*/


// ---------- Header Files ----------
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <SimpleIni.h>
#endif // _WIN32

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
#include <string>
#include <filesystem>

// ---------- Game Parameter Configuration ----------
namespace Config {
    // Window
    int WINDOW_W = 960;               // Window width (pixels)
    int WINDOW_H = 540;               // Window height (pixels)
    int FRAME_LIMIT = 60;             // Frame rate limit

    std::string FONT_FILE;            // Font file path
    float FONT_SIZE = 18.0f;          // Font size for UI text

    // Physics unit conversion
    float PPM = 30.0f;                // 1 meter = 30 pixels

    // Ball parameters (pixel units)
    float BALL_RADIUS = 12.0f;        // Ball radius (pixels)
    float BALL_DENSITY = 1.0f;        // Density (physics units)
    float BALL_RESTITUTION = 1.0f;    // Restitution coefficient (physics units)
    float BALL_INIT_SPEED = 200.0f;   // Pixels per second
    float MAX_BALL_SPEED = 20.0f;     // Meters per second (physics units)
    float SPEED_FACTOR = 1.1f;        // Speed multiplier after each collision
    float GRAVITY = 1.0f;             // Gravity strength (physics units, applied as downward force on the ball)

    // Paddle parameters (pixel units)
    float PADDLE_WIDTH = 160.0f;      // Paddle width (pixels)
    float PADDLE_HEIGHT = 30.0f;      // Paddle height (pixels)
    float PADDLE_SPEED = 800.0f;      // Pixels per second
    float PADDLE_DENSITY = 1.0f;      // Density (physics units)
    float PADDLE_RESTITUTION = 1.0f;  // Restitution coefficient (physics units)

    // Boundary wall thickness (pixels)
    float WALL_THICKNESS = 10.0f;     // Boundary wall thickness (pixels)

    // Brick parameters (pixel units)
    int BRICK_ROWS = 5;               // Number of brick rows
    int BRICK_COLS = 10;              // Number of brick columns
    float BRICK_START_X = 80.0f;      // Brick starting X position (pixels)
    float BRICK_START_Y = 60.0f;      // Brick starting Y position (pixels)  
    float BRICK_WIDTH = 70.0f;        // Brick width (pixels)
    float BRICK_HEIGHT = 25.0f;       // Brick height (pixels)
    float BRICK_SPACING_X = 80.0f;    // Brick horizontal spacing (pixels)
    float BRICK_SPACING_Y = 35.0f;    // Brick vertical spacing (pixels)
    int BRICK_SPAWN_PROB = 70;        // Brick spawn probability (percentage)
    int MIN_HEALTH = 1;               // Minimum brick health
    int MAX_HEALTH = 3;               // Maximum brick health
    int SCORE_PER_BRICK = 10;         // Score per brick

    // Initial position offsets (pixels)
    float BALL_Y_OFFSET = 50.0f;      // Ball's Y offset relative to screen center
    float PADDLE_Y_OFFSET = 50.0f;    // Paddle's distance from the bottom
}

// ---------- UI Text ----------
namespace Texts {
    std::string GameName = "Breakout";                              // Game name
    std::string StartGame = "Start Game";                           // Start game button text
    std::string Help = "Help";                                      // Help button text
    std::string Exit = "Exit";                                      // Exit button text
    std::string Controls = "Controls:";                             // Controls title
    std::string BrickHealth = "Brick Health (Color -> Hits):";      // Brick health explanation
    std::string MoveRight = "Move Right: D / Right Arrow";          // Move right explanation
    std::string MoveLeft = "Move Left: A / Left Arrow";             // Move left explanation
    std::string PauseHelp = "Pause: P / ESC";                       // Pause explanation
    std::string RedHit = "Red   -> 1 hit";                          // Red brick explanation
    std::string GreenHit = "Green -> 2 hits";                       // Green brick explanation
    std::string BlueHit = "Blue  -> 3 hits";                        // Blue brick explanation
    std::string DestroyAll = "Destroy all bricks to win!";          // Game objective explanation
    std::string Close = "Close";                                    // Close button text
    std::string ScoreFormat = "Score: %d";                          // Score display format
    std::string ScoreWindow = "Score";                              // Score window title
    std::string YouWin = "You Win!";                                // Victory text
    std::string GameOver = "Game Over";                             // Failure text
    std::string Restart = "Restart";                                // Restart button text
    std::string MainMenu = "Main Menu";                             // Main menu button text
    std::string Paused = "Paused";                                  // Paused text
    std::string Resume = "Resume";                                  // Resume button text
    std::string GamePaused = "Game Paused";                         // Game paused text
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

    ImGui::Begin(Texts::Help.c_str(), &showHelp,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar);

    // Help window title with close button at the top right corner
    float availWidth = ImGui::GetContentRegionAvail().x;
    ImGui::Text(Texts::Help.c_str());
    ImGui::SameLine(availWidth - 10);
    ButtonWithSound("X", ImVec2(20, 20), [&] { showHelp = false; });
    ImGui::Separator();
    ImGui::Spacing(); ImGui::Spacing();

    // Controls explanation with bullet points for each control action
    ImGui::Text(Texts::Controls.c_str());
    ImGui::BulletText(Texts::MoveLeft.c_str());
    ImGui::BulletText(Texts::MoveRight.c_str());
    ImGui::BulletText(Texts::PauseHelp.c_str());
    ImGui::Separator();

    // Brick health explanation
    ImGui::Text(Texts::BrickHealth.c_str());
    ImGui::BulletText(Texts::RedHit.c_str());
    ImGui::BulletText(Texts::GreenHit.c_str());
    ImGui::BulletText(Texts::BlueHit.c_str());

    // Game objective explanation at the bottom of the help window
    ImGui::Separator();
    ImGui::Text(Texts::DestroyAll.c_str());
    ImGui::Spacing(); ImGui::Spacing();

    // Close button at the bottom center of the window
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ButtonWithSound(Texts::Close.c_str(), ImVec2(80, 30), [&] { showHelp = false; });
    ImGui::End();
}

// --------- Main Menu and Game Over Window ----------
void startMenu(sf::RenderWindow& window) {
    // Display main menu window at screen center, containing game title and option buttons
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    ImGui::Begin(Texts::MainMenu.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    // Display game title at the top center of the menu
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text(Texts::GameName.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing(); ImGui::Spacing();

    // Start Game button resets the game state and starts a new game
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::StartGame.c_str(), ImVec2(120, 40), [&] {
        gameState = STATE_PLAYING;
        resetGame();
        });
    ImGui::Spacing();

    // Help button shows the help window, which contains game control instructions and objective explanation
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Help.c_str(), ImVec2(120, 40), [&] {
        showHelp = true;
        });
    ImGui::Spacing();

    // Exit button closes the game
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Exit.c_str(), ImVec2(120, 40), [&] {
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
    ImGui::Begin(Texts::GameOver.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    // Display game over or victory text at the top center of the window, colored green for victory and red for defeat
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.3f);
    if (gameWin)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), Texts::YouWin.c_str());
    else
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), Texts::GameOver.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    // Display final score below the game over/victory text, centered horizontally
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30);
    ImGui::Text(Texts::ScoreFormat.c_str(), score);
    ImGui::Spacing(); ImGui::Spacing();

    // Restart button resets the game state and starts a new game
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Restart.c_str(), ImVec2(120, 40), [&] {
        resetGame();
        gameState = STATE_PLAYING;
        });
    ImGui::Spacing();

    // Main Menu button returns to the main menu and resets the game state
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::MainMenu.c_str(), ImVec2(120, 40), [&] {
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
    ImGui::Begin(Texts::ScoreWindow.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text(Texts::ScoreFormat.c_str(), score);
    ImGui::End();
}

// ---------- Pause Menu ----------
void pauseMenu(sf::RenderWindow& window) {
    // Display pause menu at screen center, containing pause text and option buttons to resume or return to main menu
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
    ImGui::Begin(Texts::Paused.c_str(), nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    // Display "Game Paused" text at the top center of the window
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 56);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text(Texts::GamePaused.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing(); ImGui::Spacing();

    // Resume button resumes the game and returns to playing state
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::Resume.c_str(), ImVec2(120, 40), [&] {
        gameState = STATE_PLAYING;
        });
    ImGui::Spacing();

    // Main Menu button returns to the main menu and resets the game state
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    ButtonWithSound(Texts::MainMenu.c_str(), ImVec2(120, 40), [&] {
        gameState = STATE_MENU;
        resetGame(); // Reset game state when returning to main menu
        });
    ImGui::End();
}

#ifdef _WIN32
// ---------- Resource Loading Function ----------
std::string LoadResourceAsString(const char* resourceName, const char* resourceType) {

    HRSRC hRes = FindResource(nullptr, resourceName, resourceType);
    if (!hRes) {
#ifdef _DEBUG
        DWORD err = GetLastError();
        OutputDebugStringA(("FindResource failed for " + std::string(resourceName) +
            ", type=" + std::string(resourceType) +
            ", error=" + std::to_string(err)).c_str());
#endif // _DEBUG
        return "";
    }
    HGLOBAL hData = LoadResource(nullptr, hRes);
    if (!hData) {
#ifdef _DEBUG
        DWORD err = GetLastError();
        OutputDebugStringA(("LoadResource failed for " + std::string(resourceName) +
            ", type=" + std::string(resourceType) +
            ", error=" + std::to_string(err)).c_str());
#endif // _DEBUG
        return "";
    }
    DWORD size = SizeofResource(nullptr, hRes);
    const char* data = static_cast<const char*>(LockResource(hData));
    if (!data) {
#ifdef _DEBUG
        DWORD err = GetLastError();
        OutputDebugStringA(("SizeofResource failed for " + std::string(resourceName) +
            ", type=" + std::string(resourceType) +
            ", error=" + std::to_string(err)).c_str());
#endif // _DEBUG
        return "";
    }
    return std::string(data, size);
}

// ---------- Load Configuration from Resource ----------
void LoadConfigFromResource() {
    std::string content = LoadResourceAsString("CONFIG", RT_RCDATA);
    if (content.empty()) return;

    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadData(content.c_str()) != SI_OK) return;

    // Load window configuration values
    Config::WINDOW_W = ini.GetLongValue("Window", "WINDOW_W", Config::WINDOW_W);
    Config::WINDOW_H = ini.GetLongValue("Window", "WINDOW_H", Config::WINDOW_H);
    Config::FRAME_LIMIT = ini.GetLongValue("Window", "FRAME_LIMIT", Config::FRAME_LIMIT);

    // Font configuration values
    Config::FONT_FILE = ini.GetValue("Font", "FontFile", Config::FONT_FILE.c_str());
    Config::FONT_SIZE = static_cast<float>(ini.GetDoubleValue("Font", "FontSize", Config::FONT_SIZE));

    // Physics configuration values
    Config::PPM = static_cast<float>(ini.GetDoubleValue("Physics", "PPM", Config::PPM));
    Config::GRAVITY = static_cast<float>(ini.GetDoubleValue("Physics", "GRAVITY", Config::GRAVITY));

    // Ball configuration values
    Config::BALL_RADIUS = static_cast<float>(ini.GetDoubleValue("Ball", "BALL_RADIUS", Config::BALL_RADIUS));
    Config::BALL_DENSITY = static_cast<float>(ini.GetDoubleValue("Ball", "BALL_DENSITY", Config::BALL_DENSITY));
    Config::BALL_RESTITUTION = static_cast<float>(ini.GetDoubleValue("Ball", "BALL_RESTITUTION", Config::BALL_RESTITUTION));
    Config::BALL_INIT_SPEED = static_cast<float>(ini.GetDoubleValue("Ball", "BALL_INIT_SPEED", Config::BALL_INIT_SPEED));
    Config::MAX_BALL_SPEED = static_cast<float>(ini.GetDoubleValue("Ball", "MAX_BALL_SPEED", Config::MAX_BALL_SPEED));
    Config::SPEED_FACTOR = static_cast<float>(ini.GetDoubleValue("Ball", "SPEED_FACTOR", Config::SPEED_FACTOR));

    // Paddle configuration values
    Config::PADDLE_WIDTH = static_cast<float>(ini.GetDoubleValue("Paddle", "PADDLE_WIDTH", Config::PADDLE_WIDTH));
    Config::PADDLE_HEIGHT = static_cast<float>(ini.GetDoubleValue("Paddle", "PADDLE_HEIGHT", Config::PADDLE_HEIGHT));
    Config::PADDLE_SPEED = static_cast<float>(ini.GetDoubleValue("Paddle", "PADDLE_SPEED", Config::PADDLE_SPEED));
    Config::PADDLE_DENSITY = static_cast<float>(ini.GetDoubleValue("Paddle", "PADDLE_DENSITY", Config::PADDLE_DENSITY));
    Config::PADDLE_RESTITUTION = static_cast<float>(ini.GetDoubleValue("Paddle", "PADDLE_RESTITUTION", Config::PADDLE_RESTITUTION));

    // Wall configuration values
    Config::WALL_THICKNESS = static_cast<float>(ini.GetDoubleValue("Walls", "WALL_THICKNESS", Config::WALL_THICKNESS));

    // Brick configuration values
    Config::BRICK_ROWS = ini.GetLongValue("Bricks", "BRICK_ROWS", Config::BRICK_ROWS);
    Config::BRICK_COLS = ini.GetLongValue("Bricks", "BRICK_COLS", Config::BRICK_COLS);
    Config::BRICK_START_X = static_cast<float>(ini.GetDoubleValue("Bricks", "BRICK_START_X", Config::BRICK_START_X));
    Config::BRICK_START_Y = static_cast<float>(ini.GetDoubleValue("Bricks", "BRICK_START_Y", Config::BRICK_START_Y));
    Config::BRICK_WIDTH = static_cast<float>(ini.GetDoubleValue("Bricks", "BRICK_WIDTH", Config::BRICK_WIDTH));
    Config::BRICK_HEIGHT = static_cast<float>(ini.GetDoubleValue("Bricks", "BRICK_HEIGHT", Config::BRICK_HEIGHT));
    Config::BRICK_SPACING_X = static_cast<float>(ini.GetDoubleValue("Bricks", "BRICK_SPACING_X", Config::BRICK_SPACING_X));
    Config::BRICK_SPACING_Y = static_cast<float>(ini.GetDoubleValue("Bricks", "BRICK_SPACING_Y", Config::BRICK_SPACING_Y));
    Config::BRICK_SPAWN_PROB = ini.GetLongValue("Bricks", "BRICK_SPAWN_PROB", Config::BRICK_SPAWN_PROB);
    Config::MIN_HEALTH = ini.GetLongValue("Bricks", "MIN_HEALTH", Config::MIN_HEALTH);
    Config::MAX_HEALTH = ini.GetLongValue("Bricks", "MAX_HEALTH", Config::MAX_HEALTH);
    Config::SCORE_PER_BRICK = ini.GetLongValue("Bricks", "SCORE_PER_BRICK", Config::SCORE_PER_BRICK);

    // Offset configuration values
    Config::BALL_Y_OFFSET = static_cast<float>(ini.GetDoubleValue("Offsets", "BALL_Y_OFFSET", Config::BALL_Y_OFFSET));
    Config::PADDLE_Y_OFFSET = static_cast<float>(ini.GetDoubleValue("Offsets", "PADDLE_Y_OFFSET", Config::PADDLE_Y_OFFSET));
}

// ---------- Load Text Resources from Resource ----------
void LoadTextsFromResource() {
    std::string content = LoadResourceAsString("TEXTS", RT_RCDATA);
    if (content.empty()) return;

    CSimpleIniA ini;
    ini.SetUnicode();
    if (ini.LoadData(content.c_str()) != SI_OK) return;

    // Load all text resources from the INI file, using existing default values if keys are missing
    Texts::GameName = ini.GetValue("Game", "GameName", Texts::GameName.c_str());
    Texts::StartGame = ini.GetValue("Menu", "StartGame", Texts::StartGame.c_str());
    Texts::Help = ini.GetValue("Menu", "Help", Texts::Help.c_str());
    Texts::Exit = ini.GetValue("Menu", "Exit", Texts::Exit.c_str());
    Texts::Controls = ini.GetValue("Menu", "Controls", Texts::Controls.c_str());
    Texts::BrickHealth = ini.GetValue("Menu", "BrickHealth", Texts::BrickHealth.c_str());
    Texts::MoveRight = ini.GetValue("Menu", "MoveRight", Texts::MoveRight.c_str());
    Texts::MoveLeft = ini.GetValue("Menu", "MoveLeft", Texts::MoveLeft.c_str());
    Texts::PauseHelp = ini.GetValue("Menu", "PauseHelp", Texts::PauseHelp.c_str());
    Texts::RedHit = ini.GetValue("Menu", "RedHit", Texts::RedHit.c_str());
    Texts::GreenHit = ini.GetValue("Menu", "GreenHit", Texts::GreenHit.c_str());
    Texts::BlueHit = ini.GetValue("Menu", "BlueHit", Texts::BlueHit.c_str());
    Texts::DestroyAll = ini.GetValue("Menu", "DestroyAll", Texts::DestroyAll.c_str());
    Texts::Close = ini.GetValue("Menu", "Close", Texts::Close.c_str());
    Texts::ScoreFormat = ini.GetValue("Score", "ScoreFormat", Texts::ScoreFormat.c_str());
    Texts::ScoreWindow = ini.GetValue("Score", "ScoreWindow", Texts::ScoreWindow.c_str());
    Texts::YouWin = ini.GetValue("GameOver", "YouWin", Texts::YouWin.c_str());
    Texts::GameOver = ini.GetValue("GameOver", "GameOver", Texts::GameOver.c_str());
    Texts::Restart = ini.GetValue("GameOver", "Restart", Texts::Restart.c_str());
    Texts::MainMenu = ini.GetValue("GameOver", "MainMenu", Texts::MainMenu.c_str());
    Texts::Paused = ini.GetValue("Pause", "Paused", Texts::Paused.c_str());
    Texts::Resume = ini.GetValue("Pause", "Resume", Texts::Resume.c_str());
    Texts::GamePaused = ini.GetValue("Pause", "GamePaused", Texts::GamePaused.c_str());
}
#endif // _WIN32

// ---------- Load Custom Font ----------
void loadFont(ImGuiIO& io) {
    if (!Config::FONT_FILE.empty()) {
        std::error_code ec;
        if (!std::filesystem::exists(Config::FONT_FILE, ec)) {
#ifdef _DEBUG
#ifdef _WIN32
            OutputDebugStringA(("Font file not found: " + Config::FONT_FILE).c_str());
#endif
#endif
        }
        else {
            std::filesystem::path fontPath = Config::FONT_FILE;
            io.Fonts->Clear();
            ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), Config::FONT_SIZE);
            if (font) {
                ImGui::SFML::UpdateFontTexture();
            }
            else {
#ifdef _DEBUG
#ifdef _WIN32
                OutputDebugStringA(("Failed to load font: " + Config::FONT_FILE).c_str());
#endif
#endif
            }
        }
    }
}

// ---------- Main Function ----------
int main() {
    // Load configuration and text resources
#ifdef _WIN32
    LoadConfigFromResource();
    LoadTextsFromResource();
#endif // _WIN32

    // Create SFML window
    sf::RenderWindow window(sf::VideoMode({
        static_cast<unsigned int>(Config::WINDOW_W), 
        static_cast<unsigned int>(Config::WINDOW_H) }),
        Texts::GameName.c_str(), sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(Config::FRAME_LIMIT);
    ImGui::SFML::Init(window);

    // Set custom window icon (requires an icon resource with ID 1 in the executable)
#ifdef _WIN32
    HWND hwnd = window.getNativeHandle();
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)hIcon);
        SetClassLongPtr(hwnd, GCLP_HICONSM, (LONG_PTR)hIcon);
        SendMessage(hwnd, WM_NCACTIVATE, TRUE, 0);
    }
#endif // _WIN32

    // Disable ImGui automatic window position and size saving
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    loadFont(io); // Load custom font for ImGui

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

// End of code
