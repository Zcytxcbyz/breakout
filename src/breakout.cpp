// Breakout - 基于 Box2D 和 SFML 的打砖块游戏

// ---------- 头文件 ----------
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
	const int WINDOW_W = 960;               // 窗口宽度（像素）
	const int WINDOW_H = 540;               // 窗口高度（像素）
	const int FRAME_LIMIT = 60;             // 帧率限制

    // 物理单位转换
	const float PPM = 30.0f;                // 1米 = 30像素

    // 球参数（像素单位）
	const float BALL_RADIUS = 12.0f;        // 球半径（像素）
	const float BALL_DENSITY = 1.0f;        // 密度（物理单位）
	const float BALL_RESTITUTION = 1.0f;    // 反弹系数（物理单位）
    const float BALL_INIT_SPEED = 200.0f;   // 像素/秒
    const float MAX_BALL_SPEED = 20.0f;     // 米/秒（物理单位）
	const float SPEED_FACTOR = 1.05f;       // 每次碰撞后速度增加的倍率

    // 挡板参数（像素单位）
	const float PADDLE_WIDTH = 160.0f;      // 挡板宽度（像素）
	const float PADDLE_HEIGHT = 30.0f;      // 挡板高度（像素）
    const float PADDLE_SPEED = 300.0f;      // 像素/秒
	const float PADDLE_DENSITY = 1.0f;      // 密度（物理单位）
	const float PADDLE_RESTITUTION = 1.0f;  // 反弹系数（物理单位）

    // 边界墙厚度（像素）
	const float WALL_THICKNESS = 10.0f;     // 边界墙厚度（像素）

    // 砖块参数（像素单位）
	const int BRICK_ROWS = 5;               // 砖块行数
	const int BRICK_COLS = 10;              // 砖块列数
	const float BRICK_START_X = 80.0f;      // 砖块起始X位置（像素）
	const float BRICK_START_Y = 60.0f;      // 砖块起始Y位置（像素）  
	const float BRICK_WIDTH = 70.0f;        // 砖块宽度（像素）
	const float BRICK_HEIGHT = 25.0f;       // 砖块高度（像素）
	const float BRICK_SPACING_X = 80.0f;    // 砖块水平间距（像素）
	const float BRICK_SPACING_Y = 35.0f;    // 砖块垂直间距（像素）
	const int BRICK_SPAWN_PROB = 70;        // 砖块生成概率（百分比）
	const int MIN_HEALTH = 1;               // 砖块最少生命值 
	const int MAX_HEALTH = 3;               // 砖块最多生命值
	const int SCORE_PER_BRICK = 10;         // 每个砖块的分数

    // 初始位置偏移（像素）
    const float BALL_Y_OFFSET = 50.0f;      // 球相对于屏幕中心的Y偏移
    const float PADDLE_Y_OFFSET = 50.0f;    // 挡板距离底部的距离
}

// ---------- UI 文本 ----------
namespace Texts {
	const char* GameName = "Breakout";                              // 游戏名称
	const char* StartGame = "Start Game";                           // 开始游戏按钮文本
	const char* Help = "Help";                                      // 帮助按钮文本
	const char* Exit = "Exit";                                      // 退出按钮文本
	const char* Controls = "Controls:";                             // 控制说明标题
	const char* BrickHealth = "Brick Health (Color -> Hits):";      // 砖块生命值说明
	const char* MoveRight = "Move Right: D / Right Arrow";          // 向右移动说明
	const char* MoveLeft = "Move Left: A / Left Arrow";             // 向左移动说明
	const char* RedHit = "Red   -> 1 hit";                          // 红色砖块说明
	const char* GreenHit = "Green -> 2 hits";                       // 绿色砖块说明
	const char* BlueHit = "Blue  -> 3 hits";                        // 蓝色砖块说明
	const char* DestroyAll = "Destroy all bricks to win!";          // 游戏目标说明
	const char* Close = "Close";                                    // 关闭按钮文本
	const char* ScoreFormat = "Score: %d";                          // 分数显示格式
	const char* ScoreWindow = "Score";                              // 分数窗口标题
	const char* YouWin = "You Win!";                                // 胜利文本
	const char* GameOver = "Game Over";                             // 失败文本
	const char* Restart = "Restart";                                // 重新开始按钮文本
	const char* MainMenu = "Main Menu";                             // 主菜单按钮文本
}

// ---------- 全局变量 ----------
// 游戏状态枚举
enum GameState {
	STATE_MENU,         // 主菜单
	STATE_PLAYING,      // 游戏进行中
	STATE_GAMEOVER      // 游戏结束
};

GameState gameState = STATE_MENU;           // 当前游戏状态
bool gameWin = false;                       // 是否赢得游戏
int score = 0;                              // 当前分数
bool showHelp = false;                      // 是否显示帮助窗口

b2World* world = nullptr;                   // Box2D物理世界指针
b2Body* ball = nullptr;                     // 球的物理身体指针
b2Body* paddle = nullptr;                   // 挡板的物理身体指针
std::vector<b2Body*> bricks;                // 砖块的物理身体指针列表
std::vector<int> brickHealth;               // 砖块的生命值列表
std::vector<sf::RectangleShape> sfBricks;   // 砖块的SFML图形列表

sf::CircleShape sfBall;                     // 球的SFML图形
sf::RectangleShape sfPaddle;                // 挡板的SFML图形

