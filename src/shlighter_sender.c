#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef main
#undef main
#endif

#define BLEN 32
#define ALEN 16
#define DLEN 4
#define ADDRESS "192.168.2.105"
#define PORT 51721
#define FILE_HOST "host.txt"
#define THRESHOLD_FREE 10000
#define THRESHOLD_LOCK 20000

typedef enum {
	MOVE_LINE_UP,
	MOVE_LINE_RIGHT,
	MOVE_LINE_DOWN,
	MOVE_LINE_LEFT,

	MOVE_FULL_UP,
	MOVE_FULL_RIGHT,
	MOVE_FULL_DOWN,
	MOVE_FULL_LEFT,

	MOVE_STEP_UP,
	MOVE_STEP_RIGHT,
	MOVE_STEP_DOWN,
	MOVE_STEP_LEFT,

	MAKE_DOTS_0,
	MAKE_DOTS_1,
	MAKE_DOTS_2,
	MAKE_DOTS_3,

	MOVE_IN_0,
	MOVE_IN_1
} Command;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static bool run = true;

static TCPsocket socket = NULL;

static SDL_Color color = {0, 0, 0, 255};
static SDL_GameController* controller = NULL;
static bool holdLX = false, holdLY = false, holdRX = false, holdRY = false, holdLT = false, holdRT = false;

static bool init(void);
static void updateController(void);
static void readHost(char* addr, Uint16* port);
static void cleanup(void);

static void handleEvent(const SDL_Event* event);
static void eventButtonDown(const SDL_ControllerButtonEvent* button);
static void eventButtonUp(const SDL_ControllerButtonEvent* button);
static void updateColor(void);
static void eventAxis(const SDL_ControllerAxisEvent* axis);
static void eventStick(Sint16 valX, Sint16 valY, bool* holdX, bool* holdY, Command cmd);
static void eventTrigger(Sint16 val, bool* hold, Command cmd);
static void sendData(Command cmd);

#if defined(_WIN32) && !defined(_DEBUG)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main() {
#endif
	if (!init())
		return -1;

	while (run) {
		// draw window's contents
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);

		// handle events
		SDL_Event event;
		Uint32 timeout = SDL_GetTicks() + 50;
		while (SDL_PollEvent(&event) && SDL_GetTicks() < timeout)
			handleEvent(&event);
	}
	cleanup();
	return 0;
}

bool init() {
	// init libraries
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)) {
		printf("Couldn't initialize SDL:\n%s\n", SDL_GetError());
		return false;
	}
	if (SDLNet_Init()) {
		printf("Couldn't initialize SDL_net:\n%s\n", SDLNet_GetError());
		return false;
	}

	// create window and open game controller
	window = SDL_CreateWindow("SHLighter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 300, 300, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Couldn't create window:\n%s\n", SDL_GetError());
		return false;
	}
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		printf("Couldn't create renderer:\n%s\n", SDL_GetError());
		return false;
	}
	updateController();

	// connect to pi
	char addr[ALEN] = ADDRESS;
	Uint16 port = PORT;
	readHost(addr, &port);
	IPaddress ip;
	if (SDLNet_ResolveHost(&ip, addr, port)) {
		printf("Couldn't resolve host:\n%s\n", SDLNet_GetError());
		return false;
	}
	socket = SDLNet_TCP_Open(&ip);
	if (!socket) {
		printf("Couldn't connect:\n%s\n", SDLNet_GetError());
		return false;
	}
	return true;
}

void updateController() {
	// close old controller
	if (controller) {
		SDL_GameControllerClose(controller);
		controller = NULL;
	}
	// open first controller that supports xinput
	for (int i=0; i!=SDL_NumJoysticks(); i++)
		if (SDL_IsGameController(i)) {
			controller = SDL_GameControllerOpen(i);
			break;
		}
}

void readHost(char* addr, Uint16* port) {
	// attempt to read address and port from file
	FILE* file = fopen(FILE_HOST, "r");
	if (!file) {
		// if file doesn't exist create it with the current address and port
		file = fopen(FILE_HOST, "w");
		if (file) {
			fprintf(file, "%s\n%d", addr, *port);
			fclose(file);
		}
		return;
	}
	// read address
	char buff[BLEN];
	if (fgets(buff, BLEN, file))
		for (size_t i=0; i<BLEN; i++)
			if (buff[i] == '\n' || buff[i] == '\r' || buff[i] == '\0') {
				memcpy(addr, buff, i);
				addr[i] = '\0';
				break;
			}
	// read port
	*port = (Uint16)strtoul(fgets(buff, BLEN, file), NULL, 0);
	fclose(file);
}

void cleanup() {
	SDLNet_TCP_Close(socket);

	if (controller)
		SDL_GameControllerClose(controller);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDLNet_Quit();
	SDL_Quit();
}

