//
// Created by marie on 02/05/2024.
//


#include <functional>
#include <random>
#include "game/menus/gameMenu.h"
#include "raylib.h"
#include "raygui.h"
#include "song/songlist.h"
#include "game/lerp.h"
#include "game/settings.h"
#include "game/assets.h"
#include "game/menus/uiUnits.h"
#include "game/menus/uiSystem.h"
#include "raymath.h"

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH
#endif

#ifndef ENCORE_VERSION
#define ENCORE_VERSION
#endif

const float Width = (float)GetScreenWidth();
const float Height = (float)GetScreenHeight();

bool randomSongChosen = false;

std::string menuCommitHash = GIT_COMMIT_HASH;
std::string menuVersion = ENCORE_VERSION;
Settings& settings = Settings::getInstance();
SongList &songListMenu = SongList::getInstance();
Units u = Units::getInstance();
std::vector<std::string> songPartsList{ "Drums","Bass","Guitar","Vocals"};
std::vector<std::string> diffList{ "Easy","Medium","Hard","Expert" };
void Menu::DrawTopOvershell(float TopOvershell) {
    DrawRectangle(0,0,(int)GetScreenWidth(), u.hpct(TopOvershell),WHITE);
    DrawRectangle(0,0,(int)GetScreenWidth(), u.hpct(TopOvershell)-u.hinpct(0.005f),ColorBrightness(GetColor(0x181827FF),-0.25f));
}

void Menu::DrawBottomOvershell() {
    float BottomOvershell = GetScreenHeight() - u.hpct(0.15f);
    DrawRectangle(0,BottomOvershell,(float)(GetScreenWidth()), (float)GetScreenHeight(),WHITE);
    DrawRectangle(0,BottomOvershell+u.hinpct(0.005f),(float)(GetScreenWidth()), (float)GetScreenHeight(),ColorBrightness(GetColor(0x181827FF),-0.5f));
}

void Menu::DrawBottomBottomOvershell() {
    float BottomBottomOvershell = GetScreenHeight() - u.hpct(0.1f);
    DrawRectangle(0,BottomBottomOvershell,(float)(GetScreenWidth()), (float)GetScreenHeight(),WHITE);
    DrawRectangle(0,BottomBottomOvershell+u.hinpct(0.005f),(float)(GetScreenWidth()), (float)GetScreenHeight(),ColorBrightness(GetColor(0x181827FF),-0.5f));
}


