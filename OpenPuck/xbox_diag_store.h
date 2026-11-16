#pragma once
#include "xinput_auth.h"
#include <stdint.h>

// Persistent Xbox authentication counters. No keys or challenge bodies.
struct XboxDiagRecord {
	uint32_t magic, version, writes, uptimeMs;
	XinputAuthDiag auth;
	uint32_t checksum;
};
void xboxDiagTick();
bool xboxDiagRead(XboxDiagRecord *out);
