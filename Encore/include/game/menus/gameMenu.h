#pragma once
//
// Created by marie on 02/05/2024.
//

#include "GLFW/glfw3.h"
#include "song/songlist.h"
#include "game/assets.h"
#include "game/player.h"
#include "game/menus/uiSystem.h"

enum Screens {
    MENU,
    SONG_SELECT,
    READY_UP,
    GAMEPLAY,
    RESULTS,
    SETTINGS,
    CALIBRATION
};

class Menu {
private:


    Menu() {}

    template<typename CharT>
    struct Separators : public std::numpunct<CharT>
    {
        virtual std::string do_grouping()
        const
        {
            return "\003";
        }
    };

    std::string scoreCommaFormatter(int value) {
        std::stringstream ss;
        ss.imbue(std::locale(std::cout.getloc(), new Separators <char>()));
        ss << std::fixed << value;
        return ss.str();
    }
    void renderPlayerResults(Player player, Song song, Assets assets);
    void renderStars(Player player, float xPos, float yPos, Assets assets, float scale, bool left);
public:

    void DrawTopOvershell(float TopOvershell);
    void DrawBottomOvershell();
    void DrawBottomBottomOvershell();

    static Menu& getInstance() {
        static Menu instance; // This is the single instance
        return instance;
    }

    Song ChosenSong;
    int ChosenSongInt;
    Screens currentScreen;
    bool songsLoaded{};
    bool stringChosen = false;
    Texture2D AlbumArtBackground;
    bool albumArtLoaded = false;
    void showResults(Player &player, Assets assets);
    void loadMenu(GLFWgamepadstatefun gamepadStateCallbackSetControls, Assets assets,UI& ui);
    inline void loadTitleScreen() {};

    void SwitchScreen(Screens screen);
    void DrawAlbumArtBackground(Texture2D song, Assets assets);
    bool songChosen = false;

    void DrawVersion(Assets assets);
};


