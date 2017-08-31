#include <stdio.h>
#include <time.h>
#include <math.h>

#define SNAKE_ARRAY_SIZE 310

void *virtual_base;
int fd;
int loop_count;
int led_direction;
int led_mask;

#define letterP 0b1110011
#define letterL 0b111000
#define letterA 0b1110111
#define letterY 0b1101110
#define letterE 0b1111001
#define letterR 0b1010000
#define letter1 0b110
#define letter2 0b1011011
#define letterW1 0b110
#define letterW2 0b11110
#define letterI 0b110
#define letterN 0b1010100
#define letterS 0b1101101
#define letterO 0b111111
#define letterU 0b111110
#define letterD 0b1011110

#ifdef _WIN32
	//Windows Libraries
	#include <conio.h>

	//Windows Constants
	//Controls
	#define UP_ARROW 72
	#define LEFT_ARROW 75
	#define RIGHT_ARROW 77
	#define DOWN_ARROW 80
	
	#define ENTER_KEY 13
	
	const char SNAKE_HEAD = (char)177;
	const char SNAKE_BODY = (char)178;
	const char WALL = (char)219;	
	const char FOOD = (char)254;
	const char BLANK = ' ';
#else
	//Linux Libraries

	#include <stdlib.h>
	#include <termios.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/mman.h>
	#include "hwlib.h"
	#include "soc_cv_av/socal/socal.h"
	#include "soc_cv_av/socal/hps.h"
	#include "soc_cv_av/socal/alt_gpio.h"
	#include "hps_0.h"
	#include "led.h"
	#include "seg7.h"
	#include <stdbool.h>

	#define HW_REGS_BASE ( ALT_STM_OFST )
	#define HW_REGS_SPAN ( 0x04000000 )
	#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

	volatile unsigned long *h2p_lw_led_addr=NULL;
	volatile unsigned long *h2p_lw_hex_addr=NULL;

	void *virtual_base;
	int fd;

unsigned char szMap2[] = {
        63, 6, 91, 79, 102, 109, 125, 7, 
        127, 111, 119, 124, 57, 94, 121, 113
    };  // 0,1,2,....9, a, b, c, d, e, f
	
	//Linux Constants

	//Controls (arrow keys for Ubuntu) 
	#define UP_ARROW (char)'A' //Originally I used constants but borland started giving me errors, so I changed to #define - I do realize that is not the best way.
	#define LEFT_ARROW (char)'D'
	#define RIGHT_ARROW (char)'C'
	#define DOWN_ARROW (char)'B'

	#define ENTER_KEY 10
	
	const char SNAKE_HEAD = 'X';
	const char SNAKE_BODY = '#';
	const char WALL = '#';	
	const char FOOD = '*';
	const char BLANK = ' ';

	/* 
	 * PLEASE NOTE I DID NOT WRITE THESE LINUX FUNCTIONS, 
	 * THE ONLY REASON I'M USING THEM IS TO MAKE THIS GAME 
	 * CROSS PLATFORM. THESE FUNCTIONS ARE AN ALTERNATIVE TO 
	 * THE WINDOWS HEADER FILE "conio.h".
	*/
	
	//Linux Functions - These functions emulate some functions from the windows only conio header file
	//Code: http://ubuntuforums.org/showthread.php?t=549023
	void gotoxy(int x,int y)
	{
		printf("%c[%d;%df",0x1B,y,x);
	}

	//http://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html
	int kbhit(void)
	{
	  struct termios oldt, newt;
	  int ch;
	  int oldf;

	  tcgetattr(STDIN_FILENO, &oldt);
	  newt = oldt;
	  newt.c_lflag &= ~(ICANON | ECHO);
	  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	  ch = getchar();

	  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	  fcntl(STDIN_FILENO, F_SETFL, oldf);

	  if(ch != EOF)
	  {
		ungetc(ch, stdin);
		return 1;
	  }

	  return 0;
	}

	//http://www.experts-exchange.com/Programming/Languages/C/Q_10119844.html - posted by jos
	char getch()
	{
		char c;
		system("stty raw");
		c= getchar();
		system("stty sane");
		//printf("%c",c);
		return(c);
	}

	void clrscr()
	{
		system("clear");
		return;
	}
	//End linux Functions
#endif

//This should be the same on both operating systems
#define EXIT_BUTTON 27 //ESC
#define PAUSE_BUTTON 112 //P

char waitForAnyKey(void)
{
	int pressed;
	
	while(!kbhit());
	
	pressed = getch();
	//pressed = tolower(pressed);
	return((char)pressed);
}

int getGameSpeed(void)
{
	int speed;
	clrscr();
	
	do
	{
		gotoxy(10,5);
		printf("Select The game speed between 1 and 9.");
		speed = waitForAnyKey()-48;
	} while(speed < 1 || speed > 9);
	return(speed);
}

void pauseMenu(void)
{
	int i;
	
	gotoxy(28,23);
	printf("**Paused**");
	
	waitForAnyKey();
	gotoxy(28,23);
	printf("            ");

	return;
}

//Need to think of a better name for this function
//This function checks if a key has pressed, then checks if its any of the arrow keys/ p/esc key. It changes direction acording to the key pressed.
int checkKeysPressed(int direction)
{
	int pressed;
	
	if(kbhit()) //If a key has been pressed
	{
		pressed=getch();
		if (direction != pressed)
		{	
			/*This needs to be fixed up... This code prevents the user from directing the snake to slither over itself... 
			Although it works a majority of the time... If the user changes direction(and the snake doesn't move), 
			then quickly directs the snake back onto itself there's a collision and the game ends 
			perhaps anthor variable is required to prevent this. <- FIXED, Need to test.
			*/
			if(pressed == DOWN_ARROW && direction != UP_ARROW)
				direction = pressed;
			else if (pressed == UP_ARROW && direction != DOWN_ARROW)
				direction = pressed;
			else if (pressed == LEFT_ARROW && direction != RIGHT_ARROW)
				direction = pressed;
			else if (pressed == RIGHT_ARROW && direction != LEFT_ARROW)
				direction = pressed;
			else if (pressed == EXIT_BUTTON || pressed == PAUSE_BUTTON)
				pauseMenu();
		}
	}
	return(direction);
}

