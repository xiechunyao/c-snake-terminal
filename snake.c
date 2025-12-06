#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

#define COLS 100
#define ROWS 50
#define MAX_LEN 256
#define SPEED_NORMAL 100000
#define COLOR_RED "\x1b[31m"
#define COLOR_RESET "\x1b[0m"

// struct
//   position
typedef struct pos
{
    int x;
    int y;
} pos;

struct snake
{
    int len;
    pos head_pos;
    pos body_pos[MAX_LEN];
} snake;

// globol
//   frame interval in microseconds (adjust to change speed)
int play_again = 0;
int FRAME_USEC = SPEED_NORMAL;
struct termios original_tio;
pos food;
pos temp_pos;
char board[ROWS][COLS];
int quit = 0;
int gameover = 0;
int food_exists = 0;
//  current movement direction: row (y), col (x)
int dir_y = 0;
int dir_x = 1; // start moving right

// prototype
void fill_borad();
void print_board();
void draw_snake();
void clean_screen();
void read_input();
void move_snake(int delta_y, int delta_x);
void set_non_buffered_input();
void restore_terminal();
void draw_food();
int random_num_board();
int max_two(int a, int b);
void identify_situation();
void init_snake();
int print_gameover_ask_agaain(); // returns 1 to replay, 0 to quit

// MAIN
int main()
{
    do
    {
        srand(time(NULL));
        init_snake();
        food_exists = 0;        // reset food state
        gameover = 0;           // reset game over flag
        quit = 0;               // reset quit flag
        dir_y = 0;              // reset direction (moving right)
        dir_x = 1;
        play_again = 0;         // assume user quits
        
        set_non_buffered_input();
        while (!quit && !gameover)
        {
            // handle input (non-blocking): only update direction
            read_input();
            // move snake automatically according to current direction
            move_snake(dir_y, dir_x);
            // check collisions / eating
            identify_situation();
            // render frame
            clean_screen();
            fill_borad();
            draw_food();
            draw_snake();
            print_board();
            // control frame rate
            usleep(FRAME_USEC);
        }
        if (quit)
        {
            system("clear");
            printf("\n\nyou have quited the game\n\n");
            printf("your final length: %d\n", snake.len);
            printf("OS reserve all the rights\n");    
        }
        if (gameover)
        {
            play_again = print_gameover_ask_agaain(); // returns 1 if user wants to replay
        }
    } while (play_again);
    restore_terminal();
    return 0;
}

// functions
void fill_borad()
{
    // i represents ROW, j represents COL
    int i, j;
    for (i = 0; i < ROWS; ++i)
    {
        for (j = 0; j < COLS; ++j)
        {
            if (i == 0 || i == ROWS - 1)
            {
                board[i][j] = '-';
            }
            else if (j == 0 || j == COLS - 1)
            {
                board[i][j] = '|';
            }
            else
            {
                board[i][j] = ' ';
            }
        }
    }
}

void print_board()
{
    int i, j;
    for (i = 0; i < ROWS; ++i)
    {
        for (j = 0; j < COLS; ++j)
        {
            putchar(board[i][j]);
        }
        printf("\n");
    }
    printf("\nLength: %d\n", snake.len);
    printf("press wsad to move, press q to quit the game\n");
}

void draw_snake()
{
    int i;
    // body part
    for (i = 0; i < snake.len - 1; ++i)
    {
        board[snake.body_pos[i].y][snake.body_pos[i].x] = '*';
    }
    board[snake.head_pos.y][snake.head_pos.x] = '@';
}

void clean_screen()
{
    system("clear");
}

// change the direction
void read_input()
{
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0; // poll
    int retval = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
    if (retval > 0 && FD_ISSET(STDIN_FILENO, &rfds))
    {
        char ch = getchar();
        // update direction but prevent direct reverse
        if (ch == 'w' && !(dir_y == 1 && dir_x == 0))
        {
            dir_y = -1;
            dir_x = 0;
        }
        else if (ch == 's' && !(dir_y == -1 && dir_x == 0))
        {
            dir_y = 1;
            dir_x = 0;
        }
        else if (ch == 'a' && !(dir_y == 0 && dir_x == 1))
        {
            dir_y = 0;
            dir_x = -1;
        }
        else if (ch == 'd' && !(dir_y == 0 && dir_x == -1))
        {
            dir_y = 0;
            dir_x = 1;
        }
        else if (ch == 'q')
        {
            quit = 1;
        }
    }
}

