#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define BLASTOISESPEED	1
#define BLASTOISEWIDTH	80
#define BLASTOISEHEIGHT	80
#define BACKGROUNDWIDTH	8320
#define BACKGROUNDHEIGHT	480
#define GROUNDHEIGHT	114
#define GROUNDWIDTH	8320
#define DASHSPEED	2
#define DASHDURATION	250
#define JUMPDURATION	200
#define OBSTACLES	10
#define OBSTACLEWIDTH	50
#define OBSTACLEHEIGHT 100

// narysowanie napisu txt na powierzchni screen, zaczynajac od punktu (x, y)
// charset to bitmapa 128x128 zawierajaca znaki
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
// (x, y) to punkt srodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawSurfaceChanged(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
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


// rysowanie linii o dlugoœci l w pionie (gdy dx = 0, dy = 1) 
// beda poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostokata o dlugoœci boków l i k
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

// podaje dane startowe dla nowej gry
void newGame(int &backgroundPosition, double &worldTime, int &positionY, int &velocityX, int &velocityY, int &switchControls, int &dashSpeed)
{
	worldTime = 0;
	backgroundPosition = 0;
	positionY = BACKGROUNDHEIGHT / 2;
	velocityX = 0;
	velocityY = 0;
	switchControls = 0;
	dashSpeed = 1;
}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, distance, motion = BLASTOISESPEED;
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* blastoise;
	SDL_Surface* background;
	SDL_Surface* ground;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
	int positionX = BLASTOISEWIDTH;
	int positionY = BACKGROUNDHEIGHT / 2;
	int velocityX = 0;
	int velocityY = 0;
	int backgroundPosition = 0;
	int dashSpeed = BLASTOISESPEED;
	int dashTime = 0; 
	int jumpTime = 0;
	int jumpCount = 0;
	int switchControls = 0;
	int dash = 0; 
	int jump = 0;
	SDL_Rect blastoiseCollision; blastoiseCollision.w = BLASTOISEWIDTH; blastoiseCollision.h = BLASTOISEHEIGHT;
	SDL_Rect groundCollision; groundCollision.w = GROUNDWIDTH; groundCollision.h = GROUNDHEIGHT; 
	groundCollision.y = BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISESPEED;
	SDL_Rect obstacles[OBSTACLES];

	// okno konsoli nie jest widoczne, je¿eli chcemy zobaczyæ
	// komunikaty wypisywane printf-em trzeba w opcjach:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmienic na "Console"
	printf("wyjscie printfa trafia do tego okienka\n");
	printf("printf output goes here\n");

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// tryb pelnoekranowy 
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

	SDL_SetWindowTitle(window, "Robot Blastoise Attack");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, 
		SCREEN_WIDTH, SCREEN_HEIGHT);


	// wylaczenie widocznosci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
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

	// wczytanie obrazka postaci
	blastoise = SDL_LoadBMP("./blastoise.bmp");
	if (blastoise == NULL) {
		printf("SDL_LoadBMP(unicorn.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	background = SDL_LoadBMP("./background.bmp");
	if (background == NULL) {
		printf("SDL_LoadBMP(background.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	ground = SDL_LoadBMP("./ground.bmp");
	if (background == NULL) {
		printf("SDL_LoadBMP(ground.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	obstacles[0] = { 600, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[1] = { 1200, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[2] = { 2000, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[3] = { 2800, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[4] = { 3600, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[5] = { 4400, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[6] = { 5200, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[7] = { 6000, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[8] = { 6800, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };
	obstacles[9] = { 7600, BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT, OBSTACLEWIDTH, OBSTACLEHEIGHT };


	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	distance = 0;

	while (!quit) {
		t2 = SDL_GetTicks();

		// w tym momencie t2-t1 to czas w milisekundach,
		// jaki uplynal od ostatniego narysowania ekranu
		// delta to ten sam czas w sekundach
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;

		distance += motion * delta;

		if (backgroundPosition < 0 + BLASTOISEWIDTH / 2 ) {
			backgroundPosition = BLASTOISEWIDTH / 2;
			blastoiseCollision.x = BLASTOISEWIDTH / 2;
			groundCollision.x = BLASTOISEWIDTH / 2;
		}
		else if (positionY < 0 ) {
			positionY = 0;
			blastoiseCollision.y = 0;
		}
		else if (positionY > BACKGROUNDHEIGHT - BLASTOISEHEIGHT) {
			positionY = BACKGROUNDHEIGHT - BLASTOISEHEIGHT;
			blastoiseCollision.y = BACKGROUNDHEIGHT - BLASTOISEHEIGHT;
		}

		if (SDL_HasIntersection(&blastoiseCollision, &groundCollision)) {
			positionY = BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT;
		}

		DrawSurfaceChanged(screen, background, 0 - backgroundPosition, 0);
		DrawSurfaceChanged(screen, ground, 0 - backgroundPosition, BACKGROUNDHEIGHT - GROUNDHEIGHT);
		DrawSurfaceChanged(screen, blastoise, positionX / 4, positionY);

		for (int i = 0; i < OBSTACLES; i++) {
			SDL_FillRect(background, &obstacles[i], czarny);
			if (SDL_HasIntersection(&blastoiseCollision, &obstacles[i])) {
				printf("wykryto kolizje z obiektem %d\n", i + 1);
				newGame(backgroundPosition, worldTime, positionY, velocityX, velocityY, switchControls, dashSpeed);
			}
		}


		if (backgroundPosition >= BACKGROUNDWIDTH - SCREEN_WIDTH) {
			backgroundPosition = 0;
			groundCollision.x = 0;
			blastoiseCollision.x = 0;
		}

		if (switchControls == 1) {
			backgroundPosition += velocityX;
			positionY += velocityY;
			blastoiseCollision.x = backgroundPosition;
			groundCollision.x = backgroundPosition;
			blastoiseCollision.y = positionY;
		}
		else if (switchControls == 0) {
			backgroundPosition += BLASTOISESPEED * dashSpeed;
			blastoiseCollision.x = backgroundPosition;
			groundCollision.x = backgroundPosition;
			if (jump == 1) {
				positionY--;
				blastoiseCollision.y = positionY;
			}
			else if (jump == 0) {
				positionY++;
				blastoiseCollision.y = positionY;
			}
		}

		if (dash == 1) {
			dashTime++;
			if (dashTime > DASHDURATION) {
				dashTime = 0;
				dash = 0;
				dashSpeed = BLASTOISESPEED;
			}
		}

		if (positionY >= BACKGROUNDHEIGHT - GROUNDHEIGHT - BLASTOISEHEIGHT) {
			jumpCount = 0;
		}
		if (jump == 1) {
			jumpTime++;
			if (jumpTime > JUMPDURATION) {
				jumpTime = 0;
				jump = 0;
			}
			if (jumpCount > 2) {
				jump = 0;
				jumpCount = 0;
			}
		}

		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		};

		// tekst informacyjny
		DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
		sprintf(text, "Czas trwania od poczatku etapu = %.1lf s", worldTime, fps);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
		if (switchControls == 1) {
			sprintf(text, "Esc - wyjscie, strzalki - poruszanie sie, \156 - nowa gra");
		}
		else if (switchControls == 0) {
			sprintf(text, "Esc - wyjscie, \172 - skok, \170 - dash, \156 - nowa gra");
		}
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		// obsluga zdarzen (o ile jakies zaszly)
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
				else if (event.key.keysym.sym == SDLK_UP && event.key.repeat == 0 && switchControls == 1) velocityY -= BLASTOISESPEED;
				else if (event.key.keysym.sym == SDLK_DOWN && event.key.repeat == 0 && switchControls == 1) velocityY += BLASTOISESPEED;
				else if (event.key.keysym.sym == SDLK_RIGHT && event.key.repeat == 0 && switchControls == 1) velocityX += BLASTOISESPEED;
				else if (event.key.keysym.sym == SDLK_LEFT && event.key.repeat == 0 && switchControls == 1) velocityX -= BLASTOISESPEED;
				else if (event.key.keysym.sym == SDLK_n && event.key.repeat == 0) newGame(backgroundPosition, worldTime, positionY, velocityX, velocityY, switchControls, dashSpeed);
				else if (event.key.keysym.sym == SDLK_d && event.key.repeat == 0) { if (switchControls == 1) switchControls = 0; else switchControls = 1; }
				else if (event.key.keysym.sym == SDLK_x && event.key.repeat == 0 && switchControls == 0) { 
				dashSpeed += BLASTOISESPEED * DASHSPEED; 
				dashTime++; 
				dash = 1; 
				if (jumpCount == 2) {
					jumpCount--;
					}
				}
				else if (event.key.keysym.sym == SDLK_z && event.key.repeat == 0 && switchControls == 0) { jump = 1; jumpTime++; jumpCount++; }

				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_UP && switchControls == 1) velocityY = 0;
				else if (event.key.keysym.sym == SDLK_DOWN && switchControls == 1) velocityY = 0;
				else if (event.key.keysym.sym == SDLK_RIGHT && switchControls == 1) velocityX = 0;
				else if (event.key.keysym.sym == SDLK_LEFT && switchControls == 1) velocityX = 0;
				else if (event.key.keysym.sym == SDLK_z && switchControls == 0) { jump = 0; jumpTime = 0; }
				break;
			case SDL_QUIT:
				quit = 1;
				break;
			};
		};
		frames++;
	};

	// zwolnienie powierzchni
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
};