//Cycles around checking if the x y coordinates ='s the snake coordinates as one of this parts
//One thing to note, a snake of length 4 cannot collide with itself, therefore there is no need to call this function when the snakes length is <= 4
int collisionSnake (int x, int y, int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength, int detect)
{
	int i;
	for (i = detect; i < snakeLength; i++) //Checks if the snake collided with itself
	{
		if ( x == snakeXY[0][i] && y == snakeXY[1][i])
			return(1);
	}
	return(0);
}

//Generates food & Makes sure the food doesn't appear on top of the snake <- This sometimes causes a lag issue!!! Not too much of a problem tho
int generateFood(int foodXY[], int width, int height, int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength)
{
	int i;
	
	do
	{
		srand ( time(NULL) );
		foodXY[0] = rand() % (width-2) + 2;
		srand ( time(NULL) );
		foodXY[1] = rand() % (height-6) + 2;
	} while (collisionSnake(foodXY[0], foodXY[1], snakeXY, snakeLength, 0)); //This should prevent the "Food" from being created on top of the snake. - However the food has a chance to be created ontop of the snake, in which case the snake should eat it...

	gotoxy(foodXY[0] ,foodXY[1]);
	printf("%c", FOOD);
	
	return(0);
}

/*
Moves the snake array forward, i.e. 
This:
 x 1 2 3 4 5 6
 y 1 1 1 1 1 1
Becomes This:
 x 1 1 2 3 4 5
 y 1 1 1 1 1 1
 
 Then depending on the direction (in this case west - left) it becomes:
 
 x 0 1 2 3 4 5
 y 1 1 1 1 1 1
 
 snakeXY[0][0]--; <- if direction left, take 1 away from the x coordinate
*/
void moveSnakeArray(int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength, int direction)
{
	int i;
	for( i = snakeLength-1; i >= 1; i-- )
	{
		snakeXY[0][i] = snakeXY[0][i-1];
		snakeXY[1][i] = snakeXY[1][i-1];
	}	
	
	/*
	because we dont actually know the new snakes head x y, 
	we have to check the direction and add or take from it depending on the direction.
	*/
	switch(direction)
	{
		case DOWN_ARROW:
			snakeXY[1][0]++;
			break;
		case RIGHT_ARROW:
			snakeXY[0][0]++;
			break;
		case UP_ARROW:
			snakeXY[1][0]--;
			break;
		case LEFT_ARROW:
			snakeXY[0][0]--;
			break;			
	}
	
	return;
}

void move(int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength, int direction)
{
	int x;
	int y;

	//Remove the tail ( HAS TO BE DONE BEFORE THE ARRAY IS MOVED!!!!! )
	x = snakeXY[0][snakeLength-1];
	y = snakeXY[1][snakeLength-1];
	
	gotoxy(x,y);
	printf("%c",BLANK);	
	
	//Changes the head of the snake to a body part
	gotoxy(snakeXY[0][0],snakeXY[1][0]);	
	printf("%c", SNAKE_BODY);
	
	moveSnakeArray(snakeXY, snakeLength, direction);
	
	gotoxy(snakeXY[0][0],snakeXY[1][0]);	
	printf("%c",SNAKE_HEAD);
	
	gotoxy(1,1); //Gets rid of the darn flashing underscore.
	
	return;
}

int eatFood(int snakeXY[][SNAKE_ARRAY_SIZE], int foodXY[]) //This function checks if the snakes head his on top of the food, if it is then it'll generate some more food...
{
	if (snakeXY[0][0] == foodXY[0] && snakeXY[1][0] == foodXY[1])
	{
		foodXY[0] = 0;
		foodXY[1] = 0; //This should prevent a nasty bug (loops) need to check if the bug still exists...
		
		printf("\7"); //Beep
		return(1);
	}		
	
	return(0);
}

int collisionDetection(int snakeXY[][SNAKE_ARRAY_SIZE], int consoleWidth, int consoleHeight, int snakeLength ) //Need to Clean this up a bit
{
	int colision = 0;
	if ((snakeXY[0][0] == 1) || (snakeXY[1][0] == 1) || (snakeXY[0][0] == consoleWidth) || (snakeXY[1][0] == consoleHeight - 4)) //Checks if the snake collided wit the wall or it's self
		colision = 1;
	else
		if (collisionSnake(snakeXY[0][0], snakeXY[1][0], snakeXY, snakeLength, 1)) //If the snake collided with the wall, theres no point in checking if it collided with itself.
			colision = 1;
			
	return(colision);
}

void refreshInfoBar(int score, int speed)
{
	gotoxy(5,23);
	printf("Score: %d", score);
	
	gotoxy(5,24);
	printf("Speed: %d", speed);

	gotoxy(52,23);
	printf("Coder: Jeet Baru and Sudeep Kulkarni");

	return;
}

//**************HIGHSCORE STUFF**************//

//-> The highscores system seriously needs to be clean. There are some bugs, entering a name etc

void createHighScores(void)
{
	FILE *file; 
	int i;

	file = fopen("highscores.txt","w+");
	
	if(file == NULL)
	{
		printf("FAILED TO CREATE HIGHSCORES!!! EXITING!");
		exit(0);	
	}
	
	for(i = 0; i < 5; i++)
	{
		fprintf(file,"%d",i+1);
		fprintf(file,"%s","\t0\t\t\tEMPTY\n");
	}	
	
	fclose(file);
	return;
}

