#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>

typedef struct PLOT_graph {
    GString name;
    GString script_data;
    GString script_data;
    uint8_t rgb[3];
} PLOT_graph;

static char temp_dir_template[] = "/dev/shm/gtk-monitor.XXXXXX";
static char *temp_dir;

int PLOT_init()
{
    // Create temp dir inside /dev/shm
    temp_dir = mkdtemp(temp_dir_template);
    if (temp_dir == NULL)
    {
        fprintf(stderr, "mkdtemp failed: %s", strerror(errno));
        return -1;
    }

    // DEBUG INFO
    printf("Temp dir created: %s", temp_dir);
    return 0;
}

int PLOT_remove()
{
    // Remove temp dir
    if (rmdir(temp_dir) == -1)
    {
        fprintf(stderr, "rmdir failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

PLOT_graph* PLOT_new_graph(const char *graph_name)
{
    PLOT_graph *graph;
    

    graph = malloc(sizeof(PLOT_graph));

    // Create file for history
    graph->history = fopen(name_buff, "rw");

    return graph;

error:
    return NULL;
}

void PLOT_save_graph( /*path asked from gtk??*/)
{
    // Copy history file, script and graph image to path
}

void PLOT_delete_graph()
{

}

int main()
{
    PLOT_init();
    getc(stdin);
    PLOT_remove();
    return EXIT_SUCCESS;
}