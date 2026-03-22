#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <vector>
#include <random>

// ----------全局变量定义----------

// 游戏状态枚举
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
};

std::random_device rd; // 随机数生成器
std::mt19937 rng(rd()); // 随机数引擎

GameState gameState = STATE_MENU;
bool gameWin = false;   // true=胜利, false=失败
int score = 0;
bool showHelp = false;   // 控制帮助窗口显示

// 坐标转换常量 (像素/米)
const float PPM = 30.0f;

// SFML 图形对象（全局，用于渲染）
sf::CircleShape sfBall;
sf::RectangleShape sfPaddle;
std::vector<sf::RectangleShape> sfBricks;

// 存储砖块血量（与 bricks 和 sfBricks 同步）
std::vector<int> brickHealth;

// ------------------------------


void resetGame() {
    score = 0;
    gameWin = false;
}

void updateGame(sf::RenderWindow& window) {

}

// 渲染游戏对象（球、挡板、砖块）
void renderGame(sf::RenderWindow& window) {
    window.draw(sfBall);
    window.draw(sfPaddle);
    for (auto& brick : sfBricks) {
        window.draw(brick);
    }
}

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
    ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_Always);
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
            updateGame(window);
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