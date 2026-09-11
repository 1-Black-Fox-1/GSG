#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef __linux__
#include <termios.h>
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef __WIN32
printf("%s","Windows is not supported. And more likely won't.\n");
exit(0);
#endif

#define DEBUG 0

typedef struct Vector2 {
    int x;
    int y;
} Vector2;

char read_binding();
void start_game();
void append_buffer(char**, size_t*, const char*, const size_t);
void open_settings(const char*);
void save_bindings(const char*);
void set_raw_terminal();
void set_canonical_terminal();
void *read_input();
int vec2_cmp(const Vector2, const Vector2);
Vector2 vec2_sum(const Vector2, const Vector2);
Vector2 generate_food(const Vector2*, const size_t, const Vector2);

// TODO: local records board

// TODO: create struct controls and init it in main and send it in func and threads
char move_up = 'w';
char move_down = 's';
char move_right = 'd';
char move_left = 'a';

struct termios original;
Vector2 v2move;
Vector2 v2move_frame;
pthread_mutex_t v2move_mutex;

int main() {
    const char *menu =
        "----------------\n"
        "Great Snake Game\n"
        "----------------\n"
        "Play\n"
        "Settings\n"
        "Quit\n"
        "\n"
        "Input: ";

    srand(time(NULL));
    const char *config = "config.txt";
    FILE *f = fopen(config, "r");
    if (f == NULL) {
        save_bindings(config);
    } else {
        fscanf(f, "Move up = %c\nMove down = %c\nMove right = %c\nMove left = %c\n",
                &move_up, &move_down, &move_right, &move_left);
        fclose(f);
    }

    system("clear");
    while (1) {
        printf("%s", menu);
        char buffer[10];
        fgets(buffer, 10, stdin);
        // buffer[strlen(buffer) - 1] = '\0';
        buffer[strcspn(buffer, "\n")] = 0;
        if (!strcmp(buffer, "play")) {
            system("clear");
            start_game();
        } else if (!strcmp(buffer, "settings")) {
            system("clear");
            open_settings(config);
        } else if (!strcmp(buffer, "quit")) {
            system("clear");
            return 0;
        } else {
            printf("Enter play, settings or quit to proceed\n");
            sleep(1);
            system("clear");
        }
    }
    return 0;
}

void open_settings(const char *config) {
    const char *settings_proto = 
        "----------------\n"
        "Settings\n"
        "----------------\n"
        "Move up = %c\n"
        "Move down = %c\n"
        "Move right = %c\n"
        "Move left = %c\n"
        "Back\n"
        "Quit\n"
        "\n"
        "Input: ";

    while (1) {
        char *settings;
        if (!asprintf(&settings, settings_proto,
                    move_up, move_down, move_right, move_left)) {
            fprintf(stderr, "Error: Cannot init settings");
            exit(1);
        }
        printf("%s", settings);
        free(settings);
        char buffer[12];
        fgets(buffer, 12, stdin);
        // buffer[strlen(buffer) - 1] = '\0';
        buffer[strcspn(buffer, "\n")] = 0;
        if (!strcmp(buffer, "move up")) {
            printf("Print new binding\n");
            char res = read_binding();
            if (res == '\0') {
                printf("Enter one key\n");
                sleep(1);
                system("clear");
            } else {
                printf("Added successfully\n");
                move_up = res;
                save_bindings(config);
                sleep(1);
                system("clear");
            }
        } else if (!strcmp(buffer, "move down")) {
            printf("Print new binding\n");
            char res = read_binding();
            if (res == '\0') {
                printf("Enter one key\n");
                sleep(1);
                system("clear");
            } else {
                printf("Added successfully\n");
                move_down = res;
                save_bindings(config);
                sleep(1);
                system("clear");
            }
        } else if (!strcmp(buffer, "move right")) {
            printf("Print new binding\n");
            char res = read_binding();
            if (res == '\0') {
                printf("Enter one key\n");
                sleep(1);
                system("clear");
            } else {
                printf("Added successfully\n");
                move_right = res;
                save_bindings(config);
                sleep(1);
                system("clear");
            }
        } else if (!strcmp(buffer, "move left")){
            printf("Print new binding\n");
            char res = read_binding();
            if (res == '\0') {
                printf("Enter one key\n");
                sleep(1);
                system("clear");
            } else {
                printf("Added successfully\n");
                move_left = res;
                save_bindings(config);
                sleep(1);
                system("clear");
            }
        } else if (!strcmp(buffer, "back")) {
            system("clear");
            return;
        } else if (!strcmp(buffer, "quit")) {
            system("clear");
            exit(0);
        } else {
            printf("Enter move right, move down, move right, move left, back, quit to proceed\n");
            sleep(1);
            system("clear");
        }
    }
}

