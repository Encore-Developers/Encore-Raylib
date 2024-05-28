#define RAYGUI_IMPLEMENTATION

#if defined(WIN32) && defined(NDEBUG)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#include "raylib.h"
#include <vector>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include "song/song.h"
#include "song/songlist.h"
#include "game/arguments.h"
#include "game/utility.h"
#include "game/player.h"
#include "game/lerp.h"
#include "game/keybinds.h"
#include "game/settings.h"
#include "raygui.h"
#include <random>
#include "GLFW/glfw3.h"
#include "game/menus/gameMenu.h"
#include "game/assets.h"
#include "raymath.h"
#include "game/menus/uiUnits.h"
#include "game/audio.h"
#include "game/menus/uiSystem.h"
#include "game/menus/settingsOptionRenderer.h"
#include "game/timingvalues.h"
#include <thread>
#include <atomic>

Menu menu = Menu::getInstance();
Player player = Player::getInstance();
Settings& settingsMain = Settings::getInstance();
AudioManager audioManager;


vector<std::string> ArgumentList::arguments;

static bool compareNotes(const Note& a, const Note& b) {
	return a.time < b.time;
}

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH
#endif

#ifndef ENCORE_VERSION
#define ENCORE_VERSION
#endif


bool midiLoaded = false;
bool isPlaying = false;
bool streamsLoaded = false;
bool albumArtSelectedAndLoaded = false;

bool ShowHighwaySettings = true;
bool ShowCalibrationSettings = true;
bool ShowGeneralSettings = true;

bool ReadyUpMenu = false;
bool diffSelected = false;
bool diffSelection = false;
bool instSelection = false;
bool instSelected = false;
bool songEnded = false;

int curPlayingSong = 0;
std::vector<int> curNoteIdx = { 0,0,0,0,0 };
int curODPhrase = 0;
int curBeatLine = 0;
int curBPM = 0;
int selectedSongInt = 0;
int selLane = 0;
bool selSong = false;
int songSelectOffset = 0;
bool changingKey = false;
bool changing4k = false;
bool changingOverdrive = false;
bool changingAlt = false;
bool changingPause = false;
double startedPlayingSong = 0.0;
Vector2 viewScroll = { 0,0 };
Rectangle view = { 0 };

bool isCalibrating = false;
double calibrationStartTime = 0.0;
double lastClickTime = 0.0;
std::vector<double> tapTimes;
const int clickInterval = 1;

bool showInputFeedback = false;
double inputFeedbackStartTime = 0.0;
const double inputFeedbackDuration = 0.6;
float inputFeedbackAlpha = 1.0f;

std::string trackSpeedButton;

std::string encoreVersion = ENCORE_VERSION;
std::string commitHash = GIT_COMMIT_HASH;
std::vector<bool> heldFrets{ false,false,false,false,false };
std::vector<bool> heldFretsAlt{ false,false,false,false,false };
std::vector<bool> overhitFrets{ false,false,false,false,false };
std::vector<bool> tapRegistered{ false,false,false,false,false };
std::vector<bool> liftRegistered{ false,false,false,false,false };
bool overdriveHeld = false;
bool overdriveAltHeld = false;
bool overdriveHitAvailable = false;
bool overdriveLiftAvailable = false;
std::vector<bool> overdriveLanesHit{ false,false,false,false,false };
double overdriveHitTime = 0.0;
std::vector<int> lastHitLifts{-1, -1, -1, -1, -1};



SongList &songList = SongList::getInstance();
Assets assets;
double lastAxesTime = 0.0;
std::vector<float> axesValues{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
std::vector<int> buttonValues{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
std::vector<float> axesValues2{ 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f };
int pressedGamepadInput = -999;
int axisDirection = -1;
int controllerID = -1;

int currentSortValue = 0;
std::vector <std::string> sortTypes{ "Title","Artist","Length"};
static void DrawTextRubik(const char* text, float posX, float posY, float fontSize, Color color) {
    DrawTextEx(assets.rubik, text, { posX,posY }, fontSize, 0, color);
}
static void DrawTextRHDI(const char* text, float posX, float posY, float fontSize, Color color) {
    DrawTextEx(assets.redHatDisplayItalic, text, { posX,posY }, fontSize, 0, color);
}
static float MeasureTextRubik(const char* text, float fontSize) {
    return MeasureTextEx(assets.rubik, text, fontSize, 0).x;
}
static float MeasureTextRHDI(const char* text, float fontSize) {
    return MeasureTextEx(assets.redHatDisplayItalic, text, fontSize, 0).x;
}

template<typename CharT>
struct Separators : public std::numpunct<CharT>
{
    [[nodiscard]] std::string do_grouping()
    const override
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

static void handleInputs(int lane, int action){
    if (player.paused) return;
	if (lane == -2) return;
	if (settingsMain.mirrorMode && lane!=-1) {
		lane = (player.diff == 3 ? 4 : 3) - lane;
	}
	if (!streamsLoaded) {
		return;
	}
	Chart& curChart = songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff];
	float eventTime = audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle);
    if (player.instrument < 4){
	if (action == GLFW_PRESS && (lane == -1) && player.overdriveFill > 0 && !player.overdrive) {
        player.overdriveActiveTime = eventTime;
        player.overdriveActiveFill = player.overdriveFill;
        player.overdrive = true;
		overdriveHitAvailable = true;
        overdriveHitTime = eventTime;
	}

	if (lane == -1) {
		if ((action == GLFW_PRESS && !overdriveHitAvailable) || (action == GLFW_RELEASE && !overdriveLiftAvailable)) return;
		Note* curNote = &curChart.notes[0];
		for (auto & note : curChart.notes) {
			if (note.isGood(eventTime, player.InputOffset ) &&
				!note.hit) {
				curNote = &note;
				break;
			}
		}
		if (action == GLFW_PRESS && !overdriveHeld) {
			overdriveHeld = true;
		}
		else if (action == GLFW_RELEASE && overdriveHeld) {
			overdriveHeld = false;
		}
		if (action == GLFW_PRESS && overdriveHitAvailable) {
			if (curNote->isGood(eventTime, player.InputOffset) &&
				!curNote->hit) {
				for (int newlane = 0; newlane < 5; newlane++) {
					int chordLane = curChart.findNoteIdx(curNote->time, newlane);
					if (chordLane != -1) {
						Note& chordNote = curChart.notes[chordLane];
						if (!chordNote.accounted) {
							chordNote.hit = true;
							overdriveLanesHit[newlane] = true;
							chordNote.hitTime = eventTime;

							if ((chordNote.len) > 0 && !chordNote.lift) {
								chordNote.held = true;
							}
                            if (chordNote.isPerfect(eventTime, player.InputOffset)) {
								chordNote.perfect = true;

							}
							if (chordNote.perfect) player.lastNotePerfect = true;
							else player.lastNotePerfect = false;
                            chordNote.HitOffset = chordNote.time - eventTime;
                            player.HitNote(chordNote.perfect, player.instrument);
							chordNote.accounted = true;
						}
					}
				}
				overdriveHitAvailable = false;
				overdriveLiftAvailable = true;
			}
		}
		else if (action == GLFW_RELEASE && overdriveLiftAvailable) {
			if (curNote->isGood(eventTime, player.InputOffset) &&
				!curNote->hit) {
				for (int newlane = 0; newlane < 5; newlane++) {
					if (overdriveLanesHit[newlane]) {
						int chordLane = curChart.findNoteIdx(curNote->time, newlane);
						if (chordLane != -1) {
							Note& chordNote = curChart.notes[chordLane];
							if (chordNote.lift) {
								chordNote.hit = true;
                                chordNote.HitOffset = chordNote.time - eventTime;
								overdriveLanesHit[newlane] = false;
								chordNote.hitTime = eventTime;

								if (chordNote.isPerfect(eventTime, player.InputOffset)) {
									chordNote.perfect = true;

								}
                                chordNote.accounted = true;
								if (chordNote.perfect) player.lastNotePerfect = true;
								else player.lastNotePerfect = false;
							}

						}
					}
				}
				overdriveLiftAvailable = false;
			}
		}
		if (action == GLFW_RELEASE && curNote->held && (curNote->len) > 0 && overdriveLiftAvailable) {
			for (int newlane = 0; newlane < 5; newlane++) {
				if (overdriveLanesHit[newlane]) {
					int chordLane = curChart.findNoteIdx(curNote->time, newlane);
					if (chordLane != -1) {
						Note& chordNote = curChart.notes[chordLane];
						if (chordNote.held && chordNote.len > 0) {
							if (!((player.diff == 3 && settingsMain.keybinds5K[chordNote.lane]) || (player.diff != 3 && settingsMain.keybinds4K[chordNote.lane]))) {
								chordNote.held = false;
                                player.score += player.sustainScoreBuffer[chordNote.lane];
                                player.sustainScoreBuffer[chordNote.lane] = 0;
                                player.mute = true;
							}
						}
					}
				}
			}
		}
	}
	else {
		for (int i = curNoteIdx[lane]; i < curChart.notes_perlane[lane].size(); i++) {
			Note& curNote = curChart.notes[curChart.notes_perlane[lane][i]];

			if (lane != curNote.lane) continue;
			if ((curNote.lift && action == GLFW_RELEASE) || action == GLFW_PRESS) {
				if (curNote.isGood(eventTime, player.InputOffset) &&
					!curNote.hit) {
					if (curNote.lift && action == GLFW_RELEASE) {
						lastHitLifts[lane] = curChart.notes_perlane[lane][i];
					}
					curNote.hit = true;
                    curNote.HitOffset = curNote.time - eventTime;
					curNote.hitTime = eventTime;
					if ((curNote.len) > 0 && !curNote.lift) {
						curNote.held = true;
					}
					if (curNote.isPerfect(eventTime, player.InputOffset)) {
						curNote.perfect = true;
					}
					if (curNote.perfect) player.lastNotePerfect = true;
					else player.lastNotePerfect = false;
                    player.HitNote(curNote.perfect, player.instrument);
					curNote.accounted = true;
					break;
				}
				if (curNote.miss) player.lastNotePerfect = false;
			}
			if ((!heldFrets[curNote.lane] && !heldFretsAlt[curNote.lane]) && curNote.held && (curNote.len) > 0) {
				curNote.held = false;
                player.score += player.sustainScoreBuffer[curNote.lane];
                player.sustainScoreBuffer[curNote.lane] = 0;
                player.mute = true;
				// SetAudioStreamVolume(audioManager.loadedStreams[instrument].stream, missVolume);
			}

			if (action == GLFW_PRESS &&
				eventTime > songList.songs[curPlayingSong].music_start &&
				!curNote.hit &&
				!curNote.accounted &&
				((curNote.time) - goodBackend) + player.InputOffset > eventTime &&
				eventTime > overdriveHitTime + 0.05
				&& !overhitFrets[lane]) {
				if (lastHitLifts[lane] != -1) {
					if (eventTime > curChart.notes[lastHitLifts[lane]].time - 0.1 && eventTime < curChart.notes[lastHitLifts[lane]].time + 0.1)
						continue;
				}
                player.OverHit();
				if (!curChart.odPhrases.empty() && eventTime >= curChart.odPhrases[curODPhrase].start && eventTime < curChart.odPhrases[curODPhrase].end && !curChart.odPhrases[curODPhrase].missed) curChart.odPhrases[curODPhrase].missed = true;
				overhitFrets[lane] = true;
			}
		}
	}
	}
}

// what to check when a key changes states (what was the change? was it pressed? or released? what time? what window? were any modifiers pressed?)
static void keyCallback(GLFWwindow* wind, int key, int scancode, int action, int mods) {
	if (!streamsLoaded) {
		return;
	}
	if (action < 2) {  // if the key action is NOT repeat (release is 0, press is 1)
		int lane = -2;
        if (key == settingsMain.keybindPause && action==GLFW_PRESS) {
            player.paused = !player.paused;
            if(player.paused)
                audioManager.pauseStreams();
            else {
                audioManager.playStreams();
                for (int i = 0; i < (player.diff == 3 ? 5 : 4); i++) {
                    handleInputs(i, -1);
                }
            }

        }
        else if (key == settingsMain.keybindOverdrive || key == settingsMain.keybindOverdriveAlt) {
			handleInputs(-1, action);
		}
		else {
			if (player.diff == 3) {
				for (int i = 0; i < 5; i++) {
					if (key == settingsMain.keybinds5K[i] && !heldFretsAlt[i]) {
						if (action == GLFW_PRESS) {
							heldFrets[i] = true;
						}
						else if (action == GLFW_RELEASE) {
							heldFrets[i] = false;
							overhitFrets[i] = false;
						}
						lane = i;
					}
					else if (key == settingsMain.keybinds5KAlt[i] && !heldFrets[i]) {
						if (action == GLFW_PRESS) {
							heldFretsAlt[i] = true;
						}
						else if (action == GLFW_RELEASE) {
							heldFretsAlt[i] = false;
							overhitFrets[i] = false;
						}
						lane = i;
					}
				}
			}
			else {
				for (int i = 0; i < 4; i++) {
					if (key == settingsMain.keybinds4K[i] && !heldFretsAlt[i]) {
						if (action == GLFW_PRESS) {
							heldFrets[i] = true;
						}
						else if (action == GLFW_RELEASE) {
							heldFrets[i] = false;
							overhitFrets[i] = false;
						}
						lane = i;
					}
					else if (key == settingsMain.keybinds4KAlt[i] && !heldFrets[i]) {
						if (action == GLFW_PRESS) {
							heldFretsAlt[i] = true;
						}
						else if (action == GLFW_RELEASE) {
							heldFretsAlt[i] = false;
							overhitFrets[i] = false;
						}
						lane = i;
					}
				}
			}
			if (lane > -1) {
				handleInputs(lane, action);
			}
			
		}
	}
}

static void gamepadStateCallback(int jid, GLFWgamepadstate state) {
	if (!streamsLoaded) {
		return;
	}
	double eventTime = audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle);
    if (settingsMain.controllerPause >= 0) {
        if (state.buttons[settingsMain.controllerPause] != buttonValues[settingsMain.controllerPause]) {
            buttonValues[settingsMain.controllerPause] = state.buttons[settingsMain.controllerPause];
            if (state.buttons[settingsMain.controllerPause] == 1) {
                player.paused = !player.paused;
                if (player.paused)
                    audioManager.pauseStreams();
                else {
                    audioManager.playStreams();
                    for (int i = 0; i < (player.diff == 3 ? 5 : 4); i++) {
                        handleInputs(i, -1);
                    }
                }
            }
        }
    }
    else {
        if (state.axes[-(settingsMain.controllerPause + 1)] != axesValues[-(settingsMain.controllerPause + 1)]) {
            axesValues[-(settingsMain.controllerPause + 1)] = state.axes[-(settingsMain.controllerPause + 1)];
            if (state.axes[-(settingsMain.controllerPause + 1)] == 1.0f * (float)settingsMain.controllerPauseAxisDirection) {
                
                    
            }
        }
    }
	if (settingsMain.controllerOverdrive >= 0) {
		if (state.buttons[settingsMain.controllerOverdrive] != buttonValues[settingsMain.controllerOverdrive]) {
			buttonValues[settingsMain.controllerOverdrive] = state.buttons[settingsMain.controllerOverdrive];
			handleInputs(-1, state.buttons[settingsMain.controllerOverdrive]);
		}
	}
	else {
		if (state.axes[-(settingsMain.controllerOverdrive+1)] != axesValues[-(settingsMain.controllerOverdrive + 1)]) {
			axesValues[-(settingsMain.controllerOverdrive + 1)] = state.axes[-(settingsMain.controllerOverdrive + 1)];
			if (state.axes[-(settingsMain.controllerOverdrive + 1)] == 1.0f*(float)settingsMain.controllerOverdriveAxisDirection) {
				handleInputs(-1, GLFW_PRESS);
			}
			else {
				handleInputs(-1, GLFW_RELEASE);
			}
		}
	}
	if (player.diff == 3) {
		for (int i = 0; i < 5; i++) {
			if (settingsMain.controller5K[i] >= 0) {
				if (state.buttons[settingsMain.controller5K[i]] != buttonValues[settingsMain.controller5K[i]]) {
					if (state.buttons[settingsMain.controller5K[i]] == 1)
						heldFrets[i] = true;
					else {
						heldFrets[i] = false;
						overhitFrets[i] = false;
					}
						
					handleInputs(i, state.buttons[settingsMain.controller5K[i]]);
					buttonValues[settingsMain.controller5K[i]] = state.buttons[settingsMain.controller5K[i]];
				}
			}
			else {
				if (state.axes[-(settingsMain.controller5K[i] + 1)] != axesValues[-(settingsMain.controller5K[i]+1)]) {
					if (state.axes[-(settingsMain.controller5K[i] + 1)] == 1.0f * (float)settingsMain.controller5KAxisDirection[i]) {
						heldFrets[i] = true;
						handleInputs(i, GLFW_PRESS);
					}
					else {
						heldFrets[i] = false;
						overhitFrets[i] = false;
						handleInputs(i, GLFW_RELEASE);
					}
					axesValues[-(settingsMain.controller5K[i] + 1)] = state.axes[-(settingsMain.controller5K[i] + 1)];
				}
			}
		}
	}
	else {
		for (int i = 0; i < 4; i++) {
			if (settingsMain.controller4K[i] >= 0) {
				if (state.buttons[settingsMain.controller4K[i]] != buttonValues[settingsMain.controller4K[i]]) {
					if (state.buttons[settingsMain.controller4K[i]] == 1)
						heldFrets[i] = true;
					else {
						heldFrets[i] = false;
						overhitFrets[i] = false;
					}
					handleInputs(i, state.buttons[settingsMain.controller4K[i]]);
					buttonValues[settingsMain.controller4K[i]] = state.buttons[settingsMain.controller4K[i]];
				}
			}
			else {
				if (state.axes[-(settingsMain.controller4K[i] + 1)] != axesValues[-(settingsMain.controller4K[i] + 1)]) {
					if (state.axes[-(settingsMain.controller4K[i] + 1)] == 1.0f * (float)settingsMain.controller4KAxisDirection[i]) {
						heldFrets[i] = true;
						handleInputs(i, GLFW_PRESS);
					}
					else {
						heldFrets[i] = false;
						overhitFrets[i] = false;
						handleInputs(i, GLFW_RELEASE);
					}
					axesValues[-(settingsMain.controller4K[i] + 1)] = state.axes[-(settingsMain.controller4K[i] + 1)];
				}
			}
		}
	}
}

