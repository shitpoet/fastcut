// fastcut - video trimming tool

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

typedef char bool;
#define true 1
#define false 0
#define nil NULL

// command line arguments
char* fn;
int vid_w;
int vid_h;
int vid_dur_secs;
char* frame_fn = "/tmp/fastcut-frame.jpg";

int trim_start;
int trim_end;

void *window, *vbox, *vbox2, *swindow, *hbox, *hbox2, *image, *image_evbox;
int width, height;
GdkPixbuf *pixbuf;
char* pixbuf_copy;
cairo_surface_t *surface;
cairo_t *cr;
gint p_stride, p_n_channels, s_stride;
guchar *p_pixels, *s_pixels;

typedef struct {
  void* prev;
  char* pixbuf_copy;
} history_entry_t;

history_entry_t* history = nil;

// prepare copying between pixbuf and surface
void prepare_blits() {
  g_object_get(G_OBJECT(pixbuf), "width", &width, "height", &height, "rowstride",  &p_stride, "n-channels", &p_n_channels, "pixels",  &p_pixels, NULL );
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create(surface);
  s_stride = cairo_image_surface_get_stride(surface);
  s_pixels = cairo_image_surface_get_data(surface);
}

// copy from pixbuf to surface
void begin_drawing() {
  int h = height;
  void *p_ptr = p_pixels, *s_ptr = s_pixels;
  /* Copy pixel data from pixbuf to surface */
  while( h-- ) {
    gint i;
    guchar *p_iter = p_ptr;
    guchar *s_iter = s_ptr;
    for( i = 0; i < width; i++ ) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
       /* Pixbuf:  RGB(A)
        * Surface: BGRA */
       if( p_n_channels == 3 ) {
          s_iter[0] = p_iter[2];
          s_iter[1] = p_iter[1];
          s_iter[2] = p_iter[0];
          s_iter[3] = 0xff;
       } else { /* p_n_channels == 4 */
          gdouble alpha_factor = p_iter[3] / (gdouble)0xff;

          s_iter[0] = (guchar)( p_iter[2] * alpha_factor + .5 );
          s_iter[1] = (guchar)( p_iter[1] * alpha_factor + .5 );
          s_iter[2] = (guchar)( p_iter[0] * alpha_factor + .5 );
          s_iter[3] =           p_iter[3];
       }
#elif G_BYTE_ORDER == G_BIG_ENDIAN
       /* Pixbuf:  RGB(A)
        * Surface: ARGB */
       if( p_n_channels == 3 ) {
          s_iter[3] = p_iter[2];
          s_iter[2] = p_iter[1];
          s_iter[1] = p_iter[0];
          s_iter[0] = 0xff;
       } else { /* p_n_channels == 4 */
          gdouble alpha_factor = p_iter[3] / (gdouble)0xff;

          s_iter[3] = (guchar)( p_iter[2] * alpha_factor + .5 );
          s_iter[2] = (guchar)( p_iter[1] * alpha_factor + .5 );
          s_iter[1] = (guchar)( p_iter[0] * alpha_factor + .5 );
          s_iter[0] =           p_iter[3];
       }
#else /* PDP endianness */
       /* Pixbuf:  RGB(A)
        * Surface: RABG */
       if( p_n_channels == 3 ) {
          s_iter[0] = p_iter[0];
          s_iter[1] = 0xff;
          s_iter[2] = p_iter[2];
          s_iter[3] = p_iter[1];
       } else { /* p_n_channels == 4 */
          gdouble alpha_factor = p_iter[3] / (gdouble)0xff;

          s_iter[0] = (guchar)( p_iter[0] * alpha_factor + .5 );
          s_iter[1] =           p_iter[3];
          s_iter[1] = (guchar)( p_iter[2] * alpha_factor + .5 );
          s_iter[2] = (guchar)( p_iter[1] * alpha_factor + .5 );
       }
#endif
       s_iter += 4;
       p_iter += p_n_channels;
    }
    s_ptr += s_stride;
    p_ptr += p_stride;
  }
}

