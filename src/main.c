#include <pebble.h>
#include <string.h>
#define NUM_BIRD 23
#define DOUBLE_TAP_PAIR_WINDOW_MS 1500
#define ADVANCE_COOLDOWN_MS 2000

typedef struct {uint32_t resource_id; const char *common; const char *scientific;} Bird;
static const Bird BIRD[NUM_BIRD] = {
  {RESOURCE_ID_BIRD_01, "Dieffenbach\u2019s rail", "Hypotaenidia dieffenbachii"},
  {RESOURCE_ID_BIRD_02, "R\u00e9union solitaire", "Threskiornis solitarius"},
  {RESOURCE_ID_BIRD_03, "Red-headed green macaw", "Ara erythrocephala"},
  {RESOURCE_ID_BIRD_04, "Red rail", "Aphanapteryx bonasia"},
  {RESOURCE_ID_BIRD_05, "Spectacled cormorant", "Urile perspicillatus"},
  {RESOURCE_ID_BIRD_06, "Extinct island emu", "Dromaius sp."},
  {RESOURCE_ID_BIRD_07, "Lord Howe swamphen", "Porphyrio albus"},
  {RESOURCE_ID_BIRD_08, "R\u00e9union swamphen", "Porphyrio caerulescens"},
  {RESOURCE_ID_BIRD_09, "Tahiti rail", "Hypotaenidia pacifica"},
  {RESOURCE_ID_BIRD_10, "Dodo", "Raphus cucullatus"},
  {RESOURCE_ID_BIRD_11, "Norfolk pigeon", "Hemiphaga spadicea"},
  {RESOURCE_ID_BIRD_12, "Mauritius blue pigeon", "Alectroenas nitidissimus"},
  {RESOURCE_ID_BIRD_13, "Seychelles parakeet", "Psittacula wardi"},
  {RESOURCE_ID_BIRD_14, "Martinique amazon", "Amazona martinicana"},
  {RESOURCE_ID_BIRD_15, "Guadeloupe parakeet", "Psittacara labati"},
  {RESOURCE_ID_BIRD_16, "Jamaican red macaw", "Ara gossei"},
  {RESOURCE_ID_BIRD_17, "Cuban macaw", "Ara tricolor"},
  {RESOURCE_ID_BIRD_18, "R\u00e9union parrot", "Necropsittacus borbonicus"},
  {RESOURCE_ID_BIRD_19, "Broad-billed parrot", "Lophopsittacus mauritianus"},
  {RESOURCE_ID_BIRD_20, "Norfolk kaka", "Nestor productus"},
  {RESOURCE_ID_BIRD_21, "Jamaican poorwill", "Siphonorhis americana"},
  {RESOURCE_ID_BIRD_22, "O\u02bbahu \u02bb\u014d\u02bb\u014d", "Moho apicalis"},
  {RESOURCE_ID_BIRD_23, "R\u00e9union starling", "Fregilupus varius"}
};
static Window *s_window;
static BitmapLayer *s_bird_layer;
static TextLayer *s_time_layer,*s_date_layer,*s_common_layer,*s_scientific_layer;
static GBitmap *s_bird_bitmap;
static int32_t s_day=-1;
static int s_bird_index=-1;
static char s_time_buffer[16],s_date_buffer[24];
static uint8_t s_tap_count=0;
static AppTimer *s_tap_timeout=NULL;
static AppTimer *s_cooldown_timer=NULL;
static bool s_cooldown=false;