// should be reduced to just PlayerSongStats (instead of Player) eventually
void Menu::renderPlayerResults(Player player, Song song, Assets assets) {

    float cardPos = u.LeftSide + (u.winpct(0.26f) * (float)player.playerNum);


    DrawRectangle(cardPos-6, u.hpct(0.2f), u.winpct(0.22f)+12, u.hpct(0.85f), WHITE);
    DrawRectangle(cardPos, u.hpct(0.2f), u.winpct(0.22f), u.hpct(0.85f), GetColor(0x181827FF));

    DrawRectangleGradientV(cardPos,u.hpct(0.2f), u.winpct(0.22f), u.hinpct(0.2f), ColorBrightness(player.accentColor, -0.5f), GetColor(0x181827FF));

    bool rendAsFC = player.FC && !player.quit;

    if (player.quit) {
        DrawRectangleGradientV(cardPos,u.hpct(0.2f)+ u.hinpct(0.2f), u.winpct(0.22f), u.hinpct(0.63f), GetColor(0x181827FF), ColorBrightness(RED, -0.5f));
    }
    if (rendAsFC) {
        DrawRectangleGradientV(cardPos,u.hpct(0.2f)+ u.hinpct(0.2f), u.winpct(0.22f), u.hinpct(0.63f), GetColor(0x181827FF),
                               ColorContrast(ColorBrightness(GOLD, -0.5f), -0.25f));
    }
    if (player.perfectHit==player.notes && rendAsFC) {
        DrawRectangleGradientV(cardPos,u.hpct(0.2f)+ u.hinpct(0.2f), u.winpct(0.22f), u.hinpct(0.63f), GetColor(0x181827FF),
                               ColorBrightness(WHITE, -0.5f));
    }

    DrawLine(cardPos,u.hpct(0.2f) + u.hinpct(0.2f), cardPos + u.winpct(0.22f),u.hpct(0.2f) + u.hinpct(0.2f),WHITE);
    DrawLine(cardPos,u.hpct(0.2f) + u.hinpct(0.4f), cardPos + u.winpct(0.22f),u.hpct(0.2f) + u.hinpct(0.4f),WHITE);

    float scorePos = (cardPos + u.winpct(0.11f)) - (MeasureTextEx(assets.redHatDisplayItalic, scoreCommaFormatter(player.score).c_str(), u.hinpct(0.07f), 0).x /2);
    float Percent = floorf(((float)player.notesHit/ (float)player.notes) * 100.0f);

    DrawTextEx(
            assets.redHatDisplayItalic,
            scoreCommaFormatter(player.score).c_str(),
            {
                    scorePos,
                    (float)GetScreenHeight()/2},
            u.hinpct(0.065f),
            0,
            GetColor(0x00adffFF));

    renderStars(player,  (cardPos + u.winpct(0.11f)), (float)GetScreenHeight()/2 - u.hinpct(0.06f), assets, u.hinpct(0.055f),false);


    if (rendAsFC) {
        DrawTextEx(assets.redHatDisplayItalicLarge, TextFormat("%3.0f%%", Percent), {(cardPos + u.winpct(0.113f)) - (MeasureTextEx(assets.redHatDisplayItalicLarge, TextFormat("%3.0f", Percent), u.hinpct(0.1f),0).x/1.5f),u.hpct(0.243f)},u.hinpct(0.1f),0,
                   ColorBrightness(GOLD,-0.5));
        float flawlessFontSize = 0.03f;
        DrawTextEx(
                assets.rubikBoldItalic,
                "Flawless!",
                {
                        (cardPos + u.winpct(0.113f))-(MeasureTextEx(assets.rubikBoldItalic, "Flawless!", u.hinpct(flawlessFontSize), 0.0f).x/2),
                        u.hpct(0.35f)},
                u.hinpct(flawlessFontSize),
                0.0f,
                WHITE);
    } if (player.quit) {
        float flawlessFontSize = 0.05f;
        DrawTextEx(
                assets.rubikBoldItalic,
                "Quit",
                {
                        (cardPos + u.winpct(0.11f))-(MeasureTextEx(assets.rubikBoldItalic, "Quit", u.hinpct(flawlessFontSize), 0.0f).x/2),
                        u.hpct(0.335f)},
                u.hinpct(flawlessFontSize),
                0.0f,
                RED);
    }
    DrawTextEx(assets.redHatDisplayItalicLarge, TextFormat("%3.0f%%", Percent), {(cardPos + u.winpct(0.11f)) - (MeasureTextEx(assets.redHatDisplayItalicLarge, rendAsFC ? TextFormat("%3.0f", Percent) : TextFormat("%3.0f%%", Percent), u.hinpct(0.1f),0).x/(rendAsFC ? 1.5f : 2.0f)),u.hpct(0.24f)},u.hinpct(0.1f),0, rendAsFC ? YELLOW : WHITE);

    float statsHeight = u.hpct(0.2f) + u.hinpct(0.415f);
    float statsLeft = cardPos + u.winpct(0.01f);
    float statsRight = cardPos + u.winpct(0.21f);

    DrawTextEx(assets.rubik, "Perfects:", {statsLeft, statsHeight}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, "Goods:", {statsLeft, statsHeight+u.hinpct(0.035f)}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, "Missed:", {statsLeft, statsHeight+u.hinpct(0.07f)}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, "Strikes:", {statsLeft, statsHeight+u.hinpct(0.105f)}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, "Max Streak:", {statsLeft, statsHeight+u.hinpct(0.14f)}, u.hinpct(0.03f),0,WHITE);

    DrawTextEx(assets.rubikBold, TextFormat("%s %s", diffList[player.diff].c_str(), songPartsList[player.instrument].c_str()), {cardPos + u.winpct(0.11f) -
                                                                                                                                          (MeasureTextEx(assets.rubikBold, TextFormat("%s %s", diffList[player.diff].c_str(), songPartsList[player.instrument].c_str()), u.hinpct(0.03f),0).x/2), statsHeight+u.hinpct(0.20f)}, u.hinpct(0.03f),0,WHITE);

    int MaxNotes = song.parts[player.instrument]->charts[player.diff].notes.size();
    DrawTextEx(assets.rubik, TextFormat("%01i/%01i", player.perfectHit, player.notes), {statsRight - MeasureTextEx(assets.rubik, TextFormat("%01i/%01i", player.perfectHit, player.notes), u.hinpct(0.03f), 0).x, statsHeight}, u.hinpct(0.03f), 0, WHITE);
    DrawTextEx(assets.rubik, TextFormat("%01i/%01i", player.notesHit-player.perfectHit, player.notes), {statsRight - MeasureTextEx(assets.rubik, TextFormat("%01i/%01i", player.notesHit-player.perfectHit, player.notes), u.hinpct(0.03f), 0).x, statsHeight+u.hinpct(0.035f)}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, TextFormat("%01i/%01i", player.notesMissed, player.notes), {statsRight - MeasureTextEx(assets.rubik, TextFormat("%01i/%01i", player.notesMissed, player.notes), u.hinpct(0.03f), 0).x, statsHeight+u.hinpct(0.07f)}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, TextFormat("%01i", player.playerOverhits, player.notes), {statsRight - MeasureTextEx(assets.rubik, TextFormat("%01i", player.playerOverhits), u.hinpct(0.03f), 0).x, statsHeight+u.hinpct(0.105f)}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, TextFormat("%01i/%01i", player.maxCombo, player.notes), {statsRight - MeasureTextEx(assets.rubik, TextFormat("%01i/%01i", player.maxCombo, player.notes), u.hinpct(0.03f), 0).x, statsHeight+u.hinpct(0.14f)}, u.hinpct(0.03f),0,WHITE);
    DrawTextEx(assets.rubik, TextFormat("%2.2f", player.totalOffset / player.notesHit), {statsRight - MeasureTextEx(assets.rubik, TextFormat("%2.2f", player.totalOffset / player.notesHit), u.hinpct(0.03f), 0).x, statsHeight+u.hinpct(0.17f)}, u.hinpct(0.03f),0,WHITE);
};

