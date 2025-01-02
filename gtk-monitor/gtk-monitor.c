#include <sys/wait.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "serial.h"
#include "plot.h"
#include "util.h"

// compile with:
//   gcc `pkg-config --cflags gtk4` gtk-monitor.c plot.c serial.c reader_thread.c writer_thread.c -o gtk-monitor `pkg-config --libs gtk4` -O3

input_attr_t input_attr[] = {
    [INPUT_HUMIDITY] = {"humidity", "Relative humidity", "%", 60, 0, 100, 0xAEE7EE80},
    [INPUT_TEMPERATURE] = {"temperature", "Temperature", "℃", 60, -10, 50, 0xD5545280},
    [INPUT_LIGHT] = {"light", "Light", "%", 100, 0, 100, 0xFFEE5880}
};

input_sensor_t input[INPUT_SIZE];

struct LED_Colors led_colors[LED_NUMBER] = {
    [LED_BOARD_GREEN] = { .color_on = 0x22CB1FFF, .color_off = 0x1A4319FF },
    [LED_BOARD_RED]   = { .color_on = 0xFF1C1CFF, .color_off = 0x562020FF },
    [LED_BOARD_BLUE]  = { .color_on = 0x0394F1FF, .color_off = 0x09323FFF },

    [LED_GREEN]       = { .color_on = 0x22CB1FFF, .color_off = 0x1A4319FF },
    [LED_RED]         = { .color_on = 0xFF1C1CFF, .color_off = 0x562020FF },
    [LED_BLUE]        = { .color_on = 0x0394F1FF, .color_off = 0x09323FFF },
    [LED_YELLOW]      = { .color_on = 0xE3F932FF, .color_off = 0x626926FF },
};

uint32_t led_active_color[LED_NUMBER];

GtkWidget *window;

void update_css()
{
    char css_str[2048];
    size_t off = 0;

    #define MY_CSS(...) off += sprintf(css_str + off, __VA_ARGS__)

    MY_CSS(".tab_title {\n");
    MY_CSS("    padding-top: 10px;");
    MY_CSS("    font-weight: bold;\n");
    MY_CSS("}\n\n");

    MY_CSS(".tab_info {\n");
    MY_CSS("    font-size: 3em;\n");
    MY_CSS("    font-weight: bold;\n");
    MY_CSS("}\n\n");

    for (int i = 0; i < INPUT_SIZE; i++) {
        MY_CSS("#tab_info_%d {\n", i);
        MY_CSS("    color: #%06X;\n", input[i].rgba >> 8);
        MY_CSS("}\n\n");
    }

    MY_CSS(".tab_info_output {\n");
    MY_CSS("    font-size: 2em;\n");
    MY_CSS("    font-weight: bold;\n");
    MY_CSS("}\n\n");

    MY_CSS(".page_title {\n");
    MY_CSS("    font-size: 1.5em;\n");
    MY_CSS("    font-weight: bold;\n");
    MY_CSS("    padding-top: 50px;");
    MY_CSS("    padding-bottom: 20px;");
    MY_CSS("}\n\n");

    MY_CSS(".page_sub_title {\n");
    MY_CSS("    font-size: 1em;\n");
    MY_CSS("    font-weight: bold;\n");
    MY_CSS("    padding-top: 20px;");
    MY_CSS("    padding-bottom: 10px;");
    MY_CSS("}\n\n");

    MY_CSS(".page_info {\n");
    MY_CSS("    font-size: 3em;\n");
    MY_CSS("    padding-right: 50px;");
    MY_CSS("    padding-bottom: 50px;");
    MY_CSS("}\n\n");

    MY_CSS(".color_button {\n");
    MY_CSS("    min-width: 80px;\n");
    MY_CSS("    min-height: 60px;\n");
    MY_CSS("    padding-left: 50px;");
    MY_CSS("    padding-bottom: 50px;");
    MY_CSS("}\n\n");

    MY_CSS(".slider {\n");
    MY_CSS("    min-width: 250px;\n");
    MY_CSS("}\n\n");

    for (int i = 0; i < LED_NUMBER; i++) {
        MY_CSS("#led_switch_%d {\n", i);
        MY_CSS("  background-color: #%06X;\n", led_active_color[i] >> 8);
        MY_CSS("}\n\n");
    }

    #undef MY_CSS

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider, css_str);
    gtk_style_context_add_provider_for_display(
            gtk_root_get_display(GTK_ROOT(window)),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
}


