#include "xbox_diag_store.h"
#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <stddef.h>
#include <string.h>
using namespace Adafruit_LittleFS_Namespace;

static const char *const kFile = "/xauthdiag.bin";
static const uint32_t kMagic = 0x58444731UL;

static uint32_t checksum(const XboxDiagRecord &r)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(&r);
	uint32_t hash = 2166136261UL;
	for (size_t i = 0; i < offsetof(XboxDiagRecord, checksum); ++i) {
		hash ^= p[i];
		hash *= 16777619UL;
	}
	return hash;
}

bool xboxDiagRead(XboxDiagRecord *out)
{
	if (!out)
		return false;
	File f(InternalFS);
	if (!f.open(kFile, FILE_O_READ))
		return false;
	XboxDiagRecord r = {};
	const int n = f.read(reinterpret_cast<uint8_t *>(&r), sizeof r);
	f.close();
	if (n != sizeof r || r.magic != kMagic || r.version != 1 ||
	    r.checksum != checksum(r))
		return false;
	*out = r;
	return true;
}

void xboxDiagTick()
{
	// Checkpoint changed auth counters at most once every five seconds.
	static uint32_t lastCheck = 0;
	static uint32_t lastSignature = 0;
	static bool initialized = false;
	const uint32_t now = millis();
	if (now - lastCheck < 5000)
		return;
	lastCheck = now;
	XinputAuthDiag auth = {};
	xinputAuthDiagSnapshot(&auth);
	if (!auth.setupCount && !auth.authResets && !auth.cryptoFailures)
		return;
	const uint8_t *p = reinterpret_cast<const uint8_t *>(&auth);
	uint32_t sig = 2166136261UL;
	for (size_t i = 0; i < sizeof auth; ++i) {
		sig ^= p[i];
		sig *= 16777619UL;
	}
	if (initialized && sig == lastSignature)
		return;
	XboxDiagRecord old = {};
	const bool haveOld = xboxDiagRead(&old);
	XboxDiagRecord r = {};
	r.magic = kMagic;
	r.version = 1;
	r.writes = haveOld ? old.writes + 1 : 1;
	r.uptimeMs = now;
	r.auth = auth;
	r.checksum = checksum(r);
	File f(InternalFS);
	if (!f.open(kFile, FILE_O_WRITE))
		return;
	const size_t n =
		f.write(reinterpret_cast<const uint8_t *>(&r), sizeof r);
	f.close();
	if (n == sizeof r) {
		lastSignature = sig;
		initialized = true;
	}
}
