#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <random>
#include <box2d/box2d.h>
#include <algorithm>

// ---------- 游戏参数配置 ----------
namespace Config {
    // 窗口
    const int WINDOW_W = 960;
    const int WINDOW_H = 540;
    const int FRAME_LIMIT = 60;

    // 物理单位转换
    const float PPM = 30.0f;

    // 球参数（像素单位）
    const float BALL_RADIUS = 12.0f;
    const float BALL_DENSITY = 1.0f;
    const float BALL_RESTITUTION = 1.0f;
    const float BALL_INIT_SPEED = 200.0f;    // 像素/秒
    const float MAX_BALL_SPEED = 20.0f;      // 米/秒（物理单位）
    const float SPEED_FACTOR = 1.05f;

    // 挡板参数（像素单位）
    const float PADDLE_WIDTH = 160.0f;
    const float PADDLE_HEIGHT = 30.0f;
    const float PADDLE_SPEED = 300.0f;       // 像素/秒
    const float PADDLE_DENSITY = 1.0f;
    const float PADDLE_RESTITUTION = 1.0f;

    // 边界墙厚度（像素）
    const float WALL_THICKNESS = 10.0f;

    // 砖块参数（像素单位）
    const int BRICK_ROWS = 5;
    const int BRICK_COLS = 10;
    const float BRICK_START_X = 80.0f;
    const float BRICK_START_Y = 60.0f;
    const float BRICK_WIDTH = 70.0f;
    const float BRICK_HEIGHT = 25.0f;
    const float BRICK_SPACING_X = 80.0f;
    const float BRICK_SPACING_Y = 35.0f;
    const int BRICK_SPAWN_PROB = 70;         // 0-100
    const int MIN_HEALTH = 1;
    const int MAX_HEALTH = 3;
    const int SCORE_PER_BRICK = 10;

    // 初始位置偏移（像素）
    const float BALL_Y_OFFSET = 50.0f;        // 球相对于屏幕中心的Y偏移
    const float PADDLE_Y_OFFSET = 50.0f;      // 挡板距离底部的距离
}

// ---------- 全局变量 ----------
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
};

GameState gameState = STATE_MENU;
bool gameWin = false;
int score = 0;
bool showHelp = false;

b2World* world = nullptr;
b2Body* ball = nullptr;
b2Body* paddle = nullptr;
std::vector<b2Body*> bricks;
std::vector<int> brickHealth;
std::vector<sf::RectangleShape> sfBricks;

sf::CircleShape sfBall;
sf::RectangleShape sfPaddle;

std::random_device rd;
std::mt19937 rng(rd());

inline b2Vec2 toBox2D(const sf::Vector2f& pos) {
    return { pos.x / Config::PPM, (Config::WINDOW_H - pos.y) / Config::PPM };
}

inline sf::Vector2f toSFML(const b2Vec2& pos) {
    return { pos.x * Config::PPM, Config::WINDOW_H - pos.y * Config::PPM };
}

// ---------- 碰撞监听器 ----------
class ContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override {
        b2Body* bodyA = contact->GetFixtureA()->GetBody();
        b2Body* bodyB = contact->GetFixtureB()->GetBody();

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

            if ((bodyA == ball && bodyB == ground) || (bodyB == ball && bodyA == ground)) {
                gameState = STATE_GAMEOVER;
                gameWin = false;
            }
        }
    }

    std::vector<b2Body*> toDestroy;
    b2Body* ground = nullptr;
};

ContactListener contactListener;