std::random_device rd;  // 随机数种子
std::mt19937 rng(rd()); // 随机数生成器

// ---------- 坐标转换函数 ----------
inline b2Vec2 toBox2D(const sf::Vector2f& pos) {
    return { pos.x / Config::PPM, (Config::WINDOW_H - pos.y) / Config::PPM };
}

// Box2D坐标转换为SFML坐标（注意Y轴翻转）
inline sf::Vector2f toSFML(const b2Vec2& pos) {
    return { pos.x * Config::PPM, Config::WINDOW_H - pos.y * Config::PPM };
}

// ---------- 碰撞监听器 ----------
class ContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override {
        b2Body* bodyA = contact->GetFixtureA()->GetBody();
        b2Body* bodyB = contact->GetFixtureB()->GetBody();

		// 检测是否是球与其他物体的碰撞
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
			// 如果碰撞对象是挡板或砖块，增加球的速度
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

			// 如果碰撞对象是砖块，减少砖块的生命值并检查是否需要销毁
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

			// 如果碰撞对象是地面，游戏结束
            if ((bodyA == ball && bodyB == ground) || (bodyB == ball && bodyA == ground)) {
                gameState = STATE_GAMEOVER;
                gameWin = false;
            }
        }
    }

	// 碰撞结束时不需要处理
    std::vector<b2Body*> toDestroy;
    b2Body* ground = nullptr;
};

ContactListener contactListener;

// ---------- 重置游戏 ----------
void resetGame() {
	// 销毁
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

	// 创建新的物理世界
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

	// 根据配置生成砖块，随机决定每个位置是否生成以及生成的生命值
    for (int r = 0; r < Config::BRICK_ROWS; ++r) {
        for (int c = 0; c < Config::BRICK_COLS; ++c) {
			// 根据生成概率决定是否生成砖块
            if (spawnDist(rng) >= Config::BRICK_SPAWN_PROB) continue;

			// 随机生成砖块生命值    
            int health = healthDist(rng);
            float x = Config::BRICK_START_X + c * Config::BRICK_SPACING_X;
            float y = Config::BRICK_START_Y + r * Config::BRICK_SPACING_Y;
            float centerX = x + Config::BRICK_WIDTH / 2;
            float centerY = y + Config::BRICK_HEIGHT / 2;

			// 创建砖块的物理身体
            b2BodyDef brickDef;
            brickDef.type = b2_staticBody;
            brickDef.position = toBox2D(sf::Vector2f(centerX, centerY));
            b2Body* brick = world->CreateBody(&brickDef);
            b2PolygonShape brickShape;
            brickShape.SetAsBox((Config::BRICK_WIDTH / 2) / Config::PPM, (Config::BRICK_HEIGHT / 2) / Config::PPM);
            brick->CreateFixture(&brickShape, 0);

			// 将砖块信息保存到全局列表中
            bricks.push_back(brick);
            brickHealth.push_back(health);

			// 根据生命值设置砖块颜色并创建SFML图形
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

	// 重置分数和游戏状态
    score = 0;
    gameWin = false;
}

// ---------- 游戏更新 ----------
void updateGame() {
	// 根据键盘输入控制挡板移动
    float paddle_speed = Config::PADDLE_SPEED / Config::PPM;
    b2Vec2 paddle_vel(0, 0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        paddle_vel.x = -paddle_speed;
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        paddle_vel.x = paddle_speed;
    paddle->SetLinearVelocity(paddle_vel);

	// 物理步进
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

	// 销毁生命值为0的砖块并更新分数
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

	// 检查是否所有砖块都被销毁，如果是则游戏胜利
    if (bricks.empty()) {
        gameState = STATE_GAMEOVER;
        gameWin = true;
        return;
    }

	// 更新球和挡板的SFML图形位置
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
	// 在屏幕中心显示帮助窗口，包含游戏控制说明和目标说明
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

// --------- 主菜单和游戏结束窗口 ----------
void startMenu(sf::RenderWindow& window) {
	// 在屏幕中心显示主菜单窗口，包含游戏标题和选项按钮
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

// 游戏结束窗口
void gameOver(sf::RenderWindow& window) {
	// 在屏幕中心显示游戏结束或胜利的窗口，显示分数和选项按钮
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

// ---------- 显示分数窗口 ----------
void showScore() {
	// 在屏幕左上角显示当前分数，半透明背景，禁止交互
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

// ---------- 主函数 ----------
int main() {
	// 创建SFML窗口
    sf::RenderWindow window(sf::VideoMode({ Config::WINDOW_W, Config::WINDOW_H }), Texts::GameName, sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(Config::FRAME_LIMIT);
    ImGui::SFML::Init(window);

	// 禁止ImGui自动保存窗口位置和大小
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

	// 主循环
    sf::Clock deltaClock;
    while (window.isOpen()) {
		// 处理事件
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>())
                window.close();
        }

		// 更新ImGui状态
        ImGui::SFML::Update(window, deltaClock.restart());

		// 根据游戏状态显示不同的UI
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

		// 渲染游戏和UI
        window.clear();
        renderGame(window);
        ImGui::SFML::Render(window);
        window.display();
    }

	// 清理资源
    ImGui::SFML::Shutdown();
    if (world) delete world;
    return 0;
}