void reload_img(void* data) 
{
    size_t i = (size_t)data;
    
    char buffer[256];
    input[i].info.value = PLOT_get_last_value(input[i].graph);
    sprintf(buffer, "%.1f%s", input[i].info.value, input_attr[i].units);

    gtk_label_set_text(GTK_LABEL(input[i].info.page_widget), buffer);
    gtk_label_set_text(GTK_LABEL(input[i].info.tab_widget), buffer);

    gtk_picture_set_filename(GTK_PICTURE(input[i].picture.widget), input[i].picture.file_name);
}

void on_slider_value_changed(GtkRange* range, void* data)
{
    gdouble value = gtk_range_get_value(range);
    struct msg msg = { CMD_SERVO, (uint32_t)value };
    mbox_send(OUTPUT_SERVO, &msg);
}

void on_switch_state_set(GtkSwitch *widget, gboolean state, void* data)
{
    size_t led = (size_t)data;
    if (state) {
        led_active_color[led] = led_colors[led].color_on;
        struct msg msg = { CMD_LED_ON, led };
        mbox_send(OUTPUT_LED, &msg);
    } else {
        led_active_color[led] = led_colors[led].color_off;
        struct msg msg = { CMD_LED_OFF, led };
        mbox_send(OUTPUT_LED, &msg);
    }
    update_css();
}


void init_output(GtkWidget *notebook)
{
    char buffer[256];

    // Tab
    GtkWidget *tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(tab, 160, 70);

    GtkWidget *tab_title = gtk_label_new("Output");
    gtk_widget_add_css_class(tab_title, "tab_title");
    gtk_box_append(GTK_BOX(tab), tab_title);

    GtkWidget *tab_info = gtk_label_new("...");
    gtk_widget_add_css_class(tab_info, "tab_info_output");
    gtk_box_append(GTK_BOX(tab), tab_info);

    // Page
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *page_title = gtk_label_new("Output");
    gtk_widget_add_css_class(page_title, "page_title");
    gtk_box_append(GTK_BOX(page), page_title);

    GtkWidget *servo_grid = gtk_grid_new();
    gtk_widget_set_halign(servo_grid, GTK_ALIGN_CENTER); // Centrar horizontalmente
    gtk_widget_set_valign(servo_grid, GTK_ALIGN_CENTER); // Centrar verticalmente
    gtk_grid_set_row_spacing(GTK_GRID(servo_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(servo_grid), 20);
    gtk_box_append(GTK_BOX(page), servo_grid);

    GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 700.0, 3300.0, 1);
    gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
    gtk_widget_add_css_class(slider, "slider");
    gtk_range_set_value(GTK_RANGE(slider), (3300.0 + 700.0) / 2.0);
    g_signal_connect(slider, "value-changed", G_CALLBACK(on_slider_value_changed), NULL);

    gtk_grid_attach(GTK_GRID(servo_grid), gtk_label_new("Servo motor"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(servo_grid), slider, 1, 0, 1, 1);

    GtkWidget *leds_title = gtk_label_new("LEDs");
    gtk_widget_add_css_class(leds_title, "page_sub_title");
    gtk_box_append(GTK_BOX(page), leds_title);

    GtkWidget *leds_grid = gtk_grid_new();
    gtk_widget_set_halign(leds_grid, GTK_ALIGN_CENTER); // Centrar horizontalmente
    gtk_widget_set_valign(leds_grid, GTK_ALIGN_CENTER); // Centrar verticalmente
    gtk_grid_set_row_spacing(GTK_GRID(leds_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(leds_grid), 20);
    gtk_box_append(GTK_BOX(page), leds_grid);

    gtk_grid_attach(GTK_GRID(leds_grid), gtk_label_new("Green Board"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(leds_grid), gtk_label_new("Red Board"),   0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(leds_grid), gtk_label_new("Blue Board"),  0, 2, 1, 1);
    //gtk_grid_attach(GTK_GRID(leds_grid), gtk_label_new("Green"),       0, 3, 1, 1);
    //gtk_grid_attach(GTK_GRID(leds_grid), gtk_label_new("Red"),         0, 4, 1, 1);
    //gtk_grid_attach(GTK_GRID(leds_grid), gtk_label_new("Blue"),        0, 5, 1, 1);
    //gtk_grid_attach(GTK_GRID(leds_grid), gtk_label_new("Yellow"),      0, 6, 1, 1);

    for (size_t i = 0; i < LED_BOARD_NUMBER; i++) {
        gtk_widget_set_halign(gtk_grid_get_child_at(GTK_GRID(leds_grid), 0, i), GTK_ALIGN_END);
        
        led_active_color[i + LED_BOARD] = led_colors[i + LED_BOARD].color_off;
        GtkWidget *led_switch = gtk_switch_new();
        sprintf(buffer, "led_switch_%lu", i  + LED_BOARD);
        gtk_widget_set_name(led_switch, buffer);
        g_signal_connect(led_switch, "state-set", G_CALLBACK(on_switch_state_set), (void*)i + LED_BOARD);

        gtk_grid_attach(GTK_GRID(leds_grid), led_switch, 1, i, 1, 1);
    }

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, tab);
    launch_writer_thread();

    struct msg msg = { CMD_SERVO, 2000 };
    mbox_send(OUTPUT_SERVO, &msg);
}

void init_input_sensor(input_sensor_t* sensor, const input_attr_t* attr, size_t num, GtkWidget *notebook)
{
    char buffer[256];

    sensor->graph = PLOT_new_graph(attr->name, DEFAULT_WINDOWS_SIZE, DEFAULT_COLOR);
    PLOT_resize_window(sensor->graph, attr->window_size);
    PLOT_set_color_rgba(sensor->graph, attr->rgba);
    PLOT_set_limits(sensor->graph, attr->min, attr->max);

    sensor->picture.file_name = PLOT_get_img_file_name(sensor->graph);
    sensor->rgba = attr->rgba;
    sensor->info.value = attr->min;

    // Tab
    GtkWidget *tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(tab, 160, 100);

    GtkWidget *tab_title = gtk_label_new(attr->title);
    gtk_widget_add_css_class(tab_title, "tab_title");
    gtk_box_append(GTK_BOX(tab), tab_title);

    sprintf(buffer, "%.1f%s", attr->min, attr->units);
    GtkWidget *tab_info = gtk_label_new(buffer);
    sprintf(buffer, "tab_info_%lu", num);
    gtk_widget_set_name(tab_info, buffer);
    gtk_widget_add_css_class(tab_info, "tab_info");
    gtk_box_append(GTK_BOX(tab), tab_info);
    sensor->info.tab_widget = tab_info;

    // Page
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *page_title = gtk_label_new(attr->title);
    gtk_widget_add_css_class(page_title, "page_title");
    gtk_box_append(GTK_BOX(page), page_title);

    GtkWidget *picture = gtk_picture_new();
    gtk_box_append(GTK_BOX(page), picture);
    sensor->picture.widget = picture;

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    sensor->color.rgba = u32_to_GdkRGBA(attr->rgba);
    GtkColorDialog *color_dialog = gtk_color_dialog_new();
    GtkWidget *color_button = gtk_color_dialog_button_new(color_dialog);
    gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(color_button), &sensor->color.rgba);
    gtk_box_append(GTK_BOX(hbox), color_button);
    gtk_widget_add_css_class(color_button, "color_button");
    sensor->color.widget = color_button;

    sprintf(buffer, "%.1f%s", attr->min, attr->units);
    GtkWidget *page_info = gtk_label_new(buffer);
    gtk_widget_add_css_class(page_info, "page_info");
    gtk_box_append(GTK_BOX(hbox), page_info);
    sensor->info.page_widget = page_info;
    gtk_widget_set_halign(hbox, GTK_ALIGN_END);

    gtk_box_append(GTK_BOX(page), hbox);


    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, tab);
}

