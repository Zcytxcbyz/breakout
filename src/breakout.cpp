#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <random>
#include <box2d/box2d.h>
#include <algorithm>

// ----------全局变量定义----------

// 游戏状态枚举
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
};

GameState gameState = STATE_MENU;
bool gameWin = false;   // true=胜利, false=失败
int score = 0;
bool showHelp = false;   // 控制帮助窗口显示

// 物理世界与物体
b2World* world = nullptr;
b2Body* ball = nullptr;
b2Body* paddle = nullptr;
std::vector<b2Body*> bricks;
std::vector<int> brickHealth;       // 与 bricks 同步的血量
std::vector<sf::RectangleShape> sfBricks; // 图形

// SFML 图形
sf::CircleShape sfBall;
sf::RectangleShape sfPaddle;

// 坐标转换常量
const float PPM = 30.0f;            // 像素/米
const int WINDOW_W = 960;
const int WINDOW_H = 540;

std::random_device rd; // 随机数生成器
std::mt19937 rng(rd()); // 随机数引擎

inline b2Vec2 toBox2D(const sf::Vector2f& pos) {
    return { pos.x / PPM, (WINDOW_H - pos.y) / PPM };
}

inline sf::Vector2f toSFML(const b2Vec2& pos) {
    return { pos.x * PPM, WINDOW_H - pos.y * PPM };
}

// ---------- 碰撞监听器 ----------
class ContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override {
        b2Body* bodyA = contact->GetFixtureA()->GetBody();
        b2Body* bodyB = contact->GetFixtureB()->GetBody();

        // 判断球是否参与
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
            // 如果碰撞对象是砖块或挡板，则加速
            if (other == paddle || std::find(bricks.begin(), bricks.end(), other) != bricks.end()) {
                b2Vec2 vel = ball->GetLinearVelocity();
                // 加速系数（可根据需要调整）
                const float speedFactor = 1.05f;
                vel.x *= speedFactor;
                vel.y *= speedFactor;

                // 限制最大速度
                const float maxSpeed = 15.0f;
                float speed = vel.Length();
                if (speed > maxSpeed) {
                    vel *= maxSpeed / speed;
                }

                ball->SetLinearVelocity(vel);
            }

            // 判断球与砖块碰撞
            if ((bodyA == ball && bodyB != paddle) || (bodyB == ball && bodyA != paddle)) {
                b2Body* brickBody = (bodyA == ball) ? bodyB : bodyA;
                // 查找砖块索引
                auto it = std::find(bricks.begin(), bricks.end(), brickBody);
                if (it != bricks.end()) {
                    size_t idx = it - bricks.begin();
                    // 减少血量
                    brickHealth[idx]--;
                    // 更新颜色
                    if (brickHealth[idx] == 1) sfBricks[idx].setFillColor(sf::Color::Red);
                    else if (brickHealth[idx] == 2) sfBricks[idx].setFillColor(sf::Color::Green);
                    // 血量3保持蓝色（已在创建时设置）
                    // 如果血量为0，标记待销毁（不能在此处删除，因为正在遍历接触）
                    if (brickHealth[idx] == 0) {
                        // 记录待销毁的砖块，稍后统一处理
                        toDestroy.push_back(brickBody);
                    }
                }
            }
            // 球与地面碰撞（失败）
            if ((bodyA == ball && bodyB == ground) || (bodyB == ball && bodyA == ground)) {
                gameState = STATE_GAMEOVER;
                gameWin = false;
            }
        }
    }

    std::vector<b2Body*> toDestroy; // 存放需要销毁的砖块指针
    b2Body* ground = nullptr;       // 地面刚体，由外部设置
};

ContactListener contactListener;


