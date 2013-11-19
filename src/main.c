#include "pebble.h"
#include "numwords-no.h"
	
#define BUFFER_SIZE 28
#define BATTERY_LIMIT 20
	
static struct CommonWordsData {
  TextLayer *label1;
  TextLayer *label2;
  TextLayer *label3;
  
  TextLayer *connection;
  bool connected;
  uint8_t batteryState;
  Window *window;
  char buffer1[BUFFER_SIZE];
  char buffer2[BUFFER_SIZE];
  char buffer3[BUFFER_SIZE];
  char bufferconnection[BUFFER_SIZE];
} s_data;

static void update_time(struct tm* t) {
  /* time_to_words(t->tm_hour, t->tm_min, s_data.buffer, BUFFER_SIZE); */
  time_to_3words(t->tm_hour, t->tm_min, s_data.buffer1, s_data.buffer2, s_data.buffer3, BUFFER_SIZE);
  text_layer_set_text(s_data.label1, s_data.buffer1);
  text_layer_set_text(s_data.label2, s_data.buffer2);
  text_layer_set_text(s_data.label3, s_data.buffer3);
}

static void updateStatusText()
{
    if (s_data.connected)
    {
      text_layer_set_text_color(s_data.connection, GColorWhite);
      text_layer_set_background_color(s_data.connection, GColorBlack);
	  if (BATTERY_LIMIT<s_data.batteryState)
		strcpy(s_data.bufferconnection, "connected"); 
	  else snprintf(s_data.bufferconnection, BUFFER_SIZE, "conn | LOW(%i)", s_data.batteryState);
    } 
	else
    {
		strcpy(s_data.bufferconnection, BATTERY_LIMIT<s_data.batteryState ?
			   "DISCONNECTED":"DISC | LOWBATT");
	}
	text_layer_set_text(s_data.connection, s_data.bufferconnection);
	if (s_data.connected && BATTERY_LIMIT<s_data.batteryState)
		vibes_long_pulse();
	else vibes_double_pulse();
}

static void updateBatteryState()
{
  BatteryChargeState state = battery_state_service_peek();
  bool changed = state.charge_percent < BATTERY_LIMIT && BATTERY_LIMIT < s_data.batteryState;
  
  s_data.batteryState = state.charge_percent;
  if (changed)
	  updateStatusText();
}

static void toggle_connection_background()
{
  static bool bonw = true;
  if (bonw)
  {
	text_layer_set_text_color(s_data.connection, GColorBlack);
    text_layer_set_background_color(s_data.connection, GColorWhite);
  } else
  {
    text_layer_set_text_color(s_data.connection, GColorWhite);
    text_layer_set_background_color(s_data.connection, GColorBlack);
  }
  bonw = !bonw;
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
  updateBatteryState();
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (!s_data.connected || s_data.batteryState < BATTERY_LIMIT)
  {
	  toggle_connection_background();
  }
}

static void handle_bluetooth(bool connected) {
  if (s_data.connected != connected)
  {
    s_data.connected = connected;
	updateStatusText();
  }
}

TextLayer* createLayer(Layer* root, int ofs, int w, GFont font)
{
  TextLayer* layer = text_layer_create(GRect(0, ofs, w, ofs+40));
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_background_color(layer, GColorBlack);
  text_layer_set_font(layer, font);
  layer_set_clips(text_layer_get_layer(layer), false);
  layer_add_child(root, text_layer_get_layer(layer));
	
  return layer;
}

static void do_init(void) {
  s_data.window = window_create();
  const bool animated = false;
  window_stack_push(s_data.window, animated);
  window_set_background_color(s_data.window, GColorBlack);
  
  Layer *root_layer = window_get_root_layer(s_data.window);
  GRect frame = layer_get_frame(root_layer);

  s_data.label1 = createLayer(root_layer, 0, frame.size.w, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  s_data.label2 = createLayer(root_layer, 48, frame.size.w, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  s_data.label3 = createLayer(root_layer, 88, frame.size.w, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time(t);
	
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  
  s_data.connected = false;
  s_data.batteryState = 100;

  s_data.connection = text_layer_create(GRect(0, frame.size.h - 24, frame.size.w, 24));
  text_layer_set_font(s_data.connection, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_data.connection, GTextAlignmentCenter);
  layer_set_clips(text_layer_get_layer(s_data.connection), false);
  layer_add_child(root_layer, text_layer_get_layer(s_data.connection));
	
  handle_bluetooth(true);
  
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
}

static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  window_destroy(s_data.window);

  text_layer_destroy(s_data.label1);
  text_layer_destroy(s_data.label2);
  text_layer_destroy(s_data.label3);
  text_layer_destroy(s_data.connection);
}

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
