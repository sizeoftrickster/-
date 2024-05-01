#include "Main.h"
#include "Direct3D9Hook.h"
#include "Gta_sa.hpp"
#include <fstream>

AsiPlugin::AsiPlugin() {
	//AllocConsole();
	//freopen("CONOUT$", "w", stdout);
	savesettingsHook.set_cb([this](auto&&... args) { return AsiPlugin::SaveSettingsHooked(args...); });
	savesettingsHook.install();
}

AsiPlugin::~AsiPlugin() {
	if ( mainLoop.joinable() )
		mainLoop.join();
	mainwndprocHook.remove();
}

void AsiPlugin::SaveSettingsHooked( const decltype( savesettingsHook )& hook, int file ) {
	static bool isInitialized = false;
	if ( !isInitialized ) {
		dirFileConf = reinterpret_cast<char*>( 0xB71A60 );
		nameFileConf = reinterpret_cast<char*>( 0x865674 );
		gtasasetDirectory << dirFileConf << "\\" << nameFileConf;

		mainwndprocHook.set_cb([this](auto&&... args) { return AsiPlugin::MainWndProcHooked(args...); });
		mainwndprocHook.install();

		directxHook = std::make_unique<Direct3D9Hook>();

		mainLoop = std::thread( &AsiPlugin::MainLoop, this );
		mainLoop.detach();
		isInitialized = true;
	}

	std::thread( &AsiPlugin::UpdateKeys, this ).detach();

	return hook.get_trampoline()(file);
}

LRESULT AsiPlugin::MainWndProcHooked( const decltype( mainwndprocHook )& hook, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	if ( !isMenuOpened && uMsg == WM_KEYDOWN ) {
		if ( wParam == VK_OEM_PLUS ) {
			isEnabled ^= true;
		}
		if ( isEnabled ) {
			if ( GTA::IsPedDriving() ) {
				for ( auto i : onCarKeys ) {
					if ( wParam == i ) {
						isEnabled = false;
						break;
					}	
				}
			} else {
				for ( auto i : onFootKeys ) {
					if ( wParam == i ) {
						isEnabled = false;
						break;
					}	
				}
			}
		}
	}
	return hook.get_trampoline()( hWnd, uMsg, wParam, lParam );
}

void AsiPlugin::MainLoop() {
	while ( true ) {
		if ( !isMenuOpened && isEnabled ) {
			if ( GTA::IsPedDriving() ) {
				GTA::SetGameKey( GTA::KeysNum::AccelerateCar, GTA::KeysValue::OnCar );
			} else {
				GTA::SetGameKey( GTA::KeysNum::Forward, GTA::KeysValue::OnFoot );
			}
		}

		std::this_thread::sleep_for( std::chrono::milliseconds( 0 ) );
	}
}

void AsiPlugin::UpdateKeys() {
	onFootKeys.clear();
	onCarKeys.clear();

	auto foot = GTA::GetKeysNumberFromGtaSaSet( gtasasetDirectory.str(), onFootKeysAddresses);
	auto car  = GTA::GetKeysNumberFromGtaSaSet( gtasasetDirectory.str(), onCarKeysAddresses );
	for ( int value : foot ) {
		if ( value != 32 ) {
			onFootKeys.push_back( value );
		}
	}
	for ( int value : car ) {
		if ( value != 32 ) {
			onCarKeys.push_back( value );
		}
	} 
}