void destroy_input_sensor(input_sensor_t* sensor)
{
    PLOT_delete_graph(sensor->graph);
    free(sensor->picture.file_name);
}

void init_input(GtkWidget *notebook)
{
    for (size_t i = 0; i < INPUT_SIZE; ++i) {
        init_input_sensor(&input[i], &input_attr[i], i, notebook);
        PLOT_plot_to_file(input[i].graph, reload_img, (void*)i, 0);
    }
    launch_reader_thread();
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

    // IDK why they decided to remove support for async signals with this
    // single widget so in every cycle we check if color has changed
    for (size_t i = 0; i < INPUT_SIZE; i++) {
        GtkColorDialogButton * cdb = GTK_COLOR_DIALOG_BUTTON(input[i].color.widget);
        const GdkRGBA *rgba = gtk_color_dialog_button_get_rgba(cdb);
        if (rgba->alpha != input[i].color.rgba.alpha
                || rgba->red != input[i].color.rgba.red
                || rgba->blue != input[i].color.rgba.blue
                || rgba->green != input[i].color.rgba.green) {
            input[i].color.rgba = *rgba;
            input[i].rgba = u32_from_GdkRGBA(rgba);
            PLOT_set_color_rgba(input[i].graph, input[i].rgba);
            PLOT_plot_to_file(input[i].graph, reload_img, (void*)i, 0);
            update_css();
        }
    }
        
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
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "STR GTK-Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);

    init_input(notebook);
    init_output(notebook);

    gtk_window_set_child(GTK_WINDOW(window), notebook);
    update_css();
    g_timeout_add(20, on_timeout, NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
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
