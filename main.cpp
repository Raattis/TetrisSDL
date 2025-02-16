#include <iostream>
#include <vector>

#ifdef __cplusplus
    #include <cstdlib>
#else
    #include <stdlib.h>
#endif

#include <SDL/SDL.h>


// DELETE
#ifndef FB_BUILD
	namespace math { struct VC2 { VC2(float x, float y): x(x), y(y) {} float x; float y; }; };
	#define FB_PRINT(fmt) std::cout << fmt;
	#define FB_PRINTF(fmt, asdf) printf(fmt, asdf);
#endif

class Tetris
{
	typedef int8_t intris;
	typedef enum { E = -1, O = 0, I = 1, S = 2, Z = 3, T = 4, L = 5, J = 6 }  Block;

	struct Pos
	{
		Pos() : x(0), y(0) {}
		Pos(intris x, intris y) : x(x), y(y) {}
		intris x, y;
	};

	struct Piece
	{
		Piece(Block t) : bt(t)
		{
			poss[0] = Pos(0,0); poss[1] = Pos(0,1);
			switch(t)
			{
			case I:		poss[2] = Pos( 0,-1);	poss[3] = Pos( 0, 2);	maxRot = 1;	break;
			case S:		poss[2] = Pos(-1, 0);	poss[3] = Pos( 1, 1);	maxRot = 1;	break;
			case Z:		poss[2] = Pos( 1, 0);	poss[3] = Pos(-1, 1);	maxRot = 1;	break;
			case T:		poss[2] = Pos( 1, 0);	poss[3] = Pos(-1, 0);	maxRot = 3;	break;
			case L:		poss[2] = Pos( 0,-1);	poss[3] = Pos( 1,-1);	maxRot = 3;	break;
			case J:		poss[2] = Pos( 0,-1);	poss[3] = Pos(-1,-1);	maxRot = 3;	break;
			case O:
			default:	poss[2] = Pos( 1, 0);	poss[3] = Pos( 1, 1);	maxRot = 0;	bt = O;
			}
		}

		void turn()
		{
			do
            {
               turn_();
            }
            while(rotation > maxRot);
		}

		void turn_()
		{
		    intris temp = (++rotation) % 4;
		    rotation = temp;
			for(int i = 0; i < 4; ++i)
			{
                poss[i] = Pos( -poss[i].y, poss[i].x );
			}
		}

		intris rotation;	// Rotation
		intris maxRot;		// Max number of rotation
		Pos poss[4];		// Block positions
		Block bt;			// Block type
	};

	intris seed1; 			// Random seeds used for block randomization
	intris seed2;

	intris field[10][22]; 	// Playing field 10x22
	Piece p; 				// Currently dropping tetris piece
	Pos piecePos;			// Position of the dropping piece

	float dropTime;			// Time left until the piece falls on update
	int linesDestroyed;		// Total number of lines destroyed
	int score;				// Score, related to total destroyed lines and currently destroyed.

	// Returns a new random block type
	Block getRandom()
	{
		seed1 = 36969 * (seed1 & 15) + (seed1 >> 4);
		seed2 = 18000 * (seed2 & 15) + (seed2 >> 4);
		return (Block)((127 & ((seed1 << 4) + seed2))%6);
	}

	// Returns the position of the dropping tetris piece's block by block index in field coordinates.
	Pos getPPos(int i)
	{
		return Pos(p.poss[i].x + piecePos.x, p.poss[i].y + piecePos.y);
	}

	// Returns the time delay between automatic drops in update, is inversely related to total lines destroyed.
	float getDropDelay()
	{
		if(linesDestroyed > 120)
			return 0.0f;
        else
			return 1.0f / (1 << (linesDestroyed / 10));
	}

	// Stick the block to the playing field and create a new one. Check and detroy for full lines.
	void glueBlock()
	{
		for(int i = 0; i < 4; ++i)
		{
			Pos pos = getPPos(i);
			if(pos.x >= 0 && pos.x < 10 && pos.y >= 0 && pos.y < 22)
				field[pos.x][pos.y] = p.bt;
		}
		piecePos = Pos(5, 20);
		p = getRandom();

		// Checking lines
		{
			int destroyCount = 0;
			int destroyedLines[4];
			for(int j = 0; j < 22; ++j)
			{
				bool fullLine = true;
				for(int i = 0; i < 10; ++i)
				{
					if(field[i][j] == E) { fullLine = false; break; }
				}
				if(fullLine)
				{
					destroyedLines[destroyCount++] = j;
				}
			}

			for(int c = destroyCount - 1; c >= 0; --c)
			{
                for(int j = destroyedLines[c]; j < 22; ++j)
				{
					for(int i = 0; i < 10; ++i)
					{
						if(j == 21)
							field[i][j] = E;
						else
							field[i][j] = field[i][j + 1];
					}
				}
			}

			linesDestroyed += destroyCount;
			score += ((1 + linesDestroyed / 10) * 1 << destroyCount);
		}
	}

