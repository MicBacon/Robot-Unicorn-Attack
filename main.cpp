#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	1280
#define SCREEN_HEIGHT	720
#define FRAME_THICKNESS 20
#define GRAVITY 0.1
#define DASH_DURATION 3
#define ADDITIONAL_SCREEN_HEIGHT 300
#define DISTANCE 400
#define FPS 60

#define PINK 256*256*199 + 256*21 + 133
#define BLACK 0
#define BLUE  256*191 + 255


struct vector {
	double x;
	double y;
};

struct point {
	double x;
	double y;
};

struct unicorn {
	SDL_Surface* image;
	point position;
	const int width = 288;
	const int height = 195;
	vector velocity = { 5, 0 };
	bool isOnRun = true;
	bool isOnField = true;
	bool isOnDash = false;
	int jumpState;
	double remainingDashDuration = DASH_DURATION;
};

struct obstacle {
	SDL_Surface* image;
	point position;
	int width;
	int height;
	bool isOnField = false;
};

struct platform {
	SDL_Surface* image;
	point position;
	int width;
	int height;
	bool isOnScreen = false;
};



// narysowanie napisu txt na powierzchni screen, zaczynając od punktu (x, y)
// charset to bitmapa 128x128 zawierająca znaki
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt środka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


// rysowanie linii o długości l w pionie (gdy dx = 0, dy = 1) 
// bądź poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostokąta o długości boków l i k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void resetGameState(int& t1, double& worldTime, unicorn& unicorn, obstacle& obstacle_1,
	obstacle& obstacle_2, obstacle& obstacle_3, platform& platform_1, platform& platform_2,
	platform& platform_3, platform& platform_4, obstacle* obstacles[], platform* platforms[],
	double& cameraPosition);

void setObstacleParameters(obstacle& obstacle, int width, int height, double y);

void setPlatformParameters(platform& platform, int width, int height, double y);

void updateGameTime(int& t1, double& worldTime, double& delta);

void clearGameState(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* scrtex,
	unicorn& unicorn, SDL_Surface* screen, SDL_Surface* charset, obstacle& obstacle_1,
	obstacle& obstacle_2, obstacle& obstacle_3, platform& platform_1, platform& platform_2,
	platform& platform_3, platform& platform_4);

void update(unicorn& unicorn, obstacle& obstacle_1, obstacle& obstacle_2, platform& platform_1,
	platform& platform_2, platform& platform_3, platform& platform_4, double& worldTime,
	double& delta, double& cameraPosition);

bool isOnObstacle(unicorn& unicorn, obstacle& obstacle_1, double& cameraPosition);

bool isOnPlatform(unicorn& unicorn, platform& platform_1, double& cameraPosition);

void yPositionOperations(unicorn& unicorn, int highground);

void render(SDL_Surface* screen, unicorn& unicorn, obstacle& obstacle_1, obstacle& obstacle_2,
	obstacle& obstacle_3, platform& platform_1, platform& platform_2, platform& platform_3,
	platform& platform_4, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Surface* charset,
	double& worldTime, double& cameraPosition);

void generateObstaclesAndPlatfroms(unicorn& unicorn, obstacle& obstacle_1, obstacle& obstacle_2, obstacle& obstacle_3,
	platform& platform_1, platform& platform_2, platform& platform_3, platform& platform_4, SDL_Surface* screen,
	double& cameraPosition);

bool isBelowObstacle(obstacle& obstacle, int i);

void drawLayout(SDL_Surface* screen, SDL_Surface* charset, double& worldTime);

void drawUnicorn(SDL_Surface* screen, unicorn& unicorn);

bool isObstacleCollision(unicorn& unicorn, obstacle& obstacle, double& cameraPosition);

bool isPlatformCollision(unicorn& unicorn, platform& platform, double& cameraPosition);

int power(int base, int exponent);

int stringToInt(char* line, int whichNumber);