static void gamepadStateCallbackSetControls(int jid, GLFWgamepadstate state) {
	for (int i = 0; i < 6; i++) {
		axesValues2[i] = state.axes[i];
	}
	if (changingKey || changingOverdrive || changingPause) {
		for (int i = 0; i < 15; i++) {
			if (state.buttons[i] == 1) {
				if (buttonValues[i] == 0) {
					controllerID = jid;
					pressedGamepadInput = i;
					return;
				}
				else {
					buttonValues[i] = state.buttons[i];
				}
			}
		}
		for (int i = 0; i < 6; i++) {
			if (state.axes[i] == 1.0f || (i <= 3 && state.axes[i] == -1.0f)) {
				axesValues[i] = 0.0f;
				if (state.axes[i] == 1.0f) axisDirection = 1;
				else axisDirection = -1;
				controllerID = jid;
				pressedGamepadInput = -(1 + i);
				return;
			}
			else {
				axesValues[i] = 0.0f;
			}
		}
	}
	else {
		for (int i = 0; i < 15; i++) {
			buttonValues[i] = state.buttons[i];
		}
		for (int i = 0; i < 6; i++) {
			axesValues[i] = state.axes[i];
		}
		pressedGamepadInput = -999;
	}
}

int minWidth = 640;
int minHeight = 480;


Keybinds keybinds;

Song song;

bool firstInit = true;
int loadedAssets;
bool albumArtLoaded = false;

settingsOptionRenderer sor;