// todo: replace player with band stats
void Menu::renderStars(Player player, float xPos, float yPos, Assets assets, float scale, bool left) {
    int starsval = player.stars(player.songToBeJudged.parts[player.instrument]->charts[player.diff].baseScore,player.diff);
    float starPercent = (float)player.score/(float)player.songToBeJudged.parts[player.instrument]->charts[player.diff].baseScore;

    float starX = left ? 0 : scale*2.5f;
    for (int i = 0; i < 5; i++) {
        DrawTexturePro(assets.emptyStar, {0,0,(float)assets.emptyStar.width,(float)assets.emptyStar.height}, {(xPos+(i*scale)-starX),yPos, scale, scale},{0,0},0,WHITE);
    }
    for (int i = 0; i < starsval ; i++) {
        DrawTexturePro(player.goldStars?assets.goldStar:assets.star, {0,0,(float)assets.emptyStar.width,(float)assets.emptyStar.height}, {(xPos+(i*scale)-starX),yPos, scale, scale}, {0,0},0, WHITE);
    }
};
void Menu::DrawAlbumArtBackground(Texture2D song, Assets assets) {
    float diagonalLength = sqrtf((float)(GetScreenWidth() * GetScreenWidth()) + (float)(GetScreenHeight() * GetScreenHeight()));
    float RectXPos = GetScreenWidth() / 2;
    float RectYPos = diagonalLength / 2;

    BeginShaderMode(assets.bgShader);
    DrawTexturePro(song, Rectangle{0, 0, (float) song.width,
                                                 (float) song.width},
                   Rectangle{RectXPos, -RectYPos * 2,
                             diagonalLength * 2, diagonalLength * 2}, {0,0}, 45,
                   WHITE);
    EndShaderMode();
};

