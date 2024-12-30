#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "plot.h"

char TEST_STR[] = "Hello World!\n";

typedef struct {
    PLOT_graph_t* graph;
    char* msg;
} Data;

void callback(void* raw_data)
{
    Data *data = raw_data;
    printf("%s", data->msg);

    char *img_path = PLOT_get_img_file_name(data->graph);
    char *cmd = g_build_path(" ", "xdg-open", img_path, NULL);
    int status = system(cmd);

    free(img_path);
    free(cmd);
}

void press_enter()
{
    int dummy = system("read -p 'Press ENTER to continue...' var");
}

int main()
{
    PLOT_init_workdir("/dev/shm");
    PLOT_graph_t *graph = PLOT_new_graph("humidity", 15, 0xFFFFFFFF);
    PLOT_set_color_rgba(graph, 0x3bae5789); // green

    press_enter();
    
    PLOT_push_data(graph, 214);
    PLOT_push_data(graph, 100);
    PLOT_push_data(graph, 53);
    PLOT_push_data(graph, 42);
    PLOT_push_data(graph, 9);
    PLOT_push_data(graph, 23);
    PLOT_push_data(graph, 342);
    PLOT_push_data(graph, 234);

    Data data = {.graph = graph, .msg = TEST_STR};
    PLOT_plot_to_file(graph, callback, &data, sizeof(data));

    press_enter();

    int status;
    while (1) {
        // waitpid no bloqueante con WNOHANG
        pid_t pid = waitpid(-1, &status, WNOHANG);
        
        if (pid == 0) {
            printf("0\n");
            usleep(100); 
        } else if (pid == -1) {
            printf("-1\n");
            if (errno == ECHILD) {
                printf("No hay más procesos hijo que monitorear.\n");
            } else {
                perror("waitpid");
            }
            break;
        } else {
            PLOT_notify_child_end(pid);
            break;
        }
    }

    press_enter();

    char *img_name = PLOT_get_img_file_name(graph);
    printf("File name: %s\n", img_name);
    free(img_name);

    press_enter();

    PLOT_delete_graph(graph);
    PLOT_clean_workdir();
    return EXIT_SUCCESS;
}