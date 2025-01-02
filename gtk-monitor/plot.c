#define _XOPEN_SOURCE 700 // Posix 2017 in order to use mkdtemp

#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <ftw.h>
#include <pthread.h>

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
    size_t last:(8*sizeof(size_t))-1;
    gboolean full:1; 
    size_t size;

    double max, min;
    
    uint32_t rgba;

    pthread_mutex_t mtx;
} PLOT_graph_t;

#define GRAPH_LOCK(graph) pthread_mutex_lock(&graph->mtx);
#define GRAPH_UNLOCK(graph) pthread_mutex_unlock(&graph->mtx);

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
    graph->min = 0.0;
    graph->max = 1.0;

    pthread_mutex_init(&graph->mtx, NULL);

    return graph;
}

void PLOT_delete_graph(PLOT_graph_t* graph) {
    if (graph) {
        free(graph->name);
        free(graph->window);
        free(graph);
        pthread_mutex_destroy(&graph->mtx);
    }
}

void PLOT_push_data(PLOT_graph_t* graph, double x)
{
    GRAPH_LOCK(graph);
    graph->window[graph->last] = x;
    if (++graph->last == graph->size) {
        graph->full = 1;
        graph->last = 0;
    }
    GRAPH_UNLOCK(graph)
}


static char* make_file_name(const PLOT_graph_t* graph, const char* extension, size_t ext_size)
{
    char *path = g_build_path("/", temp_dir, graph->name, extension, NULL);
    strcpy(&path[strlen(path)-ext_size], extension);
    return path;
}

static void print_data(const PLOT_graph_t* graph, FILE *stream)
{
    size_t i, k;
    if (graph->full) {
        i = graph->last;
        for (k = 0; k < graph->size; ++k) {
            fprintf(stream, "%f %f\n", (double)k, graph->window[i]);
            if (++i == graph->size)
                i = 0;
        }
        fprintf(stream, "%f %f\n", k - 0.999, graph->min);
    } else {
        for (k = 0; k < graph->last; ++k) {
            fprintf(stream, "%f %f\n", (double) k, graph->window[k]);
        }
        fprintf(stream, "%f %f\n", k - 0.999, graph->min);
        for (; k < graph->size; ++k) {
            fprintf(stream, "%f %f\n", (double) k, graph->min);
        }
    }
}


static const char gnuplot_script_template[] =
    "set terminal svg size 1200,800 dynamic enhanced font 'Droid Sans Mono,10' "
        "name '%s' butt dashlength 1.0\n"
    "set output '%s'\n"
    "unset title\n"
    "unset key\n"
    "unset xlabel\n"
    "unset ylabel\n"
    "set yrange [%f:%f]\n"
    "set xrange [%u:%lu]\n"
    "set border lw 1 lc rgb '#%X'\n"
    "set grid lw 0.5 lc rgb '#%X'\n"
    "plot '%s' smooth freq with filledcurves below y=-50 "
        "lc rgb '#%X' fs transparent solid %f notitle, "
        "'' smooth freq with lines lw 1.5 lc rgb '#%X' title ' '\n";

static const char gnuplot_end_of_script[] = "exit\n";

static void print_gnuplot_script(const PLOT_graph_t* graph, FILE* stream,
        char* img_name, char* data_name)
{
    fprintf(stream, gnuplot_script_template,
            graph->name, img_name, graph->min, graph->max, 0, graph->size,
            graph->rgba >> 8, graph->rgba >> 8, data_name, graph->rgba >> 8,
            (graph->rgba & 0xFF) / 255.0, graph->rgba >> 8);
}

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

static pthread_mutex_t proc_data_mtx = PTHREAD_MUTEX_INITIALIZER;

