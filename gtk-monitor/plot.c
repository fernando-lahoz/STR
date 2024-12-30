#define _XOPEN_SOURCE 700 // Posix 2017 in order to use mkdtemp

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <ftw.h>

#include <glib.h>

#include "plot.h"

static const char temp_dir_template[] = "gtk-monitor.XXXXXX";

static const char img_file_extension[] = ".svg";
static const char data_file_extension[] = ".data";

static char* temp_dir_path_template;
static char* temp_dir;

typedef struct PLOT_graph {
    char* name;

    double* window; // Bounded queue
    size_t last;
    size_t size:(8*sizeof(size_t))-1;
    gboolean full:1; 
    
    uint32_t rgba;
} PLOT_graph_t;


int PLOT_init_workdir(const char* base_dir)
{
    temp_dir_path_template = g_build_path("/", base_dir, temp_dir_template, NULL);

    // Create temp dir inside base_dir
    temp_dir = mkdtemp(temp_dir_path_template);
    if (temp_dir == NULL)
    {
        fprintf(stderr, "mkdtemp failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int remove_callback(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int ret = 0;

    if (typeflag == FTW_F || typeflag == FTW_SL)
        ret = remove(path);
    else if (typeflag == FTW_DP)
        ret = rmdir(path); // FTW_DEPTH ensures that directories are empty

    if (ret != 0)
        fprintf(stderr, "rm failed: %s", strerror(errno));

    return ret;
}

int PLOT_clean_workdir()
{
    if (nftw(temp_dir, remove_callback, 64, FTW_DEPTH | FTW_PHYS) == -1) {
        fprintf(stderr, "nftw failed: %s\n", strerror(errno));
        return -1;
    }

    free(temp_dir_path_template);

    return 0;
}


PLOT_graph_t* PLOT_new_graph(const char* name, size_t window_size, uint32_t rgba)
{
    if (window_size == 0) {
        fprintf(stderr, "Window size must be greater than zero\n");
        return NULL;
    }

    PLOT_graph_t *graph = malloc(sizeof(PLOT_graph_t));
    if (!graph) {
        fprintf(stderr, "Failed to alloc graph: %s\n", strerror(errno));
        return NULL;
    }

    graph->name = strdup(name);
    if (!graph->name) {
        fprintf(stderr, "Failed to duplicate graph name: %s\n", strerror(errno));
        free(graph);
        return NULL;
    }

    graph->window = calloc(window_size, sizeof(*graph->window));
    if (!graph->window) {
        fprintf(stderr, "Failed to alloc window of size %lu: %s\n", window_size, strerror(errno));
        free(graph->name);
        free(graph);
        return NULL;
    }
    memset(graph->window, 0, window_size);

    graph->full = 0;
    graph->last = 0;
    graph->size = window_size;
    graph->rgba = rgba;

    return graph;
}

void PLOT_delete_graph(PLOT_graph_t* graph) {
    if (graph) {
        free(graph->name);
        free(graph->window);
        free(graph);
    }
}

void PLOT_push_data(PLOT_graph_t* graph, double x)
{
    graph->window[graph->last] = x;
    if (++graph->last == graph->size) {
        graph->full = 1;
        graph->last = 0;
    }
}


static char* make_file_name(const PLOT_graph_t* graph, const char* extension, size_t ext_size)
{
    char *path = g_build_path("/", temp_dir, graph->name, extension, NULL);
    strcpy(&path[strlen(path)-ext_size], extension);
    return path;
}

static void print_data(const PLOT_graph_t* graph, FILE *stream)
{
    size_t i = graph->full ? graph->last : 0;

    for (size_t k = 0; k < graph->size; ++k) {
        fprintf(stream, "%lu %f\n", k, graph->window[i]);
        if (++i == graph->size)
            i = 0;
    }
}


static const char gnuplot_script_template[] =
    "set terminal svg size 600,400 dynamic enhanced font 'Droid Sans Mono,10' "
        "name '%s' butt dashlength 1.0\n"
    "set output '%s'\n"
    "unset title\n"
    "unset xlabel\n"
    "unset ylabel\n"
    "set autoscale xfixmin\n"
    "set autoscale xfixmax\n"
    "set border lw 1 lc rgb '#%X'\n"
    "set grid lw 0.5 lc rgb '#%X'\n"
    "plot '%s' smooth freq with filledcurves below y=-10 "
        "lc rgb '#%X' fs transparent solid %f notitle, "
        "'' smooth freq with lines lw 1.5 lc rgb '#%X' title ''\n"
    "exit\n";

static void print_gnuplot_script(const PLOT_graph_t* graph, FILE* stream,
        char* img_name, char* data_name)
{
    fprintf(stream, gnuplot_script_template, graph->name, img_name,
            graph->rgba >> 8, graph->rgba >> 8, data_name, graph->rgba >> 8,
            (graph->rgba & 0xFF) / 255.0, graph->rgba >> 8);
} //TODO: change alpha channel

static struct process_data {
    PLOT_callback_t callback;
    size_t size;
    char* data;
    int pid;
} queue[PLOT_MAX_PROC_NUM];

static int proc_num = 0;

static void push_process(struct process_data *proc)
{
    queue[proc_num++] = *proc;
}

static gboolean fetch_n_remove_process(int pid, struct process_data* proc)
{
    int i;

    proc_num--;
    for (i = proc_num; i >= 0; i--) {
        if (queue[i].pid == pid) {
            *proc = queue[i];
            break;
        }
    }

    if (i < 0)
        return FALSE;

    for (; i < proc_num; i++) {
        queue[i] = queue[i + 1];
    }

    return TRUE;
}


int PLOT_plot_to_file(PLOT_graph_t* graph, PLOT_callback_t callback, void* data, size_t size)
{
    int ret = 0;
    struct process_data proc;

    FILE *script_file = NULL, *data_file = NULL;
    char *img_name = NULL, *data_name = NULL;
    int fds[2], rd_fd = -1, wr_fd = -1;

    img_name = make_file_name(graph, img_file_extension, sizeof(img_file_extension));
    data_name = make_file_name(graph, data_file_extension, sizeof(data_file_extension));

    // TODO: error???
    data_file = fopen(data_name, "w");
    print_data(graph, data_file);
    fclose(data_file);
    
    // TODO: error???
    pipe(fds);
    rd_fd = fds[0];
    wr_fd = fds[1];

    // TODO: error???
    script_file = fdopen(wr_fd, "w");
    print_gnuplot_script(graph, script_file, img_name, data_name);
    fclose(script_file);

    proc.callback = callback;
    proc.data = data;
    if (size > 0) {
        proc.data = malloc(size);
        memcpy(proc.data, data, size);
    }
    proc.size = size;
    
// LOCK

    if (proc_num == PLOT_MAX_PROC_NUM) {
        fprintf(stderr, "Too many plot processes.\n");
        ret = -1;
        free(proc.data);
        goto free_strings; //unlock!!!!!!!!!!!1
    }

    int pid = fork();
    if (pid == -1) {
        //...
    }

    if (pid == 0) { // Child process
        if (dup2(rd_fd, STDIN_FILENO) == -1) { // rd_fd > stdin
            fprintf(stderr, "Failed to bind stdin: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        close(rd_fd); // Only use stdin to read

        execlp("gnuplot", "gnuplot", NULL);
        fprintf(stderr, "Failed to exec gnuplot: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    close(rd_fd);

    proc.pid = pid;
    push_process(&proc);
// UNLOCK

    // write exit into pipe HERE!!!!!!!!

free_strings:
    free(img_name);
    free(data_name);

    return ret;
}

void PLOT_notify_child_end(int child_pid)
{
// LOCK
    struct process_data proc;
    if (fetch_n_remove_process(child_pid, &proc))
    {
        proc.callback(proc.data);

        if (proc.size > 0)
            free(proc.data);
    }
// UNLOCK
}


void PLOT_set_color_rgba(PLOT_graph_t* graph, uint32_t rgba)
{
    graph->rgba = rgba;
}

int PLOT_resize_window(PLOT_graph_t* graph, size_t new_size)
{
    if (new_size == graph->size)
        return 0;

    if (new_size == 0) {
        fprintf(stderr, "Window size must be greater than zero.\n");
        return -1;
    }

    typeof(graph->window) new_window = calloc(new_size, sizeof(*graph->window));
    if (!new_window) {
        fprintf(stderr, "Failed to alloc window of size %lu: %s\n", new_size, strerror(errno));
        return -1;
    }

    size_t i = 0;

    if (graph->size > new_size && (graph->full || graph->last - new_size > 0))
        i = (graph->last - new_size) % graph->size;
    else if (graph->size < new_size && graph->full)
        i = graph->last;

    for (size_t k = 0; k < new_size; ++k) {
        if (k < graph->size) {
            new_window[k] = graph->window[i];
            if (++i == graph->size)
                i = 0;
        } else {
            new_window[k] = 0.0;
        }
    }

    if (graph->size > new_size) {
        graph->last = 0;
        graph->full = 1;
    } else {
        graph->last = graph->full ? graph->size : graph->last;
        graph->full = 0;
    }

    free(graph->window);
    graph->window = new_window;
    graph->size = new_size;

    

    return 0;
}

char* PLOT_get_img_file_name(PLOT_graph_t* graph)
{
    return make_file_name(graph, img_file_extension, sizeof(img_file_extension));
}