void handleEvent(const SDL_Event* event) {
	if (event->type == SDL_CONTROLLERBUTTONDOWN)
		eventButtonDown(&event->cbutton);
	else if (event->type == SDL_CONTROLLERBUTTONUP)
		eventButtonUp(&event->cbutton);
	else if (event->type == SDL_CONTROLLERBUTTONDOWN)
		eventButtonDown(&event->cbutton);
	else if (event->type == SDL_CONTROLLERAXISMOTION)
		eventAxis(&event->caxis);
	else if (event->type == SDL_JOYDEVICEADDED || event->type == SDL_JOYDEVICEREMOVED)
		updateController();
	else if (event->type == SDL_QUIT)
		run = false;
}

void eventButtonDown(const SDL_ControllerButtonEvent* button) {
	if (button->button <= SDL_CONTROLLER_BUTTON_Y)
		updateColor();
	else if (button->button == SDL_CONTROLLER_BUTTON_DPAD_UP)
		sendData(MOVE_STEP_UP);
	else if (button->button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
		sendData(MOVE_STEP_DOWN);
	else if (button->button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
		sendData(MOVE_STEP_LEFT);
	else if (button->button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
		sendData(MOVE_STEP_RIGHT);
	else if (button->button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
		sendData(MAKE_DOTS_0);
	else if (button->button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
		sendData(MAKE_DOTS_1);
	else if (button->button == SDL_CONTROLLER_BUTTON_START)
		sendData(MOVE_IN_0);
	else if (button->button == SDL_CONTROLLER_BUTTON_BACK)
		sendData(MOVE_IN_1);
	else if (button->button == SDL_CONTROLLER_BUTTON_GUIDE)
		run = false;
}

void eventButtonUp(const SDL_ControllerButtonEvent* button) {
	if (button->button <= SDL_CONTROLLER_BUTTON_Y)
		updateColor();
}

void updateColor() {
	bool a = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
	bool b = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);
	bool x = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X);
	bool y = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y);

	if (y) {
		if (b) {
			color.r = 255;
			color.g = 80;
			color.b = 0;
		} else if (x) {
			color.r = 255;
			color.g = 0;
			color.b = 80;
		} else {
			color.r = 255;
			color.g = 255;
			color.b = 255;
		}
	} else {
		color.r = b ? 255 : 0;
		color.g = a ? 255 : 0;
		color.b = x ? 255 : 0;
	}
}

void eventAxis(const SDL_ControllerAxisEvent* axis) {
	if (axis->axis == SDL_CONTROLLER_AXIS_LEFTX)
		eventStick(axis->value, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY), &holdLX, &holdLY, MOVE_LINE_UP);
	else if (axis->axis == SDL_CONTROLLER_AXIS_LEFTY)
		eventStick(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX), axis->value, &holdLX, &holdLY, MOVE_LINE_UP);
	else if (axis->axis == SDL_CONTROLLER_AXIS_RIGHTX)
		eventStick(axis->value, SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY), &holdRX, &holdRY, MOVE_FULL_UP);
	else if (axis->axis == SDL_CONTROLLER_AXIS_RIGHTY)
		eventStick(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX), axis->value, &holdRX, &holdRY, MOVE_FULL_UP);
	else if (axis->axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
		eventTrigger(axis->value, &holdLT, MAKE_DOTS_2);
	else if (axis->axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
		eventTrigger(axis->value, &holdRT, MAKE_DOTS_3);
}

void eventStick(Sint16 valX, Sint16 valY, bool* holdX, bool* holdY, Command cmd) {
	// check if new values are above thresholdR
	bool newHX = abs(valX) > (*holdX ? THRESHOLD_FREE : THRESHOLD_LOCK);
	bool newHY = abs(valY) > (*holdY ? THRESHOLD_FREE : THRESHOLD_LOCK);

	// if stick has moved in one direction, send data to pi
	if (!(*holdX || *holdY) && (newHX != newHY)) {
		if (newHX)
			sendData((valX > 0) ? cmd+1 : cmd+3);	// change direction to right or left
		else
			sendData((valY > 0) ? cmd+2 : cmd);	// change direction to down or up
	}
	// update current state of stick
	*holdX = newHX;
	*holdY = newHY;
}

void eventTrigger(Sint16 val, bool* hold, Command cmd) {
	bool newH = val > (*hold ? THRESHOLD_FREE : THRESHOLD_LOCK);
	if (!*hold && newH)
		sendData(cmd);
	*hold = newH;
}

void sendData(Command cmd) {
	Uint8 data[DLEN] = {(Uint8)cmd, color.r, color.g, color.b};
	if (SDLNet_TCP_Send(socket, data, DLEN) < DLEN) {
		printf("Connection error:\n%s\n", SDLNet_GetError());
		run = false;
	}
}
