#include "pebble.h"
#include "numwords-no.h"

#define BUFFER_SIZE 86

static struct CommonWordsData {
  TextLayer *label;
  TextLayer *connection;
  Window *window;
  char buffer[BUFFER_SIZE];
} s_data;

static void update_time(struct tm* t) {
  time_to_words(t->tm_hour, t->tm_min, s_data.buffer, BUFFER_SIZE);
  text_layer_set_text(s_data.label, s_data.buffer);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void handle_bluetooth(bool connected) {
  if (connected)
  {
    text_layer_set_text(s_data.connection, "connected");
    text_layer_set_text_color(s_data.connection, GColorWhite);
    text_layer_set_background_color(s_data.connection, GColorBlack);
    vibes_short_pulse();
    vibes_short_pulse();
  } else
  {
    text_layer_set_text(s_data.connection, "disconnected");
	text_layer_set_text_color(s_data.connection, GColorBlack);
    text_layer_set_background_color(s_data.connection, GColorWhite);
    vibes_long_pulse();
  }
}

static void do_init(void) {
  s_data.window = window_create();
  const bool animated = true;
  window_stack_push(s_data.window, animated);

  window_set_background_color(s_data.window, GColorBlack);
  GFont font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);

  Layer *root_layer = window_get_root_layer(s_data.window);
  GRect frame = layer_get_frame(root_layer);

  s_data.label = text_layer_create(GRect(0, 0, frame.size.w, frame.size.h - 40));
  text_layer_set_text_color(s_data.label, GColorWhite);
  text_layer_set_font(s_data.label, font);
  layer_add_child(root_layer, text_layer_get_layer(s_data.label));

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time(t);
	
	/*
  s_data.connection = text_layer_create(GRect(0, frame.size.h - 40, frame.size.w, 40));
  text_layer_set_text_color(s_data.connection, GColorWhite);
  text_layer_set_font(s_data.connection, fonts_get_system_font(FONT_KEY_BITHAM_18_LIGHT_SUBSET));
  text_layer_set_text_alignment(s_data.connection, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(s_data.connection));
  handle_bluetooth(true);
	*/
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
}

static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  window_destroy(s_data.window);
  text_layer_destroy(s_data.label);
}

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