// copy from surface to buffer
void end_drawing() {
  int h = height;
  void *p_ptr = p_pixels, *s_ptr = s_pixels;
  while( h-- ) {
    gint i;
    guchar *p_iter = p_ptr, *s_iter = s_ptr;

    for( i = 0; i < width; i++ ) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
       /* Pixbuf:  RGB(A) * Surface: BGRA */
       gdouble alpha_factor = (gdouble)0xff / s_iter[3];

       p_iter[0] = (guchar)( s_iter[2] * alpha_factor + .5 );
       p_iter[1] = (guchar)( s_iter[1] * alpha_factor + .5 );
       p_iter[2] = (guchar)( s_iter[0] * alpha_factor + .5 );
       if( p_n_channels == 4 )
          p_iter[3] = s_iter[3];
#elif G_BYTE_ORDER == G_BIG_ENDIAN
       /* Pixbuf:  RGB(A) * Surface: ARGB */
       gdouble alpha_factor = (gdouble)0xff / s_iter[0];

       p_iter[0] = (guchar)( s_iter[1] * alpha_factor + .5 );
       p_iter[1] = (guchar)( s_iter[2] * alpha_factor + .5 );
       p_iter[2] = (guchar)( s_iter[3] * alpha_factor + .5 );
       if( p_n_channels == 4 )
          p_iter[3] = s_iter[0];
#else /* PDP endianness */
       /* Pixbuf:  RGB(A) * Surface: RABG */
       gdouble alpha_factor = (gdouble)0xff / s_iter[1];

       p_iter[0] = (guchar)( s_iter[0] * alpha_factor + .5 );
       p_iter[1] = (guchar)( s_iter[3] * alpha_factor + .5 );
       p_iter[2] = (guchar)( s_iter[2] * alpha_factor + .5 );
       if( p_n_channels == 4 )
          p_iter[3] = s_iter[1];
#endif
       s_iter += 4;
       p_iter += p_n_channels;
    }
    s_ptr += s_stride;
    p_ptr += p_stride;
  }
  gtk_widget_queue_draw(image);
}

void revert_pixbuf() {
  printf("revert buf\n");
  //memcpy(p_pixels, pixbuf_copy, height * p_stride);
  if (history) {
    memcpy(p_pixels, history->pixbuf_copy, height * p_stride);
    //history = history->prev;
  }
}

bool parse_args(int argc, char **argv) {
  //printf("%d command line arguments \n", argc);
  for (int i = 0; i < argc; i++) {
    //printf("%d: %s \n", i, argv[i]);
  }
  if (argc == 1 + 1 + 1 + 1 + 1) {
    fn = argv[1];
    sscanf(argv[2], "%d", &vid_w);
    sscanf(argv[3], "%d", &vid_h);
    float vid_dur_secs_f;
    sscanf(argv[4], "%f", &vid_dur_secs_f);
    vid_dur_secs = floor(vid_dur_secs_f);
    //printf("fn %s width %d height %d vid_dur_secs %d \n", fn, vid_w, vid_h, vid_dur_secs);
    return true;
  } else {
    printf("usage: fastcut filename.ext width height duration\n");
    printf("       duration is floating-point number of seconds\n");
    return false;
  }
}

char* secs_to_time(int t) {
  char* s = malloc(6 + 2 + 1);
  int secs = t % 60;
  int mins = (t / 60) % 60;
  int hours = (t / 60 / 60);
  sprintf(s, "%02d:%02d:%02d", hours, mins, secs);
  printf("%s \n", s);
  return s;
}

void get_video_frame(int t) {
  char s[1024];
  sprintf(s, "ffmpeg -ss %s -i %s -vframes 1 -q:v 1 /tmp/fastcut-frame.jpg -y > /dev/null 2> /dev/null",
    secs_to_time(t), fn);
  printf("%s \n", s);
  system(s);
}

void seek(int t) {
  get_video_frame(t);
  gtk_image_set_from_file(image, frame_fn);
}

void quit() {
  gtk_main_quit();
}

bool need_trim() {
  return trim_start != 0 || trim_end != (vid_dur_secs -1);
}

void do_trim() {
  printf("do_trim \n");
  char s[1024];
  char* start = secs_to_time(trim_start);
  char* dur = secs_to_time(trim_end - trim_start + 1);
  char* out_fn = fn; //"test.flv";
  sprintf(s,
    "ffmpeg -ss %s -i %s -to %s -c copy %s",
    start, fn, dur, out_fn
  );
  printf("%s \n", s);
  system(s);
}

