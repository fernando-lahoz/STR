#include <sys/wait.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "serial.h"
#include "plot.h"
#include "util.h"

// compile with:
//   gcc `pkg-config --cflags gtk4` gtk-monitor.c plot.c serial.c reader_thread.c writer_thread.c -o gtk-monitor `pkg-config --libs gtk4` -O3

input_attr_t input_attr[] = {
    [INPUT_HUMIDITY] = {"humidity", 60, 0, 100, 0xAEE7EE80},
    [INPUT_TEMPERATURE] = {"temperature", 60, -10, 50, 0xD5545280},
    [INPUT_LIGHT] = {"light", 100, 0, 100, 0xFFEE5880}
};

input_sensor_t input[INPUT_SIZE];

void reload_img(void* data) 
{
    size_t i = (size_t)data;
    gtk_picture_set_filename(GTK_PICTURE(input[i].picture.widget), input[i].picture.file_name);
}

void on_slider_value_changed(GtkRange *range, gpointer user_data)
{
    gdouble value = gtk_range_get_value(range);
    struct msg msg = { CMD_SERVO, (uint32_t)value };
    mbox_send(OUTPUT_SERVO, &msg);
}

void init_input_sensor(input_sensor_t* sensor, const char* name)
{
    // what if NULL?
    sensor->graph = PLOT_new_graph(name, DEFAULT_WINDOWS_SIZE, DEFAULT_COLOR);
    sensor->picture.file_name = PLOT_get_img_file_name(sensor->graph);
    sensor->picture.widget = gtk_picture_new();
}

void destroy_input_sensor(input_sensor_t* sensor)
{
    PLOT_delete_graph(sensor->graph);
    free(sensor->picture.file_name);
}

void init_input(GtkWidget *box)
{
    for (size_t i = 0; i < INPUT_SIZE; ++i) {
        init_input_sensor(&input[i], input_attr[i].name);
        PLOT_resize_window(input[i].graph, input_attr[i].window_size);
        PLOT_set_color_rgba(input[i].graph, input_attr[i].rgba);
        PLOT_set_limits(input[i].graph, input_attr[i].min, input_attr[i].max);
        PLOT_plot_to_file(input[i].graph, reload_img, (void*)i, 0);
        gtk_box_append(GTK_BOX(box), input[i].picture.widget);
    }
}

void destroy_input()
{
    for (int i = 0; i < INPUT_SIZE; ++i) {
        destroy_input_sensor(&input[i]);
    }
}

static gboolean running = TRUE;

void on_window_destroy(GtkWidget *widget, gpointer data) {
    g_print("Goodbye!!!\n");
    running = FALSE;
}

gboolean on_timeout(void*)
{
    if (!running)
        return FALSE;

    static int n = 0, it = 0;
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    
    // Does not block, only checks if a child died before this frame
    if (pid > 0) {
        PLOT_notify_child_end(pid);
    }

    return TRUE;
}

static void activate(GtkApplication* app, gpointer user_data)
{
    GtkWidget *window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "STR GTK-Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 800);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_window_set_child(GTK_WINDOW(window), box);

    init_input(box);
    g_timeout_add(20, on_timeout, NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    launch_reader_thread();
    launch_writer_thread();


    GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 700.0, 3300.0, 1);
    gtk_scale_set_draw_value(GTK_SCALE(slider), TRUE);
    gtk_range_set_value(GTK_RANGE(slider), (3300.0 + 700.0) / 2.0);
    g_signal_connect(slider, "value-changed", G_CALLBACK(on_slider_value_changed), NULL);
    
    struct msg msg = { CMD_SERVO, 2000 };
    mbox_send(OUTPUT_SERVO, &msg);

    gtk_box_append(GTK_BOX(box), slider);

    gtk_window_present(GTK_WINDOW(window));
}

int main (int argc, char **argv)
{
    GtkApplication *app;
    int status;

    if (SERIAL_open("/dev/ttyACM0"))
        return EXIT_FAILURE;

    if (PLOT_init_workdir("/dev/shm"))
        return EXIT_FAILURE;

    app = gtk_application_new("str.gtk-monitor", G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);

    terminate_reader_thread();
    terminate_writer_thread();
    SERIAL_close();

    destroy_input();

    // Await for all children to end before removing directory
    while (waitpid(-1, &status, 0) > 0);
    PLOT_clean_workdir();

    g_object_unref(app);
    return status;
}