// ---------- 重置游戏 ----------
void resetGame() {
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

    world = new b2World(b2Vec2(0, 0));
    world->SetContactListener(&contactListener);

    // ----- 边界墙 -----
    b2BodyDef wallDef;
    wallDef.type = b2_staticBody;
    b2PolygonShape wallShape;

    // 左墙
    wallShape.SetAsBox(Config::WALL_THICKNESS / 2 / Config::PPM, Config::WINDOW_H / 2 / Config::PPM);
    b2Body* leftWall = world->CreateBody(&wallDef);
    leftWall->CreateFixture(&wallShape, 0);
    leftWall->SetTransform(b2Vec2(Config::WALL_THICKNESS / 2 / Config::PPM, Config::WINDOW_H / 2 / Config::PPM), 0);

    // 右墙
    wallShape.SetAsBox(Config::WALL_THICKNESS / 2 / Config::PPM, Config::WINDOW_H / 2 / Config::PPM);
    b2Body* rightWall = world->CreateBody(&wallDef);
    rightWall->CreateFixture(&wallShape, 0);
    rightWall->SetTransform(b2Vec2((Config::WINDOW_W - Config::WALL_THICKNESS) / Config::PPM, Config::WINDOW_H / 2 / Config::PPM), 0);

    // 天花板（顶部）
    wallShape.SetAsBox(Config::WINDOW_W / 2 / Config::PPM, Config::WALL_THICKNESS / 2 / Config::PPM);
    b2Body* ceiling = world->CreateBody(&wallDef);
    ceiling->CreateFixture(&wallShape, 0);
    ceiling->SetTransform(b2Vec2(Config::WINDOW_W / 2 / Config::PPM, (Config::WINDOW_H - Config::WALL_THICKNESS / 2) / Config::PPM), 0);

    // 地面（底部，用于失败检测）
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    b2PolygonShape groundShape;
    groundShape.SetAsBox(Config::WINDOW_W / 2 / Config::PPM, Config::WALL_THICKNESS / 2 / Config::PPM);
    b2Body* ground = world->CreateBody(&groundDef);
    ground->CreateFixture(&groundShape, 0);
    ground->SetTransform(b2Vec2(Config::WINDOW_W / 2 / Config::PPM, Config::WALL_THICKNESS / 2 / Config::PPM), 0);
    contactListener.ground = ground;

    // ----- 球 -----
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

    // ----- 挡板 -----
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

    // ----- 砖块 -----
    std::uniform_int_distribution<int> healthDist(Config::MIN_HEALTH, Config::MAX_HEALTH);
    std::uniform_int_distribution<int> spawnDist(0, 99);

    for (int r = 0; r < Config::BRICK_ROWS; ++r) {
        for (int c = 0; c < Config::BRICK_COLS; ++c) {
            if (spawnDist(rng) >= Config::BRICK_SPAWN_PROB) continue;

            int health = healthDist(rng);
            float x = Config::BRICK_START_X + c * Config::BRICK_SPACING_X;
            float y = Config::BRICK_START_Y + r * Config::BRICK_SPACING_Y;
            float centerX = x + Config::BRICK_WIDTH / 2;
            float centerY = y + Config::BRICK_HEIGHT / 2;

            b2BodyDef brickDef;
            brickDef.type = b2_staticBody;
            brickDef.position = toBox2D(sf::Vector2f(centerX, centerY));
            b2Body* brick = world->CreateBody(&brickDef);
            b2PolygonShape brickShape;
            brickShape.SetAsBox((Config::BRICK_WIDTH / 2) / Config::PPM, (Config::BRICK_HEIGHT / 2) / Config::PPM);
            brick->CreateFixture(&brickShape, 0);

            bricks.push_back(brick);
            brickHealth.push_back(health);

            sf::RectangleShape rect(sf::Vector2f(Config::BRICK_WIDTH, Config::BRICK_HEIGHT));
            rect.setOrigin(sf::Vector2f(Config::BRICK_WIDTH / 2, Config::BRICK_HEIGHT / 2));
            rect.setPosition(sf::Vector2f(centerX, centerY));
            if (health == 1) rect.setFillColor(sf::Color::Red);
            else if (health == 2) rect.setFillColor(sf::Color::Green);
            else rect.setFillColor(sf::Color::Blue);
            sfBricks.push_back(rect);
        }
    }

    // 初始化球和挡板图形
    sfBall.setRadius(Config::BALL_RADIUS);
    sfBall.setFillColor(sf::Color::White);
    sfBall.setOrigin(sf::Vector2f(Config::BALL_RADIUS, Config::BALL_RADIUS));
    sfPaddle.setSize(sf::Vector2f(Config::PADDLE_WIDTH, Config::PADDLE_HEIGHT));
    sfPaddle.setFillColor(sf::Color::White);
    sfPaddle.setOrigin(sf::Vector2f(Config::PADDLE_WIDTH / 2, Config::PADDLE_HEIGHT / 2));

    score = 0;
    gameWin = false;
}