int getLowestScore()
{
	FILE *fp;
	char str[128];
	int lowestScore = 0;
	int i;
	int intLength;
	
	if((fp = fopen("highscores.txt", "r")) == NULL)
	{
		//Create the file, then try open it again.. if it fails this time exit.
		createHighScores(); //This should create a highscores file (If there isn't one)
		if((fp = fopen("highscores.txt", "r")) == NULL)
			exit(1);
	}

	while(!feof(fp))
	{
		fgets(str, 126, fp);  
	}
	fclose(fp);	
	
	i=0;
	
	//Gets the Int length
	while(str[2+i] != '\t')
	{
		i++;
	}
	
	intLength = i;
	
	//Gets converts the string to int
	for(i=0;i < intLength; i++)
	{
		lowestScore = lowestScore + ((int)str[2+i] - 48) *(10*intLength-i-1);
	}

	return(lowestScore);
}

void inputScore(int score) //This seriously needs to be cleaned up
{
	FILE *fp;
	FILE *file; 
	char str[20];
	int fScore;
	int i, s, y;
	int intLength;
	int scores[5];
	int x;
	char highScoreName[20];
	char highScoreNames[5][20];
	
	char name[20];
	
	int entered = 0;
	
	clrscr(); //clear the console
	
	if((fp = fopen("highscores.txt", "r")) == NULL)
	{
		//Create the file, then try open it again.. if it fails this time exit.
		createHighScores(); //This should create a highscores file (If there isn't one)
		if((fp = fopen("highscores.txt", "r")) == NULL)
			exit(1);
	}
	gotoxy(10,5);
	printf("Your Score made it into the top 5!!!");
	gotoxy(10,6);
	printf("Please enter your name: ");
	gets(name);
	
	x = 0;
	while(!feof(fp))
	{
		fgets(str, 126, fp);  //Gets a line of text
		
		i=0;
		
		//Gets the Int length
		while(str[2+i] != '\t')
		{
			i++;
		}
		
		s = i;
		intLength = i;
		i=0;
		while(str[5+s] != '\n')
		{
			//printf("%c",str[5+s]);
			highScoreName[i] = str[5+s];
			s++;
			i++;
		}
		//printf("\n");
		
		fScore = 0;
		//Gets converts the string to int
		for(i=0;i < intLength; i++)
		{
			//printf("%c", str[2+i]);
			fScore = fScore + ((int)str[2+i] - 48) *(10*intLength-i-1);
		}
		
		if(score >= fScore && entered != 1)
		{
			scores[x] = score;
			strcpy(highScoreNames[x], name);
			
			//printf("%d",x+1);
			//printf("\t%d\t\t\t%s\n",score, name);		
			x++;
			entered = 1;
		}
		
		//printf("%d",x+1);
		//printf("\t%d\t\t\t%s\n",fScore, highScoreName);
		//strcpy(text, text+"%d\t%d\t\t\t%s\n");
		strcpy(highScoreNames[x], highScoreName);
		scores[x] = fScore;
		
		//highScoreName = "";
		for(y=0;y<20;y++)
		{
			highScoreName[y] = NULL;
		}
		
		x++;
		if(x >= 5)
			break;
	}
	
	fclose(fp);
	
	file = fopen("highscores.txt","w+");
	
	for(i=0;i<5;i++)
	{
		//printf("%d\t%d\t\t\t%s\n", i+1, scores[i], highScoreNames[i]);
		fprintf(file, "%d\t%d\t\t\t%s\n", i+1, scores[i], highScoreNames[i]);	
	}

	fclose(file);
	
	return;
}

void displayHighScores(void) //NEED TO CHECK THIS CODE!!!
{
	FILE *fp;
	char str[128];
	int y = 5;
	
	clrscr(); //clear the console
	
	if((fp = fopen("highscores.txt", "r")) == NULL) {
		//Create the file, then try open it again.. if it fails this time exit.
		createHighScores(); //This should create a highscores file (If there isn't one)
		if((fp = fopen("highscores.txt", "r")) == NULL)
			exit(1);
	}
	
	gotoxy(10,y++);
	printf("High Scores");	
	gotoxy(10,y++);
	printf("Rank\tScore\t\t\tName");
	while(!feof(fp)) {
		gotoxy(10,y++);
		if(fgets(str, 126, fp)) 
			printf("%s", str);
	}

	fclose(fp);	//Close the file
	gotoxy(10,y++);
	
	printf("Press any key to continue...");
	waitForAnyKey();	
	return;
}

//**************END HIGHSCORE STUFF**************//

void youWinScreen(void)
{
	int x = 6, y = 7;
	gotoxy(x,y++);
	printf("'##:::'##::'#######::'##::::'##::::'##:::::'##:'####:'##::: ##:'####:");
	gotoxy(x,y++);
	printf(". ##:'##::'##.... ##: ##:::: ##:::: ##:'##: ##:. ##:: ###:: ##: ####:");
	gotoxy(x,y++);
	printf(":. ####::: ##:::: ##: ##:::: ##:::: ##: ##: ##:: ##:: ####: ##: ####:");
	gotoxy(x,y++);
	printf("::. ##:::: ##:::: ##: ##:::: ##:::: ##: ##: ##:: ##:: ## ## ##:: ##::");
	gotoxy(x,y++);
	printf("::: ##:::: ##:::: ##: ##:::: ##:::: ##: ##: ##:: ##:: ##. ####::..:::");
	gotoxy(x,y++);
	printf("::: ##:::: ##:::: ##: ##:::: ##:::: ##: ##: ##:: ##:: ##:. ###:'####:");
	gotoxy(x,y++);
	printf("::: ##::::. #######::. #######:::::. ###. ###::'####: ##::. ##: ####:");
	gotoxy(x,y++);
	printf(":::..::::::.......::::.......:::::::...::...:::....::..::::..::....::");
	gotoxy(x,y++);	
	
	waitForAnyKey();
	clrscr(); //clear the console
	return;
}