#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	//inicjalizacja
	const int frameDelay = 1000 / FPS;
	int frameDuration;
	Uint32 frameStart;
	int t1, quit = 0, rc, obstacleCount, platformCount;
	double worldTime, delta, cameraPosition;
	char line[128];
	FILE* file = fopen("gameInit.txt", "r");
	SDL_Event event;
	SDL_Surface* screen, * charset;
	unicorn unicorn;
	obstacle obstacle_1, obstacle_2, obstacle_3;
	platform platform_1, platform_2, platform_3, platform_4;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;

	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmienić na "Console"

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);

	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	SDL_SetWindowTitle(window, "Robot Unicorn Attack Project");

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	//wczytanie potrzebnych do gry obrazków 
	charset = SDL_LoadBMP("./bitmaps/cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);

	unicorn.image = SDL_LoadBMP("./bitmaps/unicorn.bmp");
	if (unicorn.image == NULL) {
		printf("SDL_LoadBMP(unicorn.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	obstacle_1.image = SDL_LoadBMP("./bitmaps/obstacle_1.bmp");
	if (obstacle_1.image == NULL) {
		printf("SDL_LoadBMP(obstacle_1.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_FreeSurface(unicorn.image);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	obstacle_2.image = SDL_LoadBMP("./bitmaps/obstacle_2.bmp");
	if (obstacle_2.image == NULL) {
		printf("SDL_LoadBMP(obstacle_2.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_FreeSurface(unicorn.image);
		SDL_FreeSurface(obstacle_1.image);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	obstacle_3.image = SDL_LoadBMP("./bitmaps/obstacle_3.bmp");
	if (obstacle_3.image == NULL) {
		printf("SDL_LoadBMP(obstacle_3.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_FreeSurface(unicorn.image);
		SDL_FreeSurface(obstacle_1.image);
		SDL_FreeSurface(obstacle_2.image);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	platform_1.image = SDL_LoadBMP("./bitmaps/platform_1.bmp");
	if (platform_1.image == NULL) {
		printf("SDL_LoadBMP(platform_1.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_FreeSurface(unicorn.image);
		SDL_FreeSurface(obstacle_1.image);
		SDL_FreeSurface(obstacle_2.image);
		SDL_FreeSurface(obstacle_3.image);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	platform_2.image = SDL_LoadBMP("./bitmaps/platform_2.bmp");
	if (platform_2.image == NULL) {
		printf("SDL_LoadBMP(platform_2.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_FreeSurface(unicorn.image);
		SDL_FreeSurface(obstacle_1.image);
		SDL_FreeSurface(obstacle_2.image);
		SDL_FreeSurface(obstacle_3.image);
		SDL_FreeSurface(platform_1.image);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	platform_3.image = SDL_LoadBMP("./bitmaps/platform_3.bmp");
	if (platform_3.image == NULL) {
		printf("SDL_LoadBMP(platform_3.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_FreeSurface(unicorn.image);
		SDL_FreeSurface(obstacle_1.image);
		SDL_FreeSurface(obstacle_2.image);
		SDL_FreeSurface(obstacle_3.image);
		SDL_FreeSurface(platform_1.image);
		SDL_FreeSurface(platform_2.image);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	platform_4.image = SDL_LoadBMP("./bitmaps/platform_3.bmp");
	if (platform_4.image == NULL) {
		printf("SDL_LoadBMP(platform_4.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_FreeSurface(unicorn.image);
		SDL_FreeSurface(obstacle_1.image);
		SDL_FreeSurface(obstacle_2.image);
		SDL_FreeSurface(obstacle_3.image);
		SDL_FreeSurface(platform_1.image);
		SDL_FreeSurface(platform_2.image);
		SDL_FreeSurface(platform_3.image);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};



	//wczytanie danych z pliku
	fgets(line, 128, file);
	obstacleCount = stringToInt(line, 1);


	obstacle* obstacles = new obstacle[obstacleCount];
	for (int i = 0; i < obstacleCount; i++) {
		fgets(line, 128, file);
		setObstacleParameters(obstacles[i], stringToInt(line, 1), stringToInt(line, 2), (double)stringToInt(line, 3));
	}

	fgets(line, 128, file);
	platformCount = stringToInt(line, 1);


	platform* platforms = new platform[platformCount];
	for (int i = 0; i < platformCount; i++) {
		fgets(line, 128, file);
		setPlatformParameters(platforms[i], stringToInt(line, 1), stringToInt(line, 2), (double)stringToInt(line, 3));
	}


	resetGameState(t1, worldTime, unicorn, obstacle_1, obstacle_2, obstacle_3, platform_1, platform_2, platform_3, platform_4, &obstacles, &platforms, cameraPosition);

	//pętla gry
	while (!(quit)) {

		//60 fps oraz funkcje odpowiadające za stan gry
		frameStart = SDL_GetTicks();

		updateGameTime(t1, worldTime, delta);
		update(unicorn, obstacle_1, obstacle_2, platform_1, platform_2, platform_3, platform_4, worldTime, delta, cameraPosition);
		render(screen, unicorn, obstacle_1, obstacle_2, obstacle_3, platform_1, platform_2, platform_3, platform_4,
			scrtex, renderer, charset, worldTime, cameraPosition);

		frameDuration = SDL_GetTicks() - frameStart;
		if (frameDuration < frameDelay) {
			SDL_Delay(frameDelay - frameDuration);
		}
		
		//sprawdzenie kolizji
		if (isObstacleCollision(unicorn, obstacle_1, cameraPosition) || isObstacleCollision(unicorn, obstacle_2, cameraPosition) ||
			isObstacleCollision(unicorn, obstacle_3, cameraPosition) || isPlatformCollision(unicorn, platform_1, cameraPosition) ||
			isPlatformCollision(unicorn, platform_2, cameraPosition) || isPlatformCollision(unicorn, platform_3, cameraPosition) ||
			isPlatformCollision(unicorn, platform_4, cameraPosition)) {
			resetGameState(t1, worldTime, unicorn, obstacle_1, obstacle_2, obstacle_3, platform_1, platform_2, platform_3, platform_4, &obstacles, &platforms, cameraPosition);
		}

		//sprawdzenie czy jednorożec nie spadł
		if (unicorn.position.y + unicorn.height >= SCREEN_HEIGHT - FRAME_THICKNESS) {
			resetGameState(t1, worldTime, unicorn, obstacle_1, obstacle_2, obstacle_3, platform_1, platform_2, platform_3, platform_4, &obstacles, &platforms, cameraPosition);
		}

		//obsługa zdarzeń, o ile jakieś zaszły
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				//wyjście
				if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
				//przełączenie sterowania
				else if (event.key.keysym.sym == SDLK_d) unicorn.isOnRun = !unicorn.isOnRun;
				//skok
				else if (event.key.keysym.sym == SDLK_z) {
					if (!unicorn.isOnDash && unicorn.jumpState < 2) {
						unicorn.jumpState++;
						unicorn.isOnField = false;
						unicorn.position.y -= 20;
						unicorn.velocity.y = -4;
					}
				}
				//dash
				else if (event.key.keysym.sym == SDLK_x) {
					if (!unicorn.isOnDash) {
						unicorn.isOnDash = true;
					}
				}
				//nowa gra
				else if (event.key.keysym.sym == SDLK_n) resetGameState(t1, worldTime, unicorn, obstacle_1, obstacle_2, obstacle_3, platform_1, platform_2, platform_3, platform_4, &obstacles, &platforms, cameraPosition);
				//sterowanie strzałkami
				else if (event.key.keysym.sym == SDLK_RIGHT && !unicorn.isOnRun) unicorn.position.x += 10;
				else if (event.key.keysym.sym == SDLK_DOWN &&
					unicorn.position.y < (SCREEN_HEIGHT - unicorn.height - FRAME_THICKNESS) &&
					!unicorn.isOnRun) unicorn.position.y += 10;
				else if (event.key.keysym.sym == SDLK_UP &&
					unicorn.position.y > FRAME_THICKNESS &&
					!unicorn.isOnRun) unicorn.position.y -= 10;
				break;
			case SDL_QUIT:
				quit = 1;
				break;
			};
		};
	};

	clearGameState(window, renderer, scrtex, unicorn, screen, charset, obstacle_1, obstacle_2, obstacle_3, platform_1,
		platform_2, platform_3, platform_4);

	return 0;

};

void resetGameState(int& t1, double& worldTime, unicorn& unicorn,
	obstacle& obstacle_1, obstacle& obstacle_2, obstacle& obstacle_3,
	platform& platform_1, platform& platform_2, platform& platform_3,
	platform& platform_4, obstacle* obstacles[], platform* platforms[], 
	double& cameraPosition) {

	t1 = SDL_GetTicks();

	worldTime = 0;
	unicorn.velocity.x = 6;
	unicorn.velocity.y = 0;
	unicorn.position.x = 0;
	unicorn.isOnField = true;
	unicorn.isOnDash = false;
	unicorn.jumpState = 0;
	unicorn.remainingDashDuration = DASH_DURATION;

	setObstacleParameters(obstacle_1, (*obstacles)[0].width, (*obstacles)[0].height, 0);
	setObstacleParameters(obstacle_2, (*obstacles)[1].width, (*obstacles)[1].height, 200);
	setObstacleParameters(obstacle_3, (*obstacles)[2].width, (*obstacles)[2].height, double(FRAME_THICKNESS - ADDITIONAL_SCREEN_HEIGHT));

	setPlatformParameters(platform_1, (*platforms)[0].width, (*platforms)[0].height, double(SCREEN_HEIGHT - FRAME_THICKNESS - 250));
	setPlatformParameters(platform_2, (*platforms)[1].width, (*platforms)[1].height, double(SCREEN_HEIGHT - FRAME_THICKNESS - 160));
	setPlatformParameters(platform_3, (*platforms)[2].width, (*platforms)[2].height, double(platform_2.position.y - unicorn.height - 150));
	setPlatformParameters(platform_4, (*platforms)[3].width, (*platforms)[3].height, double(SCREEN_HEIGHT - FRAME_THICKNESS - 450));

	
	unicorn.position.y = cameraPosition = (platform_1.position.y - unicorn.height);

}

void setObstacleParameters(obstacle& obstacle, int width, int height, double y) {
	obstacle.width = width;
	obstacle.height = height;
	obstacle.position.y = y;
}

void setPlatformParameters(platform& platform, int width, int height, double y) {
	platform.width = width;
	platform.height = height;
	platform.position.y = y;
}

void updateGameTime(int& t1, double& worldTime, double& delta) {

	int t2 = SDL_GetTicks();

	delta = (t2 - t1) * 0.001;
	t1 = t2;

	worldTime += delta;
}

void clearGameState(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* scrtex,
	unicorn& unicorn, SDL_Surface* screen, SDL_Surface* charset, obstacle& obstacle_1,
	obstacle& obstacle_2, obstacle& obstacle_3, platform& platform_1, platform& platform_2,
	platform& platform_3, platform& platform_4) {

	SDL_FreeSurface(obstacle_1.image);
	SDL_FreeSurface(obstacle_2.image);
	SDL_FreeSurface(obstacle_3.image);
	SDL_FreeSurface(platform_1.image);
	SDL_FreeSurface(platform_2.image);
	SDL_FreeSurface(platform_3.image);
	SDL_FreeSurface(platform_4.image);
	SDL_FreeSurface(unicorn.image);
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

}

void update(unicorn& unicorn, obstacle& obstacle_1, obstacle& obstacle_2,
	platform& platform_1, platform& platform_2, platform& platform_3, platform& platform_4,
	double& worldTime, double& delta, double& cameraPosition) {

	if (unicorn.isOnRun) {

		//fizyka poruszania się, skoków i dashów

		unicorn.velocity.y += GRAVITY;
	
		if (unicorn.remainingDashDuration <= 0) {
			unicorn.isOnDash = false;
			unicorn.jumpState = 1;
			unicorn.remainingDashDuration = DASH_DURATION;
		}

		if (unicorn.isOnDash) {
			unicorn.position.x += 2 * unicorn.velocity.x;
			unicorn.remainingDashDuration -= 0.1;
		}
		else {
			unicorn.position.x += unicorn.velocity.x;
		}

		//prędkość zwiększana wraz z przyrostem czasu
		unicorn.velocity.x += delta/2;

		platform actualPlatform;
		actualPlatform.position.y = platform_1.position.y;

		if ((unicorn.position.y - (platform_1.position.y - unicorn.height)) > ADDITIONAL_SCREEN_HEIGHT) {
			cameraPosition = ADDITIONAL_SCREEN_HEIGHT;
		}
		else if ((unicorn.position.y - (platform_1.position.y - unicorn.height)) < -ADDITIONAL_SCREEN_HEIGHT) {
			cameraPosition = -ADDITIONAL_SCREEN_HEIGHT;
		}
		else {
			cameraPosition = unicorn.position.y - (platform_1.position.y - unicorn.height);
		}

		if (!unicorn.isOnDash) {

			//kontrola powierzchi, na której znajduje się jednorożec
			if (isOnPlatform(unicorn, platform_1, cameraPosition)) {
				if (unicorn.isOnField) {
					actualPlatform = platform_1;
				}
				if (isOnObstacle(unicorn, obstacle_1, cameraPosition)) {
					yPositionOperations(unicorn, obstacle_1.position.y - cameraPosition);
				}
				else if (isOnPlatform(unicorn, platform_3, cameraPosition)) {
					if (unicorn.isOnField) {
						actualPlatform = platform_3;
					}
					yPositionOperations(unicorn, platform_3.position.y - cameraPosition);
				}
				else {
					yPositionOperations(unicorn, platform_1.position.y - cameraPosition);
				}
			}
			else if (isOnPlatform(unicorn, platform_2, cameraPosition)) {
				if (unicorn.isOnField) {
					actualPlatform = platform_2;
				}
				if (isOnObstacle(unicorn, obstacle_2, cameraPosition)) {
					yPositionOperations(unicorn, obstacle_2.position.y - cameraPosition);
				}
				else if (isOnPlatform(unicorn, platform_3, cameraPosition)) {
					if (unicorn.isOnField) {
						actualPlatform = platform_3;
					}
					yPositionOperations(unicorn, platform_3.position.y - cameraPosition);
				}
				else {
					yPositionOperations(unicorn, platform_2.position.y - cameraPosition);
				}
			}
			else if (isOnPlatform(unicorn, platform_4, cameraPosition)) {
				if (unicorn.isOnField) {
					actualPlatform = platform_4;
				}
				if (isOnObstacle(unicorn, obstacle_1, cameraPosition)) {
					yPositionOperations(unicorn, obstacle_1.position.y - cameraPosition);
				}
				else {
					yPositionOperations(unicorn, platform_4.position.y - cameraPosition);
				}
			}
			else {
				yPositionOperations(unicorn, SCREEN_HEIGHT + ADDITIONAL_SCREEN_HEIGHT);
			}

		}

	}


}

bool isOnObstacle(unicorn& unicorn, obstacle& obstacle, double& cameraPosition)
{

	if ((unicorn.position.x + unicorn.width) >= obstacle.position.x &&
		unicorn.position.x <= (obstacle.position.x + obstacle.width) &&
		(unicorn.position.y + unicorn.height) <= (obstacle.position.y + obstacle.height - cameraPosition)) {
		return true;
	}

	return false;

}


bool isOnPlatform(unicorn& unicorn, platform& platform, double& cameraPosition) {

	if ((unicorn.position.x + unicorn.width) >= platform.position.x &&
		unicorn.position.x <= (platform.position.x + platform.width) &&
		(unicorn.position.y + unicorn.height) <= (platform.position.y + platform.height - cameraPosition)) {
		return true;
	}

	return false;

}

void yPositionOperations(unicorn& unicorn, int highground)
{
	if (unicorn.position.y > FRAME_THICKNESS) {

		if (unicorn.position.y + unicorn.height <= highground) {
			unicorn.position.y += unicorn.velocity.y;
		}

		if (unicorn.position.y + unicorn.height >= highground) {
			unicorn.jumpState = 0;
			unicorn.isOnField = true;
		}

	}
	else if (unicorn.velocity.y > 0) {
		unicorn.position.y += unicorn.velocity.y;
	}

}

void render(SDL_Surface* screen, unicorn& unicorn, obstacle& obstacle_1, obstacle& obstacle_2, obstacle& obstacle_3,
	platform& platform_1, platform& platform_2, platform& platform_3, platform& platform_4, SDL_Texture* scrtex,
	SDL_Renderer* renderer, SDL_Surface* charset, double& worldTime, double& cameraPosition) {

	SDL_FillRect(screen, NULL, BLUE);
	SDL_RenderClear(renderer);

	//generacja przeszkód
	generateObstaclesAndPlatfroms(unicorn, obstacle_1, obstacle_2, obstacle_3, platform_1, platform_2, platform_3, platform_4,
		screen, cameraPosition);

	//rysowanie obramowania planszy, gruntu i wyświetlacza czasu
	drawLayout(screen, charset, worldTime);

	//rysowanie jednorożca
	drawUnicorn(screen, unicorn);

	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);

}


void generateObstaclesAndPlatfroms(unicorn& unicorn, obstacle& obstacle_1, obstacle& obstacle_2, obstacle& obstacle_3,
	platform& platform_1, platform& platform_2, platform& platform_3, platform& platform_4, SDL_Surface* screen,
	double& cameraPosition) {

	obstacle_1.isOnField = obstacle_2.isOnField = obstacle_3.isOnField = platform_1.isOnScreen =
		platform_2.isOnScreen = platform_3.isOnScreen = platform_4.isOnScreen = false;

	for (int i = (unicorn.position.x - 1280); i < (unicorn.position.x + SCREEN_WIDTH); i++) {

		if (i % (platform_1.width * 4) == 0) {
			platform_1.position.x = i;
			platform_1.isOnScreen = true;
			DrawSurface(screen, platform_1.image, platform_1.position.x - unicorn.position.x, platform_1.position.y - cameraPosition);
			if (i != 0 && i % 20480 == 0) {
				platform_3.position.x = i + DISTANCE;
				platform_3.isOnScreen = true;
				DrawSurface(screen, platform_3.image, platform_3.position.x - unicorn.position.x, platform_3.position.y - cameraPosition);
			}
			else if (i != 0 && i % 10240) {
				obstacle_1.position.x = i + 2 * DISTANCE;
				obstacle_1.position.y = platform_1.position.y - obstacle_1.height;
				obstacle_1.isOnField = true;
				DrawSurface(screen, obstacle_1.image, obstacle_1.position.x - unicorn.position.x, obstacle_1.position.y - cameraPosition);
			}

		}
		else if (i % 2560 == 0) {
			platform_2.position.x = i;
			platform_2.isOnScreen = true;
			DrawSurface(screen, platform_2.image, platform_2.position.x - unicorn.position.x, platform_2.position.y - cameraPosition);

			if (i % 7680 == 0) {
				platform_3.position.x = i + DISTANCE;
				platform_3.isOnScreen = true;
				DrawSurface(screen, platform_3.image, platform_3.position.x - unicorn.position.x, platform_3.position.y - cameraPosition);
			}
			else {
				obstacle_2.position.x = i + DISTANCE;
				obstacle_2.isOnField = true;
				DrawSurface(screen, obstacle_2.image, obstacle_2.position.x - unicorn.position.x, obstacle_2.position.y - cameraPosition);
			}

		}
		else if (i % 1280 == 0) {
			platform_4.position.x = i + DISTANCE;
			platform_4.isOnScreen = true;
			DrawSurface(screen, platform_4.image, platform_4.position.x - unicorn.position.x, platform_4.position.y - cameraPosition);
			if (i % 8960 == 0) {
				obstacle_1.position.x = i + DISTANCE + 200;
				obstacle_1.position.y = platform_4.position.y - obstacle_1.height + platform_4.height;
				obstacle_1.isOnField = true;
				DrawSurface(screen, obstacle_1.image, obstacle_1.position.x - unicorn.position.x, obstacle_1.position.y - cameraPosition);
			}
		}


		if (i != 0 && i % (obstacle_3.width * 16) == 0 &&
			!isBelowObstacle(obstacle_1, i) && !isBelowObstacle(obstacle_2, i)) {
			obstacle_3.position.x = i;
			obstacle_3.isOnField = true;
			DrawSurface(screen, obstacle_3.image, obstacle_3.position.x - unicorn.position.x, obstacle_3.position.y - cameraPosition);
		}

	}
}

bool isBelowObstacle(obstacle& obstacle, int i) {
	if (obstacle.position.x <= i && obstacle.position.x + obstacle.height >= i) {
		return true;
	}

	return false;
}

void drawLayout(SDL_Surface* screen, SDL_Surface* charset, double& worldTime) {

	//obramowanie ekranu
	DrawRectangle(screen, FRAME_THICKNESS, SCREEN_HEIGHT - FRAME_THICKNESS, 2 * SCREEN_WIDTH, FRAME_THICKNESS, PINK, PINK);
	DrawRectangle(screen, 0, 0, FRAME_THICKNESS, SCREEN_HEIGHT, PINK, PINK);
	DrawRectangle(screen, SCREEN_WIDTH - FRAME_THICKNESS, 0, FRAME_THICKNESS, SCREEN_HEIGHT, PINK, PINK);
	DrawRectangle(screen, 0, 0, SCREEN_WIDTH, FRAME_THICKNESS, PINK, PINK);

	//wyświetlanie czasu gry w sekundach w prawym górnym rogu ekranu
	char text[128];
	DrawRectangle(screen, SCREEN_WIDTH - FRAME_THICKNESS - 160, FRAME_THICKNESS, 160, 55, BLACK, PINK);
	sprintf(text, "Game time: %0.1f s", worldTime);
	DrawString(screen, SCREEN_WIDTH - FRAME_THICKNESS - 150, FRAME_THICKNESS + 25, text, charset);

}

void drawUnicorn(SDL_Surface* screen, unicorn& unicorn) {

	//oznaczenie koloru czarnego jako niewidoczny i narysowanie jednorożca
	SDL_SetColorKey(unicorn.image, SDL_TRUE, BLACK);
	DrawSurface(screen, unicorn.image, FRAME_THICKNESS, unicorn.position.y);

}

bool isObstacleCollision(unicorn& unicorn, obstacle& obstacle, double& cameraPosition) {

	//przeszkody leżące
	if (obstacle.position.y > FRAME_THICKNESS) {
		if (obstacle.isOnField) {
			if ((unicorn.position.x + unicorn.width) >= obstacle.position.x &&
				(unicorn.position.x + unicorn.width) <= (obstacle.position.x + obstacle.width) &&
				(unicorn.position.y + unicorn.height) >= (obstacle.position.y + 10 - cameraPosition) &&
				unicorn.position.y <= (obstacle.position.y + obstacle.height - cameraPosition)) {
				return true;
			}
		}
	}
	//stalaktyty
	else if (obstacle.isOnField) {
		if ((unicorn.position.x + unicorn.width) >= obstacle.position.x &&
			unicorn.position.x <= (obstacle.position.x + obstacle.width) &&
			unicorn.position.y <= (obstacle.position.y + obstacle.height - cameraPosition)) {
			return true;
		}
	}

	return false;

}

bool isPlatformCollision(unicorn& unicorn, platform& platform, double& cameraPosition) {

	if (platform.isOnScreen) {
		if ((unicorn.position.x + unicorn.width) >= platform.position.x &&
			(unicorn.position.x + unicorn.width) <= (platform.position.x + platform.width) &&
			(unicorn.position.y + unicorn.height) > (platform.position.y + 20 - cameraPosition) &&
			unicorn.position.y <= (platform.position.y + platform.height - cameraPosition)) {
			return true;
		}
	}

	return false;
}

int power(int base, int exponent) {

	if (exponent == 0) {
		return 1;
	}

	int sum = base;
	for (int i = 2; i <= exponent; i++) {
		sum *= base;
	}

	return sum;
}

int stringToInt(char* line, int whichNumber) {
	int i = 0;
	int sum = 0;
	int digits = 0;

	for (int j = 0; j < whichNumber; j++) {
		if (line[i] == ' ') {
			digits = 0;
			i++;
		}
		while (line[i] != '\n' && line[i] != '\0' && line[i] != ' ') {
			digits++;
			i++;
		}
	}

	if (line[i - digits] == '-') {

		for (int j = (i - digits + 1); j < i; j++) {
			sum += power(10, (i - j - 1)) * int(line[j] - 48);
		}

		return -sum;

	}
	for (int j = i - digits; j < i; j++) {
		sum += power(10, (i - j - 1)) * int(line[j] - 48);
	}

	return sum;
}

