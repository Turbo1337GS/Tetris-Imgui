#include <SFML/Audio.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#endif

/////////////////////////////////////
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <iostream>

#include <atomic>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>

struct soundFlags{
bool GamePlay = false;
bool roate = false;
}gm;


class SoundManager
{
private:
    struct SoundData
    {
        sf::SoundBuffer buffer;
        sf::Sound sound;
        std::thread soundThread;
        std::atomic<bool> isPlaying{false};

        SoundData() = default;
        SoundData(const SoundData&) = delete;
        SoundData& operator=(const SoundData&) = delete;
    };

    std::vector<std::unique_ptr<SoundData>> sounds;
    std::atomic<bool> audio{true};

public:
    SoundManager() = default;

    ~SoundManager()
    {
        // clean up threads
        for (auto& soundData : sounds)
        {
            if (soundData->soundThread.joinable())
            {
                soundData->soundThread.join();
            }
        }
    }

    void playSound(const std::string &filePath, float volume = 100.0f)
    {
        if (!audio.load()) // Check if audio is enabled before playing
            return;

        // Create a new sound data object
        auto soundData = std::make_unique<SoundData>();
        if (soundData->buffer.loadFromFile(filePath))
        {
            soundData->sound.setBuffer(soundData->buffer);
            soundData->sound.setVolume(volume);
            soundData->sound.play();
            soundData->isPlaying = true;

            // Create a new thread to monitor and control sound play
            soundData->soundThread = std::thread([this, soundDataPtr = soundData.get()]()
                                                  {
                while (soundDataPtr->isPlaying.load())
                {
                    if (!audio.load())
                    {
                        soundDataPtr->sound.pause(); // Pause if audio is disabled
                    }
                    else if (soundDataPtr->sound.getStatus() != sf::Sound::Playing)
                    {
                        soundDataPtr->sound.play(); // Replay if sound is supposed to play
                    }

                    sf::sleep(sf::milliseconds(100)); // Frequency of status checking
                }
            });

            soundData->soundThread.detach();
            sounds.push_back(std::move(soundData));
        }
    }

    void setAudio(bool val)
    {
        audio = val;
        if (!val)
        {
            for (auto& soundData : sounds)
            {
                soundData->sound.pause();
            }
        }
        else
        {
            for (auto& soundData : sounds)
            {
                if (soundData->isPlaying.load() && soundData->sound.getStatus() == sf::Sound::Paused)
                {
                    soundData->sound.play(); // Resume playing if audio is turned back on
                }
            }
        }
    }

    bool getAudio() const
    {
        return audio.load();
    }
};


SoundManager soundManager;

struct TetrisBlock
{
    int x, y;
    int id = 0;
    std::vector<std::vector<bool>> shape;
    ImVec4 color;
};

class TetrisGame
{
public:
    TetrisGame(int width = 10, int height = 20, float fallInterval = 1.0f)
        : gridWidth(width), gridHeight(height), fallInterval(fallInterval), currentTime(std::chrono::steady_clock::now())
    {
        grid.assign(gridHeight, std::vector<bool>(gridWidth, false));
        currentBlock = generateRandomBlock();
        lastDropTime = std::chrono::steady_clock::now(); // Zainicjalizuj czas ostatniego opadnięcia bloku
    }

