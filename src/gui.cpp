module;

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include <imgui.h>

#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <SDL3_image/SDL_image.h>

#include <format>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <format>
#include <iostream>

export module gui;

import sprite;
import tile;

using SdlSurfacePtr = std::unique_ptr<SDL_Surface, void (*)(SDL_Surface *)>;
using SdlWindowPtr = std::unique_ptr<SDL_Window, void (*)(SDL_Window *)>;

class InitError : public std::exception {
public:
  InitError(std::string_view msg) : msg(msg) {}
  const char *what() const noexcept override { return msg.c_str(); }

private:
  const std::string msg;
};

class TextureLoadingError : public std::exception {
public:
  TextureLoadingError(std::string_view msg) : msg(msg) {}
  const char *what() const noexcept override { return msg.c_str(); }

private:
  const std::string msg;
};

export class Gui final {
public:
  Gui();
  ~Gui();

  Gui(const Gui &) = delete;
  Gui(Gui &&) = delete;
  Gui &operator=(const Gui &) = delete;
  Gui &operator=(Gui &&) = delete;

  auto load_texture(std::string_view path) -> SdlTexturePtr;
  auto renderCharacterSelector();
  auto processEvent();
  auto loadCharacters() -> void;
  auto loadEnemies() -> void;
  auto loadTiles() -> void;
  auto showMap() -> void;

  auto frame() -> void;

  auto done() const noexcept -> bool { return _done; }

private:
  SdlWindowPtr window;
  bool _done;

  SDL_Renderer *renderer;

  size_t frameCount;
  SdlTexturePtr texture;
  Uint32 last;

  std::vector<CharacterSprite> characters;
  std::vector<CharacterSprite> enemies;
  std::vector<std::variant<Tile>> tiles;
  std::vector<std::vector<std::size_t>> map;
  std::vector<std::vector<std::size_t>> mapWall;

  size_t characterIndex;
  size_t enemyIndex;
  size_t tileIndex;
  bool checkBoxRuning;
  bool checkBoxWall;
};

Gui::Gui()
    : window{nullptr, SDL_DestroyWindow}, _done{false}, frameCount{0},
      texture{nullptr, SDL_DestroyTexture}, last{0}, characterIndex{0},
      enemyIndex{0}, tileIndex{0}, checkBoxRuning{false}, checkBoxWall{0} {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    throw InitError{std::format("SDL_Init(): {}", SDL_GetError())};

  static constexpr Uint32 window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;

  window = {SDL_CreateWindow("My app", 1280, 720, window_flags),
            SDL_DestroyWindow};
  if (!window)
    throw InitError{std::format("SDL_CreateWindow(): {}", SDL_GetError())};

  renderer = SDL_CreateRenderer(window.get(), nullptr);
  SDL_SetRenderVSync(renderer, 1);
  if (renderer == nullptr)
    throw InitError{std::format("SDL_CreateRenderer(): {}", SDL_GetError())};

  SDL_SetWindowPosition(window.get(), SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window.get());

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  texture = load_texture(
      "rsrc/0x72_DungeonTilesetII_v1.7/0x72_DungeonTilesetII_v1.7.png");
  loadCharacters();
  loadEnemies();
  loadTiles();

  map.resize(15);
  for (auto &e : map) {
    e.resize(10, std::numeric_limits<size_t>::max());
  }

  mapWall.resize(15);
  for (auto &e : mapWall) {
    e.resize(10, std::numeric_limits<size_t>::max());
  }
}

Gui::~Gui() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
}

auto Gui::loadCharacters() -> void {
  characters.emplace_back("knight f", 128, 68, 28, 16, true, true);
  characters.emplace_back("elve f", 128, 4, 28, 16, true, true);
  characters.emplace_back("wizard f", 128, 132, 28, 16, true, true);
  characters.emplace_back("dwarf f", 128, 260, 28, 16, true, true);
  characters.emplace_back("lizard f", 128, 196, 28, 16, true, true);
  characters.emplace_back("knight m", 128, 100, 28, 16, true, true);
  characters.emplace_back("elve m", 128, 4, 36, 16, true, true);
  characters.emplace_back("wizard m", 128, 164, 28, 16, true, true);
  characters.emplace_back("dwarf m", 128, 292, 28, 16, true, true);
  characters.emplace_back("lizard m", 128, 228, 28, 16, true, true);
}

auto Gui::loadEnemies() -> void {
  enemies.emplace_back("imp", 368, 64, 16, 16, true, false);
  enemies.emplace_back("wogol", 368, 249, 23, 16, true, false);
  enemies.emplace_back("skeleton", 368, 88, 16, 16, true, false);
  enemies.emplace_back("chort", 368, 273, 23, 16, true, false);
  enemies.emplace_back("doc", 368, 345, 23, 16, true, false);
}

auto Gui::loadTiles() -> void {
  tiles.emplace_back(Tile{"floor 1", 16, 64, 16, 16});
  tiles.emplace_back(Tile{"floor 2", 32, 64, 16, 16});
  tiles.emplace_back(Tile{"wall edge bottom left", 32, 168, 16, 16});
  tiles.emplace_back(Tile{"wall edge left", 32, 136, 16, 16});
  tiles.emplace_back(Tile{"wall edge mid left", 32, 152, 16, 16});
  tiles.emplace_back(Tile{"wall edge top left", 31, 120, 16, 16});
  tiles.emplace_back(Tile{"wall edge tshape bottom right", 64, 152, 16, 16});
}