void gameOverScreen(void)
{
	int x = 17, y = 3;
	
	//http://www.network-science.de/ascii/ <- Ascii Art Gen
	
	gotoxy(x,y++);
	printf(":'######::::::'###::::'##::::'##:'########:\n");
	gotoxy(x,y++);
	printf("'##... ##::::'## ##::: ###::'###: ##.....::\n");
	gotoxy(x,y++);
	printf(" ##:::..::::'##:. ##:: ####'####: ##:::::::\n");
	gotoxy(x,y++);
	printf(" ##::'####:'##:::. ##: ## ### ##: ######:::\n");
	gotoxy(x,y++);
	printf(" ##::: ##:: #########: ##. #: ##: ##...::::\n");
	gotoxy(x,y++);
	printf(" ##::: ##:: ##.... ##: ##:.:: ##: ##:::::::\n");
	gotoxy(x,y++);
	printf(". ######::: ##:::: ##: ##:::: ##: ########:\n");
	gotoxy(x,y++);
	printf(":......::::..:::::..::..:::::..::........::\n");
	gotoxy(x,y++);
	printf(":'#######::'##::::'##:'########:'########::'####:\n");
	gotoxy(x,y++);
	printf("'##.... ##: ##:::: ##: ##.....:: ##.... ##: ####:\n");
	gotoxy(x,y++);
	printf(" ##:::: ##: ##:::: ##: ##::::::: ##:::: ##: ####:\n");
	gotoxy(x,y++);
	printf(" ##:::: ##: ##:::: ##: ######::: ########::: ##::\n");
	gotoxy(x,y++);
	printf(" ##:::: ##:. ##:: ##:: ##...:::: ##.. ##::::..:::\n");
	gotoxy(x,y++);
	printf(" ##:::: ##::. ## ##::: ##::::::: ##::. ##::'####:\n");
	gotoxy(x,y++);
	printf(". #######::::. ###:::: ########: ##:::. ##: ####:\n");
	gotoxy(x,y++);
	printf(":.......::::::...:::::........::..:::::..::....::\n");

	int i;

	for(i=0;i<10;i++)
	{
		alt_write_word(h2p_lw_led_addr, 0x3FF);
		usleep(500*1000);
		alt_write_word(h2p_lw_led_addr, 0x00);
		usleep(500*1000);
	}

	for(i=0;i<SEG7_NUM;i++){
       		SEG7_SET(i, 0x00);
	}	

	waitForAnyKey();
	clrscr(); //clear the console
	return;
}

//Messy, need to clean this function up
void startGame( int snakeXY[][SNAKE_ARRAY_SIZE], int foodXY[], int consoleWidth, int consoleHeight, int snakeLength, int direction, int score, int speed)
{
	int gameOver = 0;
	clock_t endWait;
	
	//CLOCKS_PER_SEC-(n-1)*(CLOCKS_PER_SEC/10)
	int waitMili = CLOCKS_PER_SEC-(speed)*(CLOCKS_PER_SEC/10);	//Sets the correct wait time according to the selected speed
	int tempScore = 10*speed;
	int oldDirection;
	int canChangeDirection = 1;
	//int seconds = 1;

	endWait = clock() + waitMili;

	do
	{
		if(canChangeDirection)
		{
			oldDirection = direction;
			direction = checkKeysPressed(direction);
		}
		
		if(oldDirection != direction)//Temp fix to prevent the snake from colliding with itself
			canChangeDirection = 0;
			
		if(clock() >= endWait) //haha, it moves according to how fast the computer running it is...
		{
			//gotoxy(1,1);
			//printf("%d - %d",clock() , endWait);
			move(snakeXY, snakeLength, direction);
			canChangeDirection = 1;

				
			if(eatFood(snakeXY, foodXY))
			{
				generateFood( foodXY, consoleWidth, consoleHeight, snakeXY, snakeLength); //Generate More Food
				snakeLength++;
				score+=speed;
				score_fpga(score);
				//x++;
				//gotoxy(1,1);
				//printf("%d >= %d", 10*speed+score, tempScore);
				if( score >= 10*speed+tempScore)
				//if( 2 >= 2)
				{
					speed++;
					tempScore = score;

					if(speed <= 9)//this needs to be fixed
						waitMili = waitMili - (CLOCKS_PER_SEC/10);
					else
					{
						if(waitMili >= 40) //Maximum Speed (the game has to be beatable)
							waitMili = waitMili - (CLOCKS_PER_SEC/200);
						
					}
					//level++;
					//gotoxy(1,2);
					//printf("    ");
					//gotoxy(1,2);
					//printf("%d",waitMili);
					//x = 0;
				}
				
				refreshInfoBar(score, speed);
			}
			
			endWait = clock() + waitMili; //TEMP FIX, NEED TO FIND A WAY TO RESET CLOCK().. Na, seems to work fine this way...
		}
		
		gameOver = collisionDetection(snakeXY, consoleWidth, consoleHeight, snakeLength);

		if(snakeLength >= SNAKE_ARRAY_SIZE-5) //Just to make sure it doesn't get longer then the array size & crash
		{
			gameOver = 2;//You Win! <- doesn't seem to work - NEED TO FIX/TEST THIS
			score+=1500; //When you win you get an extra 1500 points!!!
		}
		
	} while (!gameOver);
	
	switch(gameOver)
	{
		case 1:
			printf("\7"); //Beep
			printf("\7"); //Beep

			gameOverScreen();

			break;
		case 2:
			youWinScreen();
			break;
	}
	
	if(score >= getLowestScore() && score != 0)
	{
		inputScore(score);
		displayHighScores();
	}
	
	return;
}

