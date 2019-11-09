#include <stdafx.h>
#include <string>
#include <game/Global.h>
#include <game/Input.h>
#include <game/SDKHooks.h>
#include <mod/Mod.h>
#include <HookLoaderInternal.h>
#include <mod/ModFunctions.h>
#include <mod/MathFunctions.h>
#include <util/JsonConfig.h>
#include <assets/AssetFunctions.h>
#include <assets/FObjectSpawnParameters.h>
#include "../Detours/src/detours.h"
#include "../SatisfactorySDK/SDK.hpp"
#include <memory/MemoryObject.h>
#include <memory/MemoryFunctions.h>
#include <math.h>

bool SML::debugOutput = false;

using namespace SML::Mod;
using namespace SML::Objects;

#define SML_VERSION "1.0.2"
#define MOD_NAME "DownsizeUpsize"

#define LOG(msg) SML::Utility::infoMod(MOD_NAME, msg)
#define INFO(msg) LOG(msg)
#define WARN(msg) SML::Utility::warningMod(MOD_NAME, msg)
#define ERR(msg) SML::Utility::errorMod(MOD_NAME, msg)

json config = SML::Utility::JsonConfig::load(MOD_NAME, {
	{ "Downsize percentage", 0.75f },
	{ "Upsize percentage", 1.25f }
	});

AFGPlayerController* player;
SDK::AFGCharacterPlayer* playerCharacter;

float currentPlayerScale = 1;
float targetPlayerScale = 1;
float clampDistance = 0.01f;
bool updatePlayerScale = false;
float playerScaleSpeed = 0.8f;

// Default character settings:
float baseCustomMovementSpeed;
float baseOutOfWaterStepHeight;
float baseStepHeight;
float baseSwimSpeed;
float baseWalkSpeed;
float baseWalkSpeedCrouched;
float baseJumpZVelocity;
float baseGravityScale;
bool sprinting = false;

float Lerp(float a, float b, float s) {
	return (b - a) * s + a;
}

float Length(SDK::FVector vector) {
	return sqrt(vector.X * vector.X + vector.Y * vector.Y + vector.Z * vector.Z);
}

void ScalePlayer(float scale) {
	targetPlayerScale = min(max(scale, 0.1f), 100.0f);
	updatePlayerScale = true;
}

Mod::Info modInfo{
	SML_VERSION,
	MOD_NAME,
	"1.0.0",
	"Mod that makes it possible to change the size of the player.",
	"MarcasRealAccount",
	{}
};

class Downsizing : public Mod {

	void beginPlay(Functions::ModReturns* modReturns, AFGPlayerController* playerIn) {
		LOG("Got Player");
		player = playerIn;
		playerCharacter = Functions::getPlayerCharacter();
		SDK::UCharacterMovementComponent* movement = playerCharacter->CharacterMovement;
		baseCustomMovementSpeed = movement->MaxCustomMovementSpeed;
		baseOutOfWaterStepHeight = movement->MaxOutOfWaterStepHeight;
		baseStepHeight = movement->MaxStepHeight;
		baseSwimSpeed = movement->MaxSwimSpeed;
		baseWalkSpeed = movement->MaxWalkSpeed;
		baseWalkSpeedCrouched = movement->MaxWalkSpeedCrouched;
		baseJumpZVelocity = movement->JumpZVelocity;
		baseGravityScale = movement->GravityScale;
	}
	
	static void SprintPressed(SDK::AFGCharacterPlayer* playerIn) {
		sprinting = true;
		updatePlayerScale = true;
	}

	static void SprintReleased(SDK::AFGCharacterPlayer* playerIn) {
		sprinting = false;
		updatePlayerScale = true;
	}

public:
	Downsizing() : Mod(modInfo) {
	}

	void setup() override {
		using namespace std::placeholders;

		SDK::InitSDK();

		void* originalSprintPressedFunction = Detours::DetourFindFunction("FactoryGame-Win64-Shipping.exe", "AFGCharacterPlayer::SprintPressed");
		void* originalSprintReleasedFunction = Detours::DetourFindFunction("FactoryGame-Win64-Shipping.exe", "AFGCharacterPlayer::SprintReleased");
		Detours::DetourTransactionBegin();
		Detours::DetourAttach(&originalSprintPressedFunction, &SprintPressed);
		Detours::DetourAttach(&originalSprintReleasedFunction, &SprintReleased);
		Detours::DetourTransactionCommit();

		::subscribe<&AFGPlayerController::BeginPlay>(std::bind(&Downsizing::beginPlay, this, _1, _2));

		::subscribe<&PlayerInput::InputKey>([this](Functions::ModReturns* modReturns, PlayerInput* playerInput, FKey key, InputEvent event, float amount, bool gamePad) {
			if (GetAsyncKeyState('G')) {
				ScalePlayer(targetPlayerScale * 0.8f);
			}
			if (GetAsyncKeyState('H')) {
				ScalePlayer(targetPlayerScale * 1.25f);
			}
			return false;
		});

		::subscribe<&UWorld::Tick>([this](Functions::ModReturns*, UWorld* world, ELevelTick tick, float delta) {
			if (updatePlayerScale && playerCharacter != nullptr) {
				currentPlayerScale = Lerp(currentPlayerScale, targetPlayerScale, delta * playerScaleSpeed);
				if (currentPlayerScale >= targetPlayerScale - clampDistance && currentPlayerScale <= targetPlayerScale + clampDistance) {
					currentPlayerScale = targetPlayerScale;
					updatePlayerScale = false;
				}

				playerCharacter->SetActorScale3D({ currentPlayerScale, currentPlayerScale, currentPlayerScale });

				SDK::UCharacterMovementComponent* movement = playerCharacter->CharacterMovement;
				movement->MaxCustomMovementSpeed = baseCustomMovementSpeed * currentPlayerScale;
				movement->MaxOutOfWaterStepHeight = baseOutOfWaterStepHeight * currentPlayerScale;
				movement->MaxStepHeight = baseStepHeight * currentPlayerScale;
				movement->MaxSwimSpeed = baseSwimSpeed * currentPlayerScale;
				movement->MaxWalkSpeed = baseWalkSpeed * currentPlayerScale * (sprinting && movement->IsMovingOnGround() ? 1.8f : 1);
				movement->MaxWalkSpeedCrouched = baseWalkSpeedCrouched * currentPlayerScale;
				movement->JumpZVelocity = baseJumpZVelocity * currentPlayerScale;
				movement->GravityScale = baseGravityScale * currentPlayerScale;
				LOG("Speed: " + std::to_string(Length(movement->Velocity)));
			}
		});

		LOG("Finished " + std::string(MOD_NAME) + " setup!");
	}

	void postSetup() override {
	}

	~Downsizing() {
		LOG(std::string(MOD_NAME) + " finished cleanup!");
	}
};

MOD_API Mod* ModCreate() {
	return new Downsizing();
}