void trim() {
  if (need_trim()) {
    //int res = system("gtdialog yesno-msgbox --text \"Trim the video?\" --no-cancel") == 1) {
    int res = system("exit `gtdialog yesno-msgbox --text \"Trim the video?\"` ");
    printf("dialog res %d \n", res);
    if (res == 256 || res == 512) {
      printf("yes or no \n");
      if (res == 256) do_trim();
      quit();
    }
  } else {
    int res = system("eit `gtdialog ok-msgbox --text \"  No changes were made  \" --no-cancel`");
    quit();
  }
}

static void cb_lscale_change(void* scale) {
  gdouble f = gtk_range_get_value(scale);
  trim_start = (int)f;
  seek(trim_start);
  printf("left trim pos change %d \n", trim_start);
}

static void cb_rscale_change(void* scale) {
  gdouble f = gtk_range_get_value(scale);
  trim_end = (int)f;
  seek(trim_end);
  printf("right trim pos change %d \n", trim_end);
}

bool keypress(GtkWidget* widget, GdkEventKey *event, gpointer data) {
  int key = event->keyval;
  printf("key press %d\n", key);
  if (event->keyval == GDK_KEY_Escape) {
      quit();
      return TRUE;
  }
  return FALSE;
}

void cb_trim(GtkButton *button, gpointer data) {
  trim();
}

int main(int argc, char **argv) {
  if (parse_args(argc, argv)) {
    trim_start = 0;
    trim_end = vid_dur_secs - 1;

    gtk_init( &argc, &argv );

    GError* error = NULL;
    pixbuf = gdk_pixbuf_new_from_file(frame_fn, &error);
    if (pixbuf == NULL) {
        printf("cant open frame file\n");
        exit(1);
    }

    window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    //gtk_window_move(window, 10, 300);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(cb_trim), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(keypress), NULL);

    vbox = gtk_vbox_new( FALSE, 6 );
    gtk_container_add( GTK_CONTAINER( window ), vbox );

    swindow = gtk_scrolled_window_new( NULL, NULL );
    gtk_box_pack_start( GTK_BOX( vbox ), swindow, TRUE, TRUE, 0 );

    hbox = gtk_hbox_new( TRUE, 6 );
    vbox2 = gtk_vbox_new( TRUE, 6 );

    //pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, FALSE, 8, 200, 200 );
    //gdk_pixbuf_fill( pixbuf, 0xffff00ff );
    image = gtk_image_new_from_pixbuf( pixbuf );
    //g_object_unref( G_OBJECT( pixbuf ) );

    prepare_blits();

    gtk_window_set_default_size(GTK_WINDOW(window), width+100, height+150);

    gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( swindow ), hbox );

    image_evbox = gtk_event_box_new();
    gtk_container_add(image_evbox, image);

    //gtk_box_pack_start( GTK_BOX( hbox ), image, FALSE, FALSE, 0 );
    gtk_box_pack_start(hbox, vbox2, FALSE, FALSE, 0);
    gtk_box_pack_start(vbox2, image_evbox, FALSE, FALSE, 0);

    hbox = gtk_hbox_new( FALSE, 6 );
    gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 0 );

    //GtkWidget* button = gtk_button_new_with_label( "undo" );
    //g_signal_connect( G_OBJECT( button ), "clicked", G_CALLBACK(cb_undo), NULL );
    //gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );

    GtkWidget* lscale = gtk_hscale_new_with_range(0, vid_dur_secs -1, 1);
    gtk_scale_set_draw_value(GTK_SCALE(lscale), false);
    g_signal_connect(lscale, "value_changed", G_CALLBACK(cb_lscale_change), NULL);
    gtk_box_pack_start(GTK_BOX( hbox ), lscale, true, true, 20 );

    GtkWidget* rscale = gtk_hscale_new_with_range(0, vid_dur_secs - 1, 1);
    gtk_scale_set_draw_value(GTK_SCALE(rscale), false);
    gtk_range_set_value(GTK_WIDGET(rscale), vid_dur_secs - 1);
    g_signal_connect(rscale, "value_changed", G_CALLBACK(cb_rscale_change), NULL);
    gtk_box_pack_start(GTK_BOX( hbox ), rscale, true, true, 20 );

    GtkWidget* button = gtk_button_new_with_label( "trim" );
    g_signal_connect( G_OBJECT( button ), "clicked", G_CALLBACK(cb_trim), NULL );
    gtk_box_pack_end( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );

    gtk_widget_show_all( window );
    seek(0);
    gtk_main();
    return 0;
  } else {
    return 1;
  }
}
