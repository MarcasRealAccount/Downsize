/*
  ____  __  __ _
 / ___||  \/  | |
 \___ \| |\/| | |
  ___) | |  | | |___
 |____/|_|  |_|_____|

 SatisfactoryModLoader is a tool to load dll mods into satisfactory.
 To get started with modding, take a look at BaseMod/ExampleMod.cpp.
 If you have any questions, please contact SuperCoder79 on the modding discord.

 Known issues:
 * Everything crashes 10000% more than it should
 * SML 1.0.0 has been delayed by 2000 years
 * People still can't install this properly (even though there's instructions everywhere ffs)
*/
#define WIN32_LEAN_AND_MEAN
#include "../SatisfactorySdk/SDK.hpp"
#include <stdafx.h>
#include <SatisfactoryModLoader.h>
#include <string>
#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <functional>
#include <chrono>
#include <thread>
#include <assets/AssetLoader.h>
#include <util/Utility.h>
#include <util/JsonConfig.h>
#include <util/EnvironmentValidity.h>
#include <mod/Hooks.h>
#include <mod/Coremods.h>
#include <mod/ModFunctions.h>

namespace SML {
	extern "C" const SML_API char SML::smlVersion[] = "1.0.2";
	const int targetVersion = 106504; //CL of Satisfactory
	static const char* logName = "SatisfactoryModLoader.log";
	Mod::ModHandler modHandler;
	bool loadConsole = true;
	bool debugOutput = false;
	bool supressErrors = false;
	bool chatCommands = true;
	bool crashReporter = true;
	bool unsafeMode = false;

	// Main DLL for loading mod DLLs
	void startSML() {
		// launch the game's internal console and hook into it
		Utility::logFile.open(logName, std::ios_base::out);
		AllocConsole();
		ShowWindow(GetConsoleWindow(), SW_HIDE);
		FILE* fp;
		freopen_s(&fp, "CONOIN$", "r", stdin);
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);

		Utility::info("Attached SatisfactoryModLoader v", modLoaderVersion, " to Satisfactory");

		// load up all of the configuration information
		readConfig();
		Utility::info("Validating system files...");
		Utility::checkForValidEnvironment();
		if (unsafeMode) {
			Utility::invalidateEnvironment();
			Utility::warning("Unsafe mode enabled! SML will do things that are generally not allowed, so you can't report bugs!");
		}

		//make sure that SML's target and satisfactory's versions are the same
		Utility::checkVersion(targetVersion);
		//load the console if enabled by the config
		if (loadConsole) {
			ShowWindow(GetConsoleWindow(), SW_SHOW);
		}

		// load sdk and assetloader
		SDK::InitSDK();
		Assets::AssetLoader::init();
		Utility::info("Initialized SDK");


		// get path
		char p[MAX_PATH];
		GetModuleFileNameA(NULL, p, MAX_PATH);

		//load coremods
		Mod::startLoadingCoremods(p);

		// load mods
		modHandler.loadMods(p);
		if (modHandler.mods.size() != 0) {
			modHandler.setupMods();
			modHandler.checkDependencies();
			modHandler.postSetupMods();
		}
		Mod::Functions::broadcastEvent("beforeHooks");
		Mod::Hooks::hookFunctions();
		Mod::Functions::broadcastEvent("afterHooks");
		modHandler.currentStage = Mod::GameStage::INITIALIZING;
			
		// log mod list size
		size_t listSize = modHandler.mods.size();
		Utility::info("Loaded ", listSize, " mod", (listSize > 1 || listSize == 0 ? "s" : ""));

		//Display info about registries
		if (debugOutput) {
			Utility::info("Registered ", modHandler.commandRegistry.size(), " Command", (modHandler.commandRegistry.size() > 1 || modHandler.commandRegistry.size() == 0 ? "s" : ""));
			Utility::info("Registered ", modHandler.APIRegistry.size(), " API function", (modHandler.APIRegistry.size() > 1 || modHandler.APIRegistry.size() == 0 ? "s" : ""));
			Utility::info("Registered ", modHandler.eventRegistry.size(), " Custom event", (modHandler.eventRegistry.size() > 1 || modHandler.eventRegistry.size() == 0 ? "s" : ""));
		}

		//display condensed form of mod information
		std::string modList = "[";
		for (auto&& mod : modHandler.mods) {
			modList.append(mod->info.name + "@" + mod->info.version + ", ");
		}

		if (listSize > 0) {
			Utility::info("Loaded mods: ", modList.substr(0, modList.length() - 2), "]");
		}

		Utility::info("SatisfactoryModLoader initialization complete. Launching Satisfactory...");
	}

	//read the config file
	void readConfig() {
		Utility::info("Finding config file...");
		json config = Utility::JsonConfig::load("SatisfactoryModLoader", {
			{"Console", true},
			{"Debug" , false},
			{"Supress Errors", false},
			{"Chat Commands", true},
			{"Disable Crash Reporter", true},
			{"Enable Unsafe Mode", false}
		}, false);

		loadConsole = config["Console"].get<bool>();
		debugOutput = config["Debug"].get<bool>();
		supressErrors = config["Supress Errors"].get<bool>();
		chatCommands = config["Chat Commands"].get<bool>();
		crashReporter = config["Disable Crash Reporter"].get<bool>();
		unsafeMode = config["Enable Unsafe Mode"].get<bool>();
	}

	//cleans up when the program is killed
	void cleanup() {
		modHandler.mods.clear();

		char path_c[MAX_PATH];
		GetModuleFileNameA(NULL, path_c, MAX_PATH);
		std::string path = std::string(path_c);			 // ..\FactoryGame\Binaries\Win64\.exe
		path = path.substr(0, path.find_last_of("/\\")); // ..\FactoryGame\Binaries\Win64
		path = path.substr(0, path.find_last_of("/\\")); // ..\FactoryGame\Binaries
		path = path.substr(0, path.find_last_of("/\\")); // ..\FactoryGame
		Utility::enableCrashReporter(path);

		if (!Utility::isEnvironmentValid) {
			Utility::error("This log is not valid for bug reports because you have installed coremods, a mod does memory editing, or unsafe mode is enabled in the config. Fix these issues before submitting a crash report!");
		}

		Utility::info("SML shutting down...");
		Utility::logFile.flush();
		Utility::logFile.close();
	}
}