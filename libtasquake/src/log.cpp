#include "libtasquake/log.hpp"
#include <cstdarg>
#include <cstdio>

static TASQuake::LogFunc func = nullptr;
static char FORMAT_BUFFER[1024];

void TASQuake::SetLog(LogFunc f) {
    func = f;
}

void TASQuake::Log(const char* fmt, ...) {
	if(func == nullptr)
        return;

	va_list args;
	va_start(args, fmt);
	vsprintf(FORMAT_BUFFER, fmt, args);

    func(FORMAT_BUFFER);
}