void save_bindings(const char *config) {
    FILE *f = fopen(config, "w");
        if (f == NULL) {
            fprintf(stderr, "Error: cant write to file %s\n", config);
            exit(1);
        }
        fprintf(f, "Move up = %c\nMove down = %c\nMove right = %c\nMove left = %c\n",
                move_up, move_down, move_right, move_left);
    fclose(f);
}

char read_binding() {
    char buffer[2];
    fgets(buffer, 2, stdin);
    if (strlen(buffer) == 1){
        return buffer[0];
    } else {
        return '\0';
    }
}

int vec2_cmp(const Vector2 a, const Vector2 b) {
    if (a.x == b.x && a.y == b.y)
        return 0;
    return 1;
}

void start_game() {
#if DEBUG
    unsigned long long int frames_calc = 0;
#endif
    const useconds_t wait = 500000;
    const Vector2 field = {40, 20};
    size_t snake_size = 10;
    size_t snake_i = 0;
    Vector2 *snake = malloc(snake_size * sizeof(*snake));
    if (snake == NULL) {
        fprintf(stderr, "Error: Cannot init snake\n");
        exit(1);
    }
    snake[snake_i] = (Vector2){field.x / 2, field.y / 2};
    snake_i += 1;
    set_raw_terminal();
    Vector2 food;
#if DEBUG
    food = (Vector2){25, 10};
#else
    food = generate_food(snake, snake_size, field);
#endif
    const char *str_game_over = "Game over";
    int game_over_pos = 0;
    Vector2 v2_game_over = {field.x / 2 - 4, field.y / 2};

    v2move = (Vector2){1, 0};
    v2move_frame = v2move;
    pthread_mutex_init(&v2move_mutex, NULL);
    pthread_t id_read_input;
    if (pthread_create(&id_read_input, NULL, read_input, NULL)) exit(1);

    write(STDOUT_FILENO, "\x1b[?25l", 6);
    int game_over = 1;
    while(game_over) {
        Vector2 head = snake[0];
        size_t game_screen_b_size = 1;
        char *game_screen_b = malloc(game_screen_b_size);
        game_screen_b[0] = 0;
 
#if DEBUG
        char *debug_str;
        // for (size_t i = 0; i < snake_i; 
        if (!asprintf(&debug_str,
                "Debug: p_x = %d, p_y = %d\n"
                "Debug: frames_calc = %llu\n"
                /* TODO: print all snakes nodes */
                "Debug: head_coor = %d, %d\n",
                food.x, food.y, frames_calc, head.x, head.y)) {
            fprintf(stderr, "Cannot init debug string");
        }
        append_buffer(&game_screen_b, &game_screen_b_size,
                debug_str, strlen(debug_str));
        free(debug_str);
#endif

        for (size_t i = 1; i < snake_i; ++i) {
            if (!vec2_cmp(head, snake[i])) {
                game_over = 0;
                break;
            }
        }

        if (head.x == 0 || head.x == (field.x - 1) 
                || head.y == 0 || head.y == (field.y - 1)) {
            game_over = 0;
        }

        for (int y = 0; y < field.y; ++y) {
            for (int x = 0; x < field.x; ++x) {
                if (y == 0 && x == 0) {
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            // "┌", strlen("┌"));
                            "┌", 3);
                    // printf("┌");
                    continue;
                }
                if (y == 0 && x == (field.x - 1)) {
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            "┐", strlen("┐"));
                    // printf("┐");
                    continue;
                }
                if (y == (field.y - 1) && x == 0) {
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            "└", strlen("└"));
                    // printf("└");
                    continue;
                }
                if (y == (field.y - 1) && x == (field.x - 1)) {
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            "┘", strlen("┘"));
                    // printf("┘");
                    continue;
                }
                if (y == 0 || y == (field.y - 1)) {
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            "-", strlen("-"));
                    // printf("-");
                    continue;
                }
                if (x == 0 || x == (field.x - 1)) {
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            "│", strlen("│"));
                    // printf("│");
                    continue;
                }

                if (!game_over && vec2_cmp(v2_game_over, (Vector2){x, y})
                        && str_game_over[game_over_pos] != 0) {
                    // printf("%c", str_game_over[game_over_pos]);
                    char *tmp;
                    if (!asprintf(&tmp, "%c", str_game_over[game_over_pos])){
                        fprintf(stderr, "Cannot init game over string");
                        exit(1);
                    }
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            tmp, strlen(tmp));
                    free(tmp);
                    game_over_pos += 1;
                    v2_game_over = vec2_sum(v2_game_over, (Vector2){1, 0});
                    continue;
                }

                int find;
                for (size_t i = 0; i < snake_i; ++i) {
                    find = vec2_cmp(snake[i], (Vector2){x, y});
                    if (!find) {
                        append_buffer(&game_screen_b, &game_screen_b_size,
                                "•", strlen("•"));
                        // printf("•");
                        break;
                    }
                }
                if (!find) {
                    continue;
                }

                if (!vec2_cmp(food, (Vector2){x, y})) {
                    append_buffer(&game_screen_b, &game_screen_b_size,
                            // "*", strlen("*"));
                            "$", strlen("$"));
                    // printf("*");
                    continue;
                }
                append_buffer(&game_screen_b, &game_screen_b_size,
                        " ", strlen(" "));
                // printf(" ");

            }
            append_buffer(&game_screen_b, &game_screen_b_size,
                    "\n", strlen("\n"));
            // printf("\n");
        }
        char *game_labels;
        if (!asprintf(&game_labels, "Score: %zu\n" "Quit - <Ctrl-C>\n", snake_i - 1)){
            fprintf(stderr, "Cannot init game labels");
            exit(1);
        }
        append_buffer(&game_screen_b, &game_screen_b_size,
                game_labels, strlen(game_labels));
        free(game_labels);
        // printf("Score: %d\n", snake_i);
        // printf("Quit\n");

        write(STDOUT_FILENO, game_screen_b, game_screen_b_size);