void Menu::DrawVersion(Assets assets) {
    DrawTextEx(assets.josefinSansItalic, TextFormat("%s-%s",menuVersion.c_str() , menuCommitHash.c_str()), {u.wpct(0.0025f), u.hpct(0.0025f)}, u.hinpct(0.025f), 0, WHITE);
};


// todo: text box rendering for splashes, cleanup of buttons
void Menu::loadMenu(GLFWgamepadstatefun gamepadStateCallbackSetControls, Assets assets, UI& ui) {

    std::filesystem::path directory = GetPrevDirectoryPath(GetApplicationDirectory());

    std::ifstream splashes;
    splashes.open((directory / "Assets/ui/splashes.txt"));

    static std::string result;
    std::string line;
    if (!stringChosen) {
        for (std::size_t n = 0; std::getline(splashes, line); n++) {
            int rng = GetRandomValue(0, n);
            if (rng < 1)
                result = line;
        }
        stringChosen = true;
    }
    if (std::filesystem::exists("songCache.encr") && songListMenu.songs.size()>0) {
        if (!albumArtLoaded) {
            AlbumArtBackground = assets.highwayTexture;

            if (!songChosen && songsLoaded) {
                if (!randomSongChosen) {
                    SetRandomSeed(std::chrono::system_clock::now().time_since_epoch().count() * GetTime());
                    int my = GetRandomValue(0, (int) songListMenu.songs.size() - 1);

                    ChosenSong = songListMenu.songs[my];
                    ChosenSong.LoadAlbumArt(ChosenSong.albumArtPath);
                    ChosenSongInt = my;
                    randomSongChosen = true;
                } else {
                    ChosenSong.LoadAlbumArt(ChosenSong.albumArtPath);
                }

                AlbumArtBackground = ChosenSong.albumArtBlur;
                TraceLog(LOG_INFO, ChosenSong.title.c_str());
                songChosen = true;
            } else {
                AlbumArtBackground = assets.highwayTexture;
            };
            albumArtLoaded = true;

        };
        DrawAlbumArtBackground(AlbumArtBackground, assets);
    }
    float SplashFontSize = u.hinpct(0.03f);
    float SplashHeight = MeasureTextEx(assets.josefinSansItalic, result.c_str(), SplashFontSize, 0).y;
    float SplashWidth = MeasureTextEx(assets.josefinSansItalic, result.c_str(), SplashFontSize, 0).x;
    Vector2 StringBox = {u.RightSide - SplashWidth - u.winpct(0.01f),  u.hpct(0.2f) - u.hinpct(0.1f) - (SplashHeight/2)};
    DrawTopOvershell(0.2f);
    DrawBottomOvershell();
    DrawBottomBottomOvershell();
    menuCommitHash.erase(7);
    float logoHeight = u.hinpct(0.145f);
    DrawVersion(assets);
    DrawTextEx(assets.josefinSansItalic, result.c_str(), StringBox, SplashFontSize, 0, WHITE);

    Rectangle LogoRect = { u.LeftSide + u.winpct(0.01f), u.hpct(0.04f), Remap(assets.encoreWhiteLogo.height, 0, assets.encoreWhiteLogo.width / 4.25, 0, u.winpct(0.5f)), logoHeight};
    DrawTexturePro(assets.encoreWhiteLogo, {0,0,(float)assets.encoreWhiteLogo.width,(float)assets.encoreWhiteLogo.height}, LogoRect, {0,0}, 0, WHITE);
    ui.Update();
    ui.Draw();
    if (std::filesystem::exists("songCache.encr")) {
        ui.GetButton("play")->text = "Play";
        if (ui.GetButton("play")->isClicked()) {

            for (Song &songi: songListMenu.songs) {
                songi.titleScrollTime = GetTime();
                songi.titleTextWidth = assets.MeasureTextRubik(songi.title.c_str(), 24);
                songi.artistScrollTime = GetTime();
                songi.artistTextWidth = assets.MeasureTextRubik(songi.artist.c_str(), 20);
            }
            Menu::SwitchScreen(SONG_SELECT);
        }
    }else{
        GuiSetStyle(BUTTON,BASE_COLOR_NORMAL, ColorToInt(Color{128,0,0,255}));
        ui.GetButton("play")->text = "Invalid cache!";
        songListMenu.ScanSongs(settings.songPaths); 
        songsLoaded = false;
        DrawRectanglePro({((float) GetScreenWidth() / 2) - 125, ((float) GetScreenHeight() / 2) - 120, 250, 60},{0,0},0, Color{0,0,0,64});
        GuiSetStyle(BUTTON,BASE_COLOR_NORMAL, 0x181827FF);
    }
        if (ui.GetButton("options")->isClicked()) {
            glfwSetGamepadStateCallback(gamepadStateCallbackSetControls);
            Menu::SwitchScreen(SETTINGS);
        }
        if (ui.GetButton("quit")->isClicked()) {
            exit(0);
        }
        if (ui.GetButton("github")->isClicked()) {
            OpenURL("https://github.com/Encore-Developers/Encore-Raylib");
        }
        if (ui.GetButton("discord")->isClicked()) {
            OpenURL("https://discord.gg/GhkgVUAC9v");
        }
        if (ui.GetButton("splash")->isClicked()) {
            stringChosen = false;
        }
        if (ui.GetButton("rescan")->isClicked()) {
            songsLoaded = false;
            songListMenu.ScanSongs(settings.songPaths);
        }
        DrawTextureEx(assets.github, {(float) GetScreenWidth() - 54, (float) GetScreenHeight() - 54 - u.hpct(0.15f) }, 0, 0.2, WHITE);
        DrawTextureEx(assets.discord, {(float) GetScreenWidth() - 113, (float) GetScreenHeight() - 48 - u.hpct(0.15f) }, 0, 0.075,
                      WHITE);
}

