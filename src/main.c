//56b37c6f-792a-480f-b962-9a0db8c32aa4
//58ed427c-0a4f-405f-9de0-37417179023c
/*
Save settings upon exit
Restore settings upon start, except as defined by settings

Start ->
Back = Exit
    Up = BPM Mode
Select =

tried the sample tap ap here, but it sucks hardcore
http://developer.getpebble.com/guides/pebble-apps/sensors/accelerometer/


*/
#include "pebble.h"

static Window *main_window;
static Layer *graphics_layer, *graph_layer;
bool pause=false;
uint8_t offset=0;

int16_t z[256];
int16_t graph[144];
int16_t zoom=8;

int32_t abs32(int32_t x) {return (x ^ (x >> 31)) - (x >> 31);}
//int16_t abs16(int16_t x) {return (x ^ (x >> 15)) - (x >> 15);}
int16_t abs16(int16_t x) {
  return x>=0 ? x : 0 - x;
}
// ------------------------------------------------------------------------ //
//  Timer Functions
// ------------------------------------------------------------------------ //
#define UPDATE_MS 50 // Refresh rate in milliseconds
static void timer_callback(void *data) {
  layer_mark_dirty(graphics_layer);                    // Schedule redraw of screen
  app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
}

void accel_data_handler(AccelRawData *data, uint32_t num_samples, uint64_t timestamp) {
//void accel_data_handler(AccelData *data, uint32_t num_samples) {
if(!pause)
  for(uint32_t i=0; i<num_samples; i++) {
    z[offset]=data[i].z;//>>3;
    offset++;
  }
}

// timestamp
// not vibrate
// call function when tap is detected

// ------------------------------------------------------------------------ //
//  Button Functions
// ------------------------------------------------------------------------ //
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //vibes_long_pulse();
  zoom--;
}
void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //vibes_short_pulse();
  pause=!pause;
}
void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  zoom++;
}
  
void click_config_provider(void *context) {
  //window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  //window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_single_click_handler);
}
  
// ------------------------------------------------------------------------ //
//  Drawing Functions
// ------------------------------------------------------------------------ //
int16_t Q1=0;

void draw_textbox(GContext *ctx, int16_t x, int16_t y, int16_t number) {
  static char text[40];  //Buffer to hold text
  graphics_context_set_text_color(ctx, GColorWhite);   // Text Color
  graphics_context_set_fill_color(ctx, GColorBlack);   // BG Color
  graphics_context_set_stroke_color(ctx, GColorWhite); // Line Color
  int16_t w;
  if(number<0) {
    if(number>-10) w=20;
    else if(number>-100) w=28;
    else if(number>-1000) w=36;
    else w=44;
  } else {
    if(number<10) w=10; //12
    else if(number<100) w=16; //20
    else if(number<1000) w=22; //28
    else w=28; //36
  }
  
  GRect textframe = GRect(x, y, w, 13);  // Text Box Position and Size: x, y, w, h
  graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
  snprintf(text, sizeof(text), "%d", number);  // What text to draw
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(x, y-3, w, 17), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text
}


// ------------------------------------------------------------------------ //
//  Draw Graph
// ------------------------------------------------------------------------ //
static void graph_layer_update(Layer *me, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite); // Line Color
  graphics_draw_line(ctx, GPoint(0, 0), GPoint(144,0));
  
  // cleanup graph
  for(uint8_t i=0; i<144; i++)
    graph[i] = z[(i+offset+(256-144))%256];
  
  // calculate average line
  int32_t avg=0;
  for(int16_t i=0; i<32; i++)
    avg+=graph[143-i];
  avg/=32;
  
  // fit average line better by discarding outliers
  int32_t count=0, total=0;
  for(int16_t i=0; i<32; i++)
    if(abs16(graph[143-i]-avg)<100)
      {count+=graph[143-i]; total++;}
  if(total>0) avg=count/total;
  
  // draw average line
//   graphics_draw_line(ctx, GPoint(0, 50), GPoint(144,50));
  
  // graph
  for(int16_t i=0; i<143; i++)
    graphics_draw_line(ctx, GPoint(i, ((graph[i]-avg))/zoom+50), GPoint(i, ((graph[i+1]-avg))/zoom+50));
  graphics_draw_pixel(ctx, GPoint(143, ((graph[143]-avg))/zoom+50));
  
  draw_textbox(ctx, 0, 2, (50*zoom)/8); // Show what value top of graph shows
  draw_textbox(ctx, 0, 50, zoom);       // Show current zoom level
  
  
}

// ------------------------------------------------------------------------ //
//  Draw Dots
// ------------------------------------------------------------------------ //
static void graphics_layer_update(Layer *me, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite); // Line Color
  // cleanup graph
  for(uint8_t i=0; i<144; i++)
    graph[i] = z[(i+offset+(256-144))%256];
  
  bool tapping; tapping=false;
  int16_t gap, strength;
  for(int16_t i=0; i<143; i++) {
    int16_t diff = graph[i]>graph[i+1] ? graph[i]-graph[i+1] : graph[i+1]-graph[i];
    diff /= 8;
    if(tapping) {
      if(diff < 12) {  // subtle vibrations, but tap might still be going
        gap++;
        if(gap>5) {      // too many subtle vibrations, tap done
          
          if((i+1-Q1-gap)<30)
            for(int16_t j=Q1; j<(i+1-gap); j++)
              graphics_draw_pixel(ctx, GPoint(j,62-strength));
          
          Q1=i+1-Q1-gap; // Q1 now = point where tap ended - point were tap started = length of vibrations
          if(Q1<20)
            draw_textbox(ctx, i+1-Q1-gap, 26, Q1);
          
          gap=0;
          tapping=false;
        }
      } else { // still major vibrations, tap still going on
        gap=0;
      }
    } else {
      if(diff >= 20) {   // tap begins! sensitivity=20
        //draw_textbox(ctx, i, 6, diff);
        strength=diff/10;
        if(strength>60) strength=60;
        Q1=i;  // Q1 = point where tap started
        tapping=true;
        gap=0;
      }
    }

    if(tapping)
      //if((i+1-Q1-gap)<30)
        graphics_draw_line(ctx, GPoint(i, 0), GPoint(i,2)); // Very top
  }
  //draw_textbox(ctx, 0, 150, offset);
}

// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  graphics_layer = layer_create(layer_get_frame(window_layer));
  layer_set_update_proc(graphics_layer, graphics_layer_update);
  layer_add_child(window_layer, graphics_layer);

  graph_layer = layer_create(GRect(0,68,144,100));
  layer_set_update_proc(graph_layer, graph_layer_update);
  layer_add_child(window_layer, graph_layer);
  
  timer_callback(NULL);
}

static void main_window_unload(Window *window) {
  layer_destroy(graph_layer);
  layer_destroy(graphics_layer);
}

static void init(void) {
  // Set up and push main window
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(main_window, click_config_provider);
  window_set_background_color(main_window, GColorBlack);
  window_set_fullscreen(main_window, true);

  //Set up other stuff
  srand(time(NULL));  // Seed randomizer
  //accel_data_service_subscribe(5, accel_data_handler);  // We will be using the accelerometer: 1 sample_per_update
  accel_raw_data_service_subscribe(5, accel_data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);  // This seems to break it ... maybe it doesnt...
  
  
  //Begin
  window_stack_push(main_window, true /* Animated */); // Display window.  Callback will be called once layer is dirtied then written
}
  
static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