void loadEnviroment(int consoleWidth, int consoleHeight)//This can be done in a better way... FIX ME!!!! Also i think it doesn't work properly in ubuntu <- Fixed
{
	int i;
	int x = 1, y = 1;
	int rectangleHeight = consoleHeight - 4;
	clrscr(); //clear the console
	
	gotoxy(x,y); //Top left corner
	
	for (; y < rectangleHeight; y++)
	{
		gotoxy(x, y); //Left Wall 
		printf("%c",WALL);
		
		gotoxy(consoleWidth, y); //Right Wall
		printf("%c",WALL);
	}
	
	y = 1;
	for (; x < consoleWidth+1; x++)
	{
		gotoxy(x, y); //Left Wall 
		printf("%c",WALL);
		
		gotoxy(x, rectangleHeight); //Right Wall
		printf("%c",WALL);
	}
	
/*
	for (i = 0; i < 80; i++)
	{
		printf("%c",WALL);
	}
	
	for (i = 0; i < 17; i++)
	{
		printf("%c\n",WALL);
	}

	for (i = 0; i < 21; i++)
	{
		printf("%c\n",WALL);
		gotoxy(80,i);
	}
	
	for (i = 0; i < 81; i++)
	{
		printf("%c",WALL);
	}	
*/	
	return;
}

void loadSnake(int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength)
{
	int i;
	/*
	First off, The snake doesn't actually have enough XY coordinates (only 1 - the starting location), thus we use
	these XY coordinates to "create" the other coordinates. For this we can actually use the function used to move the snake.
	This helps create a "whole" snake instead of one "dot", when someone starts a game.
	*/
	//moveSnakeArray(snakeXY, snakeLength); //One thing to note ATM, the snake starts of one coordinate to whatever direction it's pointing...
	
	//This should print out a snake :P
	for (i = 0; i < snakeLength; i++)
	{
		gotoxy(snakeXY[0][i], snakeXY[1][i]);
		printf("%c", SNAKE_BODY); //Meh, at some point I should make it so the snake starts off with a head...
	}
	
	return;
}

/* NOTE, This function will only work if the snakes starting direction is left!!!! 
Well it will work, but the results wont be the ones expected.. I need to fix this at some point.. */
void prepairSnakeArray(int snakeXY[][SNAKE_ARRAY_SIZE], int snakeLength)
{
	int i, x;
	int snakeX = snakeXY[0][0];
	int snakeY = snakeXY[1][0];
	
	// this is used in the function move.. should maybe create a function for it...
	/*switch(direction)
	{
		case DOWN_ARROW:
			snakeXY[1][0]++;
			break;
		case RIGHT_ARROW:
			snakeXY[0][0]++;
			break;
		case UP_ARROW:
			snakeXY[1][0]--;
			break;
		case LEFT_ARROW:
			snakeXY[0][0]--;
			break;			
	}
	*/
	
	
	for(i = 1; i <= snakeLength; i++)
	{
		snakeXY[0][i] = snakeX + i;
		snakeXY[1][i] = snakeY;
	}
	
	return;
}

//This function loads the enviroment, snake, etc
void loadGame(void)
{
	int snakeXY[2][SNAKE_ARRAY_SIZE]; //Two Dimentional Array, the first array is for the X coordinates and the second array for the Y coordinates
	
	int snakeLength = 4; //Starting Length
	
	int direction = LEFT_ARROW; //DO NOT CHANGE THIS TO RIGHT ARROW, THE GAME WILL INSTANTLY BE OVER IF YOU DO!!! <- Unless the prepairSnakeArray function is changed to take into account the direction....
	
	int foodXY[] = {5,5};// Stores the location of the food
	
	int score = 0;
	//int level = 1;
	
	//Window Width * Height - at some point find a way to get the actual dimensions of the console... <- Also somethings that display dont take this dimentions into account.. need to fix this...
	int consoleWidth = 80;
	int consoleHeight = 25;
	
	int speed = getGameSpeed();
	
	//The starting location of the snake
	snakeXY[0][0] = 40; 
	snakeXY[1][0] = 10;
	
	loadEnviroment(consoleWidth, consoleHeight); //borders
	prepairSnakeArray(snakeXY, snakeLength);
	loadSnake(snakeXY, snakeLength);
	generateFood( foodXY, consoleWidth, consoleHeight, snakeXY, snakeLength);
	refreshInfoBar(score, speed); //Bottom info bar. Score, Level etc
	startGame(snakeXY, foodXY, consoleWidth, consoleHeight, snakeLength, direction, score, speed);

	return;
}

//**************MENU STUFF**************//

int menuSelector(int x, int y, int yStart)
{
	char key;
	int i = 0;
	x = x - 2;
	gotoxy(x,yStart);
	
	printf(">");
	
	gotoxy(1,1);

	
	do
	{
		key = waitForAnyKey();
		//printf("%c %d", key, (int)key);
		if ( key == (char)UP_ARROW )
		{
			gotoxy(x,yStart+i);
			printf(" ");
			
			if (yStart >= yStart+i ) 
				i = y - yStart - 2;
			else
				i--;
			gotoxy(x,yStart+i);
			printf(">");
		}
		else
			if ( key == (char)DOWN_ARROW )
			{
				gotoxy(x,yStart+i);
				printf(" ");
				
				if (i+2 >= y - yStart ) 
					i = 0;
				else
					i++;
				gotoxy(x,yStart+i);
				printf(">");				
			}	
			//gotoxy(1,1);
			//printf("%d", key);
	} while(key != (char)ENTER_KEY); //While doesn't equal enter... (13 ASCII code for enter) - note ubuntu is 10
	return(i);
}

