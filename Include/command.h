#include <stdint.h>

#define MAX_ARGC 6

struct command {
    uint32_t argc;
    char *argv[MAX_ARGC];
};

typedef int (*cmd_handler_t)(int argc, char *argv[]);

struct cmd_entry {
    const char *name;
    const char *help_msg;
    cmd_handler_t handler;
};

int tokenize(char *line, struct command *cmd);
void command_dispatch(struct command cmd);

int help_handler(int argc, char *argv[]);
int led_handler(int argc, char *argv[]);
int echo_handler(int argc, char *argv[]);
int btn_handler(int argc, char *argv[]);
int uptime_handler(int argc, char *argv[]);
int adc_handler(int argc, char *argv[]);