	// Returns whether the dropping piece is on top of field blocks.
	bool pieceCollides()
	{
        for(int i = 0; i < 4; ++i)
		{
			Pos pos = getPPos(i);
			if(pos.x < 0 || pos.x >= 10 || pos.y < 0) return true;
			if(pos.y >= 22) continue;
			if(field[pos.x][pos.y] != E) return true;
		}
		return false;
	}

public:
    std::vector<int8_t> input_record;
    int seedNumber;

	// Gimme your seed!
	Tetris(int seed = 41)
		: seed1(seed + 1)
		, seed2((seed + 1) * 17 + 1)
		, field()
		, p(getRandom())
		, piecePos(5,20)
		, dropTime(1)
		, linesDestroyed(0)
		, score(0)
		, input_record()
		, seedNumber(seed)
		, gameOver(false)
	{
		for(int i = 0; i < 10; ++i) for(int j = 0; j < 22; ++j) field[i][j] = E;
	}

    int getScore() { return score; }
	// Game is over when this is true. Read-only usage.
	bool gameOver;

	// Update, called as often as the user desires. DeltaTime is the time between calls.
	// arg: deltaTime should be the time between update-calls
	bool update(float deltaTime)
	{
		if(gameOver) return false;
		dropTime -= deltaTime;
		if(dropTime <= 0)
		{
			dropTime = getDropDelay();
			return input(2);
		}
		return false;
	}

	// User input, called as often as there is user input.
	// arg: 0 = left, 1 = right, 2 = down, 3 = drop, 4 = rotate
	// ret: true = moved/turned successfully without collision, false = ... not
	bool input(int d)
	{
		if(gameOver) return false;
		if(d != 3)
            input_record.push_back(d);

		if(d < 3) // Move
		{
			Pos oldPos = piecePos;
				 if(d == 0)
                piecePos.x -= 1;
			else if(d == 1)
                piecePos.x += 1;
			else
                { piecePos.y -= 1; dropTime = getDropDelay(); }
			if(pieceCollides())
			{
				piecePos = oldPos;
				if(d == 2)
				{
					glueBlock();
                    if(pieceCollides()) gameOver = true;
				}
				return false;
			}
			return true;
		}
		if(d == 3) // Drop
		{
		    while(input(2));
			return false;
		}
		intris oldRot = p.rotation;
		p.turn();
		if(pieceCollides())
		{
			int escape = 10;
			while(p.rotation != oldRot && --escape > 0) { p.turn(); }
		}
		return true;
	}

	// Convert Pos to math::VC2, for output
	static math::VC2 getVC2(int x, int y) { return math::VC2(x/10.0f, y/22.0f); }

	// Returns every block position in field and in the dropping block.
	// arg: position vector for the result
	void getFieldPositions(std::vector<math::VC2>& result)
	{
		for(int j = 21; j >= 0; --j)
		{
			for(int i = 0; i < 10; ++i)
			{
				if(field[i][j] != E)
				{
                    result.push_back(getVC2(i,j));
				}
			}
		}
	}

	void getPiecePositions(std::vector<math::VC2>& result)
	{
		for(int i = 0; i < 4; ++i)
		{
			Pos pos = getPPos(i);
			result.push_back(getVC2(pos.x, pos.y));
		}
	}

	// Debug print
	void printAscii()
	{
		Pos poss[4];
		for(int i = 0; i < 4; ++i)
		{
			poss[i] = getPPos(i);
		}

		for(int j = 21; j >= 0; --j)
		{
			FB_PRINT("<");
			for(int i = 0; i < 10; ++i)
			{
				if(field[i][j] != E)
				{
					FB_PRINT("O");
					continue;
				}
				bool hit = false;
				for(int k = 0; k < 4; ++k)
				{
					if(poss[k].x == i && poss[k].y == j)
					{
						hit = true;
						FB_PRINT("X");
						break;
					}
				}
				if(!hit)
					FB_PRINT(" ");
			}
			FB_PRINT(">\n");
		}
	}
};

