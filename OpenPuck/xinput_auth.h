#pragma once
#include <Adafruit_TinyUSB.h>

void xinputAuthPrepare();
void xinputAuthReset();
const char *xinputAuthSerial();
bool xinputAuthControl(uint8_t rhport, uint8_t stage,
		       const tusb_control_request_t *request);

// Only the dedicated worker may call the singleton crypto library.
void xinputAuthProcess();
bool xinputAuthBusy();
void xinputAuthYield();
bool xinputAuthReceived(uint16_t length);
bool xinputAuthStartWorker();
void xinputAuthNotify();
void xinputAuthBoardSerial(char serial[13]);
uint32_t xinputAuthLock();
void xinputAuthUnlock(uint32_t mask);
// RAM-only diagnostics. No USB descriptors, secrets, or timing are changed.
// Read with a debugger or expose through an existing configuration channel.
struct XinputAuthDiagnostics {
	uint32_t resets;
	uint32_t identifyRequests;
	uint32_t initChallenges;
	uint32_t verifyChallenges;
	uint32_t initSuccess;
	uint32_t verifySuccess;
	uint32_t cryptoFailures;
	uint32_t receiveFailures;
	uint32_t workerStartFailures;
	uint32_t rejectedRequests;
	uint8_t currentState;
	bool workerReady;
};
void xinputAuthDiagSnapshot(XinputAuthDiagnostics *out);
