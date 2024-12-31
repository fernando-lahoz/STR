#ifndef GTK_MONITOR_PLOT_H
#define GTK_MONITOR_PLOT_H

#include <stdint.h>

#include <glib.h>


/**
 * @brief Creates a temporary directory located in `base_dir` in order to store
 *        image, data and other temporary files needed at runtime.
 * 
 * @param base_dir Base directory to store temporary files.
 * 
 * @return If workdir has been succesfully created, it returns 0.
 *         In case of error, it returns -1.
 * 
 * @note For better performance `base_dir` should be located on a ram-stored
 *       file system. Some linux distributions have directories where such
 *       file systems are mounted by default (e.g. '/dev/shm' on Debian). It
 *       is also possible to mount a tmpfs partition under a user-defined
 *       directory.
 */
int PLOT_init_workdir(const char* base_dir);


/**
 * @brief Removes all temporary files from workdir, and the workdir itself.
 * 
 * @return If workdir has been succesfully removed, it returns 0.
 *         In case of error, it returns -1.
 */
int PLOT_clean_workdir();


typedef struct PLOT_graph PLOT_graph_t;

/**
 * @brief Allocs and initializes a new graph.
 * 
 * @warning `name` must be unique.
 */
PLOT_graph_t* PLOT_new_graph(const char* name, size_t window_size, uint32_t rgba);

void PLOT_delete_graph(PLOT_graph_t* graph);

void PLOT_push_data(PLOT_graph_t* graph, double x);


typedef void (*PLOT_callback_t)(void*);

#define PLOT_MAX_PROC_NUM (32)

/**
 * @brief Sets up a process to generate an image file from graph.
 * 
 * @param graph Graph to be printed.
 * @param callback Function to be called after the process ends.
 * @param data Function data to be passed to `callback`.
 * @param size Size of `data`. Must not be greater than `PLOT_MAX_DATA_SIZE`.
 * 
 * @warning Data is moved into a buffer, so take care of any dangling pointer
 *          or ownership of pointed structures. Note that if size is 0, nothing
 *          is copied (useful for constant values).
 */
int PLOT_plot_to_file(PLOT_graph_t* graph, PLOT_callback_t callback, void* data, size_t size);

/**
 * @brief Notifies the end of a child process in order to execute a callback.
 * 
 * @param child_pid `pid` of the ended process.
 * 
 * @warning Should not be called from signal capture function.
 */
void PLOT_notify_child_end(int child_pid);

void PLOT_set_color_rgba(PLOT_graph_t* graph, uint32_t rgba);

void PLOT_set_limits(PLOT_graph_t* graph, double min, double max);

int PLOT_resize_window(PLOT_graph_t* graph, size_t new_size);

char* PLOT_get_img_file_name(PLOT_graph_t* graph);

#endif