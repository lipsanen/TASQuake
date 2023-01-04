#include "libtasquake/utils.hpp"
#include <cstdarg>
#include <cstdio>

static TASQuake::LibTASQuakeSettings lib_settings;
static char FORMAT_BUFFER[1024];

TASQuake::LibTASQuakeSettings::LibTASQuakeSettings() = default;

void TASQuake::InitSettings(const LibTASQuakeSettings& settings) {
	lib_settings = settings;
}

bool TASQuake::IsConvar(char* text) {
	if(lib_settings.isConvar == nullptr)
		return true;
	return lib_settings.isConvar(text);
}

void TASQuake::Log(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	if(lib_settings.logger == nullptr) {
		vprintf(fmt, args);
	} else {
		vsprintf(FORMAT_BUFFER, fmt, args);
    	lib_settings.logger(FORMAT_BUFFER);
	}
	va_end(args);
}

int TASQuake::GetNumBackups() {
	if(lib_settings.numBackupsFunc == nullptr)
		return 100;
	return lib_settings.numBackupsFunc();
}


bool TASQuake::GamePaused() {
	if(lib_settings.gamePausedFunc == nullptr) {
		return true;
	}

	return lib_settings.gamePausedFunc();
}