static int32_t absolute_day(const struct tm *t) {
  int32_t year=t->tm_year+1900,y=year-1;
  return 365*y+y/4-y/100+y/400+t->tm_yday;
}
// A seeded full-cycle permutation: no repeat until all 44 have appeared.
static int daily_bird(int32_t day) {return (int)(((uint32_t)day*7u+31u)%NUM_BIRD);}
static void display_bird(int index) {
  if (index<0 || index>=NUM_BIRD) return;
  GBitmap *replacement=gbitmap_create_with_resource(BIRD[index].resource_id);
  if (!replacement) return;
  bitmap_layer_set_bitmap(s_bird_layer,NULL);
  if(s_bird_bitmap)gbitmap_destroy(s_bird_bitmap);
  s_bird_bitmap=replacement;
  s_bird_index=index;
  bitmap_layer_set_bitmap(s_bird_layer,s_bird_bitmap);
  text_layer_set_text(s_common_layer,BIRD[index].common);
  text_layer_set_text(s_scientific_layer,BIRD[index].scientific);
}
static void update_display(struct tm *tick) {
  int32_t day=absolute_day(tick);
  if(day!=s_day) {s_day=day;display_bird(daily_bird(day));}
  strftime(s_time_buffer,sizeof(s_time_buffer),clock_is_24h_style()?"%H:%M":"%I:%M",tick);
  if(!clock_is_24h_style() && s_time_buffer[0]=='0')memmove(s_time_buffer,s_time_buffer+1,strlen(s_time_buffer));
  strftime(s_date_buffer,sizeof(s_date_buffer),"%a, %b %e",tick);
  text_layer_set_text(s_time_layer,s_time_buffer);
  text_layer_set_text(s_date_layer,s_date_buffer);
}
static void tick_handler(struct tm *tick,TimeUnits changed){update_display(tick);}
static void reset_taps(void *context) { s_tap_count=0; s_tap_timeout=NULL; }
static void cooldown_finished(void *context) {
  s_cooldown=false;
  s_cooldown_timer=NULL;
}
// Pebble emits an event for its recognized double-tap gesture.
// Require two such events, then advance silently and lock out repeats.
static void accel_tap_handler(AccelAxisType axis,int32_t direction) {
  if(s_cooldown) return;
  if(s_tap_timeout) {app_timer_cancel(s_tap_timeout);s_tap_timeout=NULL;}
  s_tap_count++;
  if(s_tap_count>=2) {
    s_tap_count=0;
    display_bird((s_bird_index+1)%NUM_BIRD);
    s_cooldown=true;
    s_cooldown_timer=app_timer_register(ADVANCE_COOLDOWN_MS,cooldown_finished,NULL);
  } else {
    s_tap_timeout=app_timer_register(DOUBLE_TAP_PAIR_WINDOW_MS,reset_taps,NULL);
  }
}
static TextLayer *make_label(GRect frame,const char *font_key,GTextAlignment align) {
  TextLayer *layer=text_layer_create(frame);
  text_layer_set_background_color(layer,GColorClear);
  text_layer_set_text_color(layer,GColorBlack);
  text_layer_set_font(layer,fonts_get_system_font(font_key));
  text_layer_set_text_alignment(layer,align);
  layer_add_child(window_get_root_layer(s_window),text_layer_get_layer(layer));
  return layer;
}
static void init(void) {
  s_window=window_create();
  window_set_background_color(s_window,GColorWhite);
  window_stack_push(s_window,true);
  Layer *root=window_get_root_layer(s_window);GRect bounds=layer_get_bounds(root);int w=bounds.size.w;
  s_bird_layer=bitmap_layer_create(GRect(6,15,w-12,106));
  bitmap_layer_set_alignment(s_bird_layer,GAlignCenter);
  bitmap_layer_set_background_color(s_bird_layer,GColorWhite);
  bitmap_layer_set_compositing_mode(s_bird_layer,GCompOpAssign);
  layer_add_child(root,bitmap_layer_get_layer(s_bird_layer));
  s_common_layer=make_label(GRect(4,125,w-8,22),FONT_KEY_GOTHIC_18_BOLD,GTextAlignmentCenter);
  s_scientific_layer=make_label(GRect(4,147,w-8,22),FONT_KEY_GOTHIC_14,GTextAlignmentCenter);
  s_time_layer=make_label(GRect(2,171,w-4,45),FONT_KEY_BITHAM_42_BOLD,GTextAlignmentCenter);
  s_date_layer=make_label(GRect(4,210,w-8,18),FONT_KEY_GOTHIC_14,GTextAlignmentCenter);
  time_t now=time(NULL);update_display(localtime(&now));
  tick_timer_service_subscribe(MINUTE_UNIT,tick_handler);
  accel_tap_service_subscribe(accel_tap_handler);
}
static void deinit(void) {
  accel_tap_service_unsubscribe();tick_timer_service_unsubscribe();
  if(s_tap_timeout)app_timer_cancel(s_tap_timeout);
  if(s_cooldown_timer)app_timer_cancel(s_cooldown_timer);
  if(s_bird_bitmap)gbitmap_destroy(s_bird_bitmap);
  bitmap_layer_destroy(s_bird_layer);
  text_layer_destroy(s_time_layer);text_layer_destroy(s_date_layer);
  text_layer_destroy(s_common_layer);text_layer_destroy(s_scientific_layer);
  window_destroy(s_window);
}
int main(void) {init();app_event_loop();deinit();}