void move_snake(int delta_y, int delta_x)
{
    temp_pos = snake.body_pos[snake.len - 2];
    int i;
    // the latter follows the former
    for (i = snake.len - 2; i > 0; --i)
    {
        snake.body_pos[i] = snake.body_pos[i - 1];
    }
    // the first body part follows the head
    snake.body_pos[0] = snake.head_pos;
    // update the position of head
    snake.head_pos.y += delta_y;
    snake.head_pos.x += delta_x;
}

// Set the terminal to non-canonical mode
void set_non_buffered_input()
{
    struct termios new_tio;

    // Get the current terminal settings
    tcgetattr(STDIN_FILENO, &original_tio);

    // Copy the current settings to a new structure
    new_tio = original_tio;

    // Set to non-canonical mode (ICANON) and no input echoing (ECHO)
    new_tio.c_lflag &= ~(ICANON | ECHO);

    // Set the minimum number of bytes to read and the waiting time
    new_tio.c_cc[VMIN] = 1;  // 至少读取 1 个字符
    new_tio.c_cc[VTIME] = 0; // 不设置等待时间

    // Apply new terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

// Restore the terminal to its original canonical mode
void restore_terminal()
{
    // Restore the original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void draw_food()
{
    if (!food_exists)
    {
        int x, y, i, collision;
        do
        {
            x = (rand() % (COLS - 2)) + 1; // 1 .. COLS-2
            y = (rand() % (ROWS - 2)) + 1; // 1 .. ROWS-2
            collision = 0;
            // avoid placing on head
            if (x == snake.head_pos.x && y == snake.head_pos.y)
            {
                collision = 1;
            }
            // avoid placing on body
            for (i = 0; i < snake.len - 1 && !collision; ++i)
            {
                if (x == snake.body_pos[i].x && y == snake.body_pos[i].y)
                {
                    collision = 1;
                }
            }
        } while (collision);
        food.x = x;
        food.y = y;
        food_exists = 1;
    }
    board[food.y][food.x] = '$';
}

// called by draw_food
int random_num_board()
{
    int min = 1;
    int max = max_two(ROWS, COLS) - 1;
    // res from min to max (include min and max)
    int res = (rand() % (max - min + 1)) + min;
    return res;
}

int max_two(int a, int b)
{
    if (a >= b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

void identify_situation()
{
    //  edge
    if (snake.head_pos.x == 0)
    {
        snake.head_pos.x = COLS - 2;
    }
    else if (snake.head_pos.x == COLS - 1)
    {
        snake.head_pos.x = 1;
    }
    else if (snake.head_pos.y == 0)
    {
        snake.head_pos.y = ROWS - 2;
    }
    else if (snake.head_pos.y == ROWS - 1)
    {
        snake.head_pos.y = 1;
    }
    // body part
    int i;
    for (i = 0; i < snake.len; ++i)
    {
        if (snake.head_pos.x == snake.body_pos[i].x && snake.head_pos.y == snake.body_pos[i].y)
        {
            gameover = 1;
        }
    }
    // eat food
    if (snake.head_pos.x == food.x && snake.head_pos.y == food.y)
    {
        ++snake.len;
        snake.body_pos[snake.len - 2] = temp_pos;
        food_exists = 0;
    }
}

void init_snake()
{
    snake.len = 3;
    snake.head_pos.x = 5;
    snake.head_pos.y = 5;
    snake.body_pos[0].x = 5;
    snake.body_pos[0].y = 6;
    snake.body_pos[1].x = 5;
    snake.body_pos[1].y = 7;
}

int print_gameover_ask_agaain()
{
    system("clear");
    printf("\n\n\n");
    printf(COLOR_RED "GAME OVER!" COLOR_RESET);
    printf("\n\n\n");
    printf("\nFinal Length: %d\n", snake.len);
    printf("\nPress R to replay, or any other key to quit\n");    
    char ch = getchar();
    if (ch == 'r' || ch == 'R')
    {
        return 1; // replay
    }
    else
    {
        return 0; // quit
    }
}