    TetrisBlock generateRandomBlock()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 6);
        static std::uniform_int_distribution<> x_dis(0, gridWidth - 1);
        static std::uniform_real_distribution<float> color_dis(0.0f, 1.0f);
        static int idCounter = 1;

        std::vector<std::vector<std::vector<bool>>> shapes = {
            {{1, 1, 1, 1}},         // Line
            {{1, 1}, {1, 1}},       // Square
            {{0, 1, 0}, {1, 1, 1}}, // T
            {{0, 1, 1}, {1, 1, 0}}, // S
            {{1, 1, 0}, {0, 1, 1}}, // Z
            {{1, 0, 0}, {1, 1, 1}}, // L
            {{0, 0, 1}, {1, 1, 1}}  // J
        };

        TetrisBlock block;
        block.id = idCounter++;
        block.x = x_dis(gen);
        block.y = 0;
        if (block.x > gridWidth - 4)
            block.x -= 4;
        block.shape = shapes[dis(gen)];
        block.color = ImVec4(color_dis(gen), color_dis(gen), color_dis(gen), 1.0f);
        return block;
    }

    void processInput(int key)
    {
        switch (key)
        {
        case 'A': // Left
            std::cout << "Moving block to the left" << std::endl;
            moveBlock(-1);

            break;
        case 'D': // Right
            std::cout << "Moving block to the right" << std::endl;
            moveBlock(1);
            break;
        case 'W': // UP
          { 
       

            rotateBlock();}
            break;
        case 'S':
            if (!checkCollision(currentBlock.x, currentBlock.y + 1, currentBlock.shape))
            {
                std::cout << "Dropping block" << std::endl;
                currentBlock.y += 1;
            }
            break;
        }
    }

    void moveBlock(int dx)
    {

        int newX = currentBlock.x + dx;
        if (newX >= 0 && newX + static_cast<int>(currentBlock.shape[0].size()) <= gridWidth && !checkCollision(newX, currentBlock.y, currentBlock.shape))
        {
            currentBlock.x = newX;
        }
    }

    void dropBlock()
    {
        auto now = std::chrono::steady_clock::now();
        float elapsedSinceLastDrop = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDropTime).count() / 1000.0f;

        if (elapsedSinceLastDrop >= blockDropDelay)
        {
            if (!checkCollision(currentBlock.x, currentBlock.y + 1, currentBlock.shape))
            {
                currentBlock.y += 1;
                lastDropTime = now;
            }
            else
            {
                placeBlock();
                currentBlock = generateRandomBlock();
                if (checkCollision(currentBlock.x, currentBlock.y, currentBlock.shape))
                {
                    std::cout << "Game Over!" << std::endl;
                    resetGame();
                }
                lastDropTime = now;
            }
        }
    }
    void rotateBlock()
    {

        const size_t M = currentBlock.shape.size();
        const size_t N = currentBlock.shape[0].size();

        std::vector<std::vector<bool>> rotatedShape(N, std::vector<bool>(M));

        for (size_t i = 0; i < M; ++i)
        {
            for (size_t j = 0; j < N; ++j)
            {
                rotatedShape[j][M - i - 1] = currentBlock.shape[i][j];
            }
        }

        if (!checkCollision(currentBlock.x, currentBlock.y, rotatedShape))
        {
            currentBlock.shape = rotatedShape;
        }
    }

    bool checkCollision(int x, int y, const std::vector<std::vector<bool>> &shape)
    {
        for (int i = 0; i < static_cast<int>(shape.size()); ++i)
        {
            for (int j = 0; j < static_cast<int>(shape[i].size()); ++j)
            {
                if (shape[i][j])
                {
                    int checkX = x + j;
                    int checkY = y + i;
                    if (checkX < 0 || checkX >= gridWidth || checkY >= gridHeight || (checkY >= 0 && grid[checkY][checkX]))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void placeBlock()
    {
        for (int i = 0; i < static_cast<int>(currentBlock.shape.size()); ++i)

        {
            for (int j = 0; j < static_cast<int>(currentBlock.shape[i].size()); ++j)

            {
                if (currentBlock.shape[i][j])
                {
                    grid[currentBlock.y + i][currentBlock.x + j] = true;
                }
            }
        }
        checkForCompleteLines();
    }

    void checkForCompleteLines()
    {
        bool lineCleared = false;
        for (int row = gridHeight - 1; row >= 0; --row)
        {
            if (std::all_of(grid[row].begin(), grid[row].end(), [](bool val)
                            { return val; }))
            {
                removeLine(row);
                score += 100;
                lineCleared = true;
                row++;
            }
        }
        if (lineCleared)
        {
        }
    }

    void removeLine(int lineIndex)
    {
        for (int row = lineIndex; row > 0; --row)
        {
            grid[row] = grid[row - 1];
        }
        grid[0] = std::vector<bool>(gridWidth, false);
    }

    void render(int sizex, int sizey)
    {
        if(gm.GamePlay ==false)
        soundManager.playSound("./audio/gameplay.wav", 10);
        gm.GamePlay=true;
        ImGui::SetNextWindowSize(ImVec2((float)sizex, (float)sizey));
        ImGui::SetNextWindowBgAlpha(0.95f); // Semi-transparent background
        ImGui::SetNextWindowPos({0, 0});
        ImGui::Begin("Tetris Game", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("Score: %d", score);
        bool currentAudioState = soundManager.getAudio();
        if (ImGui::Checkbox("Sounds", &currentAudioState))
        {
            soundManager.setAudio(currentAudioState);
        }

        for (int y = 0; y < gridHeight; ++y)
        {
            for (int x = 0; x < gridWidth; ++x)
            {
                ImVec4 color = grid[y][x] ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
                if (currentBlock.y <= y && y < currentBlock.y + static_cast<int>(currentBlock.shape.size()) && currentBlock.x <= x && x < currentBlock.x + static_cast<int>(currentBlock.shape[0].size()) && currentBlock.shape[y - currentBlock.y][x - currentBlock.x])
                {
                    color = currentBlock.color;
                }
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                ImGui::PushID(y * gridWidth + x);
                ImGui::Button("##btn", ImVec2(20.0f, 20.0f)); // Invisible label for button
                ImGui::PopID();
                ImGui::PopStyleColor(1);
                if (x < gridWidth - 1)
                    ImGui::SameLine();
            }
        }

        ImGui::End();
    }

    void resetGame()
    {
        currentTime = std::chrono::steady_clock::now();
        grid.assign(gridHeight, std::vector<bool>(gridWidth, false));
        currentBlock = generateRandomBlock();
    }

private:
    int gridWidth, gridHeight;
    std::vector<std::vector<bool>> grid;
    float fallInterval;
    std::chrono::time_point<std::chrono::steady_clock> currentTime;
    TetrisBlock currentBlock;
    float blockDropDelay = 0.5f;
    int score = 0;
    std::chrono::time_point<std::chrono::steady_clock> lastDropTime;
};
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        TetrisGame *game = reinterpret_cast<TetrisGame *>(glfwGetWindowUserPointer(window));
        game->processInput(key);
    }
}
static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char **)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    TetrisGame game;

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int monitorWidth = mode->width;
    int monitorHeight = mode->height;

    // Calculate window size as 3/4 of the monitor resolution
    int windowWidth = monitorWidth * 3 / 5;
    int windowHeight = monitorHeight * 3 / 4;

    // Create a windowed mode window and its OpenGL context
    GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Tetris with imgui by Turbo1337", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    int windowPosX = (monitorWidth - windowWidth) / 2;
    int windowPosY = (monitorHeight - windowHeight) / 2;
    glfwSetWindowPos(window, windowPosX, windowPosY);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glfwSetKeyCallback(window, key_callback);

    glfwSetWindowUserPointer(window, &game);
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    [[maybe_unused]] ImFont *font = io.Fonts->AddFontFromFileTTF("./Poppins-Medium.ttf", 29.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = {0, 0, 0, 1};

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        game.dropBlock();
        int x, y;
        glfwGetWindowSize(window, &x, &y);
        game.render(x, y);
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