int PLOT_plot_to_file(PLOT_graph_t* graph, PLOT_callback_t callback, void* data, size_t size)
{
    // This is VERY VERY important: if data is not printed with the right locale
    // gnuplot won't generate a correct graph, but this function must not change
    // global locale, only thread-local locale, that will be reset at the end of
    // the function.
    locale_t thread_locale = newlocale(LC_ALL_MASK, "en_US.UTF-8", NULL);
    locale_t old_locale = uselocale(thread_locale);

    int ret = 0;
    struct process_data proc;

    FILE *script_file = NULL, *data_file = NULL;
    char *img_name = NULL, *data_name = NULL;
    int fds[2], rd_fd = -1, wr_fd = -1;

    img_name = make_file_name(graph, img_file_extension, sizeof(img_file_extension));
    data_name = make_file_name(graph, data_file_extension, sizeof(data_file_extension));

    data_file = fopen(data_name, "w");
    if (!data_file) {
        fprintf(stderr, "Failed to create data file: %s\n", strerror(errno));
        ret = -1;
        goto free_resources;
    }
    GRAPH_LOCK(graph);
    print_data(graph, data_file);
    GRAPH_UNLOCK(graph);
    fflush(data_file);
    fclose(data_file);
    
    if (pipe(fds) == -1) {
        fprintf(stderr, "Failed to create pipe: %s\n", strerror(errno));
        ret = -1;
        goto free_resources;
    }
    rd_fd = fds[0];
    wr_fd = fds[1];

    script_file = fdopen(wr_fd, "w");
    if (!script_file) {
        fprintf(stderr, "Failed to make FILE* out of pipe fd: %s\n", strerror(errno));
        ret = -1;
        close(rd_fd);
        close(wr_fd);
        goto free_resources;
    }
    GRAPH_LOCK(graph);
    print_gnuplot_script(graph, script_file, img_name, data_name);
    GRAPH_UNLOCK(graph);
    fflush(script_file);

    proc.callback = callback;
    proc.data = data;
    if (size > 0) {
        proc.data = malloc(size);
        memcpy(proc.data, data, size);
    }
    proc.size = size;
    
/* LOCK */ pthread_mutex_lock(&proc_data_mtx);

    if (proc_num == PLOT_MAX_PROC_NUM) {
    /* UNLOCK */ pthread_mutex_unlock(&proc_data_mtx);
        //fprintf(stderr, "Too many plot processes.\n");
        ret = -1;
        free(proc.data == data ? NULL : proc.data);
        close(rd_fd);
        goto free_resources;
    }

    int pid = fork();
    if (pid == -1) {
    /* UNLOCK */ pthread_mutex_unlock(&proc_data_mtx);
        fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
        ret = -1;
        free(proc.data == data ? NULL : proc.data);
        close(rd_fd);
        goto free_resources;
    }

    if (pid == 0) { // Child process
        close(wr_fd);
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

/* UNLOCK */ pthread_mutex_unlock(&proc_data_mtx);

    // Child process won't die until this is executed
    fprintf(script_file, "%s\n", gnuplot_end_of_script);
    fflush(script_file);

free_resources:
    fclose(script_file);
    free(img_name);
    free(data_name);

    uselocale(old_locale);
    return ret;
}

void PLOT_notify_child_end(int child_pid)
{
/* LOCK */ pthread_mutex_lock(&proc_data_mtx);
    struct process_data proc;
    if (fetch_n_remove_process(child_pid, &proc))
    {
        proc.callback(proc.data);

        if (proc.size > 0)
            free(proc.data);
    }
/* UNLOCK */ pthread_mutex_unlock(&proc_data_mtx);
}


void PLOT_set_color_rgba(PLOT_graph_t* graph, uint32_t rgba)
{
    GRAPH_LOCK(graph);
    graph->rgba = rgba;
    GRAPH_UNLOCK(graph);
}

void PLOT_set_limits(PLOT_graph_t* graph, double min, double max)
{
    GRAPH_LOCK(graph);
    graph->min = min;
    graph->max = max;
    GRAPH_UNLOCK(graph);
}

int PLOT_resize_window(PLOT_graph_t* graph, size_t new_size)
{
    GRAPH_LOCK(graph);
    if (new_size == graph->size)
        return 0;

    if (new_size == 0) {
        fprintf(stderr, "Window size must be greater than zero.\n");
        return -1;
    }

    double *new_window = calloc(new_size, sizeof(*graph->window));
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

    GRAPH_UNLOCK(graph);

    return 0;
}

char* PLOT_get_img_file_name(PLOT_graph_t* graph)
{
    return make_file_name(graph, img_file_extension, sizeof(img_file_extension));
}

double PLOT_get_last_value(PLOT_graph_t* graph)
{
    double res;
    GRAPH_LOCK(graph);
    res = graph->window[(graph->last - 1) % graph->size];
    GRAPH_UNLOCK(graph);
    return res;
}