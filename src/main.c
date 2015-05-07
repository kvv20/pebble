#include <pebble.h>

  
static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont s_time_font;
static TextLayer *s_date_layer;
//static GFont s_date_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap_sun;
static GBitmap *s_background_bitmap_moon;
static struct tm *tick_time;

static void update_sky(){
  time_t temp = time(NULL); 
  tick_time = localtime(&temp);
  
  int x_shift = 0;
  char battery_buffer[16];
  BatteryChargeState charge_state = battery_state_service_peek();
  snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", charge_state.charge_percent);
  // Create a long-lived buffer
  static char date_buffer[] = "Thu, Apr 01      100% ";
  strftime(date_buffer, sizeof("Thu, Apr 01      100%"), "%a, %b %d      ", tick_time);
  strcat(date_buffer, battery_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
  

  
  int hour = tick_time->tm_hour;
  APP_LOG(APP_LOG_LEVEL_INFO, "Updating sky");
  if (hour > 6 && hour < 21){
    //range from -80 to 80
    x_shift = hour * 10 - 144;
    layer_set_frame(bitmap_layer_get_layer(s_background_layer), GRect(x_shift, 0, 144, 58));
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap_sun);
  }
  else{
    if (hour < 12) {
      //early morning
      x_shift = hour * 16;
      layer_set_frame(bitmap_layer_get_layer(s_background_layer), GRect(x_shift, 0, 144 - x_shift, 58));
    }
    else{
      //late evening
      x_shift = (hour-12) * 6;
      layer_set_frame(bitmap_layer_get_layer(s_background_layer), GRect(x_shift, 0, 144 - x_shift, 58));
      
    }
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap_moon);
  }
}

static void update_time() {
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
  
  static int hour = 25; //make sure it will get updated
  if (hour != tick_time->tm_hour){
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
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
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
  update_time();
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

  // Make sure the time is displayed from the start
  update_time();
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