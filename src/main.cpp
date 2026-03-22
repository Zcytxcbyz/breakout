#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
};

GameState gameState = STATE_MENU;
bool gameWin = false;   // true=胜利, false=失败
int score = 0;

void resetGame() {

}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Breakout", sf::Style::Titlebar | sf::Style::Close);
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

        // ========== 1. 主菜单 ==========
        if (gameState == STATE_MENU) {
            // 居中窗口
            sf::Vector2u winSize = window.getSize();
            ImGui::SetNextWindowPos(ImVec2(winSize.x / 2.0f, winSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(300, 180), ImGuiCond_Always);
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
            ImGui::End();
        }

        // ========== 2. 游戏进行中的分数窗口 ==========
        if (gameState == STATE_PLAYING) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(200, 60), ImGuiCond_Always);
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

        // ========== 3. 胜利/失败窗口 ==========
        if (gameState == STATE_GAMEOVER) {
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

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}