// ---------- 重置游戏 ----------
void resetGame() {
    // 清理旧世界
    if (world) {
        // 先销毁所有砖块刚体（因为世界销毁时会自动销毁，但我们需要清空容器）
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

    // 创建新世界
    world = new b2World(b2Vec2(0, 0)); // 无重力

    // 设置碰撞监听器
    world->SetContactListener(&contactListener);

    // ----- 边界墙 -----
    b2BodyDef wallDef;
    wallDef.type = b2_staticBody;
    b2PolygonShape wallShape;

    // 左墙
    wallShape.SetAsBox(10.0f / PPM, WINDOW_H / 2 / PPM);
    b2Body* leftWall = world->CreateBody(&wallDef);
    leftWall->CreateFixture(&wallShape, 0);
    leftWall->SetTransform(b2Vec2(10.0f / PPM, WINDOW_H / 2 / PPM), 0);

    // 右墙
    wallShape.SetAsBox(10.0f / PPM, WINDOW_H / 2 / PPM);
    b2Body* rightWall = world->CreateBody(&wallDef);
    rightWall->CreateFixture(&wallShape, 0);
    rightWall->SetTransform(b2Vec2((WINDOW_W - 10.0f) / PPM, WINDOW_H / 2 / PPM), 0);

    // 天花板
    wallShape.SetAsBox(WINDOW_W / 2 / PPM, 10.0f / PPM);
    b2Body* ceiling = world->CreateBody(&wallDef);
    ceiling->CreateFixture(&wallShape, 0);
    ceiling->SetTransform(b2Vec2(WINDOW_W / 2 / PPM, (WINDOW_H - 10.0f) / PPM), 0);

    // 地面（用于失败检测）
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    b2PolygonShape groundShape;
    groundShape.SetAsBox(WINDOW_W / 2 / PPM, 10.0f / PPM);
    b2Body* ground = world->CreateBody(&groundDef);
    ground->CreateFixture(&groundShape, 0);
    ground->SetTransform(b2Vec2(WINDOW_W / 2 / PPM, 10.0f / PPM), 0);
    contactListener.ground = ground;   // 供监听器使用

    // ----- 球 -----
    b2BodyDef ballDef;
    ballDef.type = b2_dynamicBody;
    ballDef.position = toBox2D(sf::Vector2f(WINDOW_W / 2, WINDOW_H / 2 + 50));
    ball = world->CreateBody(&ballDef);
    b2CircleShape circle;
    circle.m_radius = 12.0f / PPM;
    b2FixtureDef ballFix;
    ballFix.shape = &circle;
    ballFix.density = 1.0f;
    ballFix.restitution = 1.0f;      // 完全弹性
    ball->CreateFixture(&ballFix);
    ball->SetLinearVelocity(b2Vec2(150.0f / PPM, -150.0f / PPM));

    // ----- 挡板 -----
    b2BodyDef paddleDef;
    paddleDef.type = b2_kinematicBody;
    paddleDef.position = toBox2D(sf::Vector2f(WINDOW_W / 2, WINDOW_H - 50));
    paddle = world->CreateBody(&paddleDef);
    b2PolygonShape paddleShape;
    paddleShape.SetAsBox(80.0f / PPM, 15.0f / PPM);
    b2FixtureDef paddleFix;
    paddleFix.shape = &paddleShape;
    paddleFix.density = 1.0f;
    paddleFix.restitution = 1.0f;
    paddle->CreateFixture(&paddleFix);

    // ----- 砖块（随机生成，三种血量）-----
    const int rows = 5, cols = 10;
    const float startX = 80.0f, startY = 60.0f;
    const float brickW = 70.0f, brickH = 25.0f;
    const float spacingX = 80.0f, spacingY = 35.0f;

    std::uniform_int_distribution<int> healthDist(1, 3);
    std::uniform_int_distribution<int> spawnDist(0, 99); // 0~99

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (spawnDist(rng) >= 70) continue;   // 70% 生成概率

            int health = healthDist(rng);
            float x = startX + c * spacingX;
            float y = startY + r * spacingY;

            // 物理刚体
            b2BodyDef brickDef;
            brickDef.type = b2_staticBody;
            brickDef.position = toBox2D(sf::Vector2f(x + brickW / 2, y + brickH / 2));
            b2Body* brick = world->CreateBody(&brickDef);
            b2PolygonShape brickShape;
            brickShape.SetAsBox(brickW / 2 / PPM, brickH / 2 / PPM);
            brick->CreateFixture(&brickShape, 0);

            bricks.push_back(brick);
            brickHealth.push_back(health);

            // SFML 图形
            sf::RectangleShape rect(sf::Vector2f(brickW, brickH));
            rect.setOrigin(sf::Vector2f(brickW / 2, brickH / 2));
            rect.setPosition(sf::Vector2f(x + brickW / 2, y + brickH / 2));
            if (health == 1) rect.setFillColor(sf::Color::Red);
            else if (health == 2) rect.setFillColor(sf::Color::Green);
            else rect.setFillColor(sf::Color::Blue);
            sfBricks.push_back(rect);
        }
    }

    // 初始化球和挡板图形
    sfBall.setRadius(12);
    sfBall.setFillColor(sf::Color::White);
    sfBall.setOrigin(sf::Vector2f(12, 12));
    sfPaddle.setSize(sf::Vector2f(160, 30));
    sfPaddle.setFillColor(sf::Color::White);
    sfPaddle.setOrigin(sf::Vector2f(80, 15));

    // 重置分数和胜利标志
    score = 0;
    gameWin = false;
}


