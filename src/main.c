#include "pebble.h"
#include "numwords-no.h"
    
#define BUFFER_SIZE 28
#define BATTERY_LIMIT 20
    
static struct CommonWordsData {
  TextLayer *label1;
  TextLayer *label2;
  TextLayer *label3;
  TextLayer *connection;
  TextLayer *connectionBackground;

  bool connected;
  uint8_t batteryState;
    
  Window *window;
  char buffer1[BUFFER_SIZE];
  char buffer2[BUFFER_SIZE];
  char buffer3[BUFFER_SIZE];
  char bufferconnection[BUFFER_SIZE];
} s_data;

static void updateTime(struct tm* t) {
  /* time_to_words(t->tm_hour, t->tm_min, s_data.buffer, BUFFER_SIZE); */
  time_to_3words(t->tm_hour, t->tm_min, s_data.buffer1, s_data.buffer2, s_data.buffer3, BUFFER_SIZE);
    
  text_layer_set_text(s_data.label1, s_data.buffer1);
  text_layer_set_text(s_data.label2, s_data.buffer2);
  text_layer_set_text(s_data.label3, s_data.buffer3);
}

static void updateStatus();

static void updateBatteryState()
{
  BatteryChargeState state = battery_state_service_peek();
  bool changed = (state.charge_percent < BATTERY_LIMIT && BATTERY_LIMIT < s_data.batteryState) ||
      (s_data.batteryState < BATTERY_LIMIT && BATTERY_LIMIT < state.charge_percent);
  
  s_data.batteryState = state.charge_percent;
  if (changed)
      updateStatus();
}

static void toggleConnectionBackground(bool reset)
{
  static bool bonw = false;
 
  if (reset)
      bonw = false;
    
  if (bonw)
  {
    text_layer_set_text_color(s_data.connection, GColorBlack);
    text_layer_set_background_color(s_data.connectionBackground, GColorWhite);
  } 
  else
  {
    text_layer_set_text_color(s_data.connection, GColorWhite);
    text_layer_set_background_color(s_data.connectionBackground, GColorBlack);
  }
  bonw = !bonw;
}


static void handleMinuteTick(struct tm *tick_time, TimeUnits units_changed) {
  updateTime(tick_time);
  updateBatteryState();
}

static void handleSecondTick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed != SECOND_UNIT)
      handleMinuteTick(tick_time, units_changed);
  if (!s_data.connected || s_data.batteryState < BATTERY_LIMIT)
    toggleConnectionBackground(false);
}

static void updateStatus()
{
    if (s_data.connected)
    {
      if (BATTERY_LIMIT<s_data.batteryState)
        strcpy(s_data.bufferconnection, "connected"); 
      else snprintf(s_data.bufferconnection, BUFFER_SIZE, "LOW BATTERY (%i)", s_data.batteryState);
    } 
    else
    {
      if(BATTERY_LIMIT<s_data.batteryState)
        strcpy(s_data.bufferconnection, "DISCONNECTED");
      else snprintf(s_data.bufferconnection, BUFFER_SIZE, "DISC | LOW(%i)", s_data.batteryState);
    }
    text_layer_set_text(s_data.connection, s_data.bufferconnection);
    
    if (s_data.connected)
    {
        vibes_short_pulse();
        toggleConnectionBackground(true);
        tick_timer_service_subscribe(MINUTE_UNIT, &handleMinuteTick);
    }
    else 
    {
        // Vibe pattern: ON for 200ms, OFF for 100ms, etc
        static const uint32_t const segments[] = { 200, 100, 200, 100, 200, 100, 200, 100, 200 };
        VibePattern pat = { 
            .durations = segments,
            .num_segments = ARRAY_LENGTH(segments),
        };
        vibes_enqueue_custom_pattern(pat);
        tick_timer_service_subscribe(SECOND_UNIT, &handleSecondTick);
    }
}

static void handleBluetooth(bool connected) 
{
  if (s_data.connected != connected)
  {
    s_data.connected = connected;
    updateStatus();
  }
}

TextLayer* createLayer(Layer* root, int ofs, int w, GFont font)
{
  TextLayer* layer = text_layer_create(GRect(0, ofs, w, 42));
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
  int fullw = frame.size.w;

  s_data.label3 = createLayer(root_layer, 76, fullw, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  s_data.label2 = createLayer(root_layer, 38, fullw, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));  
  s_data.label1 = createLayer(root_layer, 0, fullw, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  updateTime(t);
  tick_timer_service_subscribe(MINUTE_UNIT, &handleMinuteTick);    
    
  s_data.connected = bluetooth_connection_service_peek();
  s_data.batteryState = 100;

  s_data.connection = text_layer_create(GRect(0, 0, fullw, 24));
  text_layer_set_font(s_data.connection, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_data.connection, GTextAlignmentCenter);
  text_layer_set_background_color(s_data.connection, GColorClear);

  s_data.connectionBackground = text_layer_create(GRect(0, frame.size.h - 32, fullw, 32));
  layer_add_child(text_layer_get_layer(s_data.connectionBackground), text_layer_get_layer(s_data.connection));
  layer_add_child(root_layer, text_layer_get_layer(s_data.connectionBackground));
  
  updateStatus();
  bluetooth_connection_service_subscribe(&handleBluetooth);
}

static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  text_layer_destroy(s_data.label1);
  text_layer_destroy(s_data.label2);
  text_layer_destroy(s_data.label3);
    
  text_layer_destroy(s_data.connection);
  text_layer_destroy(s_data.connectionBackground);

  window_destroy(s_data.window);
}

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