Song selectedSong;
int main(int argc, char* argv[])
{
    Units u = Units::getInstance();
    commitHash.erase(7);
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetConfigFlags(FLAG_VSYNC_HINT);
	
	//SetTraceLogLevel(LOG_NONE);

	// 800 , 600
	InitWindow(1, 1, "Encore");
    SetWindowState(FLAG_MSAA_4X_HINT);


    bool windowToggle = true;
    ArgumentList::InitArguments(argc, argv);

    std::string FPSCapStringVal = ArgumentList::GetArgValue("fpscap");
    int targetFPSArg = 0;



    if (!FPSCapStringVal.empty())
    {
#ifdef NDEBUG
        str2int(&targetFPSArg, FPSCapStringVal.c_str());
#else
        assert(str2int(&targetFPSArg, FPSCapStringVal.c_str()) == STR2INT_SUCCESS);
#endif
        TraceLog(LOG_INFO, "Argument overridden target FPS: %d", targetFPSArg);
    }


    //https://www.raylib.com/examples/core/loader.html?name=core_custom_frame_control

    double previousTime = GetTime();
    double currentTime = 0.0;
    double updateDrawTime = 0.0;
    double waitTime = 0.0;
    float deltaTime = 0.0f;

    float timeCounter = 0.0f;

    std::filesystem::path executablePath(GetApplicationDirectory());

    std::filesystem::path directory = executablePath.parent_path();

    if (std::filesystem::exists(directory / "keybinds.json")) {
        settingsMain.migrateSettings(directory / "keybinds.json", directory / "settings.json");
    }
    settingsMain.loadSettings(directory / "settings.json");
    player.InputOffset = settingsMain.inputOffsetMS / 1000.0f;
    player.VideoOffset = settingsMain.avOffsetMS / 1000.0f;
    int targetFPS = targetFPSArg == 0 ? GetMonitorRefreshRate(GetCurrentMonitor()) : targetFPSArg;
    if (!settingsMain.fullscreen) {
        if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowState(FLAG_MSAA_4X_HINT);
        }
        SetWindowSize(GetMonitorWidth(GetCurrentMonitor()) * 0.75f, GetMonitorHeight(GetCurrentMonitor()) * 0.75f);
        SetWindowPosition((GetMonitorWidth(GetCurrentMonitor()) * 0.5f) - (GetMonitorWidth(GetCurrentMonitor()) * 0.375f),
                          (GetMonitorHeight(GetCurrentMonitor()) * 0.5f) -
                          (GetMonitorHeight(GetCurrentMonitor()) * 0.375f));
    } else {
        SetWindowState(FLAG_WINDOW_UNDECORATED + FLAG_MSAA_4X_HINT);
        int CurrentMonitor = GetCurrentMonitor();
        SetWindowPosition(0, 0);
        SetWindowSize(GetMonitorWidth(CurrentMonitor), GetMonitorHeight(CurrentMonitor));
    }
    std::vector<std::string> songPartsList{ "Drums","Bass","Guitar","Vocals"};
    std::vector<std::string> diffList{ "Easy","Medium","Hard","Expert" };
    TraceLog(LOG_INFO, "Target FPS: %d", targetFPS);

    audioManager.Init();
    SetExitKey(0);

    Camera3D camera = { 0 };

    // Y UP!!!! REMEMBER!!!!!!
    //							  x,    y,     z
    // 0.0f, 5.0f, -3.5f
    //								 6.5f
    camera.position = Vector3{ 0.0f, 7.0f, -10.0f };
    // 0.0f, 0.0f, 6.5f
    camera.target = Vector3{ 0.0f, 0.0f, 13.0f };

    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 34.5f;
    camera.projection = CAMERA_PERSPECTIVE;






    trackSpeedButton = "Track Speed " + truncateFloatString(settingsMain.trackSpeedOptions[settingsMain.trackSpeed]) + "x";



    ChangeDirectory(GetApplicationDirectory());

    GLFWkeyfun origKeyCallback = glfwSetKeyCallback(glfwGetCurrentContext(), keyCallback);
    GLFWgamepadstatefun origGamepadCallback = glfwSetGamepadStateCallback(gamepadStateCallback);
    glfwSetKeyCallback(glfwGetCurrentContext(), origKeyCallback);
    glfwSetGamepadStateCallback(origGamepadCallback);
    // GuiLoadStyle((directory / "Assets/ui/encore.rgs").string().c_str());

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, ColorToInt(ColorBrightness(player.accentColor, -0.5)));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, ColorToInt(ColorBrightness(player.accentColor, -0.3)));
    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL, 0xFFFFFFFF);
    GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED, 0xFFFFFFFF);
    GuiSetStyle(BUTTON, BORDER_COLOR_PRESSED, 0xFFFFFFFF);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0x505050ff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xcbcbcbFF);
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED, 0xFFFFFFFF);
    GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED, 0xFFFFFFFF);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 28);


    GuiSetStyle(TOGGLE, BASE_COLOR_NORMAL, 0x181827FF);
    GuiSetStyle(TOGGLE, BASE_COLOR_FOCUSED, ColorToInt(ColorBrightness(player.accentColor, -0.5)));
    GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED, ColorToInt(ColorBrightness(player.accentColor, -0.3)));
    GuiSetStyle(TOGGLE, BORDER_COLOR_NORMAL, 0xFFFFFFFF);
    GuiSetStyle(TOGGLE, BORDER_COLOR_FOCUSED, 0xFFFFFFFF);
    GuiSetStyle(TOGGLE, BORDER_COLOR_PRESSED, 0xFFFFFFFF);
    GuiSetStyle(TOGGLE, TEXT_COLOR_FOCUSED, 0xFFFFFFFF);
    GuiSetStyle(TOGGLE, TEXT_COLOR_PRESSED, 0xFFFFFFFF);


    Mesh sustainPlane = GenMeshPlane(0.8f,1.0f,1,1);

    assets.FirstAssets();
    SetWindowIcon(assets.icon);
    GuiSetFont(assets.rubik);
    assets.LoadAssets();
    RenderTexture2D notes_tex  = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    UI menuUI;
    menuUI.AddRow();
    menuUI.AddElement(0, new Button("play", { ((float)GetScreenWidth() / 2) - 100, ((float)GetScreenHeight() / 2) - 120, 200, 60 }, "Play"));
    menuUI.AddRow();
    menuUI.AddElement(1, new Button("options", { ((float)GetScreenWidth() / 2) - 100, ((float)GetScreenHeight() / 2) - 30, 200, 60 }, "Options"));
    menuUI.AddRow();
    menuUI.AddElement(2, new Button("quit", { ((float)GetScreenWidth() / 2) - 100, ((float)GetScreenHeight() / 2) + 60, 200, 60 }, "Quit"));

    while (!WindowShouldClose())
    {

        u.calcUnits();
        GuiSetStyle(DEFAULT, TEXT_SIZE, (int)u.hinpct(0.03f));
        GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
        double curTime = GetTime();
        float bgTime = curTime / 5.0f;

        if (GetScreenWidth() < minWidth) {
            if (GetScreenHeight() < minHeight)
                SetWindowSize(minWidth, minHeight);
            else
                SetWindowSize(minWidth, GetScreenHeight());
        }
        if (GetScreenHeight() < minHeight) {
            if (GetScreenWidth() < minWidth)
                SetWindowSize(minWidth, minHeight);
            else
                SetWindowSize(GetScreenWidth(), minHeight);
        }

        float diffDistance = player.diff == 3 ? 2.0f : 1.5f;
        float lineDistance = player.diff == 3 ? 1.5f : 1.0f;
        BeginDrawing();

        // menu.loadTitleScreen();

        ClearBackground(DARKGRAY);

        
        SetShaderValue(assets.bgShader, assets.bgTimeLoc, &bgTime, SHADER_UNIFORM_FLOAT);
        

        switch (menu.currentScreen) {
            case MENU: {
                if (!menu.songsLoaded) {
                    if (std::filesystem::exists("songCache.encr")) {
                        songList = songList.LoadCache(settingsMain.songPaths);
                        menu.songsLoaded = true;
                    }
                }

                menu.loadMenu(gamepadStateCallbackSetControls, assets,menuUI);
                break;
            }
            case CALIBRATION: {

                static bool sampleLoaded = false;
                if (!sampleLoaded) {
                    audioManager.loadSample("Assets/kick.wav", "click");
                    sampleLoaded = true;
                }

                if (GuiButton({ (float)GetScreenWidth() / 2 - 250, (float)GetScreenHeight() - 120, 200, 60 }, "Start Calibration")) {
                    isCalibrating = true;
                    calibrationStartTime = GetTime();
                    lastClickTime = calibrationStartTime;
                    tapTimes.clear();
                }
                if (GuiButton({ (float)GetScreenWidth() / 2 + 50, (float)GetScreenHeight() - 120, 200, 60 }, "Stop Calibration")) {
                    isCalibrating = false;
                    
                    if (tapTimes.size() > 1) {
                        double totalDifference = 0.0;
                        for (double tapTime : tapTimes) {
                            double expectedClickTime = round((tapTime - calibrationStartTime) / clickInterval) * clickInterval + calibrationStartTime;
                            totalDifference += (tapTime - expectedClickTime);
                        }
                        settingsMain.avOffsetMS = static_cast<int>((totalDifference / tapTimes.size()) * 1000);  // Convert to milliseconds
                        settingsMain.inputOffsetMS = settingsMain.avOffsetMS;
                        std::cout << static_cast<int>((totalDifference / tapTimes.size()) * 1000) << "ms of latency detected" << std::endl;
                    }
                    std::cout << "Stopped Calibration" << std::endl;
                    tapTimes.clear();
                }
                
                if (isCalibrating) {
                    double currentTime = GetTime();
                    double elapsedTime = currentTime - lastClickTime;

                    if (elapsedTime >= clickInterval) {
                        audioManager.playSample("click");
                        lastClickTime += clickInterval;  // Increment by the interval to avoid missing clicks
                        std::cout << "Click" << std::endl;
                    }

                    if (IsKeyPressed(settingsMain.keybindOverdrive)) {
                        tapTimes.push_back(currentTime);
                        std::cout << "Input Registered" << std::endl;

                        showInputFeedback = true;
                        inputFeedbackStartTime = currentTime;
                        inputFeedbackAlpha = 1.0f;
                    }
                }

                if (showInputFeedback) {
                    double currentTime = GetTime();
                    double timeSinceInput = currentTime - inputFeedbackStartTime;
                    if (timeSinceInput > inputFeedbackDuration) {
                        showInputFeedback = false;
                    } else {
                        inputFeedbackAlpha = 1.0f - (timeSinceInput / inputFeedbackDuration);
                    }
                }

                if (showInputFeedback) {
                    Color feedbackColor = {0, 255, 0, static_cast<unsigned char>(inputFeedbackAlpha * 255)};
                    DrawTextEx(assets.rubikBold, "Input Registered", {static_cast<float>((GetScreenWidth() - u.hinpct(0.35f)) / 2), static_cast<float>(GetScreenHeight() / 2)}, u.hinpct(0.05f), 0, feedbackColor);
                }

                if (GuiButton({ ((float)GetScreenWidth() / 2) - 350,((float)GetScreenHeight() - 60),100,60 }, "Cancel")) {
                    isCalibrating = false;
                    settingsMain.avOffsetMS = settingsMain.prevAvOffsetMS;
                    settingsMain.inputOffsetMS = settingsMain.prevInputOffsetMS;
                    tapTimes.clear();

                    settingsMain.saveSettings(directory / "settings.json");
                    menu.SwitchScreen(SETTINGS);
                }

                if (GuiButton({ ((float)GetScreenWidth() / 2) + 250,((float)GetScreenHeight() - 60),100,60 }, "Apply")) {
                    isCalibrating = false;
                    settingsMain.prevAvOffsetMS = settingsMain.avOffsetMS;
                    settingsMain.prevInputOffsetMS = settingsMain.inputOffsetMS;
                    tapTimes.clear();

                    settingsMain.saveSettings(directory / "settings.json");
                    menu.SwitchScreen(SETTINGS);
                }
            
                break;
            }
            case SETTINGS: {
                if (menu.songsLoaded)
                    menu.DrawAlbumArtBackground(menu.ChosenSong.albumArtBlur, assets);
                if (settingsMain.controllerType == -1 && controllerID != -1) {
                    std::string gamepadName = std::string(glfwGetGamepadName(controllerID));
                    settingsMain.controllerType = keybinds.getControllerType(gamepadName);
                }
                float TextPlacementTB = u.hpct(0.15f) - u.hinpct(0.11f);
                float TextPlacementLR = u.wpct(0.01f);
                DrawRectangle(u.LeftSide, 0, u.winpct(1.0f), GetScreenHeight(), Color{0,0,0,128});
                DrawLineEx({u.LeftSide + u.winpct(0.0025f),0},{u.LeftSide + u.winpct(0.0025f),(float)GetScreenHeight()}, u.winpct(0.005f), WHITE);
                DrawLineEx({u.RightSide - u.winpct(0.0025f),0},{u.RightSide - u.winpct(0.0025f),(float)GetScreenHeight()}, u.winpct(0.005f), WHITE);

                menu.DrawTopOvershell(0.15f);
                menu.DrawVersion(assets);
                menu.DrawBottomOvershell();
                menu.DrawBottomBottomOvershell();
                DrawTextEx(assets.redHatDisplayBlack, "Options", {TextPlacementLR, TextPlacementTB}, u.hinpct(0.10f),0, WHITE);

                float OvershellBottom = u.hpct(0.15f);
                if (GuiButton({ ((float)GetScreenWidth() / 2) - 350,((float)GetScreenHeight() - 60),100,60 }, "Cancel") && !(changingKey || changingOverdrive || changingPause)) {
                    glfwSetGamepadStateCallback(origGamepadCallback);
                    settingsMain.keybinds4K = settingsMain.prev4K;
                    settingsMain.keybinds5K = settingsMain.prev5K;
                    settingsMain.keybinds4KAlt = settingsMain.prev4KAlt;
                    settingsMain.keybinds5KAlt = settingsMain.prev5KAlt;
                    settingsMain.keybindOverdrive = settingsMain.prevOverdrive;
                    settingsMain.keybindOverdriveAlt = settingsMain.prevOverdriveAlt;
                    settingsMain.keybindPause = settingsMain.prevKeybindPause;

                    settingsMain.controller4K = settingsMain.prevController4K;
                    settingsMain.controller4KAxisDirection = settingsMain.prevController4KAxisDirection;
                    settingsMain.controller5K = settingsMain.prevController5K;
                    settingsMain.controller5KAxisDirection = settingsMain.prevController5KAxisDirection;
                    settingsMain.controllerOverdrive = settingsMain.prevControllerOverdrive;
                    settingsMain.controllerOverdriveAxisDirection = settingsMain.prevControllerOverdriveAxisDirection;
                    settingsMain.controllerType = settingsMain.prevControllerType;
                    settingsMain.controllerPause = settingsMain.prevControllerPause;

                    settingsMain.highwayLengthMult = settingsMain.prevHighwayLengthMult;
                    settingsMain.trackSpeed = settingsMain.prevTrackSpeed;
                    settingsMain.inputOffsetMS = settingsMain.prevInputOffsetMS;
                    settingsMain.avOffsetMS = settingsMain.prevAvOffsetMS;
                    settingsMain.missHighwayColor = settingsMain.prevMissHighwayColor;
                    settingsMain.mirrorMode = settingsMain.prevMirrorMode;
                    settingsMain.fullscreen = settingsMain.fullscreenPrev;

                    menu.SwitchScreen(MENU);
                }
                if (GuiButton({ ((float)GetScreenWidth() / 2) + 250,((float)GetScreenHeight() - 60),100,60 }, "Apply") && !(changingKey || changingOverdrive || changingPause)) {
                    glfwSetGamepadStateCallback(origGamepadCallback);
                    if (settingsMain.fullscreen) {
                        SetWindowState(FLAG_WINDOW_UNDECORATED);
                        SetWindowState(FLAG_MSAA_4X_HINT);
                        int CurrentMonitor = GetCurrentMonitor();
                        SetWindowPosition(0,0);
                        SetWindowSize(GetMonitorWidth(CurrentMonitor), GetMonitorHeight(CurrentMonitor));
                    } else {
                        if (IsWindowState(FLAG_WINDOW_UNDECORATED)){
                            ClearWindowState(FLAG_WINDOW_UNDECORATED);
                            SetWindowState(FLAG_MSAA_4X_HINT);
                        }
                        if (settingsMain.fullscreen != settingsMain.fullscreenPrev) {
                            SetWindowSize(GetMonitorWidth(GetCurrentMonitor()) * 0.75,
                                          GetMonitorHeight(GetCurrentMonitor()) * 0.75);
                            SetWindowPosition((GetMonitorWidth(GetCurrentMonitor()) * 0.5) -
                                              (GetMonitorWidth(GetCurrentMonitor()) * 0.375),
                                              (GetMonitorHeight(GetCurrentMonitor()) * 0.5) -
                                              (GetMonitorHeight(GetCurrentMonitor()) * 0.375));
                        }
                    }
                    settingsMain.prev4K = settingsMain.keybinds4K;
                    settingsMain.prev5K = settingsMain.keybinds5K;
                    settingsMain.prev4KAlt = settingsMain.keybinds4KAlt;
                    settingsMain.prev5KAlt = settingsMain.keybinds5KAlt;
                    settingsMain.prevOverdrive = settingsMain.keybindOverdrive;
                    settingsMain.prevOverdriveAlt = settingsMain.keybindOverdriveAlt;
                    settingsMain.prevKeybindPause = settingsMain.keybindPause;

                    settingsMain.prevController4K = settingsMain.controller4K;
                    settingsMain.prevController4KAxisDirection = settingsMain.controller4KAxisDirection;
                    settingsMain.prevController5K = settingsMain.controller5K;
                    settingsMain.prevController5KAxisDirection = settingsMain.controller5KAxisDirection;
                    settingsMain.prevControllerOverdrive = settingsMain.controllerOverdrive;
                    settingsMain.prevControllerPause = settingsMain.controllerPause;
                    settingsMain.prevControllerOverdriveAxisDirection = settingsMain.controllerOverdriveAxisDirection;
                    settingsMain.prevControllerType = settingsMain.controllerType;

                    settingsMain.prevHighwayLengthMult = settingsMain.highwayLengthMult;
                    settingsMain.prevTrackSpeed = settingsMain.trackSpeed;
                    settingsMain.prevInputOffsetMS = settingsMain.inputOffsetMS;
                    settingsMain.prevAvOffsetMS = settingsMain.avOffsetMS;
                    settingsMain.prevMissHighwayColor = settingsMain.missHighwayColor;
                    settingsMain.prevMirrorMode = settingsMain.mirrorMode;
                    settingsMain.fullscreenPrev = settingsMain.fullscreen;

                    player.InputOffset = settingsMain.inputOffsetMS / 1000.0f;
                    player.VideoOffset = settingsMain.avOffsetMS / 1000.0f;

                    settingsMain.saveSettings(directory / "settings.json");



                    menu.SwitchScreen(MENU);
                }
                static int selectedTab = 0;
                static int displayedTab = 0;

                GuiToggleGroup({ u.LeftSide + u.winpct(0.005f),OvershellBottom,(u.winpct(0.989f) / 3 ),u.hinpct(0.05) }, "Main;Keyboard Controls;Gamepad Controls", &selectedTab);
                if (!changingKey && !changingOverdrive && !changingPause) {
                    displayedTab = selectedTab;
                }
                else {
                    selectedTab = displayedTab;
                }
                float EntryFontSize = u.hinpct(0.03f);
                float EntryHeight = u.hinpct(0.05f);
                float EntryTop = OvershellBottom + u.hinpct(0.1f);
                float HeaderTextLeft = u.LeftSide + u.winpct(0.015f);
                float EntryTextLeft = u.LeftSide + u.winpct(0.025f);
                float EntryTextTop = EntryTop + u.hinpct(0.01f);
                float OptionLeft = u.LeftSide+u.winpct(0.005f)+u.winpct(0.989f) / 3;
                float OptionWidth = u.winpct(0.989f) / 3;
                float OptionRight = OptionLeft + OptionWidth;

                float underTabsHeight = OvershellBottom + u.hinpct(0.05f);

                GuiSetStyle(SLIDER, BASE_COLOR_NORMAL, 0x181827FF);
                GuiSetStyle(SLIDER, BASE_COLOR_PRESSED, ColorToInt(ColorBrightness(player.accentColor, -0.25f)));
                GuiSetStyle(SLIDER, TEXT_COLOR_FOCUSED, ColorToInt(ColorBrightness(player.accentColor, -0.5f)));
                GuiSetStyle(SLIDER, BORDER, 0xFFFFFFFF);
                GuiSetStyle(SLIDER, BORDER_COLOR_FOCUSED, 0xFFFFFFFF);
                GuiSetStyle(SLIDER, BORDER_WIDTH, 2);

                if (displayedTab == 0) { // Main settings tab


                    float trackSpeedFloat = settingsMain.trackSpeed;



                    // header 1
                    DrawRectangle(u.wpct(0.005f),OvershellBottom + u.hinpct(0.05f),OptionWidth*2, EntryHeight, Color{0,0,0,128});
                    DrawTextEx(assets.rubikBoldItalic, "Highway", {HeaderTextLeft, OvershellBottom + u.hinpct(0.055f)}, u.hinpct(0.04f), 0, WHITE);
                    if (GuiButton({OptionLeft-EntryHeight,underTabsHeight,EntryHeight,EntryHeight}, ShowHighwaySettings ? "v" : ">")) {
                        ShowHighwaySettings = !ShowHighwaySettings;
                    }

                    if (ShowHighwaySettings) {
                        settingsMain.trackSpeed = sor.sliderEntry(settingsMain.trackSpeed, 0,
                                                                  settingsMain.trackSpeedOptions.size() - 1, 1,
                                                                  "Track Speed", 1.0f, assets);
                        // highway length

                        DrawRectangle(u.wpct(0.005f), underTabsHeight + (EntryHeight * 2), OptionWidth * 2, EntryHeight,
                                      Color{0, 0, 0, 64});
                        settingsMain.highwayLengthMult = sor.sliderEntry(settingsMain.highwayLengthMult, 0.25f, 2.5f, 2,
                                                                         "Highway Length Multiplier", 0.25f, assets);

                        // miss color
                        settingsMain.missHighwayDefault = sor.toggleEntry(settingsMain.missHighwayDefault, 3,
                                                                          "Highway Miss Color", assets);

                        // lefty flip
                        DrawRectangle(u.wpct(0.005f), underTabsHeight + (EntryHeight * 4), OptionWidth * 2, EntryHeight,
                                      Color{0, 0, 0, 64});
                        settingsMain.mirrorMode = sor.toggleEntry(settingsMain.mirrorMode, 4, "Mirror/Lefty Mode",
                                                                  assets);
                    }
                    // calibration header
                    int calibrationMenuOffset = ShowHighwaySettings ? 5 : 1;

                    DrawRectangle(u.wpct(0.005f),underTabsHeight+(EntryHeight*calibrationMenuOffset),OptionWidth*2, EntryHeight, Color{0,0,0,128});
                    DrawTextEx(assets.rubikBoldItalic, "Calibration", {HeaderTextLeft, OvershellBottom + u.hinpct(0.055f) + (EntryHeight * calibrationMenuOffset)}, u.hinpct(0.04f), 0, WHITE);
                    if (GuiButton({OptionLeft-EntryHeight,underTabsHeight+(EntryHeight*calibrationMenuOffset),EntryHeight,EntryHeight}, ShowCalibrationSettings ? "v" : ">")) {
                        ShowCalibrationSettings = !ShowCalibrationSettings;
                    }

                    if (ShowCalibrationSettings) {
                        // av offset

                        settingsMain.avOffsetMS = sor.sliderEntry(settingsMain.avOffsetMS, -500.0f, 500.0f,
                                                                  calibrationMenuOffset + 1, "Audio/Visual Offset", 1,
                                                                  assets);

                        // input offset
                        DrawRectangle(u.wpct(0.005f), underTabsHeight + (EntryHeight * (calibrationMenuOffset + 2)),
                                      OptionWidth * 2, EntryHeight, Color{0, 0, 0, 64});
                        settingsMain.inputOffsetMS = sor.sliderEntry(settingsMain.inputOffsetMS, -500.0f, 500.0f,
                                                                     calibrationMenuOffset + 2, "Input Offset", 1,
                                                                     assets);

                        float calibrationTop = EntryTop + (EntryHeight * (calibrationMenuOffset + 2));
                        float calibrationTextTop = EntryTextTop + (EntryHeight * (calibrationMenuOffset + 2));
                        DrawTextEx(assets.rubikBold, "Automatic Calibration", {EntryTextLeft, calibrationTextTop},
                                   EntryFontSize, 0, WHITE);
                        if (GuiButton({OptionLeft, calibrationTop, OptionWidth, EntryHeight}, "Start Calibration")) {
                            menu.SwitchScreen(CALIBRATION);
                        }
                    }

                    int generalOffset = ShowHighwaySettings ? (ShowCalibrationSettings ? 9 : 6 ): (ShowCalibrationSettings ? 5 : 2);
                    // general header
                    DrawRectangle(u.wpct(0.005f),underTabsHeight+(EntryHeight*generalOffset),OptionWidth*2, EntryHeight, Color{0,0,0,128});
                    DrawTextEx(assets.rubikBoldItalic, "General", {HeaderTextLeft, OvershellBottom + u.hinpct(0.055f) + (EntryHeight * generalOffset)}, u.hinpct(0.04f), 0, WHITE);
                    if (GuiButton({OptionLeft-EntryHeight,underTabsHeight+(EntryHeight*generalOffset),EntryHeight,EntryHeight}, ShowGeneralSettings ? "v" : ">")) {
                        ShowGeneralSettings = !ShowGeneralSettings;
                    }

                    // fullscreen
                    if (ShowGeneralSettings) {
                        settingsMain.fullscreen = sor.toggleEntry(settingsMain.fullscreen, generalOffset + 1,
                                                                  "Fullscreen", assets);
                    }

                }
                else if (displayedTab == 1) { //Keyboard bindings tab
                    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
                    for (int i = 0; i < 5; i++) {
                        sor.keybindEntryText(i+1, "Lane " + to_string(i+1), assets);
                        sor.keybindEntry(settingsMain.keybinds5K[i], i+1, "Lane " + to_string(i+1), assets, keybinds, i);
                        sor.keybindAltEntry(settingsMain.keybinds5KAlt[i], i+1, "Lane " + to_string(i+1), assets, keybinds, i);
                    }
                    for (int i = 0; i < 4; i++) {
                        sor.keybindEntryText(i+7, "Lane " + to_string(i+1), assets);
                        sor.keybindEntry(settingsMain.keybinds4K[i], i+7, "Lane " + to_string(i+1), assets, keybinds, i);
                        sor.keybindAltEntry(settingsMain.keybinds4KAlt[i], i+7, "Lane " + to_string(i+1), assets, keybinds, i);
                    }
                    if (GuiButton({ ((float)GetScreenWidth() / 2) - 130,480,120,60 }, keybinds.getKeyStr(settingsMain.keybindOverdrive).c_str())) {
                        sor.changingAlt = false;
                        sor.changingKey = false;
                        sor.changingOverdrive = true;
                    }
                    if (GuiButton({ ((float)GetScreenWidth() / 2) + 10,480,120,60 }, keybinds.getKeyStr(settingsMain.keybindOverdriveAlt).c_str())) {
                        sor.changingAlt = true;
                        sor.changingKey = false;
                        sor.changingOverdrive = true;
                    }
                    if (GuiButton({ ((float)GetScreenWidth() / 2) - 60,560,120,60 }, keybinds.getKeyStr(settingsMain.keybindPause).c_str())) {
                        sor.changingKey = false;
                        sor.changingPause = true;
                    }
                    if (sor.changingKey) {
                        std::vector<int>& bindsToChange = sor.changingAlt ? (sor.changing4k ? settingsMain.keybinds4KAlt : settingsMain.keybinds5KAlt) : (sor.changing4k ? settingsMain.keybinds4K : settingsMain.keybinds5K);
                        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), { 0,0,0,200 });
                        std::string keyString = (sor.changing4k ? "4k" : "5k");
                        std::string altString = (sor.changingAlt ? " alt" : "");
                        std::string changeString = "Press a key for " + keyString + altString + " lane ";
                        DrawTextRubik(changeString.c_str(), ((float)GetScreenWidth() - MeasureTextRubik(changeString.c_str(), 20)) / 2, (float)GetScreenHeight() / 2 - 30, 20, WHITE);
                        int pressedKey = GetKeyPressed();
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 130, GetScreenHeight() - 60.0f, 120,40 }, "Unbind Key")) {
                            pressedKey = -1;
                        }
                        if (GuiButton({ ((float)GetScreenWidth() / 2) + 10, GetScreenHeight() - 60.0f, 120,40 }, "Cancel")) {
                            sor.selLane = 0;
                            sor.changingKey = false;
                        }
                        if (pressedKey != 0) {
                            bindsToChange[sor.selLane] = pressedKey;
                            sor.selLane = 0;
                            sor.changingKey = false;
                        }
                    }
                    if (sor.changingOverdrive) {
                        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), { 0,0,0,200 });
                        std::string altString = (sor.changingAlt ? " alt" : "");
                        std::string changeString = "Press a key for " + altString + " overdrive";
                        DrawTextRubik(changeString.c_str(), ((float)GetScreenWidth() - MeasureTextRubik(changeString.c_str(), 20)) / 2, (float)GetScreenHeight() / 2 - 30, 20, WHITE);
                        int pressedKey = GetKeyPressed();
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 130, GetScreenHeight() - 60.0f, 120,40 }, "Unbind Key")) {
                            pressedKey = -1;
                        }
                        if (GuiButton({ ((float)GetScreenWidth() / 2) + 10, GetScreenHeight() - 60.0f, 120,40 }, "Cancel")) {
                            sor.changingAlt = false;
                            sor.changingOverdrive = false;
                        }
                        if (pressedKey != 0) {
                            if(changingAlt)
                                settingsMain.keybindOverdriveAlt = pressedKey;
                            else
                                settingsMain.keybindOverdriveAlt = pressedKey;
                            sor.changingOverdrive = false;
                        }
                    }
                    if (sor.changingPause) {
                        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), { 0,0,0,200 });
                        DrawTextRubik("Press a key for Pause", ((float)GetScreenWidth() - MeasureTextRubik("Press a key for Pause", 20)) / 2, (float)GetScreenHeight() / 2 - 30, 20, WHITE);
                        int pressedKey = GetKeyPressed();
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 130, GetScreenHeight() - 60.0f, 120,40 }, "Unbind Key")) {
                            pressedKey = -1;
                        }
                        if (GuiButton({ ((float)GetScreenWidth() / 2) + 10, GetScreenHeight() - 60.0f, 120,40 }, "Cancel")) {
                            sor.changingAlt = false;
                            sor.changingPause = false;
                        }
                        if (pressedKey != 0) {
                            settingsMain.keybindPause = pressedKey;
                            sor.changingPause = false;
                        }
                    }
                    GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
                }
                else if (displayedTab == 2) { //Controller bindings tab
                    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
                    for (int i = 0; i < 5; i++) {
                        float j = (float)i - 2.0f;
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 40 + (80 * j),240,80,60 }, keybinds.getControllerStr(controllerID, settingsMain.controller5K[i], settingsMain.controllerType, settingsMain.controller5KAxisDirection[i]).c_str())) {
                            changing4k = false;
                            selLane = i;
                            changingKey = true;
                        }
                    }
                    for (int i = 0; i < 4; i++) {
                        float j = (float)i - 1.5f;
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 40 + (80 * j),360,80,60 }, keybinds.getControllerStr(controllerID, settingsMain.controller4K[i], settingsMain.controllerType, settingsMain.controller4KAxisDirection[i]).c_str())) {
                            changing4k = true;
                            selLane = i;
                            changingKey = true;
                        }
                    }
                    if (GuiButton({ ((float)GetScreenWidth() / 2) - 40,480,80,60 }, keybinds.getControllerStr(controllerID, settingsMain.controllerOverdrive, settingsMain.controllerType, settingsMain.controllerOverdriveAxisDirection).c_str())) {
                        changingKey = false;
                        changingOverdrive = true;
                    } 
                    if (GuiButton({ ((float)GetScreenWidth() / 2) - 40,560,80,60 }, keybinds.getControllerStr(controllerID, settingsMain.controllerPause, settingsMain.controllerType, settingsMain.controllerPauseAxisDirection).c_str())) {
                        changingKey = false;
                        changingOverdrive = true;
                    }
                    if (changingKey) {
                        std::vector<int>& bindsToChange = (changing4k ? settingsMain.controller4K : settingsMain.controller5K);
                        std::vector<int>& directionToChange = (changing4k ? settingsMain.controller4KAxisDirection : settingsMain.controller5KAxisDirection);
                        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), { 0,0,0,200 });
                        std::string keyString = (changing4k ? "4k" : "5k");
                        std::string changeString = "Press a button/axis for controller " + keyString + " lane " + std::to_string(selLane + 1);
                        DrawTextRubik(changeString.c_str(), ((float)GetScreenWidth() - MeasureTextRubik(changeString.c_str(), 20)) / 2, GetScreenHeight() / 2 - 30, 20, WHITE);
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 60, GetScreenHeight() - 60.0f, 120,40 }, "Cancel")) {
                            changingKey = false;
                        }
                        if (pressedGamepadInput != -999) {
                            bindsToChange[selLane] = pressedGamepadInput;
                            if (pressedGamepadInput < 0) {
                                directionToChange[selLane] = axisDirection;
                            }
                            selLane = 0;
                            changingKey = false;
                            pressedGamepadInput = -999;
                        }
                    }
                    if (changingOverdrive) {
                        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), { 0,0,0,200 });
                        std::string changeString = "Press a button/axis for controller overdrive";
                        DrawTextRubik(changeString.c_str(), ((float)GetScreenWidth() - MeasureTextRubik(changeString.c_str(), 20)) / 2, GetScreenHeight() / 2 - 30, 20, WHITE);
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 60, GetScreenHeight() - 60.0f, 120,40 }, "Cancel")) {
                            changingOverdrive = false;
                        }
                        if (pressedGamepadInput != -999) {
                            settingsMain.controllerOverdrive = pressedGamepadInput;
                            if (pressedGamepadInput < 0) {
                                settingsMain.controllerOverdriveAxisDirection = axisDirection;
                            }
                            changingOverdrive = false;
                            pressedGamepadInput = -999;
                        }
                    } 
                    if (changingPause) {
                        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), { 0,0,0,200 });
                        std::string changeString = "Press a button/axis for controller pause";
                        DrawTextRubik(changeString.c_str(), ((float)GetScreenWidth() - MeasureTextRubik(changeString.c_str(), 20)) / 2, GetScreenHeight() / 2 - 30, 20, WHITE);
                        if (GuiButton({ ((float)GetScreenWidth() / 2) - 60, GetScreenHeight() - 60.0f, 120,40 }, "Cancel")) {
                            changingPause = false;
                        }
                        if (pressedGamepadInput != -999) {
                            settingsMain.controllerPause = pressedGamepadInput;
                            if (pressedGamepadInput < 0) {
                                settingsMain.controllerPauseAxisDirection = axisDirection;
                            }
                            changingPause = false;
                            pressedGamepadInput = -999;
                        }
                    }                   
                    GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
                }
                break;
            }
            case SONG_SELECT: {
                if (!menu.songsLoaded) {

                    songList = songList.LoadCache(settingsMain.songPaths);
                    menu.songsLoaded = true;

                }
                u.calcUnits();
                streamsLoaded = false;
                midiLoaded = false;
                isPlaying = false;
                songEnded = false;
                player.overdrive = false;
                curNoteIdx = { 0,0,0,0,0 };
                curODPhrase = 0;
                curBeatLine = 0;
                curBPM = 0;

                if (selSong) 
                    selectedSongInt = curPlayingSong;
                else
                    selectedSongInt = menu.ChosenSongInt;



                SetTextureWrap(selectedSong.albumArtBlur, TEXTURE_WRAP_REPEAT);
                SetTextureFilter(selectedSong.albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);

                Vector2 mouseWheel = GetMouseWheelMoveV();
                int lastIntChosen = (int)mouseWheel.y;
                // set to specified height
                if (songSelectOffset <= songList.songs.size() && songSelectOffset >= 0) {
                    songSelectOffset -= (int)mouseWheel.y;
                }

                // prevent going past top
                if (songSelectOffset < 0)
                    songSelectOffset = 0;

                // prevent going past bottom
                if (songSelectOffset >= songList.songs.size() - 12)
                    songSelectOffset = (int)songList.songs.size() - 12;

                if (!albumArtLoaded) {
                    selectedSong = menu.ChosenSong;
                    selectedSongInt = menu.ChosenSongInt;
                    selectedSong.LoadAlbumArt(selectedSong.albumArtPath);
                    if (!selSong)
                        songSelectOffset = menu.ChosenSongInt - 5;
                    albumArtLoaded = true;
                }

                BeginShaderMode(assets.bgShader);
                if (selSong){

                    menu.DrawAlbumArtBackground(selectedSong.albumArtBlur, assets);
                }
                else {

                    Song art = menu.ChosenSong;
                    menu.DrawAlbumArtBackground(art.albumArtBlur, assets);
                }
                EndShaderMode();

                float TopOvershell = u.hpct(0.15f);
                DrawRectangle((int)u.LeftSide,0, u.RightSide - u.LeftSide, (float)GetScreenHeight(), Color(0,0,0,128));
                DrawLineEx({u.LeftSide + u.winpct(0.0025f),0},{u.LeftSide + u.winpct(0.0025f),(float)GetScreenHeight()}, u.winpct(0.005f), WHITE);

                menu.DrawTopOvershell(0.15f);


                menu.DrawVersion(assets);
                int AlbumX = u.RightSide - u.winpct(0.25f);
                int AlbumY = u.hpct(0.075f);
                int AlbumHeight = u.winpct(0.25f);
                int AlbumOuter = u.hinpct(0.01f);
                int AlbumInner = u.hinpct(0.005f);
                int BorderBetweenAlbumStuff = (u.RightSide - u.LeftSide) - u.winpct(0.25f);


                DrawTextEx(assets.josefinSansItalic, TextFormat("Sorted by: %s", sortTypes[currentSortValue].c_str()), {AlbumX - (AlbumOuter * 2) - MeasureTextEx(assets.josefinSansItalic, TextFormat("Sorted by: %s", sortTypes[currentSortValue].c_str()), u.hinpct(0.025f), 0).x, AlbumY + u.hinpct(0.0075)}, u.hinpct(0.025f), 0, WHITE);
                DrawTextEx(assets.josefinSansItalic, TextFormat("Songs loaded: %01i", songList.songs.size()), {AlbumX - (AlbumOuter * 2) - MeasureTextEx(assets.josefinSansItalic, TextFormat("Songs loaded: %01i", songList.songs.size()), u.hinpct(0.025f), 0).x, AlbumY +u.hinpct(0.04f)}, u.hinpct(0.025f), 0, WHITE);


                float songEntryHeight = u.hinpct(0.06f);
                for (int i = songSelectOffset; i < songList.songs.size() && i < songSelectOffset+12; i++) {
                    if (songList.songs.size() == i)
                        break;
                    Font& songFont = i == menu.ChosenSongInt ? assets.rubikBoldItalic : assets.rubikBold;
                    Font& artistFont = i == menu.ChosenSongInt ? assets.josefinSansItalic : assets.josefinSansItalic;
                    Song& songi = songList.songs[i];
                    // float buttonX = ((float)GetScreenWidth()/2)-(((float)GetScreenWidth()*0.86f)/2);
                    //LerpState state = lerpCtrl.createLerp("SONGSELECT_LERP_" + std::to_string(i), EaseOutCirc, 0.4f);
                    float songXPos = u.LeftSide+u.winpct(0.005f)-2;
                    float songYPos = (u.hpct(0.15f)-2) + ((songEntryHeight-1) * (i - songSelectOffset));

                    if (i == menu.ChosenSongInt) {
                        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(ColorBrightness(player.accentColor, -0.4)));
                    }
                    BeginScissorMode(u.LeftSide, u.hpct(0.15f), BorderBetweenAlbumStuff,u.hinpct(0.7f));
                    if (GuiButton(Rectangle{ songXPos, songYPos,(u.winpct(0.75f)), songEntryHeight }, "")) {
                        curPlayingSong = i;
                        selSong = true;
                        albumArtSelectedAndLoaded = false;
                        albumArtLoaded = false;
                        menu.ChosenSong = songList.songs[i];
                        menu.ChosenSongInt = i;
                    }
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);
                    EndScissorMode();
                    // DrawTexturePro(song.albumArt, Rectangle{ songXPos,0,(float)song.albumArt.width,(float)song.albumArt.height }, { songXPos+5,songYPos + 5,50,50 }, Vector2{ 0,0 }, 0.0f, RAYWHITE);
                    int songTitleWidth = (u.winpct(0.3f))-6;

                    int songArtistWidth = (u.winpct(0.5f))-6;

                    if (songi.titleTextWidth >= (float)songTitleWidth) {
                        if (curTime > songi.titleScrollTime && curTime < songi.titleScrollTime + 3.0)
                            songi.titleXOffset = 0;

                        if (curTime > songi.titleScrollTime + 3.0) {
                            songi.titleXOffset -= 1;

                            if (songi.titleXOffset < -(songi.titleTextWidth - (float)songTitleWidth)) {
                                songi.titleXOffset = -(songi.titleTextWidth - (float)songTitleWidth);
                                songi.titleScrollTime = curTime + 3.0;
                            }
                        }
                    }
                    auto LightText = Color{203, 203, 203, 255};
                    BeginScissorMode((int)songXPos + 20, (int)songYPos, songTitleWidth, songEntryHeight);
                    DrawTextEx(songFont,songi.title.c_str(), {songXPos + 20 + songi.titleXOffset + (i == menu.ChosenSongInt ? u.winpct(0.005f) : 0), songYPos + u.hinpct(0.0125f)}, u.hinpct(0.035f),0, i == menu.ChosenSongInt ? WHITE : LightText);
                    EndScissorMode();

                    if (songi.artistTextWidth > (float)songArtistWidth) {
                        if (curTime > songi.artistScrollTime && curTime < songi.artistScrollTime + 3.0)
                            songi.artistXOffset = 0;

                        if (curTime > songi.artistScrollTime + 3.0) {
                            songi.artistXOffset -= 1;
                            if (songi.artistXOffset < -(songi.artistTextWidth - (float)songArtistWidth)) {
                                songi.artistScrollTime = curTime + 3.0;
                            }
                        }
                    }

                    auto SelectedText = WHITE;
                    BeginScissorMode((int)songXPos + 30 + (int)songTitleWidth, (int)songYPos, songArtistWidth, songEntryHeight);
                    DrawTextEx(artistFont, songi.artist.c_str(), {songXPos + 30 + (float)songTitleWidth + songi.artistXOffset, songYPos + u.hinpct(0.02f)}, u.hinpct(0.025f), 0, i == menu.ChosenSongInt ? WHITE : LightText);
                    EndScissorMode();
                }

                DrawRectangle(AlbumX-AlbumOuter,AlbumY+AlbumHeight,AlbumHeight+AlbumOuter,AlbumHeight+u.hinpct(0.01f), WHITE);
                DrawRectangle(AlbumX-AlbumInner,AlbumY+AlbumHeight,AlbumHeight,u.hinpct(0.075f)+AlbumHeight, GetColor(0x181827FF));
                DrawRectangle(AlbumX-AlbumOuter,AlbumY-AlbumInner,AlbumHeight+AlbumOuter,AlbumHeight+AlbumOuter, WHITE);
                DrawRectangle(AlbumX-AlbumInner,AlbumY,AlbumHeight,AlbumHeight, BLACK);


                // bottom
                if (selSong) {
                    DrawTexturePro(selectedSong.albumArt, Rectangle{0, 0, (float) selectedSong.albumArt.width,
                                                                    (float) selectedSong.albumArt.width},
                                   Rectangle{(float)AlbumX-AlbumInner, (float)AlbumY, (float)AlbumHeight, (float)AlbumHeight}, {0, 0}, 0,
                                   WHITE);
                } else {
                    Song art = menu.ChosenSong;
                    DrawTexturePro(art.albumArt, Rectangle{0, 0, (float) art.albumArt.width,
                                                           (float) art.albumArt.width},
                                   Rectangle{(float)AlbumX-AlbumInner, (float)AlbumY, (float)AlbumHeight, (float)AlbumHeight}, {0, 0}, 0,
                                   WHITE);
                }
                // hehe

                float TextPlacementTB = u.TopSide + u.hinpct(0.15f) - u.hinpct(0.11f);
                float TextPlacementLR = u.LeftSide + u.winpct(0.01f);
                DrawTextEx(assets.redHatDisplayBlack, "Song Select", {TextPlacementLR, TextPlacementTB}, u.hinpct(0.10f),0, WHITE);

                menu.DrawBottomOvershell();
                float BottomOvershell = (float)GetScreenHeight() - 120;

                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(ColorBrightness(player.accentColor, -0.25)));
                if (GuiButton(Rectangle{u.LeftSide, GetScreenHeight() - u.hpct(0.1475f), u.winpct(0.2f), u.hinpct(0.05f)}, "Play Song")) {
                        curPlayingSong = menu.ChosenSongInt;
                        songList.songs[curPlayingSong].LoadSong(songList.songs[curPlayingSong].songInfoPath);
                        menu.SwitchScreen(READY_UP);
                        albumArtLoaded = false;
                }
                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);

                if (GuiButton(Rectangle{u.LeftSide + u.winpct(0.4f)-2, GetScreenHeight() - u.hpct(0.1475f), u.winpct(0.2f), u.hinpct(0.05f)}, "Sort")) {
                    currentSortValue++;
                    if (currentSortValue == 3) currentSortValue = 0;
                    if (selSong)
                        songList.sortList(currentSortValue, curPlayingSong);
                    else
                        songList.sortList(currentSortValue, menu.ChosenSongInt);
                }
                if (GuiButton(Rectangle{u.LeftSide + u.winpct(0.2f)-1, GetScreenHeight() - u.hpct(0.1475f), u.winpct(0.2f), u.hinpct(0.05f)}, "Back")) {
                    for (Song& songi : songList.songs) {
                        songi.titleXOffset = 0;
                        songi.artistXOffset = 0;
                    }
                    albumArtLoaded = false;
                    menu.albumArtLoaded = false;
                    menu.songsLoaded = true;
                    menu.songChosen = false;
                    selSong = false;
                    menu.SwitchScreen(MENU);
                }
                menu.DrawBottomBottomOvershell();
                break;
            }
            case READY_UP: {

                if (!albumArtLoaded) {
                    selectedSong = songList.songs[curPlayingSong];
                    selectedSong.LoadAlbumArt(selectedSong.albumArtPath);
                    albumArtLoaded = true;
                }
                SetTextureWrap(selectedSong.albumArtBlur, TEXTURE_WRAP_REPEAT);
                SetTextureFilter(selectedSong.albumArtBlur, TEXTURE_FILTER_ANISOTROPIC_16X);
                menu.DrawAlbumArtBackground(selectedSong.albumArtBlur, assets);

                float AlbumArtLeft = u.LeftSide;
                float AlbumArtTop = u.hpct(0.05f);
                float AlbumArtRight = u.winpct(0.15f);
                float AlbumArtBottom = u.winpct(0.15f);
                DrawRectangle(0,0, (int)GetScreenWidth(), (int)GetScreenHeight(), Color(0,0,0,128));

                menu.DrawTopOvershell(0.2f);
                menu.DrawVersion(assets);

                DrawRectangle((int)u.LeftSide,(int)AlbumArtTop,(int)AlbumArtRight+12, (int)AlbumArtBottom+12, WHITE);
                DrawRectangle((int)u.LeftSide + 6,(int)AlbumArtTop+6,(int)AlbumArtRight, (int)AlbumArtBottom,BLACK);
                DrawTexturePro(selectedSong.albumArt, Rectangle{0,0,(float)selectedSong.albumArt.width,(float)selectedSong.albumArt.width}, Rectangle {u.LeftSide + 6, AlbumArtTop+6,AlbumArtRight,AlbumArtBottom}, {0,0}, 0, WHITE);
                


                float BottomOvershell = u.hpct(1)-u.hinpct(0.15f);
                float TextPlacementTB = AlbumArtTop;
                float TextPlacementLR = AlbumArtRight + AlbumArtLeft+ 32;
                DrawTextEx(assets.redHatDisplayBlack, songList.songs[curPlayingSong].title.c_str(), {TextPlacementLR, TextPlacementTB-5}, u.hinpct(0.1f), 0, WHITE);
                DrawTextEx(assets.rubikBoldItalic, selectedSong.artist.c_str(), {TextPlacementLR, TextPlacementTB+u.hinpct(0.09f)}, u.hinpct(0.05f), 0, LIGHTGRAY);
                // todo: allow this to be run per player
                // load midi
                if (!midiLoaded) {
                    if (!songList.songs[curPlayingSong].midiParsed) {
                        smf::MidiFile midiFile;
                        midiFile.read(songList.songs[curPlayingSong].midiPath.string());
                        songList.songs[curPlayingSong].getTiming(midiFile, 0, midiFile[0]);
                        for (int i = 0; i < midiFile.getTrackCount(); i++)
                        {
                            std::string trackName;
                            for (int j = 0; j < midiFile[i].getSize(); j++) {
                                if (midiFile[i][j].isMeta()) {
                                    if ((int)midiFile[i][j][1] == 3) {
                                        for (int k = 3; k < midiFile[i][j].getSize(); k++) {
                                            trackName += midiFile[i][j][k];
                                        }
                                        SongParts songPart = song.partFromString(trackName);
                                        if (trackName == "BEAT")
                                            songList.songs[curPlayingSong].parseBeatLines(midiFile, i, midiFile[i]);
                                        else if (trackName == "EVENTS") {
                                            songList.songs[curPlayingSong].getStartEnd(midiFile, i, midiFile[i]);
                                        }
                                        else {
                                            if (songPart != SongParts::Invalid) {
                                                for (int diff = 0; diff < 4; diff++) {
                                                    Chart newChart;
                                                    std::cout << trackName << " " << diff << endl;
                                                    newChart.parseNotes(midiFile, i, midiFile[i], diff, (int)songPart);
                                                    if (newChart.notes.size() > 0) {
                                                        songList.songs[curPlayingSong].parts[(int)songPart]->hasPart = true;
                                                    }
                                                    std::sort(newChart.notes.begin(), newChart.notes.end(), compareNotes);
                                                    int noteIdx = 0;
                                                    for (Note& note : newChart.notes) {
                                                        newChart.notes_perlane[note.lane].push_back(noteIdx);
                                                        noteIdx++;
                                                    }
                                                    songList.songs[curPlayingSong].parts[(int)songPart]->charts.push_back(newChart);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        songList.songs[curPlayingSong].midiParsed = true;
                    }
                    midiLoaded = true;
                    if (player.firstReadyUp || !songList.songs[curPlayingSong].parts[player.instrument]->hasPart) {
                        instSelection = true;
                    }
                    else if (songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].notes.size() < 1) {
                        diffSelection = true;
                    }
                    else if (!player.firstReadyUp) {
                        ReadyUpMenu = true;
                    }
                }
                // load instrument select
                else if (midiLoaded && instSelection) {
                    if (GuiButton({ 0,0,60,60 }, "<")) {
                        if (player.firstReadyUp || !songList.songs[curPlayingSong].parts[player.instrument]->hasPart) {
                            instSelection = false;
                            diffSelection = false;
                            instSelected = false;
                            diffSelected = false;
                            midiLoaded = false;
                            albumArtSelectedAndLoaded = false;
                            albumArtLoaded = false;
                            menu.SwitchScreen(SONG_SELECT);
                        }
                        else {
                            instSelection = false;
                            ReadyUpMenu = true;
                        }
                        
                    }
                    // DrawTextRHDI(TextFormat("%s - %s", songList.songs[curPlayingSong].title.c_str(), songList.songs[curPlayingSong].artist.c_str()), 70,7, WHITE);
                    for (int i = 0; i < 4; i++) {
                        if (songList.songs[curPlayingSong].parts[i]->hasPart) {
                            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, i == player.instrument && instSelected ? ColorToInt(ColorBrightness(player.accentColor, -0.25)) : 0x181827FF);
                            if (GuiButton({ u.LeftSide,BottomOvershell - 60 - (60 * (float)i),300,60 }, "")) {
                                instSelected = true;
                                player.instrument = i;
                                int isBassOrVocal = 0;
                                if (player.instrument == 1 || player.instrument == 3) {
                                    isBassOrVocal = 1;
                                }
                                SetShaderValue(assets.odMultShader, assets.isBassOrVocalLoc, &isBassOrVocal, SHADER_UNIFORM_INT);
                            }
                            
                            
                            

                            DrawTextRubik(songPartsList[i].c_str(), u.LeftSide + 20, BottomOvershell - 45 - (60 * (float)i), 30, WHITE);
                            DrawTextRubik((std::to_string(songList.songs[curPlayingSong].parts[i]->diff + 1) + "/7").c_str(), u.LeftSide + 220, BottomOvershell - 45 - (60 * (float)i), 30, WHITE);
                        } else {

                            GuiButton({ u.LeftSide,BottomOvershell - 60 - (60 * (float)i),300,60 }, "");
                            DrawRectangle( u.LeftSide+2,BottomOvershell+2 - 60 - (60 * (float)i),300-4,60-4, Color{0,0,0,128});
                        }
                        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);
                        if (instSelected) {
                            if (GuiButton({ u.LeftSide, BottomOvershell - 280, 300, 40 }, "Done")) {
                                if (player.firstReadyUp) {
                                    instSelection = false;
                                    diffSelection = true;
                                }
                                else if (songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].notes.size() < 1) {
                                    instSelection = false;
                                    diffSelection = true;
                                }
                                else {
                                    instSelection = false;
                                    ReadyUpMenu = true;
                                }
                            }
                        }
                    }
                }
                menu.DrawBottomOvershell();
                menu.DrawBottomBottomOvershell();
                // load difficulty select
                if (midiLoaded && diffSelection) {
                    for (int a = 0; a < 4; a++) {
                        if (songList.songs[curPlayingSong].parts[player.instrument]->charts[a].notes.size() > 0) {
                            GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, a == player.diff && diffSelected ? ColorToInt(ColorBrightness(player.accentColor, -0.25)) : 0x181827FF);
                            if (GuiButton({ u.LeftSide,BottomOvershell - 60 - (60 * (float)a),300,60 }, "")) {
                                player.diff = a;
                                diffSelected = true;
                                isPlaying = true;
                                startedPlayingSong = GetTime();
                                
                            }
                            DrawTextRubik(diffList[a].c_str(), u.LeftSide + 150 - (MeasureTextRubik(diffList[a].c_str(), 30) / 2), BottomOvershell - 45 - (60 * (float)a), 30, WHITE);
                        } else {
                            GuiButton({ u.LeftSide,BottomOvershell - 60 - (60 * (float)a),300,60 }, "");
                            DrawRectangle( u.LeftSide+2,BottomOvershell+2 - 60 - (60 * (float)a),300-4,60-4, Color{0,0,0,128});
                        }
                        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x181827FF);
                        if (diffSelected) {
                            if (GuiButton({ u.LeftSide, BottomOvershell-280, 300, u.hinpct(0.05f)}, "Done")) {
                                diffSelection = false;
                                ReadyUpMenu = true;
                                player.firstReadyUp = false;
                            }
                        }
                        if (GuiButton({ 0,0,60,60 }, "<")) {
                            if (player.firstReadyUp || !songList.songs[curPlayingSong].parts[player.instrument]->hasPart) {
                                midiLoaded = false;
                                instSelection = false;
                                diffSelection = false;
                                instSelected = false;
                                diffSelected = false;
                                menu.SwitchScreen(SONG_SELECT);
                            }
                            else {
                                instSelection = false;
                                diffSelection = false;
                                instSelected = false;
                                diffSelected = false;
                                ReadyUpMenu = true;
                            }
                        }
                    }
                }
                
                if (midiLoaded && ReadyUpMenu) {
                    if (GuiButton({ u.LeftSide,BottomOvershell - 60,300,60 }, "")) {
                            ReadyUpMenu = false;
                            diffSelection = true;
                    }
                    DrawTextRubik("Difficulty", u.LeftSide+15, BottomOvershell - 45, 30, WHITE);
                    DrawTextEx(assets.rubikBold, diffList[player.diff].c_str(), {u.LeftSide + 285 - MeasureTextEx(assets.rubikBold, diffList[player.diff].c_str(),30,0).x, BottomOvershell - 45}, 30, 0, WHITE);

                    if (GuiButton({ u.LeftSide,BottomOvershell - 120,300,60 }, "")) {
                        ReadyUpMenu = false;
                        instSelection = true;
                    }
                    DrawTextRubik("Instrument", u.LeftSide+15, BottomOvershell - 105, 30, WHITE);
                    DrawTextEx(assets.rubikBold, songPartsList[player.instrument].c_str(), { u.LeftSide + 285 - MeasureTextEx(assets.rubikBold, songPartsList[player.instrument].c_str(),30,0).x, BottomOvershell - 105 }, 30, 0, WHITE);

                      
                    if (GuiButton({ u.LeftSide, BottomOvershell, 300, u.hinpct(0.05f)}, "Ready Up!")) {
                        ReadyUpMenu = false;
                        menu.SwitchScreen(GAMEPLAY);
                        glfwSetKeyCallback(glfwGetCurrentContext(), keyCallback);
                        glfwSetGamepadStateCallback(gamepadStateCallback);
                    }
                        
                        if (GuiButton({ 0,0,60,60 }, "<")) {
                            midiLoaded = false;
                            instSelection = false;
                            diffSelection = false;
                            instSelected = false;
                            diffSelected = false;
                            ReadyUpMenu = false;
                            menu.SwitchScreen(SONG_SELECT);
                        }
                    
                }
                break;
            }
            case GAMEPLAY: {
                notes_tex.texture.height = GetScreenHeight();
                notes_tex.texture.width = GetScreenWidth();
                // IMAGE BACKGROUNDS??????
                ClearBackground(BLACK);
                player.songToBeJudged = songList.songs[curPlayingSong];

                float scorePos = u.RightSide;
                float scoreY = u.hpct(0.15f);
                float starY = scoreY + u.hinpct(0.05f);
                float comboY = starY + u.hinpct(0.055f);
                menu.DrawAlbumArtBackground(menu.ChosenSong.albumArtBlur, assets);
                DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(), Color{0,0,0,128});
                DrawTextureEx(assets.songBackground, {0,0},0, (float)GetScreenHeight()/assets.songBackground.height,WHITE);

                int starsval = player.stars(songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].baseScore,player.diff);
                float starPercent = (float)player.score/(float)player.songToBeJudged.parts[player.instrument]->charts[player.diff].baseScore;
                for (int i = 0; i < 5; i++) {
                    bool firstStar = (i == 0);
                    float starX = scorePos - u.hinpct(0.26) + (i*u.hinpct(0.05));
                    float starWH = u.hinpct(0.05);
                    Rectangle emptyStarWH = {0,0,(float)assets.emptyStar.width, (float)assets.emptyStar.height};
                    Rectangle starRect = {starX, starY, starWH,starWH};

                    DrawTexturePro(assets.emptyStar, emptyStarWH, starRect, {0,0},0,WHITE);
                    float yMaskPos = Remap(starPercent, firstStar ? 0 : player.xStarThreshold[i-1], player.xStarThreshold[i], 0, u.hinpct(0.05));
                    BeginScissorMode(starX, (starY+starWH)-yMaskPos, starWH, yMaskPos);
                    DrawTexturePro(assets.star, emptyStarWH, starRect, {0,0},0,i != starsval ? WHITE : Color{192,192,192,128});
                    EndScissorMode();
                }

                if (starPercent >= player.xStarThreshold[4]) {

                    float starWH = u.hinpct(0.05);
                    Rectangle emptyStarWH = {0,0,(float)assets.goldStar.width, (float)assets.goldStar.height};


                    float yMaskPos = Remap(starPercent, player.xStarThreshold[4], player.xStarThreshold[5], 0, u.hinpct(0.05));
                    BeginScissorMode(scorePos - (starWH*6), (starY+starWH)-yMaskPos, scorePos, yMaskPos);
                    for (int i = 0; i < 5; i++) {
                        float starX = scorePos - u.hinpct(0.26) + (i*u.hinpct(0.05));
                        Rectangle starRect = {starX, starY, starWH,starWH};
                        DrawTexturePro(player.goldStars ? assets.goldStar : assets.goldStarUnfilled, emptyStarWH, starRect,
                                       {0, 0}, 0, WHITE);
                    }
                    EndScissorMode();

                }
                DrawFPS(u.LeftSide,u.hpct(0.0025f) + u.hinpct(0.025f));
                menu.DrawVersion(assets);

                // DrawTextRubik(TextFormat("%s", starsDisplay), 5, GetScreenHeight() - 470, 48, goldStars ? GOLD : WHITE);
                int totalScore = player.score + player.sustainScoreBuffer[0] + player.sustainScoreBuffer[1] + player.sustainScoreBuffer[2] + player.sustainScoreBuffer[3] + player.sustainScoreBuffer[4];

                DrawTextRHDI(scoreCommaFormatter(totalScore).c_str(), u.RightSide - u.winpct(0.01f) - MeasureTextRHDI(scoreCommaFormatter(totalScore).c_str(), u.hinpct(0.05f)), scoreY, u.hinpct(0.05f), Color{107, 161, 222,255});
                DrawTextRHDI(scoreCommaFormatter(player.combo).c_str(), u.RightSide - u.winpct(0.01f) - MeasureTextRHDI(scoreCommaFormatter(player.combo).c_str(), u.hinpct(0.05f)), comboY, u.hinpct(0.05f), player.FC ? GOLD : (player.combo <= 3) ? RED : WHITE);
                DrawTextEx(assets.rubikBold, TextFormat("%s", player.FC ? "FC" : ""), {5, GetScreenHeight() -u.hinpct(0.05f)}, u.hinpct(0.04), 0, GOLD);

                DrawTextEx(assets.rubikBold, TextFormat("Video Offset: %2.3f", player.VideoOffset), {u.LeftSide, u.hinpct(0.5f)}, u.hinpct(0.04), 0, WHITE);
                DrawTextEx(assets.rubikBold, TextFormat("Input Offset: %2.3f", player.InputOffset), {u.LeftSide, u.hinpct(0.55f)}, u.hinpct(0.04), 0, WHITE);

                float multFill = (!player.overdrive ? (float)(player.multiplier(player.instrument) - 1) : ((float)(player.multiplier(player.instrument) / 2) - 1)) / (float)player.maxMultForMeter(player.instrument);
                SetShaderValue(assets.odMultShader, assets.multLoc, &multFill, SHADER_UNIFORM_FLOAT);
                SetShaderValue(assets.multNumberShader, assets.uvOffsetXLoc, &player.uvOffsetX, SHADER_UNIFORM_FLOAT);
                SetShaderValue(assets.multNumberShader, assets.uvOffsetYLoc, &player.uvOffsetY, SHADER_UNIFORM_FLOAT);
                float comboFill = player.comboFillCalc(player.instrument);
                SetShaderValue(assets.odMultShader, assets.comboCounterLoc, &comboFill, SHADER_UNIFORM_FLOAT);
                SetShaderValue(assets.odMultShader, assets.odLoc, &player.overdriveFill, SHADER_UNIFORM_FLOAT);
                if (player.extraGameplayStats) {
                    DrawTextRubik(TextFormat("Perfect Hit: %01i", player.perfectHit), 5, GetScreenHeight() - 280, 24,
                                  (player.perfectHit > 0) ? GOLD : WHITE);
                    DrawTextRubik(TextFormat("Max Combo: %01i", player.maxCombo), 5, GetScreenHeight() - 130, 24, WHITE);
                    DrawTextRubik(TextFormat("Multiplier: %01i", player.multiplier(player.instrument)), 5, GetScreenHeight() - 100, 24,
                                  (player.multiplier(player.instrument) >= 4) ? SKYBLUE : WHITE);
                    DrawTextRubik(TextFormat("Notes Hit: %01i", player.notesHit), 5, GetScreenHeight() - 250, 24, player.FC ? GOLD : WHITE);
                    DrawTextRubik(TextFormat("Notes Missed: %01i", player.notesMissed), 5, GetScreenHeight() - 220, 24,
                                  ((player.combo == 0) && (!player.FC)) ? RED : WHITE);
                    DrawTextRubik(TextFormat("Strikes: %01i", player.playerOverhits), 5, GetScreenHeight() - 40, 24, player.FC ? GOLD : WHITE);
                }

                if ((player.overdrive ? player.multiplier(player.instrument) / 2 : player.multiplier(player.instrument))>= (player.instrument == 1 || player.instrument == 3 ? 6 : 4)) {
                    assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                    assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                    assets.smasherBoard.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                    assets.smasherBoardEMH.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                } else {

                    assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                    assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                    assets.smasherBoard.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                    assets.smasherBoardEMH.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                }

                if (!streamsLoaded && !player.quit) {
                    audioManager.loadStreams(songList.songs[curPlayingSong].stemsPath);
                    streamsLoaded = true;
                    audioManager.BeginPlayback(audioManager.loadedStreams[0].handle);
                    player.resetPlayerStats();
                }
                else {
                    float songPlayed = audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle);
                    int songLength = audioManager.GetMusicTimeLength(audioManager.loadedStreams[0].handle);
                    int playedMinutes = audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle)/60;
                    int playedSeconds = (int)audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle) % 60;
                    int songMinutes = audioManager.GetMusicTimeLength(audioManager.loadedStreams[0].handle)/60;
                    int songSeconds = (int)audioManager.GetMusicTimeLength(audioManager.loadedStreams[0].handle) % 60;
                    const char* textTime = TextFormat("%i:%02i / %i:%02i ", playedMinutes,playedSeconds,songMinutes,songSeconds);
                    float textLength = MeasureTextEx(assets.rubik, textTime, u.hinpct(0.04f), 0).x;

                    DrawTextEx(assets.rubik, textTime,{GetScreenWidth() - textLength,GetScreenHeight()-u.hinpct(0.05f)},u.hinpct(0.04f),0,WHITE);
                    double songEnd = songList.songs[curPlayingSong].end == 0 ? audioManager.GetMusicTimeLength(audioManager.loadedStreams[0].handle) : songList.songs[curPlayingSong].end;
                    if (songEnd <= songPlayed) {
                        glfwSetKeyCallback(glfwGetCurrentContext(), origKeyCallback);
                        glfwSetGamepadStateCallback(origGamepadCallback);
                        // notes = (int)songList.songs[curPlayingSong].parts[instrument]->charts[diff].notes.size();
                        player.overdrive = false;
                        player.overdriveFill = 0.0f;
                        player.overdriveActiveFill = 0.0f;
                        player.overdriveActiveTime = 0.0;
                        menu.ChosenSong.LoadAlbumArt(menu.ChosenSong.albumArtPath);
                        midiLoaded = false;
                        isPlaying = false;
                        songEnded = true;
                        songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].resetNotes();
                        curODPhrase = 0;
                        assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.multBar.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr3.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr5.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                        assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                        menu.SwitchScreen(RESULTS);
                    }
                    for (auto& stream : audioManager.loadedStreams) {
                        if (player.instrument == stream.instrument)
                            audioManager.SetAudioStreamVolume(stream.handle, player.mute ? player.missVolume : player.selInstVolume);
                        else
                            audioManager.SetAudioStreamVolume(stream.handle, player.otherInstVolume);

                    }
                }

                float highwayLength = player.defaultHighwayLength * settingsMain.highwayLengthMult;
                double musicTime = audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle) - player.VideoOffset;
                if (player.overdrive) {

                    // assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTextureOD;
                    // assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTextureOD;
                    assets.multBar.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFillActive;
                    assets.multCtr3.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFillActive;
                    assets.multCtr5.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFillActive;
                    player.overdriveFill = player.overdriveActiveFill - (float)((musicTime - player.overdriveActiveTime) / (1920 / songList.songs[curPlayingSong].bpms[curBPM].bpm));
                    if (player.overdriveFill <= 0) {
                        player.overdrive = false;
                        player.overdriveActiveFill = 0;
                        player.overdriveActiveTime = 0.0;

                        assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.multBar.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr3.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr5.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;

                    }
                }
                for (int i = curBPM; i < songList.songs[curPlayingSong].bpms.size(); i++) {
                    if (musicTime > songList.songs[curPlayingSong].bpms[i].time && i < songList.songs[curPlayingSong].bpms.size() - 1)
                        curBPM++;
                }

                if (musicTime < 7.5) {
                    DrawTextEx(assets.rubikBoldItalic, songList.songs[curPlayingSong].title.c_str(), {25, (float)((GetScreenHeight()/3)*2) - u.hpct(0.08f)}, u.hpct(0.04f), 0, WHITE);
                    DrawTextEx(assets.rubikItalic, songList.songs[curPlayingSong].artist.c_str(), {35, (float)((GetScreenHeight()/3)*2) - u.hpct(0.04f)}, u.hpct(0.04f), 0, LIGHTGRAY);
                    //DrawTextRHDI(songList.songs[curPlayingSong].artist.c_str(), 5, 130, WHITE);
                }

                BeginMode3D(camera);
                if (player.diff == 3) {
                    float highwayPosShit = ((20) * (1 - settingsMain.highwayLengthMult));
                    DrawModel(assets.expertHighwaySides, Vector3{ 0,0,settingsMain.highwayLengthMult < 1.0f ? -(highwayPosShit* (0.875f)) : 0 }, 1.0f, WHITE);
                    DrawModel(assets.expertHighway, Vector3{ 0,0,settingsMain.highwayLengthMult < 1.0f ? -(highwayPosShit* (0.875f)) : 0 }, 1.0f, WHITE);
                    if (settingsMain.highwayLengthMult > 1.0f) {
                        DrawModel(assets.expertHighway, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-20 }, 1.0f, WHITE);
                        DrawModel(assets.expertHighwaySides, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-20 }, 1.0f, WHITE);
                        if (highwayLength > 23.0f) {
                            DrawModel(assets.expertHighway, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-40 }, 1.0f, WHITE);
                            DrawModel(assets.expertHighwaySides, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-40 }, 1.0f, WHITE);
                        }
                    }

                    if (player.overdrive) {DrawModel(assets.odHighwayX, Vector3{0,0.001f,0},1,WHITE);}

                    for (int i = 0; i < 5; i++) {
                        if (heldFrets[i] || heldFretsAlt[i]) {
                            DrawModel(assets.smasherPressed, Vector3{ diffDistance - (float)(i), 0.01f, player.smasherPos }, 1.0f, WHITE);
                        }
                        else {
                            DrawModel(assets.smasherReg, Vector3{ diffDistance - (float)(i), 0.01f, player.smasherPos }, 1.0f, WHITE);
                        }
                    }
                    //DrawModel(assets.lanes, Vector3 {0,0.1f,0}, 1.0f, WHITE);

                    for (int i = 0; i < 4; i++) {
                        float radius = (i == (settingsMain.mirrorMode ? 2 : 1)) ? 0.05 : 0.02;

                        DrawCylinderEx(Vector3{ lineDistance - (float)i, 0, player.smasherPos + 0.5f }, Vector3{ lineDistance - i, 0, (highwayLength *1.5f) + player.smasherPos }, radius, radius, 15, Color{ 128,128,128,128 });
                    }

                    DrawModel(assets.smasherBoard, Vector3{ 0, 0.001f, 0 }, 1.0f, WHITE);
                }
                else {
                    float highwayPosShit = ((20) * (1 - settingsMain.highwayLengthMult));
                    DrawModel(assets.emhHighwaySides, Vector3{ 0,0,settingsMain.highwayLengthMult < 1.0f ? -(highwayPosShit* (0.875f)) : 0 }, 1.0f, WHITE);
                    DrawModel(assets.emhHighway, Vector3{ 0,0,settingsMain.highwayLengthMult < 1.0f ? -(highwayPosShit* (0.875f)) : 0 }, 1.0f, WHITE);
                    if (settingsMain.highwayLengthMult > 1.0f) {
                        DrawModel(assets.emhHighway, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-20 }, 1.0f, WHITE);
                        DrawModel(assets.emhHighwaySides, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-20 }, 1.0f, WHITE);
                        if (highwayLength > 23.0f) {
                            DrawModel(assets.emhHighway, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-40 }, 1.0f, WHITE);
                            DrawModel(assets.emhHighwaySides, Vector3{ 0,0,((highwayLength*1.5f)+player.smasherPos)-40 }, 1.0f, WHITE);
                        }
                    }
                    if (player.overdrive) {DrawModel(assets.odHighwayEMH, Vector3{0,0.001f,0},1,WHITE);}

                    for (int i = 0; i < 4; i++) {
                        if (heldFrets[i] || heldFretsAlt[i]) {
                            DrawModel(assets.smasherPressed, Vector3{ diffDistance - (float)(i), 0.01f, player.smasherPos }, 1.0f, WHITE);
                        }
                        else {
                            DrawModel(assets.smasherReg, Vector3{ diffDistance - (float)(i), 0.01f, player.smasherPos }, 1.0f, WHITE);

                        }
                    }
                    for (int i = 0; i < 3; i++) {
                        float radius = (i == 1) ? 0.03 : 0.01;
                        DrawCylinderEx(Vector3{ lineDistance - (float)i, 0, player.smasherPos + 0.5f }, Vector3{ lineDistance - (float)i, 0, (highwayLength *1.5f) + player.smasherPos }, radius,
                                       radius, 4.0f, Color{ 128, 128, 128, 128 });
                    }
                    DrawModel(assets.smasherBoardEMH, Vector3{ 0, 0.001f, 0 }, 1.0f, WHITE);
                }
                if (songList.songs[curPlayingSong].beatLines.size() >= 0) {
                    for (int i = curBeatLine; i < songList.songs[curPlayingSong].beatLines.size(); i++) {
                        if (songList.songs[curPlayingSong].beatLines[i].first >= songList.songs[curPlayingSong].music_start-1 && songList.songs[curPlayingSong].beatLines[i].first <= songList.songs[curPlayingSong].end) {
                            double relTime = ((songList.songs[curPlayingSong].beatLines[i].first - musicTime)) * settingsMain.trackSpeedOptions[settingsMain.trackSpeed]  * ( 11.5f / highwayLength);
                            if (relTime > 1.5) break;
                            float radius = songList.songs[curPlayingSong].beatLines[i].second ? 0.05f : 0.01f;
                            DrawCylinderEx(Vector3{ -diffDistance - 0.5f,0,player.smasherPos + (highwayLength * (float)relTime) }, Vector3{ diffDistance + 0.5f,0,player.smasherPos + (highwayLength * (float)relTime) }, radius, radius, 4, DARKGRAY);
                            if (relTime < -1 && curBeatLine < songList.songs[curPlayingSong].beatLines.size() - 1) {
                                curBeatLine++;

                            }
                        }
                    }
                }

                // DrawTriangle3D(Vector3{ 2.5f,0.0f,0.0f }, Vector3{ -2.5f,0.0f,0.0f }, Vector3{ -2.5f,0.0f,20.0f }, BLACK);
                // DrawTriangle3D(Vector3{ 2.5f,0.0f,0.0f }, Vector3{ -2.5f,0.0f,20.0f }, Vector3{ 2.5f,0.0f,20.0f }, BLACK);

                player.notes = (int)songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].notes.size();
                DrawModel(assets.odFrame, Vector3{ 0,1.0f,-0.3f }, 0.8f, WHITE);
                DrawModel(assets.odBar, Vector3{ 0,1.0f,-0.3f }, 0.8f, WHITE);
                DrawModel(assets.multFrame, Vector3{ 0,1.0f,-0.3f }, 0.8f, WHITE);
                DrawModel(assets.multBar, Vector3{ 0,1.0f,-0.3f }, 0.8f, WHITE);
                if (player.instrument == 1 || player.instrument == 3) {

                    DrawModel(assets.multCtr5, Vector3{ 0,1.0f,-0.3f }, 0.8f, WHITE);
                }
                else {

                    DrawModel(assets.multCtr3, Vector3{ 0,1.0f,-0.3f }, 0.8f, WHITE);
                }
                DrawModel(assets.multNumber, Vector3{ 0,1.0f,-0.3f }, 0.8f, WHITE);


                // DrawLine3D(Vector3{ 2.5f, 0.05f, 2.0f }, Vector3{ -2.5f, 0.05f, 2.0f}, WHITE);
                double songEnd = songList.songs[curPlayingSong].end == 0 ? audioManager.GetMusicTimeLength(audioManager.loadedStreams[0].handle) : songList.songs[curPlayingSong].end;
                Chart& curChart = songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff];
                if (!curChart.odPhrases.empty()) {

                    float odStart = (float)((curChart.odPhrases[curODPhrase].start - musicTime)) * settingsMain.trackSpeedOptions[settingsMain.trackSpeed] * (11.5f / highwayLength);
                    float odEnd = (float)((curChart.odPhrases[curODPhrase].end - musicTime)) * settingsMain.trackSpeedOptions[settingsMain.trackSpeed] * (11.5f / highwayLength);

                    // horrifying.

                    DrawCylinderEx(Vector3{ player.diff == 3 ? 2.7f : 2.2f,0,(float)(player.smasherPos + (highwayLength * odStart)) >= (highwayLength * 1.5f) + player.smasherPos ? (highwayLength * 1.5f) + player.smasherPos : (float)(player.smasherPos + (highwayLength * odStart)) }, Vector3{ player.diff == 3 ? 2.7f : 2.2f,0,(float)(player.smasherPos + (highwayLength * odEnd)) >= (highwayLength * 1.5f) + player.smasherPos ? (highwayLength * 1.5f) + player.smasherPos : (float)(player.smasherPos + (highwayLength * odEnd)) }, 0.07, 0.07, 10, RAYWHITE);
                    DrawCylinderEx(Vector3{ player.diff == 3 ? -2.7f : -2.2f,0,(float)(player.smasherPos + (highwayLength * odStart)) >= (highwayLength * 1.5f) + player.smasherPos ? (highwayLength * 1.5f) + player.smasherPos : (float)(player.smasherPos + (highwayLength * odStart)) }, Vector3{ player.diff == 3 ? -2.7f : -2.2f,0,(float)(player.smasherPos + (highwayLength * odEnd)) >= (highwayLength * 1.5f) + player.smasherPos ? (highwayLength * 1.5f) + player.smasherPos : (float)(player.smasherPos + (highwayLength * odEnd)) }, 0.07, 0.07, 10, RAYWHITE);

                }
                EndMode3D();
                BeginTextureMode(notes_tex);
                ClearBackground({0,0,0,0});
                BeginMode3D(camera);
                    for (int lane = 0; lane < (player.diff == 3 ? 5 : 4); lane++) {
                        for (int i = curNoteIdx[lane]; i < curChart.notes_perlane[lane].size(); i++) {

                            Note & curNote = curChart.notes[curChart.notes_perlane[lane][i]];
                            if (curNote.hit) {
                                player.totalOffset += curNote.HitOffset;
                            }

                            if (!curChart.odPhrases.empty()) {

                                if (curNote.time >= curChart.odPhrases[curODPhrase].start &&
                                    curNote.time <= curChart.odPhrases[curODPhrase].end &&
                                    !curChart.odPhrases[curODPhrase].missed) {
                                    if (curNote.hit) {
                                        if (curNote.hit && !curNote.countedForODPhrase) {
                                            curChart.odPhrases[curODPhrase].notesHit++;
                                            curNote.countedForODPhrase = true;
                                        }
                                    }
                                    curNote.renderAsOD = true;

                                }
                                if (curChart.odPhrases[curODPhrase].missed) {
                                    curNote.renderAsOD = false;
                                }
                                if (curChart.odPhrases[curODPhrase].notesHit ==
                                    curChart.odPhrases[curODPhrase].noteCount &&
                                    !curChart.odPhrases[curODPhrase].added && player.overdriveFill < 1.0f) {
                                    player.overdriveFill += 0.25f;
                                    if (player.overdriveFill > 1.0f) player.overdriveFill = 1.0f;
                                    if (player.overdrive) {
                                        player.overdriveActiveFill = player.overdriveFill;
                                        player.overdriveActiveTime = musicTime;
                                    }
                                    curChart.odPhrases[curODPhrase].added = true;
                                }
                            }
                            if (!curNote.hit && !curNote.accounted && curNote.time + 0.1 < musicTime+player.VideoOffset-player.InputOffset && !songEnded) {
                                curNote.miss = true;
                                player.MissNote();
                                if (!curChart.odPhrases.empty() && !curChart.odPhrases[curODPhrase].missed &&
                                    curNote.time >= curChart.odPhrases[curODPhrase].start &&
                                    curNote.time < curChart.odPhrases[curODPhrase].end)
                                    curChart.odPhrases[curODPhrase].missed = true;
                                curNote.accounted = true;
                            }


                            double relTime = ((curNote.time - musicTime)) *
                                             settingsMain.trackSpeedOptions[settingsMain.trackSpeed] * (11.5f / highwayLength);
                            double relEnd = (((curNote.time + curNote.len) - musicTime)) *
                                            settingsMain.trackSpeedOptions[settingsMain.trackSpeed] * (11.5f / highwayLength);
                            float notePosX = diffDistance - (1.0f *
                                                             (float) (settingsMain.mirrorMode ? (player.diff == 3 ? 4 : 3) -
                                                                                            curNote.lane
                                                                                          : curNote.lane));
                            if (relTime > 1.5) {
                                break;
                            }
                            if (relEnd > 1.5) relEnd = 1.5;
                            if (curNote.lift && !curNote.hit) {
                                // lifts						//  distance between notes
                                //									(furthest left - lane distance)
                                if (curNote.renderAsOD)                    //  1.6f	0.8
                                    DrawModel(assets.liftModelOD, Vector3{notePosX, 0, player.smasherPos +
                                                                                       (highwayLength *
                                                                                        (float) relTime)}, 1.1f, WHITE);
                                    // energy phrase
                                else
                                    DrawModel(assets.liftModel, Vector3{notePosX, 0, player.smasherPos +
                                                                                     (highwayLength * (float) relTime)},
                                              1.1f, WHITE);
                                // regular
                            } else {
                                // sustains
                                if ((curNote.len) > 0) {
                                    if (curNote.hit && curNote.held) {
                                        if (curNote.heldTime <
                                            (curNote.len * settingsMain.trackSpeedOptions[settingsMain.trackSpeed])) {
                                            curNote.heldTime = 0.0 - relTime;
                                            player.sustainScoreBuffer[curNote.lane] =
                                                    (float) (curNote.heldTime / curNote.len) * (12 * curNote.beatsLen) *
                                                    player.multiplier(player.instrument);
                                            if (relTime < 0.0) relTime = 0.0;
                                        }
                                        if (relEnd <= 0.0) {
                                            if (relTime < 0.0) relTime = relEnd;
                                            player.score += player.sustainScoreBuffer[curNote.lane];
                                            player.sustainScoreBuffer[curNote.lane] = 0;
                                            curNote.held = false;
                                        }
                                    } else if (curNote.hit && !curNote.held) {
                                        relTime = relTime + curNote.heldTime;
                                    }

                                    /*Color SustainColor = Color{ 69,69,69,255 };
                                    if (curNote.held) {
                                        if (od) {
                                            Color SustainColor = Color{ 217, 183, 82 ,255 };
                                        }
                                        Color SustainColor = Color{ 172,82,217,255 };
                                    }*/
                                    float sustainLen =
                                            (highwayLength * (float) relEnd) - (highwayLength * (float) relTime);
                                    Matrix sustainMatrix = MatrixMultiply(MatrixScale(1, 1, sustainLen),
                                                                          MatrixTranslate(notePosX, 0.01f,
                                                                                          player.smasherPos +
                                                                                          (highwayLength *
                                                                                           (float) relTime) +
                                                                                          (sustainLen / 2.0f)));
                                    BeginBlendMode(BLEND_ALPHA);
                                    if (curNote.held && !curNote.renderAsOD) {
                                        DrawMesh(sustainPlane, assets.sustainMatHeld, sustainMatrix);
                                        DrawCube(Vector3{notePosX, 0.1, player.smasherPos}, 0.4f, 0.2f, 0.4f,
                                                 player.accentColor);
                                        //DrawCylinderEx(Vector3{ notePosX, 0.05f, player.smasherPos + (highwayLength * (float)relTime) }, Vector3{ notePosX,0.05f, player.smasherPos + (highwayLength * (float)relEnd) }, 0.1f, 0.1f, 15, player.accentColor);
                                    }
                                    if (curNote.renderAsOD && curNote.held) {
                                        DrawMesh(sustainPlane, assets.sustainMatHeldOD, sustainMatrix);
                                        DrawCube(Vector3{notePosX, 0.1, player.smasherPos}, 0.4f, 0.2f, 0.4f, WHITE);
                                        //DrawCylinderEx(Vector3{ notePosX, 0.05f, player.smasherPos + (highwayLength * (float)relTime) }, Vector3{ notePosX,0.05f, player.smasherPos + (highwayLength * (float)relEnd) }, 0.1f, 0.1f, 15, Color{ 255, 255, 255 ,255 });
                                    }
                                    if (!curNote.held && curNote.hit || curNote.miss) {

                                        DrawMesh(sustainPlane, assets.sustainMatMiss, sustainMatrix);
                                        //DrawCylinderEx(Vector3{ notePosX, 0.05f, player.smasherPos + (highwayLength * (float)relTime) }, Vector3{ notePosX,0.05f, player.smasherPos + (highwayLength * (float)relEnd) }, 0.1f, 0.1f, 15, Color{ 69,69,69,255 });
                                    }
                                    if (!curNote.hit && !curNote.accounted && !curNote.miss) {
                                        if (curNote.renderAsOD) {
                                            DrawMesh(sustainPlane, assets.sustainMatOD, sustainMatrix);
                                            //DrawCylinderEx(Vector3{ notePosX, 0.05f, player.smasherPos + (highwayLength * (float)relTime) }, Vector3{ notePosX,0.05f, player.smasherPos + (highwayLength * (float)relEnd) }, 0.1f, 0.1f, 15, Color{ 200, 200, 200 ,255 });
                                        } else {
                                            DrawMesh(sustainPlane, assets.sustainMat, sustainMatrix);
                                            /*DrawCylinderEx(Vector3{notePosX, 0.05f,
                                                                    player.smasherPos + (highwayLength * (float)relTime) },
                                                           Vector3{ notePosX, 0.05f,
                                                                    player.smasherPos + (highwayLength * (float)relEnd) }, 0.1f, 0.1f, 15,
                                                           player.accentColor);*/
                                        }
                                    }
                                    EndBlendMode();

                                    // DrawLine3D(Vector3{ diffDistance - (1.0f * curNote.lane),0.05f,smasherPos + (12.5f * (float)relTime) }, Vector3{ diffDistance - (1.0f * curNote.lane),0.05f,smasherPos + (12.5f * (float)relEnd) }, Color{ 172,82,217,255 });
                                }
                                // regular notes
                                if (((curNote.len) > 0 && (curNote.held || !curNote.hit)) ||
                                    ((curNote.len) == 0 && !curNote.hit)) {
                                    if (curNote.renderAsOD) {
                                        if ((!curNote.held && !curNote.miss) || !curNote.hit) {
                                            DrawModel(assets.noteModelOD, Vector3{notePosX, 0, player.smasherPos +
                                                                                               (highwayLength *
                                                                                                (float) relTime)}, 1.1f,
                                                      WHITE);
                                        }

                                    } else {
                                        if ((!curNote.held && !curNote.miss) || !curNote.hit) {
                                            DrawModel(assets.noteModel, Vector3{notePosX, 0, player.smasherPos +
                                                                                             (highwayLength *
                                                                                              (float) relTime)}, 1.1f,
                                                      WHITE);
                                        }

                                    }

                                }
                                assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                            }
                            if (curNote.miss) {
                                DrawModel(curNote.lift ? assets.liftModel : assets.noteModel,
                                          Vector3{notePosX, 0, player.smasherPos + (highwayLength * (float) relTime)},
                                          1.0f, RED);
                                if (audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle) <
                                    curNote.time + 0.4 && settingsMain.missHighwayColor) {
                                    assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = RED;
                                } else {
                                    assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = player.accentColor;
                                }
                            }
                            if (curNote.hit && audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle) <
                                               curNote.hitTime + 0.15f) {
                                DrawCube(Vector3{ notePosX, 0.125 , player.smasherPos }, 1.0f, 0.25f, 0.5f,
                                         curNote.perfect ? Color{255, 215, 0, 150} : Color{255, 255, 255, 150});
                                if (curNote.perfect) {
                                    DrawCube(Vector3{player.diff == 3 ? 3.3f : 2.8f, 0, player.smasherPos}, 1.0f, 0.01f,
                                             0.5f, ORANGE);

                                }
                            }
                            // DrawText3D(assets.rubik, TextFormat("%01i", combo), Vector3{2.8f, 0, smasherPos}, 32, 0.5,0,false,FC ? GOLD : (combo <= 3) ? RED : WHITE);


                            if (relEnd < -1 && curNoteIdx[lane] < curChart.notes_perlane[lane].size() - 1)
                                curNoteIdx[lane] = i + 1;


                        }

                    }
                    EndMode3D();
                    EndTextureMode();



                    DrawTextureRec(notes_tex.texture, { 0, 0, (float)notes_tex.texture.width, (float)-notes_tex.texture.height },{0,0}, WHITE);
                if (!curChart.odPhrases.empty() && curODPhrase<curChart.odPhrases.size() - 1 && musicTime>curChart.odPhrases[curODPhrase].end && (curChart.odPhrases[curODPhrase].added ||curChart.odPhrases[curODPhrase].missed)) {
                    curODPhrase++;
                }

                int songPlayed = audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle);
                int songLength = songList.songs[curPlayingSong].end == 0 ? audioManager.GetMusicTimeLength(audioManager.loadedStreams[0].handle) : songList.songs[curPlayingSong].end;
                int playedMinutes = songPlayed/60;
                int playedSeconds = songPlayed % 60;
                int songMinutes = songLength/60;
                int songSeconds = songLength % 60;
                GuiSetStyle(PROGRESSBAR, BORDER_WIDTH, 0);
                GuiSetStyle(PROGRESSBAR, BASE_COLOR_NORMAL, ColorToInt(player.FC ? GOLD : player.accentColor));
                GuiSetStyle(PROGRESSBAR, BASE_COLOR_FOCUSED, ColorToInt(player.FC ? GOLD : player.accentColor));
                GuiSetStyle(PROGRESSBAR, BASE_COLOR_DISABLED, ColorToInt(player.FC ? GOLD : player.accentColor));
                GuiSetStyle(PROGRESSBAR, BASE_COLOR_PRESSED, ColorToInt(player.FC ? GOLD : player.accentColor));

                float floatSongLength = audioManager.GetMusicTimePlayed(audioManager.loadedStreams[0].handle);
                GuiProgressBar(Rectangle {0,(float)GetScreenHeight()-u.hinpct(0.005f),(float)GetScreenWidth(),u.hinpct(0.01f)}, "", "", &floatSongLength, 0, (float)songLength);



                if (player.paused) {
                    DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(), Color{0,0,0,64});
                    DrawTextEx(assets.rubikBoldItalic, "PAUSED", {(GetScreenWidth()/2) - (MeasureTextEx(assets.rubikBoldItalic, "PAUSED",u.hinpct(0.1f),0).x/2), u.hpct(0.05f)}, u.hinpct(0.1f), 0, WHITE);

                    if (GuiButton({((float) GetScreenWidth() / 2) - 100, ((float) GetScreenHeight() / 2) - 150, 200, 60}, "Resume")) {
                        audioManager.playStreams();
                        player.paused = false;
                    }
                    if (GuiButton({((float) GetScreenWidth() / 2) - 100, ((float) GetScreenHeight() / 2)-30, 200, 60}, "Restart")) {
                        for (Note& note : songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].notes) {
                            note.accounted = false;
                            note.hit = false;
                            note.miss = false;
                            note.held = false;
                            note.heldTime = 0;
                            note.hitTime = 0;
                            note.perfect = false;
                            note.countedForODPhrase = false;
                        }
                        for (odPhrase& phrase : songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].odPhrases) {
                            phrase.missed = false;
                            phrase.notesHit = 0;
                            phrase.added = false;
                        }
                        player.overdrive = false;
                        player.overdriveFill = 0.0f;
                        player.overdriveActiveFill = 0.0f;
                        player.overdriveActiveTime = 0.0;
                        curODPhrase = 0;
                        curNoteIdx = { 0,0,0,0,0 };
                        curBeatLine = 0;
                        player.resetPlayerStats();
                        assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.multBar.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr3.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr5.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                        assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                        for (auto& stream : audioManager.loadedStreams) {
                            audioManager.restartStreams();
                            player.paused = false;
                        }

                        startedPlayingSong = GetTime();
                        
                    }
                    if (GuiButton({((float) GetScreenWidth() / 2) - 100, ((float) GetScreenHeight() / 2) + 90, 200, 60}, "Drop Out")) {
                        glfwSetKeyCallback(glfwGetCurrentContext(), origKeyCallback);
                        glfwSetGamepadStateCallback(origGamepadCallback);
                        // notes = songList.songs[curPlayingSong].parts[instrument]->charts[diff].notes.size();
                        // notes = songList.songs[curPlayingSong].parts[instrument]->charts[diff];
                        menu.SwitchScreen(RESULTS);
                        menu.ChosenSong.LoadAlbumArt(menu.ChosenSong.albumArtPath);
                        player.overdrive = false;
                        player.overdriveFill = 0.0f;
                        player.overdriveActiveFill = 0.0f;
                        player.overdriveActiveTime = 0.0;
                        curODPhrase = 0;
                        player.paused = false;
                        assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = assets.highwayTexture;
                        assets.multBar.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr3.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.multCtr5.materials[0].maps[MATERIAL_MAP_EMISSION].texture = assets.odMultFill;
                        assets.expertHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                        assets.emhHighway.materials[0].maps[MATERIAL_MAP_ALBEDO].color = DARKGRAY;
                        midiLoaded = false;
                        isPlaying = false;
                        songEnded = true;
                        songList.songs[curPlayingSong].parts[player.instrument]->charts[player.diff].resetNotes();
                        player.quit = true;
                    }
                }

                break;

            }
            case RESULTS: {
                
                if (streamsLoaded) {
                    audioManager.unloadStreams();
                    streamsLoaded = false;
                }


                menu.DrawAlbumArtBackground(menu.ChosenSong.albumArtBlur, assets);
                menu.showResults(player, assets);
                if (GuiButton({ 0,0,60,60 }, "<")) {
                    player.quit = false;
                    menu.SwitchScreen(SONG_SELECT);
                }
                break;
            }
        }
        EndDrawing();

        //SwapScreenBuffer();

        currentTime = GetTime();
        updateDrawTime = currentTime - previousTime;

        if (targetFPS > 0)
        {
            waitTime = (1.0f / (float)targetFPS) - updateDrawTime;
            if (waitTime > 0.0)
            {
                WaitTime((float)waitTime);
                currentTime = GetTime();
                deltaTime = (float)(currentTime - previousTime);
            }
        }
        else deltaTime = (float)updateDrawTime;

        previousTime = currentTime;
    }
    CloseWindow();
    return 0;
}