auto Gui::processEvent() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_QUIT)
      _done = true;
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == SDL_GetWindowID(window.get()))
      _done = true;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
      size_t x = event.button.x / 32;
      size_t y = event.button.y / 32;
      if (x < map.size() && y < map[x].size()) {
        if (checkBoxWall)
          mapWall[x][y] = tileIndex;
        else
          map[x][y] = tileIndex;
      }
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_RIGHT) {
      size_t x = event.button.x / 32;
      size_t y = event.button.y / 32;
      if (x < map.size() && y < map[x].size()) {
        if (checkBoxWall)
          mapWall[x][y] = std::numeric_limits<size_t>::max();
        else
          map[x][y] = std::numeric_limits<size_t>::max();
      }
    }

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_A) {
      characters[characterIndex].setHit();
    } else if (event.type == SDL_EVENT_KEY_DOWN &&
               event.key.key == SDLK_RIGHT) {
      characters[characterIndex].setRunning(false);
    } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_LEFT) {
      characters[characterIndex].setRunning(true);
    } else if (event.type == SDL_EVENT_KEY_UP &&
               (event.key.key == SDLK_RIGHT || event.key.key == SDLK_LEFT)) {
      characters[characterIndex].setIdle();
    }
  }
}

auto Gui::renderCharacterSelector() {
  ImGui::Begin("Character Selector");
  if (ImGui::BeginCombo("Character Selector",
                        characters[characterIndex].name().c_str())) {
    for (auto i = 0; i < characters.size(); ++i) {
      if (ImGui::Selectable(characters[i].name().c_str(), characterIndex == i))
        characterIndex = i;

      if (characterIndex == i)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  if (ImGui::BeginCombo("Enemy Selector", enemies[enemyIndex].name().c_str())) {
    for (auto i = 0; i < enemies.size(); ++i) {
      if (ImGui::Selectable(enemies[i].name().c_str(), characterIndex == i))
        enemyIndex = i;

      if (enemyIndex == i)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (ImGui::Checkbox("running", &checkBoxRuning)) {
    if (checkBoxRuning)
      enemies[enemyIndex].setRunning(false);
    else
      enemies[enemyIndex].setIdle();
  }

  std::visit(
      [this](auto &tile) {
        if (ImGui::BeginCombo("Tile Selector", tile.name().c_str())) {
          for (auto i = 0; i < tiles.size(); ++i) {
            std::visit(
                [this, i](auto &tile) {
                  if (ImGui::Selectable(tile.name().c_str(), tileIndex == i))
                    tileIndex = i;
                },
                tiles[i]);

            if (tileIndex == i)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
      },
      tiles[tileIndex]);
    ImGui::Checkbox("wall", &checkBoxWall);

    ImGui::End();
}

auto Gui::frame() -> void {
  auto now = SDL_GetTicks();
  auto fps = now - last;
  if (fps >= 1000 / 30) {
    last = now;
    ++frameCount;
  } else {
    return;
  }

  processEvent();

  if (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  ImGui::Text("frame ms: %d", (int)fps);

  renderCharacterSelector();

  ImGui::Render();
  SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  int winHeight{0};
  SDL_GetWindowSize(window.get(), NULL, &winHeight);

  characters[characterIndex].render(renderer, texture, 100, 100, frameCount,
                                    winHeight);
  enemies[enemyIndex].render(renderer, texture, 300, 100, frameCount,
                             winHeight);

  showMap();

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
  SDL_RenderPresent(renderer);
}

auto Gui::load_texture(std::string_view path) -> SdlTexturePtr {

  auto *iostr = SDL_IOFromFile(path.data(), "r");
  if (iostr == NULL)
    throw TextureLoadingError{
        std::format("SDL_IOFromFile(): {}", SDL_GetError())};

  SdlSurfacePtr surfaceWizard{IMG_LoadPNG_IO(iostr), SDL_DestroySurface};
  if (!surfaceWizard)
    throw TextureLoadingError{
        std::format("IMG_LoadPNG_IO(): {}", SDL_GetError())};

  SdlTexturePtr texture = {
      SDL_CreateTextureFromSurface(renderer, surfaceWizard.get()),
      SDL_DestroyTexture};
  if (!texture)
    throw TextureLoadingError{
        std::format("SDL_CreateTextureFromSurface(): {}", SDL_GetError())};

  SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST);
  return texture;
}

auto Gui::showMap() -> void {
  for (auto i = 0; i < map.size(); ++i) {
    for (auto j = 0; j < map[i].size(); ++j) {
      if (map[i][j] >= tiles.size())
        continue;
      std::visit(
          [this, i, j](auto &tile) {
            tile.render(renderer, texture, i * 32, j * 32);
          },
          tiles[map[i][j]]);
    }
  }

  for (auto i = 0; i < mapWall.size(); ++i) {
    for (auto j = 0; j < mapWall[i].size(); ++j) {
      if (mapWall[i][j] >= tiles.size())
        continue;
      std::visit(
          [this, i, j](auto &tile) {
            tile.render(renderer, texture, i * 32, j * 32);
          },
          tiles[mapWall[i][j]]);
    }
  }
}