#if DEBUG
        frames_calc += 1;
#endif
        free(game_screen_b);

        Vector2 old_tail = snake[snake_i - 1];
        for (size_t i = snake_i - 1; i > 0; --i) {
            snake[i] = snake[i - 1];
        }
        /* mutex? */
        snake[0] = vec2_sum(snake[0], v2move);
        v2move_frame = v2move;

        if (!vec2_cmp(head, food)) {
            if (snake_i + 1 == snake_size) {
                snake_size *= 2;
                snake = realloc(snake, snake_size * sizeof(*snake));
                if (snake == NULL) exit(1);
            }
            snake[snake_i] = old_tail;
            snake_i += 1;

            food = generate_food(snake, snake_i, field);
        }

        usleep(wait);
        write(STDOUT_FILENO, "\x1b[1J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        // system("clear");
    }
    free(snake);
    sleep(1);
    pthread_cancel(id_read_input);
    system("clear");
    write(STDOUT_FILENO, "\x1b[?25h", 6);
    set_canonical_terminal();
    return;
}

void set_raw_terminal() {
    struct termios my_ter;
    tcgetattr(STDIN_FILENO, &original);
    my_ter = original;
    atexit(&set_canonical_terminal);
    my_ter.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &my_ter);
}

void set_canonical_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

Vector2 vec2_sum(const Vector2 a, const Vector2 b) {
    Vector2 res = {a.x + b.x, a.y + b.y};
    return res;
}

Vector2 generate_food(const Vector2 *snake,
        const size_t snake_size,
        const Vector2 field) {
    int success = 1;
    Vector2 res;
    while (success) {
        int f_x = ((rand() % (field.x - 2)) + 1);
        int f_y = ((rand() % (field.y - 2)) + 1);
        res = (Vector2){f_x, f_y};
        for (size_t i = 0; i < snake_size; ++i) {
            if (!vec2_cmp(res, snake[i])) {
                break;
            }
            success = 0;
        }
    }
    return res;
}

void append_buffer(char **b, size_t *b_s, const char *s, const size_t s_s) {
    size_t new_size = *b_s + s_s;
    *b = realloc(*b, new_size);
    if (*b == NULL) exit(1);
    *b_s = new_size;
    strncat(*b, s, s_s);
    /* dunno this breaks all */
    // *b[new_size - 1] = 0;
}

void *read_input() {
    char c;
    const Vector2 move_up_vec2 = {0, -1};
    const Vector2 move_down_vec2 = {0, 1};
    const Vector2 move_right_vec2 = {1, 0};
    const Vector2 move_left_vec2 = {-1, 0};
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (iscntrl(c)) {
            // printf("%d\n", c);
        } else if (c == move_up
                && vec2_cmp(v2move_frame, move_down_vec2))
                    {
            pthread_mutex_lock(&v2move_mutex);
            v2move = move_up_vec2;
            pthread_mutex_unlock(&v2move_mutex);
        } else if (c == move_down
                && vec2_cmp(v2move_frame, move_up_vec2)) {
            pthread_mutex_lock(&v2move_mutex);
            v2move = move_down_vec2;
            pthread_mutex_unlock(&v2move_mutex);
        } else if (c == move_right
                && vec2_cmp(v2move_frame, move_left_vec2)) {
            pthread_mutex_lock(&v2move_mutex);
            v2move = move_right_vec2;
            pthread_mutex_unlock(&v2move_mutex);
        } else if (c == move_left 
                && vec2_cmp(v2move_frame, move_right_vec2)) {
            pthread_mutex_lock(&v2move_mutex);
            v2move = move_left_vec2;
            pthread_mutex_unlock(&v2move_mutex);
        }
    }
    return NULL;
}