// ---------- 游戏更新 ----------
void updateGame() {
    float paddle_speed = Config::PADDLE_SPEED / Config::PPM;
    b2Vec2 paddle_vel(0, 0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        paddle_vel.x = -paddle_speed;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        paddle_vel.x = paddle_speed;
    paddle->SetLinearVelocity(paddle_vel);

    world->Step(1.0f / Config::FRAME_LIMIT, 8, 3);

    // 挡板边界限制
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

    if (bricks.empty()) {
        gameState = STATE_GAMEOVER;
        gameWin = true;
        return;
    }

    sfBall.setPosition(toSFML(ball->GetPosition()));
    sfPaddle.setPosition(toSFML(paddle->GetPosition()));
    for (size_t i = 0; i < bricks.size(); ++i) {
        sfBricks[i].setPosition(toSFML(bricks[i]->GetPosition()));
    }
}

// ---------- 渲染游戏 ----------
void renderGame(sf::RenderWindow& window) {
    window.draw(sfBall);
    window.draw(sfPaddle);
    for (auto& brick : sfBricks) {
        window.draw(brick);
    }
}

// ---------- UI 窗口函数 ----------
void helpWindow(sf::RenderWindow& window, sf::Vector2u& winSize) {
    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Help", &showHelp, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Controls:");
    ImGui::BulletText("Move Left: A / Left Arrow");
    ImGui::BulletText("Move Right: D / Right Arrow");
    ImGui::Separator();
    ImGui::Text("Brick Health (Color -> Hits):");
    ImGui::BulletText("Red   -> 1 hit");
    ImGui::BulletText("Green -> 2 hits");
    ImGui::BulletText("Blue  -> 3 hits");
    ImGui::Separator();
    ImGui::Text("Destroy all bricks to win!");
    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2(80, 30))) {
        showHelp = false;
    }
    ImGui::End();
}

void startMenu(sf::RenderWindow& window) {
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_Always);
    ImGui::Begin("MainMenu", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("Breakout");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing(); ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button("Start Game", ImVec2(120, 40))) {
        gameState = STATE_PLAYING;
        resetGame();
    }
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button("Help", ImVec2(120, 40))) {
        showHelp = true;
    }
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button("Exit", ImVec2(120, 40))) {
        window.close();
    }
    ImGui::End();

    if (showHelp) {
        helpWindow(window, winSize);
    }
}

void gameOver(sf::RenderWindow& window) {
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
    ImGui::Begin("GameOver", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.3f);
    if (gameWin)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "You Win!");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Game Over");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30);
    ImGui::Text("Score: %d", score);
    ImGui::Spacing(); ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button("Restart", ImVec2(120, 40))) {
        resetGame();
        gameState = STATE_PLAYING;
    }
    ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button("Main Menu", ImVec2(120, 40))) {
        gameState = STATE_MENU;
        resetGame();
    }
    ImGui::End();
}

void showScore() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(100, 40), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Score", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text("Score: %d", score);
    ImGui::End();
}

// ---------- 主函数 ----------
int main() {
    sf::RenderWindow window(sf::VideoMode({ Config::WINDOW_W, Config::WINDOW_H }), "Breakout", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(Config::FRAME_LIMIT);
    ImGui::SFML::Init(window);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

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

        window.clear();
        renderGame(window);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    if (world) delete world;
    return 0;
}