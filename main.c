#include <pebble.h>

// ─── Claves de persistencia ──────────────────────────────────────────────────
#define PERSIST_KEY_THEME 1

// ─── Ventana y capas ────────────────────────────────────────────────────────
static Window      *s_window;
static BitmapLayer *s_escudo_layer;
static TextLayer   *s_hour_layer;
static TextLayer   *s_min_layer;
static TextLayer   *s_ampm_layer;

// ─── Recursos ────────────────────────────────────────────────────────────────
static GBitmap     *s_escudo_bitmap;
static GFont        s_font;

// ─── Buffers de tiempo ───────────────────────────────────────────────────────
static char s_hour_buf[3];
static char s_min_buf[3];
static char s_ampm_buf[3];

// ─── Temas ───────────────────────────────────────────────────────────────────
typedef struct { GColor bg; GColor time_color; } Theme;

static const Theme THEMES[] = {
  { .bg = {.argb = 0b11000000}, .time_color = {.argb = 0b11111100} }, // Clásico: negro + amarillo
  { .bg = {.argb = 0b11110000}, .time_color = {.argb = 0b11111111} }, // Necaxa: rojo + blanco
  { .bg = {.argb = 0b11111111}, .time_color = {.argb = 0b11110000} }, // Invertido: blanco + rojo
  { .bg = {.argb = 0b11000000}, .time_color = {.argb = 0b11111111} }, // Noche: negro + blanco
};
#define THEME_COUNT 4

static int s_current_theme = 0;

// ────────────────────────────────────────────────────────────────────────────
//  APLICAR TEMA
// ────────────────────────────────────────────────────────────────────────────

static void apply_theme() {
  Theme t = THEMES[s_current_theme];
  window_set_background_color(s_window, t.bg);
  text_layer_set_text_color(s_hour_layer, t.time_color);
  text_layer_set_text_color(s_min_layer,  t.time_color);
}

// ────────────────────────────────────────────────────────────────────────────
//  MENSAJES DESDE EL TELÉFONO (Clay los maneja automáticamente)
// ────────────────────────────────────────────────────────────────────────────

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *theme_tuple = dict_find(iter, MESSAGE_KEY_THEME);
  if (theme_tuple) {
    s_current_theme = (int)theme_tuple->value->int32;
    if (s_current_theme < 0) s_current_theme = 0;
    if (s_current_theme >= THEME_COUNT) s_current_theme = THEME_COUNT - 1;
    persist_write_int(PERSIST_KEY_THEME, s_current_theme);
    apply_theme();
  }
}

// ────────────────────────────────────────────────────────────────────────────
//  HORA
// ────────────────────────────────────────────────────────────────────────────

static void update_time() {
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  strftime(s_hour_buf, sizeof(s_hour_buf), "%I", t);
  strftime(s_min_buf,  sizeof(s_min_buf),  "%M", t);
  strftime(s_ampm_buf, sizeof(s_ampm_buf), "%p", t);
  text_layer_set_text(s_hour_layer, s_hour_buf);
  text_layer_set_text(s_min_layer,  s_min_buf);
  text_layer_set_text(s_ampm_layer, s_ampm_buf);
}

static void tick_handler(struct tm *t, TimeUnits u) { update_time(); }

// ────────────────────────────────────────────────────────────────────────────
//  VENTANA
// ────────────────────────────────────────────────────────────────────────────

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_escudo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMG_ESCUDO);
  s_escudo_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_escudo_layer, s_escudo_bitmap);
  bitmap_layer_set_compositing_mode(s_escudo_layer, GCompOpSet);
  layer_add_child(root, bitmap_layer_get_layer(s_escudo_layer));

  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_METRO_55));

  s_hour_layer = text_layer_create(GRect(90, -2, 95, 70));
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_font(s_hour_layer, s_font);
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(s_hour_layer));

  s_min_layer = text_layer_create(GRect(90, 40, 95, 70));
  text_layer_set_background_color(s_min_layer, GColorClear);
  text_layer_set_font(s_min_layer, s_font);
  text_layer_set_text_alignment(s_min_layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(s_min_layer));

  s_ampm_layer = text_layer_create(GRect(88, 90, 95, 30));
  text_layer_set_background_color(s_ampm_layer, GColorClear);
  text_layer_set_text_color(s_ampm_layer, GColorWhite);
  text_layer_set_font(s_ampm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_ampm_layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(s_ampm_layer));

  apply_theme();
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_min_layer);
  text_layer_destroy(s_ampm_layer);
  bitmap_layer_destroy(s_escudo_layer);
  gbitmap_destroy(s_escudo_bitmap);
  fonts_unload_custom_font(s_font);
}

// ────────────────────────────────────────────────────────────────────────────
//  INIT / DEINIT
// ────────────────────────────────────────────────────────────────────────────

static void init() {
  if (persist_exists(PERSIST_KEY_THEME)) {
    s_current_theme = persist_read_int(PERSIST_KEY_THEME);
  }

  app_message_register_inbox_received(inbox_received);
  app_message_open(128, 128);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