// ---------- 游戏更新 ----------
void updateGame() {
    // 挡板控制
    float paddle_speed = 300.0f / PPM; // 米/秒
    b2Vec2 paddle_vel(0, 0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        paddle_vel.x = -paddle_speed;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        paddle_vel.x = paddle_speed;
    paddle->SetLinearVelocity(paddle_vel);

    // 物理步进
    world->Step(1.0f / 60.0f, 8, 3);

    // 挡板边界限制
    float halfWidth = 80.0f / PPM;               // 挡板半宽（米）
    float leftBound = 10.0f / PPM;               // 左墙内侧X
    float rightBound = (WINDOW_W - 10.0f) / PPM; // 右墙内侧X
    b2Vec2 paddlePos = paddle->GetPosition();
    if (paddlePos.x < leftBound + halfWidth) {
        paddlePos.x = leftBound + halfWidth;
        paddle->SetLinearVelocity(b2Vec2(0, 0)); // 停止移动
    }
    else if (paddlePos.x > rightBound - halfWidth) {
        paddlePos.x = rightBound - halfWidth;
        paddle->SetLinearVelocity(b2Vec2(0, 0));
    }
    paddle->SetTransform(paddlePos, 0);          // 更新位置

    // 处理待销毁的砖块（从监听器中收集）
    for (b2Body* brick : contactListener.toDestroy) {
        auto it = std::find(bricks.begin(), bricks.end(), brick);
        if (it != bricks.end()) {
            size_t idx = it - bricks.begin();
            world->DestroyBody(brick);
            bricks.erase(bricks.begin() + idx);
            brickHealth.erase(brickHealth.begin() + idx);
            sfBricks.erase(sfBricks.begin() + idx);
            score += 10;   // 每块砖加10分
        }
    }
    contactListener.toDestroy.clear();

    // 胜利条件
    if (bricks.empty()) {
        gameState = STATE_GAMEOVER;
        gameWin = true;
        return;
    }

    // 同步图形位置
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
// 显示帮助窗口，提供游戏控制说明和砖块颜色对应的生命值信息
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

// 显示主菜单窗口，提供开始游戏、帮助和退出选项
void startMenu(sf::RenderWindow& window) {
    // 居中窗口
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
        resetGame();   // 重置所有游戏数据（分数、球、砖块等）
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

    // 帮助窗口（仅在 showHelp 为 true 时显示）
    if (showHelp) {
        helpWindow(window, winSize);
    }
}

// 显示游戏结束窗口，显示胜利或失败信息以及最终分数，并提供返回主菜单的按钮
void gameOver(sf::RenderWindow& window) {
    sf::Vector2u winSize = window.getSize();
    ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
    ImGui::Begin("GameOver", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    // 显示胜利或失败信息
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 40);
    ImGui::SetWindowFontScale(1.3f);
    if (gameWin)
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "You Win!");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Game Over");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    // 显示最终分数
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30);
    ImGui::Text("Score: %d", score);
    ImGui::Spacing(); ImGui::Spacing();

    // 重新开始按钮
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button("Restart", ImVec2(120, 40))) {
        resetGame();
        gameState = STATE_PLAYING;
    }
    ImGui::Spacing();

    // 返回主菜单按钮
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 60);
    if (ImGui::Button("Main Menu", ImVec2(120, 40))) {
        gameState = STATE_MENU;
        resetGame();   // 重置游戏数据（可选）
    }
    ImGui::End();
}

// 显示分数的窗口，固定在左上角
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
    sf::RenderWindow window(sf::VideoMode({ 960, 540 }), "Breakout", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // 主菜单
        if (gameState == STATE_MENU) {
			startMenu(window);
        }

        // 游戏进行中
        if (gameState == STATE_PLAYING) {
            showScore();
            updateGame();
        }

        // 胜利/失败窗口
        if (gameState == STATE_GAMEOVER) {
			gameOver(window);
        }

        window.clear();
        renderGame(window);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}