bool AlbumArtLoadingStuff = false;
void Menu::showResults(Player &player, Assets assets) {

    for (int i = 0; i < 4; i++) {
        renderPlayerResults(player, ChosenSong, assets);
    }

    DrawTopOvershell(0.2f);
    DrawBottomOvershell();
    DrawBottomBottomOvershell();

    DrawTextEx(assets.josefinSansItalic, TextFormat("%s-%s",menuVersion.c_str() , menuCommitHash.c_str()), {u.wpct(0), u.hpct(0)}, u.hinpct(0.025f), 0, WHITE);

    float songNamePos = (float)GetScreenWidth()/2 - MeasureTextEx(assets.redHatDisplayBlack,player.songToBeJudged.title.c_str(), u.hinpct(0.09f), 0).x/2;
    float bigScorePos = (float)GetScreenWidth()/2 - u.winpct(0.04f) - MeasureTextEx(assets.redHatDisplayItalicLarge,scoreCommaFormatter(player.score).c_str(), u.hinpct(0.08f), 0).x;
    float bigStarPos = (float)GetScreenWidth()/2 + u.winpct(0.005f);


    DrawTextEx(assets.redHatDisplayBlack, player.songToBeJudged.title.c_str(), {songNamePos,u.hpct(0.01f)},u.hinpct(0.09f),0,WHITE);
    DrawTextEx(assets.redHatDisplayItalicLarge, scoreCommaFormatter(player.score).c_str(), {bigScorePos,u.hpct(0.1f)},u.hinpct(0.08f),0, GetColor(0x00adffFF));
    renderStars(player, bigStarPos, u.hpct(0.1125f), assets, u.hinpct(0.055f),true);
    // assets.DrawTextRHDI(player.songToBeJudged.title.c_str(),songNamePos, 50, WHITE);
}

void Menu::SwitchScreen(Screens screen){
    currentScreen = screen;
    switch (screen) {
        case MENU:
            // reset lerps
            stringChosen = false;
            break;
        case SONG_SELECT:
            break;
        case READY_UP:
            break;
        case GAMEPLAY:
            break;
        case RESULTS:
            AlbumArtLoadingStuff = false;
            break;
        case SETTINGS:
            break;
        case CALIBRATION:
            break;
    }
}