void welcomeArt(void)
{
	clrscr(); //clear the console
	//Ascii art reference: http://www.chris.com/ascii/index.php?art=animals/reptiles/snakes
	printf("\n");	
	printf("\t\t    _________         _________ 			\n");	
	printf("\t\t   /         \\       /         \\ 			\n");	
	printf("\t\t  /  /~~~~~\\  \\     /  /~~~~~\\  \\ 			\n");	
	printf("\t\t  |  |     |  |     |  |     |  | 			\n");		
	printf("\t\t  |  |     |  |     |  |     |  | 			\n");
	printf("\t\t  |  |     |  |     |  |     |  |         /	\n");
	printf("\t\t  |  |     |  |     |  |     |  |       //	\n");
	printf("\t\t (o  o)    \\  \\_____/  /     \\  \\_____/ / 	\n");
	printf("\t\t  \\__/      \\         /       \\        / 	\n");
	printf("\t\t    |        ~~~~~~~~~         ~~~~~~~~ 		\n");
	printf("\t\t    ^											\n");
	printf("\t		Welcome To The Snake Game!			\n");
	printf("\t			    Press Any Key To Continue...	\n");			
	printf("\n");
	
	waitForAnyKey();
	return;
}

void controls(void)
{
	int x = 10, y = 5;
	clrscr(); //clear the console
	gotoxy(x,y++);
	printf("Controls\n");
	gotoxy(x++,y++);
	printf("Use the following arrow keys to direct the snake to the food: ");
	gotoxy(x,y++);
	printf("Right Arrow");
	gotoxy(x,y++);
	printf("Left Arrow");
	gotoxy(x,y++);
	printf("Top Arrow");
	gotoxy(x,y++);
	printf("Bottom Arrow");
	gotoxy(x,y++);
	gotoxy(x,y++);
	printf("P & Esc pauses the game.");
	gotoxy(x,y++);
	gotoxy(x,y++);
	printf("Press any key to continue...");
	waitForAnyKey();
	return;
}

int exitYN(void)
{
	char pressed;
	gotoxy(9,8);
	printf("Are you sure you want to exit(Y/N)\n");
	
	do
	{
		pressed = waitForAnyKey();
		pressed = tolower(pressed);
	} while (!(pressed == 'y' || pressed == 'n'));
	
	if (pressed == 'y')
	{
		clrscr(); //clear the console
		return 1;
	}
	return 0;
}

int mainMenu(void)
{
	int x = 10, y = 5;
	int yStart = y;
	
	int selected;
	
	clrscr(); //clear the console
	//Might be better with arrays of strings???
	gotoxy(x,y++);
	printf("New Game\n");
	gotoxy(x,y++);
	printf("High Scores\n");
	gotoxy(x,y++);
	printf("Controls\n");
	gotoxy(x,y++);
	printf("Exit\n");
	gotoxy(x,y++);

	selected = menuSelector(x, y, yStart);

	return(selected);
}

int led_lcd_init()
{
	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}
	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );	
	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return(1);
	}
	h2p_lw_led_addr=virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + LED_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
	h2p_lw_hex_addr=virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + SEG7_IF_BASE ) & ( unsigned long)( HW_REGS_MASK ) );

	return 0;

}

int led_lcd_end()
{
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );

	}
	close( fd );
	return 0;
}

int score_fpga(int score)
{	

	int i;
 	for(i=0;i<SEG7_NUM;i++){
        	SEG7_SET(i, 0x00);
	}

	int a;
	i =0;
		
	while(score != 0)
	{
		a=score%10;
		SEG7_SET(i,szMap2[a]);
		score = score / 10;
		i++;
	}
	return 0;

}

//**************END MENU STUFF**************//

int snakes() 
{

	welcomeArt();

	int exit = 0;

	alt_write_word(h2p_lw_led_addr, 0x00);

	int i;
  	for(i=0;i<SEG7_NUM;i++){
        	SEG7_SET(i, 0x00);
	}
	
	do
	{	
		switch(mainMenu())
		{
			case 0:
				loadGame();
				break;
			case 1:
				displayHighScores();
				break;	
			case 2:
				controls();
				break;		
			case 3:
				exit = exitYN();
				break;			
		}		
	} while(!exit);	//
	
	return(0);
}

