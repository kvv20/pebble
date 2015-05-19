#include <pebble.h>
  
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1


static Window *s_main_window;

static TextLayer *s_date_layer;
static BitmapLayer *s_background_layer;
static InverterLayer *inverter_layer;
static TextLayer *s_time_layer;
static GFont s_time_font;

static GBitmap *s_background_bitmap_sun;
static GBitmap *s_background_bitmap_moon;

static struct tm *tick_time;

static char date_buffer[] = "Thu, Apr 01    -10C  100% ";
static char temperature_buffer[] = "      ";


static void update_sky(){
  GBitmap *curBitmap;
  time_t temp = time(NULL); 
  tick_time = localtime(&temp);
  
  int x_shift = 0;
  int y_shift = 0;
  char battery_buffer[16];
  BatteryChargeState charge_state = battery_state_service_peek();
  snprintf(battery_buffer, sizeof(battery_buffer), " %d%%", charge_state.charge_percent);
  // Create a long-lived buffer
 
  strftime(date_buffer, sizeof("Thu, Apr 01  -10C   100%"), "%a, %b %d  ", tick_time);
  strcat(date_buffer, temperature_buffer);
  strcat(date_buffer, battery_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
  

  
  int hour = tick_time->tm_hour;
  //hour = 20;
  APP_LOG(APP_LOG_LEVEL_INFO, "Updating sky");
  if (hour > 6 && hour < 21){
    //day
    //range from -70 to 70
    x_shift = hour * 10.8 - 145;
    curBitmap = s_background_bitmap_sun;
  }
  else{
    //night
    if (hour < 12) {
      //early morning - add 24 to get consistent range e.g. 21-30
      hour = hour + 24;
    }
    x_shift = hour * 15.6 - 397;
    curBitmap = s_background_bitmap_moon;
  }
  //x_shift = 70;
  y_shift = abs(x_shift) / 3;
  
  layer_set_frame(bitmap_layer_get_layer(s_background_layer), GRect(x_shift, y_shift, 144, 58));
  bitmap_layer_set_bitmap(s_background_layer, curBitmap);
}

static void update_time(bool force_date) {
   APP_LOG(APP_LOG_LEVEL_INFO, "Updating time");
  // Get a tm structure
  time_t temp = time(NULL); 
  tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char time_buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(time_buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(time_buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, time_buffer);
  
  static int hour = 25; //make sure it will get updated first time
  if (hour != tick_time->tm_hour || force_date){
    if (hour != tick_time->tm_hour){
      app_message_outbox_send();
    }
    hour = tick_time->tm_hour;
    update_sky();
  }
}


static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap_sun = gbitmap_create_with_resource(RESOURCE_ID_SUN);
  s_background_bitmap_moon = gbitmap_create_with_resource(RESOURCE_ID_MOON);

  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 58));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap_sun);
  // Set LayerUpdateProc to draw bitmap
  //layer_set_update_proc(s_background_layer, sky_update_proc);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(5, 58, 139, 90));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
//  text_layer_set_text(s_time_layer, "12:34");
  
  // Create GFont for time
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MURDER_72));
  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);

  // Improve the layout to be more like a watchface
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(5, 148, 139, 20));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);

  // Apply to TextLayer for date
  //text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_font(s_time_layer, s_time_font);

  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  //invert evrything
  inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_layer));
}

static void main_window_unload(Window *window) {
  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  
  // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
  
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap_sun);
  gbitmap_destroy(s_background_bitmap_moon);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(false);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
     APP_LOG(APP_LOG_LEVEL_INFO, "Key %d processing", (int)t->key);
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), " %dC ", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:

      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
//    update_time(true);
    update_sky();

    // Look for next item
    t = dict_read_next(iterator);
  }
}


static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


static void init() {
// Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  //tick_timer_service_subscribe(HOUR_UNIT, update_sky);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  
  // Make sure the time is displayed from the start
  update_time(false); 
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}