void playback(SDL_Surface* screen, int seedNumber, std::vector<int8_t> record)
{
    math::VC2 resolution = math::VC2(screen->w, screen->h);
    SDL_Rect r = {1,1,(Uint16)(resolution.x / 10), (Uint16)(resolution.y / 22)};

    Uint32 c = SDL_MapRGB(screen->format, 255, 255, 255);
    Uint32 p = SDL_MapRGB(screen->format, 190, 255, 200);

    Tetris t = Tetris(seedNumber);
    for(Uint32 i = 0; i < record.size(); ++i)
    {
        SDL_Delay(record[i] == 2 ? 10 : 100);
        t.input((int)(record[i]));
        printf("%d\r\n", record[i]);

        SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
        //SDL_BlitSurface(bmp, 0, screen, &bgrect);

        std::vector<math::VC2> blocks;
        t.getFieldPositions(blocks);
        for(Uint32 i = 0; i < blocks.size(); ++i)
        {
            r.x = resolution.x * blocks[i].x + 0.5;
            r.y = resolution.y-resolution.y * blocks[i].y - r.h + 0.5;
            SDL_FillRect(screen, &r, c);
        }
        printf("\n");
        blocks.clear();
        t.getPiecePositions(blocks);
        for(Uint32 i = 0; i < blocks.size(); ++i)
        {
            r.x = resolution.x * blocks[i].x + 0.5;
            r.y = std::max(resolution.y - resolution.y * blocks[i].y - r.h + 0.5f, 0.0f);
            SDL_FillRect(screen, &r, p);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT: return;
                case SDL_KEYDOWN: if(event.key.keysym.sym == SDLK_ESCAPE) return;
                default: break;
            }
        }
        SDL_Flip(screen);
    }
}


int main(int argc, char** argv)
{
	printf("Starting\n");

    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }
    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    math::VC2 resolution = math::VC2(300, 660);

    // create a new window
    SDL_Surface* screen = SDL_SetVideoMode(resolution.x, resolution.y, 16, SDL_HWSURFACE|SDL_DOUBLEBUF);
	//SDL_Surface* screen = NULL;
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    // load an image
    SDL_Surface* bmp = SDL_LoadBMP("cb.bmp");
    if (!bmp)
    {
        printf("Unable to load bitmap: %s\n", SDL_GetError());
        return 1;
    }

    // centre the bitmap on screen
    SDL_Rect bgrect;
    bgrect.x = (screen->w - bmp->w) / 2;
    bgrect.y = (screen->h - bmp->h) / 2;

    Uint32 time = SDL_GetTicks();

    bool movementHappened = true;
    bool done = false;
    Tetris t = Tetris(42);
    while(!done)
    {
        if(t.gameOver)
        {
            playback(screen, t.seedNumber, t.input_record);
            FB_PRINTF("Score: %d\n", t.getScore());
            t = Tetris(time);
            movementHappened = true;
        }

        // Keyboard input
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT: done = true; break;
            case SDL_KEYDOWN:
                int input = -1;
                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE: done = true; break;
                    case SDLK_LEFT:  input = 0; break; // Move left
                    case SDLK_RIGHT: input = 1; break; // Move right
                    case SDLK_DOWN:  input = 2; break; // Move down
                    case SDLK_SPACE: input = 3; break; // Drop
                    case SDLK_UP:    input = 4; break; // Rotate
                    default: break;
                }
                if(input != -1)
                {
                    t.input(input);
                    movementHappened = true;
                }
                break;
            }
        }

        Uint32 oldTime = time;
        time = SDL_GetTicks();
        float deltaTime = (time - oldTime) / 1000.0f; // Elapsed time in seconds

        movementHappened = movementHappened || t.update(deltaTime);

        // DRAWING STARTS HERE
        if(movementHappened)
        {
            movementHappened = false;
            SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
            SDL_BlitSurface(bmp, 0, screen, &bgrect);
            Uint32 c = SDL_MapRGB(screen->format, (time + 100)%256, (time + 200)%256, time%256);
            Uint32 p = SDL_MapRGB(screen->format, 190, 255, 200);

            std::vector<math::VC2> blocks;
            t.getFieldPositions(blocks);
            SDL_Rect r = {1,1,(Uint16)(resolution.x / 10), (Uint16)(resolution.y / 22)};
            for(Uint32 i = 0; i < blocks.size(); ++i)
            {
                r.x = resolution.x * blocks[i].x + 0.5;
                r.y = resolution.y-resolution.y * blocks[i].y - r.h + 0.5;
                SDL_FillRect(screen, &r, c);
            }
            blocks.clear();
            t.getPiecePositions(blocks);
            for(Uint32 i = 0; i < blocks.size(); ++i)
            {
                r.x = resolution.x * blocks[i].x + 0.5;
                r.y = resolution.y-resolution.y * blocks[i].y - r.h + 0.5;
                SDL_FillRect(screen, &r, p);
            }
            SDL_Flip(screen);

        }
        // DRAWING ENDS HERE

        SDL_Delay(8);
    } // end main loop

    SDL_FreeSurface(bmp);
    // free loaded bitmap
	return 0;
}
