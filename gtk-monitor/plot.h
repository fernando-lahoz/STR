#ifndef GTK_MONITOR_PLOT_H
#define GTK_MONITOR_PLOT_H

#include <stdint.h>

#include <glib/gstring.h>


void PLOT_init_workdir(const char* dir);

void PLOT_clean_workdir();


typedef struct PLOT_graph PLOT_graph_t;

PLOT_graph_t* PLOT_new_graph(const char* name, size_t window_size);

void PLOT_delete_graph(PLOT_graph_t* graph);

void PLOT_push_data(PLOT_graph_t* graph, double x, double y);


typedef void (*PLOT_callback_t)(void*);

/**
 * @brief Sets up a process to generate an image file from graph.
 * 
 * @param graph Graph to be printed.
 * @param callback Function to be called after the process ends.
 * @param data Function data to be passed to `callback`.
 */
void PLOT_plot_to_file(PLOT_graph_t* graph, PLOT_callback_t callback, void* data);

/**
 * @brief Notifies the end of a child process in order to execute a callback.
 * 
 * @param child_pid `pid` of the ended process.
 * 
 * @warning Should not be called from signal capture function.
 */
void PLOT_notify_child_end(int child_pid);

void PLOT_set_color_rgba(PLOT_graph_t* graph, uint32_t rgba);

void PLOT_resize_window(PLOT_graph_t* graph, size_t new_size);

GString PLOT_get_graph_file_name(PLOT_graph_t* graph);

#endif