int tic_tac()
{
  int i = 0;                                   /* Loop counter                         */
  int player = 0;                              /* Player number - 1 or 2               */
  int go = 0;                                  /* Square selection number for turn     */
  int row = 0;                                 /* Row index for a square               */  
  int column = 0;                              /* Column index for a square            */
  int line = 0;                                /* Row or column index in checking loop */
  int winner = 0;                              /* The winning player                   */
  char board[3][3] = {                         /* The board                            */
                       {'1','2','3'},          /* Initial values are reference numbers */
                       {'4','5','6'},          /* used to select a vacant square for   */
                       {'7','8','9'}           /* a turn.                              */
                     };

   /* The main game loop. The game continues for up to 9 turns */
   /* As long as there is no winner                            */
   for( i = 0; i<9 && winner==0; i++)
   {

	clrscr();
      /* Display the board */
    printf("\n\n");
      printf(" %c | %c | %c\n", board[0][0], board[0][1], board[0][2]);
      printf("---+---+---\n");
      printf(" %c | %c | %c\n", board[1][0], board[1][1], board[1][2]);
      printf("---+---+---\n");
      printf(" %c | %c | %c\n", board[2][0], board[2][1], board[2][2]);
      
      player = i%2 + 1;                           /* Select player */

      if(player == 1)
      {
	SEG7_SET(0, letter1);
	SEG7_SET(1, letterR);
	SEG7_SET(2, letterY);
	SEG7_SET(3, letterA);
	SEG7_SET(4, letterL);
	SEG7_SET(5, letterP);
      }

      if(player == 2)
      {
	SEG7_SET(0, letter2);
	SEG7_SET(1, letterR);
	SEG7_SET(2, letterY);
	SEG7_SET(3, letterA);
	SEG7_SET(4, letterL);
	SEG7_SET(5, letterP);
      }
 
      /* Get valid player square selection */
      do
      {
         printf("\nPlayer %d, please enter the number of the square "
       "where you want to place your %c: ", player,(player==1)?'X':'O');
         scanf("%d", &go);


         row = --go/3;                                 /* Get row index of square      */
         column = go%3;                                /* Get column index of square   */
	
	//clrscr();
      }while(go<0 || go>9 || board[row][column]>'9');

      board[row][column] = (player == 1) ? 'X' : 'O';        /* Insert player symbol   */

      /* Check for a winning line - diagonals first */     
      if((board[0][0] == board[1][1] && board[0][0] == board[2][2]) ||
         (board[0][2] == board[1][1] && board[0][2] == board[2][0]))
        winner = player;
      else
      /* Check rows and columns for a winning line */
        for(line = 0; line <= 2; line ++)
          if((board[line][0] == board[line][1] && board[line][0] == board[line][2])||
             (board[0][line] == board[1][line] && board[0][line] == board[2][line]))
            winner = player;
      

   }
 
  clrscr();

   /* Game is over so display the final board */
   printf("\n\n");
   printf(" %c | %c | %c\n", board[0][0], board[0][1], board[0][2]);
   printf("---+---+---\n");
   printf(" %c | %c | %c\n", board[1][0], board[1][1], board[1][2]);
   printf("---+---+---\n");
   printf(" %c | %c | %c\n", board[2][0], board[2][1], board[2][2]);

   /* Display result message */
   if(winner == 0)
      printf("\nHow boring, it is a draw\n");
   else
      printf("\nCongratulations, player %d, YOU ARE THE WINNER!\n", winner);

  int j;

if (winner == 1)
{
  for(j=0;j<10;j++)
  {
	alt_write_word(h2p_lw_led_addr, 0x3FF);
	SEG7_SET(0, letter1);
	SEG7_SET(1, letterR);
	SEG7_SET(2, letterY);
	SEG7_SET(3, letterA);
	SEG7_SET(4, letterL);
	SEG7_SET(5, letterP);
	usleep(500*1000);
	alt_write_word(h2p_lw_led_addr, 0x00);
	SEG7_SET(1, letterS);
	SEG7_SET(2, letterN);
	SEG7_SET(3, letterI);
	SEG7_SET(4, letterW2);
	SEG7_SET(5, letterW1);
	SEG7_SET(0, 0x00);
	usleep(500*1000);
  }
}

else if (winner == 2)
{
  for(j=0;j<10;j++)
  {
	alt_write_word(h2p_lw_led_addr, 0x3FF);
	SEG7_SET(0, letter2);
	SEG7_SET(1, letterR);
	SEG7_SET(2, letterY);
	SEG7_SET(3, letterA);
	SEG7_SET(4, letterL);
	SEG7_SET(5, letterP);
	usleep(500*1000);
	alt_write_word(h2p_lw_led_addr, 0x00);
	SEG7_SET(1, letterS);
	SEG7_SET(2, letterN);
	SEG7_SET(3, letterI);
	SEG7_SET(4, letterW2);
	SEG7_SET(5, letterW1);
	SEG7_SET(0, 0x00);
	usleep(500*1000);
  }
}

else
{
  SEG7_SET(1, letterW2);
  SEG7_SET(2, letterW1);
  SEG7_SET(3, letterA);
  SEG7_SET(4, letterR);
  SEG7_SET(5, letterD);
  SEG7_SET(0, 0x00);

  for(j=0;j<10;j++)
  {
	alt_write_word(h2p_lw_led_addr, 0x3FF);
	usleep(500*1000);
	alt_write_word(h2p_lw_led_addr, 0x00);
	usleep(500*1000);
  }
}

  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int minesweeper_game()
{
    char grid[6][6],field[6][6],m[2],menu,tempstr[10];
    int gridn[6][6]={0};
    int i,j,x,y,xs,ys,mine=0,i1,j1,nof,a=0;

    clrscr();

    for(;;)
    {
        clrscr();
        printf("Minesweeper 6x6\nversion 1.0\n\n");
        printf("1.Play Minesweeper\n2.Rules\n3.Exit\n");
        menu=getch();
        if(menu==49)
            break;
        else if(menu==50)
        {
            clrscr();
            printf("This is the classic game of minesweeper written in C.\nRules for minesweeper:\n");
            printf("1. You are in a mine field and you have to successfully flag all the places which have a mine(%c). If you flag all the mines, You win!\n",15);
            printf("2.In your first turn, you have to choose a starting square. You can open a square by entering its row number(x) and column number(y)\n(Note: row and column number starts from 1)\n");
            printf("3.There are three modes of operation:\n");
            printf("(i) open mode. type 'o' in mode option. This mode lets the user to open a square\n");
            printf("(ii) flag mode. type 'f' in mode option. This mode lets the user to flag a particular square. Flagged square is denoted by an 'F'\n");
            printf("(iii) remove flag mode. type 'r' in mode option. This mode lets the user to remove a particular flag from a flagged square\n");
            printf("4.If you open a square with a mine, you lose\n");
            printf("5.If you open a square with a number written on it. The number shows that how many mines are there in the adjacent 8 squares\n");
            printf("For eg:\n%c %c %c\n\n%c 4 %c\n\n%c %c %c",219,219,219,219,219,219,219,219);
            printf("\nHere 4 denoted that there are 4 mines in the remaining uncovered squares");
            printf("\n\npress enter to continue.....");
            fflush(stdin);
            gets(tempstr);
        }
	else
	   return 0;
    }
    clrscr();
    for(i=0;i<6;i++)
    {
        for(j=0;j<6;j++)
            field[i][j]=219;
    }
    for(i=0;i<6;i++)
    {
        for(j=0;j<30;j++)
            printf(" ");
        for(j=0;j<6;j++)
            printf("%c ",field[i][j]);
        printf("\n\n");
    }

    for(;;)
    {
	SEG7_SET(0, 0x00);
	SEG7_SET(1, 0x00);
	SEG7_SET(2, letterY);
	SEG7_SET(3, letterA);
	SEG7_SET(4, letterL);
	SEG7_SET(5, letterP);
    printf("\nOpen the starting square.\nx:");
    scanf("%d",&xs);
    printf("y:");
    scanf("%d", &ys);
    if(xs>6 || ys>6 || xs<1 || ys<1)
    {
        printf("Row or column not defined. Try again\n");
        continue;
    }
    else break;
    }
    xs=xs-1;
    ys=ys-1;
    srand(time(NULL));
    for(;;)
    {
        i=rand()%6;
        j=rand()%6;
        if(grid[i][j]!=42 && i!=xs && j!=ys)
            grid[i][j]=42;
        else
        continue;
        mine++;
        if(mine==9)
            break;
    }
    nof=mine;
    for(i=0;i<6;i++)
    {
        for(j=0;j<6;j++)
        {
            if(grid[i][j]!=42)
            {
                for(i1=i-1;i1<=i+1;i1++)
                {
                    for(j1=j-1;j1<=j+1;j1++)
                    {
                        if(grid[i1][j1]==42 && 0<=i1 && 0<=j1 &&i1<=5 && j1<=5)
                            gridn[i][j]++;
                    }
                }
                grid[i][j]=gridn[i][j] + 48;

            }
        }
    }
    for(i=xs-1;i<=xs+1;i++)
    {
        for(j=ys-1;j<=ys+1;j++)
        {
            if(grid[i][j]!=42)
                field[i][j]=grid[i][j];
        }
    }
    x=xs;
    y=ys;
    for(;;)
    {
        clrscr();
        /*for(i=0;i<6;i++)
        {
            for(j=0;j<6;j++)
                printf("%c ", grid[i][j]);
            printf("\n\n");
        }*/
        for(i=0;i<6;i++)
        {
            for(j=0;j<30;j++)
                printf(" ");
            for(j=0;j<6;j++)
                printf("%c ", field[i][j]);
            printf("\n\n");
        }
        printf("no of flags: %d\n", nof);
        if(grid[x][y]==42 && strcmp(m,"o")==0)
        {
            printf("\ngame over\n");
	    SEG7_SET(0, letterE);
	    SEG7_SET(1, letterS);
	    SEG7_SET(2, letterO);
	    SEG7_SET(3, letterL);
	    SEG7_SET(4, 0x00);
	    SEG7_SET(5, letterU);
	    for(j=0;j<10;j++)
	    {
		alt_write_word(h2p_lw_led_addr, 0x3FF);
		usleep(500*1000);		
		alt_write_word(h2p_lw_led_addr, 0x00);
		usleep(500*1000);
  	    }
            break;
        }
        for(i=0;i<6;i++)
        {
            for(j=0;j<6;j++)
            {
                if(grid[i][j]==42 && field[i][j]==70)
                    a++;
            }
        }
        if(a==mine)
        {
            printf("You win\n");
	    SEG7_SET(0, letterN);
	    SEG7_SET(1, letterI);
	    SEG7_SET(2, letterW2);
	    SEG7_SET(3, letterW1);
	    SEG7_SET(4, 0x00);
	    SEG7_SET(5, letterU);
	    for(j=0;j<10;j++)
 	    {
		alt_write_word(h2p_lw_led_addr, 0x3FF);
		usleep(500*1000);
		alt_write_word(h2p_lw_led_addr, 0x00);
		usleep(500*1000);
  	   }
           break;
        }
        a=0;
        printf("mode:");
        scanf("%s",&m);
        printf("x:");
        scanf("%d",&x);
        printf("y:");
        scanf("%d",&y);
        x=x-1;
        y=y-1;
        if(strcmp(m,"o")==0)
            field[x][y]=grid[x][y];
        if(strcmp(m,"f")==0 && field[x][y]!=70 && field[x][y]==-37)
        {field[x][y]=70;
            nof--;}
        if(strcmp(m,"r")==0)
        {
            if(field[x][y]==70)
            {
                field[x][y]=219;
                nof++;
            }
        }
        if(grid[x][y]==42 && strcmp(m,"o")==0)
        {
            for(i=0;i<6;i++)
            {
                for(j=0;j<6;j++)
                {
                    if(grid[i][j]==42)
                        field[i][j]=grid[i][j];
                }
            }
        }
    }
    return 0;
}

void delay(clock_t a)
{
    clock_t start;
    start=clock();
    while(clock()-start<a)
    {

    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main ()
{

	char choice;

	led_lcd_init();
	int i;

	while(1)
	{	
		clrscr();

		for(i=0;i<6;i++)
			SEG7_SET(i,0x00);
	
		printf("\n\n\t Welcome What game would you like to play \n\n");
		printf("\t\t 1. Snakes\n\n");
		printf("\t\t 2. TicTacToe\n\n");
		printf("\t\t 3. Minesweeper\n\n");
		printf("\t\t 4. Exit\n\n");

		do
		{
			choice = waitForAnyKey();
		}while (!(choice == '1' || choice == '2' || choice == '3' || choice == '4'));
		
		if(choice == '1')
		{
			snakes();
			continue;
		}
		else if(choice == '2')
		{
			tic_tac();
			continue;
		}
		else if(choice == '3')
		{
			minesweeper_game();
			continue;
		}
		else if(choice == '4')
		{
			clrscr();
			exit(1);
		}
	}
	led_lcd_end();

	return 0;

}


	