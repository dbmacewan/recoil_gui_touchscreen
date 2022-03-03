#include <lvgl.h>
#include <ILI9341_t3.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include "recoil.h"
#include "USBHost_t36.h"


// Calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define MOUSE_SHOOT 0x03 // left and right click mouse press
#define CS_PIN 8
#define TFT_DC 9
#define TFT_CS 10

#define TFT_RST 255  // 255 = unused, connect to 3.3V
#define TFT_MOSI 11
#define TFT_SCLK 13
#define TFT_MISO 12

#define TIRQ_PIN  2


// ********************** Global Variables **************************
//XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts
//XPT2046_Touchscreen ts(CS_PIN, 255);  // Param 2 - 255 - No interrupts
XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);  // Param 2 - Touch IRQ Pin - interrupt enabled polling

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO); // initialize the display

// timer object used to report time slices to GUI
IntervalTimer guiTimer;

// ILI9341 screen resolution
static const uint32_t screenWidth  = 320;
static const uint32_t screenHeight = 240;

// screen display buffer for LVGL
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * 10 ];

// initialize and register USB
USBHost usb_host;
USBHub hub(usb_host);
USBHIDParser hid(usb_host);
MouseController mouse(usb_host);

Recoil recoil; // initialize the recoil object

elapsedMicros sinceInject; // system timer in us
int8_t x = 0; // injected mouse x position
int8_t y = 0; // injected mouse y position
double RECOIL_INTERVAL; // update rate in us

lv_obj_t * screen1 = NULL; // main menu screen
lv_obj_t * screen2 = NULL; // settings menu screen


// ************************ FUNCTION PROTOTYPES ************************
static void btn_reset_cb(lv_event_t * e); // reset button
void lv_btn_reset(void); // create the reset button (looks like trash can)
static void msgbox_sens_cb(lv_event_t * e); // popup dialog window
void lv_msgbox_sens(const char * msgString); // create a popup dialog window
static void textarea_event_handler(lv_event_t * e); // process data entered from numberpad
static void btnm_event_handler(lv_event_t * e); // numberpad
void lv_kb_number(void); // create numberpad
static void btn_home_cb(lv_event_t * e); // returns to main menu screen
void lv_btn_home(void); // create home button (floppy disc icon)
static void btn_fov_cb(lv_event_t * e); // creates FOV menu
void lv_btn_fov(void);// create fov button (display icon)
static void btn_sens_cb(lv_event_t * e); // creates sens menu
void lv_btn_sens(void); // create sens button (mouse icon)
lv_style_t * lv_button_style_checked(void); // button style checked RED
static lv_color_t darken(const lv_color_filter_dsc_t * dsc, lv_color_t color, lv_opa_t opa); // color filter for pressed button
lv_style_t * lv_button_style_pressed(void); // button style pressed. darkens existing BG color
lv_style_t * lv_button_style(void); // create a button style
lv_style_t* lv_desktop_style(void); // desktop wallpaper style
static void toggle_silencer_refresh_cb(lv_event_t * e); // refresh silencer settings and update labels
static void toggle_silencer_cb(lv_event_t * e); // adjust silencer settings and update labels
void lv_toggle_silencer(void); // create silencer toggle button
static void dropdown_sight_init_style_cb(lv_event_t * e); // apply style to the dropdown menus
static void dropdown_sight_refresh_cb(lv_event_t * e); // refresh the current selected text in sight dropdown
static void dropdown_sight_cb(lv_event_t * e); // adjust sight settings and update labels
void lv_dropdown_sight(void); // create dropdown menu for sight
static void arc_fov_cb(lv_event_t * e); // adjust fov settings
void lv_arc_fov(void); // create arc slider for FOV
void lv_label_current_equipment(uint8_t arg); // create text labels to display current equipment
static void dropdown_weapon_init_style_cb(lv_event_t * e); // initialize the weapon dropdown menu style
static void dropdown_weapon_refresh_cb(lv_event_t * e); // update current highlighted selection in weapon dropdown
static void dropdown_weapon_cb(lv_event_t * e); // adjust weapon settings
void lv_dropdown_weapon(void); // Create a dropdown list
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p ); // Display flushing
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data ); // Read the touchscreen
void guiInc(); // update the gui timer
void iniMainMenu(void); // initialize the main menu screen
void iniFOVMenu(void); // initialize the fov settings menu screen
void iniSensMenu(void); // initialize the sensitivity menu screen
void injectMouse(); // sends injected mouse movement
void updateMouse(); // sends normal mouse movement
void handleRecoil(); // injects mouse movement
#if LV_USE_LOG != 0
void report_inject_to_serial(); // for debugging
#endif


void setup() {
#if LV_USE_LOG != 0
   Serial.begin( 115200 ); // serial debug
#endif

   recoil.setSensitivity(.3);
   recoil.setFOV(75);
   recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_AK);

   usb_host.begin(); // start usb host and device tasks

   tft.begin(); // start display
   tft.setRotation(3);
   ts.begin(); // start touchscreen
   ts.setRotation(1);

   lv_init(); // initialize LVGL GUI

#if LV_USE_LOG != 0
   lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

   LV_LOG_USER("LVGL Version 8.0.X");

   lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * 10 ); // initialize the GUI screen buffer

   // Initialize the display
   static lv_disp_drv_t disp_drv;
   lv_disp_drv_init( &disp_drv );
   // Set the display resolution
   disp_drv.hor_res = screenWidth;
   disp_drv.ver_res = screenHeight;
   disp_drv.flush_cb = my_disp_flush;
   disp_drv.draw_buf = &draw_buf;
   lv_disp_drv_register( &disp_drv );

   // Initialize the (dummy) input device driver
   static lv_indev_drv_t indev_drv;
   lv_indev_drv_init( &indev_drv );
   indev_drv.type = LV_INDEV_TYPE_POINTER;
   indev_drv.read_cb = my_touchpad_read;
   lv_indev_drv_register( &indev_drv );

   guiTimer.begin(guiInc, 5000);
   LV_LOG_USER("Setup complete");
   iniMainMenu(); // start main menu GUI screen1
}

void loop() {
   usb_host.Task(); // update host/device

   if (mouse.available()){
      updateMouse(); // passthrough regular mouse movement
      if (mouse.getButtons() == MOUSE_SHOOT){
         recoil.setActive();
         RECOIL_INTERVAL = recoil.getUpdateRate() * 1000; // returned in ms, converting to us
      }
      else {
         recoil.setInactive();
      }
      mouse.mouseDataClear(); // clear old data
  }

  if (recoil.isActive() && sinceInject >= RECOIL_INTERVAL){
     handleRecoil(); // injects mouse movement, resets sinceInject timer
   }
   
   lv_timer_handler(); // must be called at minimum every 5 ms to keep the GUI responsive
}


// reset button
static void btn_reset_cb(lv_event_t * e){
   recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_NONE);
   recoil.setEquipment(RECOIL_EQUIPMENT::BARREL_NONE);
   recoil.setEquipment(RECOIL_EQUIPMENT::SIGHT_NONE);
   uint32_t numChildren = screen1->spec_attr->child_cnt;
   lv_obj_t * toggle_btn;
   for (int i = 0; i < numChildren; i++){
      toggle_btn = lv_obj_get_child(lv_scr_act(), i);
      lv_event_send(toggle_btn, LV_EVENT_REFRESH, NULL);
   }
   LV_LOG_USER("Returning to caller");
}

// create the reset button (looks like trash can)
void lv_btn_reset(void){
   LV_IMG_DECLARE(reset2_img);
   lv_obj_t * reset_btn = lv_btn_create(lv_scr_act());
   lv_obj_t * reset_img = lv_img_create(reset_btn);
   // Remove the styles coming from the theme
   // Note that size and position are also stored as style properties
   // so lv_obj_remove_style_all will remove the set size and position too
   lv_obj_remove_style_all(reset_btn);
   lv_obj_set_size(reset_btn, 44, 44);
   
   lv_img_set_src(reset_img, &reset2_img);
   
   lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
   lv_obj_center(reset_img);
   lv_obj_set_style_bg_img_src(reset_btn, reset_img, 0);
   lv_obj_add_style(reset_btn, lv_button_style(), 0);
   lv_obj_add_style(reset_btn, lv_button_style_pressed(), LV_STATE_PRESSED);
   lv_obj_add_event_cb(reset_btn, btn_reset_cb, LV_EVENT_CLICKED, NULL);
}

// popup dialog window
static void msgbox_sens_cb(lv_event_t * e){
   lv_obj_t * obj = lv_event_get_current_target(e);

   LV_LOG_USER(lv_msgbox_get_active_btn_text(obj));
   if (strcmp(lv_msgbox_get_active_btn_text(obj), "Close") == 0){
      // destroy window
      lv_obj_del(lv_obj_get_parent(obj));
   }
}

// create a popup dialog window
void lv_msgbox_sens(const char * msgString){
   static const char * btns[] = {"Close", ""};
   lv_obj_t * mbox1 = lv_msgbox_create(NULL, "", msgString, btns, false);
   lv_obj_add_event_cb(mbox1, msgbox_sens_cb, LV_EVENT_VALUE_CHANGED, NULL);
   lv_obj_center(mbox1);
}

// process data entered from numberpad
static void textarea_event_handler(lv_event_t * e){
   lv_obj_t * ta = lv_event_get_target(e);
   LV_LOG_USER("Enter was pressed. The current text is: ");
   LV_LOG_USER(lv_textarea_get_text(ta));

   char * inputBuffer = lv_textarea_get_text(ta);
   inputBuffer[strlen(inputBuffer) + 1] = '\0';
   float newSens = 0;
   int8_t intDigits = 0;
   int8_t fracDigits = 0;
   int8_t decimal = 0;
   int8_t i, j;

   // find the number of whole digits in the string
   while (inputBuffer[intDigits] != '.' && inputBuffer[intDigits]){
      intDigits++;
   }
   if (inputBuffer[intDigits] == '.'){
      decimal = 1;
      // find number of fractional digits in the string
      while (inputBuffer[(intDigits + 1) + fracDigits]){
         fracDigits++;
      }
      // parse whole digits
      j = intDigits - 1;
      for (i = 0; i < intDigits; i++, j--){
         newSens += static_cast<float>((inputBuffer[i] - 48) * pow(10, j));
      }
      // parse fractional digits
      j = -fracDigits;
      for (i = intDigits + fracDigits; j < 0; i--, j++){
         Serial.println(inputBuffer[i]);
         newSens += static_cast<float>((inputBuffer[i] - 48) * pow(10, j));
      }
   }
   else {
      // parse whole digits
      j = intDigits - 1;
      for (i = 0; i < intDigits; i++, j--){
         newSens += static_cast<float>((inputBuffer[i] - 48) * pow(10, j));
      }
   }

   recoil.setSensitivity(newSens);
   std::string sensMsg = "Sensitivity updated: ";
   sensMsg += recoil.getSensitivity();
   lv_textarea_set_text(ta, "");
   lv_textarea_set_placeholder_text(ta, sensMsg.c_str());
   LV_LOG_USER(sensMsg.c_str());
   lv_msgbox_sens(sensMsg.c_str()); // show pop up for user feedback
}

// numberpad
static void btnm_event_handler(lv_event_t * e){
   lv_obj_t * obj = lv_event_get_target(e);
   lv_obj_t * ta = lv_event_get_user_data(e);
   const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
   const char * txtString = lv_textarea_get_text(ta);

   if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0){
     lv_textarea_del_char(ta);
   }
   else if(strcmp(txt, LV_SYMBOL_NEW_LINE) == 0){
     lv_event_send(ta, LV_EVENT_READY, NULL);
   }
   else if (strcmp(txt, ".") == 0){ // prevent user adding more than 1 decimal
      for (int i = 0; txtString[i]; i++){
         if (txtString[i] == '.'){
            return; // another decimal already exists, do not add it to the string
         }
      }
      lv_textarea_add_text(ta, txt);
   }
   else {
     lv_textarea_add_text(ta, txt);
   }

}

// create numberpad
void lv_kb_number(void){
   lv_obj_t * ta = lv_textarea_create(lv_scr_act());
   lv_textarea_set_one_line(ta, true);
   lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 0);
   lv_textarea_set_max_length(ta, 10);
   lv_obj_set_size(ta, 200, 40);
   std::string sensMsg = "Current Sensitivity: ";
   sensMsg += recoil.getSensitivity();
   lv_textarea_set_placeholder_text(ta, sensMsg.c_str());
   lv_obj_add_event_cb(ta, textarea_event_handler, LV_EVENT_READY, ta);
   lv_obj_add_state(ta, LV_STATE_FOCUSED); // To be sure the cursor is visible

   static const char * btnm_map[] = {"1", "2", "3", "\n",
                              "4", "5", "6", "\n",
                              "7", "8", "9", "\n",
                              "0", ".", LV_SYMBOL_BACKSPACE, "\n",
                              LV_SYMBOL_NEW_LINE, ""};

   lv_obj_t * btnm = lv_btnmatrix_create(lv_scr_act());
   lv_obj_set_size(btnm, 200, 190);
   lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, 0);
   lv_obj_add_event_cb(btnm, btnm_event_handler, LV_EVENT_VALUE_CHANGED, ta);
   lv_obj_clear_flag(btnm, LV_OBJ_FLAG_CLICK_FOCUSABLE); // Keep the text area focused on button clicks
   lv_btnmatrix_set_map(btnm, btnm_map);
}

// returns to main menu screen
static void btn_home_cb(lv_event_t * e){
   iniMainMenu();
   lv_obj_del(screen2);
   screen2 = NULL;
   LV_LOG_USER("returning to caller");
}

// create home button (floppy disc icon)
void lv_btn_home(void){
   LV_IMG_DECLARE(floppy1_img);
   lv_obj_t * home_btn = lv_btn_create(lv_scr_act());
   lv_obj_t * home_img = lv_img_create(home_btn);
   // Remove the styles coming from the theme
   lv_obj_remove_style_all(home_btn);
   lv_obj_set_size(home_btn, 44, 44);
   
   lv_img_set_src(home_img, &floppy1_img);
   
   lv_obj_align(home_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
   lv_obj_center(home_img);
   lv_obj_set_style_bg_img_src(home_btn, home_img, 0);
   lv_obj_add_style(home_btn, lv_button_style(), 0);
   lv_obj_add_style(home_btn, lv_button_style_pressed(), LV_STATE_PRESSED);
   lv_obj_add_event_cb(home_btn, btn_home_cb, LV_EVENT_CLICKED, NULL);
}

// creates FOV menu
static void btn_fov_cb(lv_event_t * e){
   iniFOVMenu();
   lv_obj_del(screen1);
   screen1 = NULL;
   LV_LOG_USER("Returning to caller");
}

// create fov button (display icon)
void lv_btn_fov(void){
   LV_IMG_DECLARE(settings5_img);
   lv_obj_t * fov_btn = lv_btn_create(lv_scr_act());
   lv_obj_t * fov_img = lv_img_create(fov_btn);
   // Remove the styles coming from the theme
   lv_obj_remove_style_all(fov_btn);
   lv_obj_set_size(fov_btn, 44, 44);
   
   lv_img_set_src(fov_img, &settings5_img);
   
   lv_obj_align(fov_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
   lv_obj_center(fov_img);
   lv_obj_set_style_bg_img_src(fov_btn, fov_img, 0);
   lv_obj_add_style(fov_btn, lv_button_style(), 0);
   lv_obj_add_style(fov_btn, lv_button_style_pressed(), LV_STATE_PRESSED);
   lv_obj_add_event_cb(fov_btn, btn_fov_cb, LV_EVENT_CLICKED, NULL);
}

// creates sens menu
static void btn_sens_cb(lv_event_t * e){
   iniSensMenu();
   lv_obj_del(screen1);
   screen1 = NULL;
   LV_LOG_USER("Returning to caller");
}

// create sens button (mouse icon)
void lv_btn_sens(void){
   LV_IMG_DECLARE(mouse1_img);
   lv_obj_t * sens_btn = lv_btn_create(lv_scr_act());
   lv_obj_t * sens_img = lv_img_create(sens_btn);
   // Remove the styles coming from the theme
   lv_obj_remove_style_all(sens_btn);
   lv_obj_set_size(sens_btn, 44, 44);
   
   lv_img_set_src(sens_img, &mouse1_img);
   
   lv_obj_align(sens_btn, LV_ALIGN_BOTTOM_RIGHT, -74, -10);
   lv_obj_center(sens_img);
   lv_obj_set_style_bg_img_src(sens_btn, sens_img, 0);
   lv_obj_add_style(sens_btn, lv_button_style(), 0);
   lv_obj_add_style(sens_btn, lv_button_style_pressed(), LV_STATE_PRESSED);
   lv_obj_add_event_cb(sens_btn, btn_sens_cb, LV_EVENT_CLICKED, NULL);
}

// button style checked RED
lv_style_t * lv_button_style_checked(void){
   static bool button_style_checked_ini = false;
   static lv_style_t style_btn_checked;
   lv_color_t c = lv_palette_main(LV_PALETTE_RED);
   lv_color16_t my_color; // 16bit color struct R 5 bits, G 6 bits, B 5 bits
   my_color.full = lv_color_to16(c); // converts the 24 bit color to the 16bit color
   
   if (!button_style_checked_ini){
      lv_style_init(&style_btn_checked);
      lv_style_set_bg_color(&style_btn_checked, my_color);
      lv_style_set_radius(&style_btn_checked, 10);
      lv_style_set_bg_opa(&style_btn_checked, LV_OPA_COVER);
      return &style_btn_checked;
   }
   else {
      return &style_btn_checked;
   }
}

// color filter for pressed button
static lv_color_t darken(const lv_color_filter_dsc_t * dsc, lv_color_t color, lv_opa_t opa){
   LV_UNUSED(dsc);
   return lv_color_darken(color, opa);
}

// button style pressed. darkens existing BG color
lv_style_t * lv_button_style_pressed(void){
   static bool button_style_pressed_ini = false;
   static lv_style_t style_btn_pressed;
   static lv_color_filter_dsc_t color_filter;
   if (!button_style_pressed_ini){
      // Create a style for the pressed state.
      // Use a color filter to simply modify all colors in this state
      lv_color_filter_dsc_init(&color_filter, darken);
      lv_style_init(&style_btn_pressed);
      lv_style_set_color_filter_dsc(&style_btn_pressed, &color_filter);
      lv_style_set_color_filter_opa(&style_btn_pressed, LV_OPA_20);
      button_style_pressed_ini = true;
      return &style_btn_pressed;
   }
   else {
      return &style_btn_pressed;
   }
   
}

// create a button style
lv_style_t * lv_button_style(void){
   static lv_style_t button_style;
   static bool button_style_ini = false;
   // Create a custom color - neon yellow
   // *****************
   lv_color_t c; // color struct 
   c.ch.red   = 0xDE;
   c.ch.green = 0xFE;
   c.ch.blue  = 0x47;
   lv_color16_t my_color; // 16bit color struct R 5 bits, G 6 bits, B 5 bits
   my_color.full = lv_color_to16(c); // converts the 24 bit color to the 16bit color
   // *****************

   // Create a custom color - neon blue
   // *****************
   lv_color_t c2; // color struct 
   c2.ch.red   = 0x00;
   c2.ch.green = 0x16;
   c2.ch.blue  = 0xEE;
   lv_color16_t my_color2; // 16bit color struct R 5 bits, G 6 bits, B 5 bits
   my_color2.full = lv_color_to16(c2); // converts the 24 bit color to the 16bit color
   // *****************

   // Create a custom color - neon pink
   // *****************
   lv_color_t c3; // color struct 
   c3.ch.red   = 0xFE;
   c3.ch.green = 0x00;
   c3.ch.blue  = 0xFE;
   lv_color16_t my_color3; // 16bit color struct R 5 bits, G 6 bits, B 5 bits
   my_color3.full = lv_color_to16(c3); // converts the 24 bit color to the 16bit color
   // *****************

   if (!button_style_ini){
      lv_style_init(&button_style);
      lv_style_set_bg_color(&button_style, my_color);
      lv_style_set_radius(&button_style, 10);
      lv_style_set_bg_opa(&button_style, LV_OPA_COVER);

      lv_style_set_border_color(&button_style, my_color2);
      lv_style_set_border_opa(&button_style, LV_OPA_COVER);
      lv_style_set_border_width(&button_style, 2);

      button_style_ini = true;
      return &button_style;
   }
   else {
      return &button_style;
   }
}

// desktop wallpaper style
lv_style_t* lv_desktop_style(void){
   // Create a custom color - light pink - top color
   // *****************
   lv_color_t c; // color struct 
   c.ch.red   = 0xFD;
   c.ch.green = 0x80;
   c.ch.blue  = 0x90;
   lv_color16_t my_color; // 16bit color struct R 5 bits, G 6 bits, B 5 bits
   my_color.full = lv_color_to16(c); // converts the 24 bit color to the 16bit color
   // *****************

   // Create a custom color - light blue - bottom color
   // *****************
   lv_color_t c2; // color struct 
   c2.ch.red   = 0xC4;
   c2.ch.green = 0xFF;
   c2.ch.blue  = 0xFF;
   lv_color16_t my_color2; // 16bit color struct R 5 bits, G 6 bits, B 5 bits
   my_color2.full = lv_color_to16(c2); // converts the 24 bit color to the 16bit color
   // *****************

   // Set the background of the display. The background is not visible if the screen is 100% opaque.
   // In other words, this does nothing unless part of the current screen is < 100% opacity.
   //***************
   //lv_disp_set_bg_color(NULL, my_color); // sets the background of the display
   //lv_disp_set_bg_opa(NULL, LV_OPA_COVER); // sets the opacity of the display background
   //The disp parameter of these functions can be NULL to refer it to the default display.
   //***************

   // Create a style and apply this to the current screen.
   // This changes the background color.
   //***************
   //static lv_style_t desktop_style;
   //lv_style_init(&desktop_style);
   //lv_style_set_bg_color(&desktop_style, my_color2);
   //lv_obj_add_style(lv_scr_act(), &desktop_style, LV_PART_MAIN);
   //***************

   // make an image converted in a .c file to the wallpaper
   // **************************
   //static lv_style_t desktop_style;
   //lv_style_init(&desktop_style);
   //lv_obj_t * wallpaper = lv_img_create(lv_scr_act());
   //lv_img_set_src(wallpaper, &bg_img); /*From variable*/
   //lv_style_set_bg_img_src(&desktop_style, &bg_img);
   //lv_obj_add_style(lv_scr_act(), &desktop_style, LV_PART_MAIN);
   // ***************************

   // ****************************
   // create a gradient style and return the style object
   static bool desktop_gradient_ini = false;
   static lv_style_t desktop_style;
   // if the style hasn't been initialized
   if (!desktop_gradient_ini){
      lv_style_init(&desktop_style);

      lv_style_set_bg_color(&desktop_style, my_color);
      lv_style_set_bg_opa(&desktop_style, LV_OPA_COVER);
      lv_style_set_bg_grad_dir(&desktop_style, LV_GRAD_DIR_VER);
      lv_style_set_bg_grad_color(&desktop_style, my_color2);
      lv_style_set_bg_grad_stop(&desktop_style, 255); // 0 is the top or left and 255 is the bottom or right
      lv_style_set_blend_mode(&desktop_style, LV_BLEND_MODE_NORMAL); // ADDITIVE / SUBTRACTIVE / NORMAL
      
      desktop_gradient_ini = true;

      return &desktop_style;
   }
   else {
      return &desktop_style;
   }
}

// refresh silencer settings and update labels
static void toggle_silencer_refresh_cb(lv_event_t * e){
   lv_obj_t * silencer = lv_event_get_target(e);
   lv_obj_t * silencer_label = lv_obj_get_child(silencer, 0);
   if (strcmp(recoil.getBarrel().c_str(), "none") == 0){
      lv_obj_clear_state(silencer, LV_STATE_CHECKED);
      lv_label_set_text(silencer_label, "Silencer Off");
      lv_label_current_equipment(0); // use 0 to update the label only
   }
   LV_LOG_USER("Returning to caller");
}

// adjust silencer settings and update labels
static void toggle_silencer_cb(lv_event_t * e){
   lv_obj_t * silencer = lv_event_get_target(e);
   lv_obj_t * silencer_label = lv_obj_get_child(silencer, 0);

   if (lv_obj_get_state(silencer) == (LV_STATE_CHECKED | LV_STATE_FOCUSED)){
      recoil.setEquipment(RECOIL_EQUIPMENT::BARREL_SUPPR);
      lv_label_set_text(silencer_label, "Silencer On");
   }
   else {
      recoil.setEquipment(RECOIL_EQUIPMENT::BARREL_NONE);
      lv_label_set_text(silencer_label, "Silencer Off");
   }
   lv_label_current_equipment(0); // use 0 to update the label only
   LV_LOG_USER("Returning to caller");
}

// create silencer toggle button
void lv_toggle_silencer(void){
   lv_obj_t * silencer_toggle = lv_btn_create(lv_scr_act());
   lv_obj_remove_style_all(silencer_toggle);
   lv_obj_align(silencer_toggle, LV_ALIGN_RIGHT_MID, -10, -20);
   lv_obj_add_flag(silencer_toggle, LV_OBJ_FLAG_CHECKABLE);
   lv_obj_set_size(silencer_toggle, 122, 44);
   lv_obj_add_style(silencer_toggle, lv_button_style(), LV_PART_MAIN);
   lv_obj_add_style(silencer_toggle, lv_button_style_checked(), LV_STATE_CHECKED);
   lv_obj_add_event_cb(silencer_toggle, toggle_silencer_cb, LV_EVENT_VALUE_CHANGED, NULL);
   lv_obj_add_event_cb(silencer_toggle, toggle_silencer_refresh_cb, LV_EVENT_REFRESH, NULL);

   lv_obj_t * label;
   label = lv_label_create(silencer_toggle);
   if (strcmp(recoil.getBarrel().c_str(), "Suppressor") == 0){
      lv_label_set_text(label, "Silencer On");
      lv_obj_add_state(silencer_toggle, LV_STATE_CHECKED);
   }
   else {
      lv_label_set_text(label, "Silencer Off");
   }
   lv_obj_center(label);
}

// apply style to the dropdown menus
static void dropdown_sight_init_style_cb(lv_event_t * e){
   lv_obj_t * dropdown = lv_event_get_target(e);
   lv_obj_t * list = lv_dropdown_get_list(dropdown); // get the list object within the dropdown menu
   if (!list){
      LV_LOG_USER("list is null");
   }
   else {
      lv_obj_add_style(list, lv_button_style(), LV_PART_MAIN);
      lv_obj_add_style(list, lv_button_style_checked(), LV_PART_SELECTED | LV_STATE_CHECKED);
      LV_LOG_USER("list style executed");
   }
}

// refresh the current selected text in sight dropdown
static void dropdown_sight_refresh_cb(lv_event_t * e){
   lv_obj_t * dropdown = lv_event_get_target(e);
   if (strcmp(recoil.getSight().c_str(), "none") == 0){
      lv_dropdown_set_selected(dropdown, 0); // select the none option
      lv_label_current_equipment(0); // use 0 to update the label only
   }
}

// adjust sight settings and update labels
static void dropdown_sight_cb(lv_event_t * e){
   lv_obj_t * sight = lv_event_get_target(e);
   char buf[64];
   lv_dropdown_get_selected_str(sight, buf, sizeof(buf));
   LV_LOG_USER(buf);
    
   if (strcmp(buf, "None") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::SIGHT_NONE);
   }
   else if (strcmp(buf, "Holo") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::SIGHT_HOLO);
   }
   else if (strcmp(buf, "8x") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::SIGHT_8X);
   }
   else if (strcmp(buf, "Simple") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::SIGHT_SIMPLE);
   }
   else {
      LV_LOG_USER("Error - no matching string in dropdown_sight_cb: ");
   }

   lv_label_current_equipment(0); // use 0 to update the label only
}

// create dropdown menu for sight
void lv_dropdown_sight(void){
   lv_obj_t * sight_dropdown = lv_dropdown_create(lv_scr_act());
   lv_obj_align(sight_dropdown, LV_ALIGN_TOP_RIGHT, -10, 10);
   lv_dropdown_set_options(sight_dropdown, "None\n"
                                      "Holo\n"
                                      "8x\n"
                                      "Simple");

   // Set a fixed text to display on the button of the drop-down list
   lv_dropdown_set_text(sight_dropdown, "Sight");

   if (strcmp(recoil.getSight().c_str(), "none") == 0){
      lv_dropdown_set_selected(sight_dropdown, 0);
   }
   else if (strcmp(recoil.getSight().c_str(), "Holo") == 0){
      lv_dropdown_set_selected(sight_dropdown, 1);
   }
   else if (strcmp(recoil.getSight().c_str(), "8-Times") == 0){
      lv_dropdown_set_selected(sight_dropdown, 2);
   }
   else if (strcmp(recoil.getSight().c_str(), "Simple") == 0){
      lv_dropdown_set_selected(sight_dropdown, 3);
   }

   lv_obj_add_style(sight_dropdown, lv_button_style(), LV_PART_MAIN);
   lv_obj_add_event_cb(sight_dropdown, dropdown_sight_cb, LV_EVENT_VALUE_CHANGED, NULL);
   lv_obj_add_event_cb(sight_dropdown, dropdown_sight_refresh_cb, LV_EVENT_REFRESH, NULL);
   lv_obj_add_event_cb(sight_dropdown, dropdown_sight_init_style_cb, LV_EVENT_RELEASED, NULL);
}

// adjust fov settings
static void arc_fov_cb(lv_event_t * e){
   lv_obj_t * arc = lv_event_get_target(e);
   lv_obj_t * arc_label = lv_obj_get_child(arc, 0);

   uint16_t temp = lv_arc_get_value(arc);
   recoil.setFOV(static_cast<float>(temp));

   std::string newVal = recoil.getFOV();
   lv_label_set_text_fmt(arc_label, "Field Of View\n\n          %s", newVal.c_str());
   LV_LOG_USER(newVal.c_str());
   LV_LOG_USER("Returning to caller");
}

// create arc slider for FOV
void lv_arc_fov(void){
   lv_obj_t * fov_arc = lv_arc_create(lv_scr_act());
   lv_obj_set_size(fov_arc, 150, 150);
   lv_arc_set_rotation(fov_arc, 135);
   lv_arc_set_bg_angles(fov_arc, 0, 270);
   lv_arc_set_range(fov_arc, 75, 120);

   std::string fov = recoil.getFOV();
   int j = fov.size() - 1;
   uint16_t result = 0;
   for (int i = 0; fov[i]; i++, j--){
      result += static_cast<uint16_t>(fov[i] - 48) * pow(10, j);
   }

   lv_arc_set_value(fov_arc, result);
   lv_obj_center(fov_arc);

   lv_obj_t * fov_arc_label = lv_label_create(fov_arc);
   std::string label_val = recoil.getFOV();
   lv_label_set_text_fmt(fov_arc_label, "Field Of View\n\n          %s", label_val.c_str());
   lv_obj_center(fov_arc_label);

   lv_obj_add_event_cb(fov_arc, arc_fov_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// create text labels to display current equipment
void lv_label_current_equipment(uint8_t arg){
   static lv_obj_t * weapon_label;
   static lv_obj_t * barrel_label;
   static lv_obj_t * sight_label;

   std::string weapon = "Weapon: " + recoil.getWeapon();
   std::string barrel = "Barrel: " + recoil.getBarrel();
   std::string sight = "Sight: " + recoil.getSight();


   if (arg){
      weapon_label = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_align(weapon_label, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_align(weapon_label, LV_ALIGN_LEFT_MID, 10, -44);
      lv_label_set_long_mode(weapon_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_width(weapon_label, 122);

      sight_label = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_align(sight_label, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_align(sight_label, LV_ALIGN_LEFT_MID, 10, -24);
      lv_label_set_long_mode(sight_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_width(sight_label, 122);

      barrel_label = lv_label_create(lv_scr_act());
      lv_obj_set_style_text_align(barrel_label, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_align(barrel_label, LV_ALIGN_LEFT_MID, 10, -4);
      lv_label_set_long_mode(barrel_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_width(barrel_label, 122);
   }

   lv_label_set_text(weapon_label, weapon.c_str());
   lv_label_set_text(sight_label, sight.c_str());
   lv_label_set_text(barrel_label, barrel.c_str());
}

// initialize the weapon dropdown menu style
static void dropdown_weapon_init_style_cb(lv_event_t * e){
   lv_obj_t * dropdown = lv_event_get_target(e);
   lv_obj_t * list = lv_dropdown_get_list(dropdown); // get the list object within the dropdown menu
      if (!list){
         LV_LOG_USER("list is null");
      }
      else {
         lv_obj_add_style(list, lv_button_style(), LV_PART_MAIN);
         lv_obj_add_style(list, lv_button_style_checked(), LV_PART_SELECTED | LV_STATE_CHECKED);
         LV_LOG_USER("list style executed");
      }
   LV_LOG_USER("Returning to caller");
}

// update current highlighted selection in weapon dropdown
static void dropdown_weapon_refresh_cb(lv_event_t * e){
   lv_obj_t * dropdown = lv_event_get_target(e);
   if (strcmp(recoil.getWeapon().c_str(), "None") == 0){
      lv_dropdown_set_selected(dropdown, 0);
      lv_label_current_equipment(0); // use 0 as argument to only update
   }
}

// adjust weapon settings
static void dropdown_weapon_cb(lv_event_t * e){
   lv_obj_t * dropdown = lv_event_get_target(e);
   char buf[64];
   lv_dropdown_get_selected_str(dropdown, buf, sizeof(buf));
   LV_LOG_USER(buf);
    
   if (strcmp(buf, "None") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_NONE);
   }
   else if (strcmp(buf, "Assault Rifle") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_AK);
   }
   else if (strcmp(buf, "LR-300") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_LR);
   }
   else if (strcmp(buf, "MP5") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_MP5);
   }
   else if (strcmp(buf, "Thompson") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_TOMMY);
   }
   else if (strcmp(buf, "Custom") == 0){
     recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_CUST);
   }
   else if (strcmp(buf, "M249") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_M249);
   }
   else if (strcmp(buf, "M39") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_M39);
   }
   else if (strcmp(buf, "M92") == 0){
     recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_M92);
   }
   else if (strcmp(buf, "Python") == 0){
     recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_PYTHON);
   }
   else if (strcmp(buf, "Revolver") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_REVVY);
   }
   else if (strcmp(buf, "P2") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_P2);
   }
   else if (strcmp(buf, "SAR") == 0){
      recoil.setEquipment(RECOIL_EQUIPMENT::WEAPON_SAR);
   }
   else {
      LV_LOG_USER("Error - no matching string");
   }

   lv_label_current_equipment(0); // use 0 as argument to only update
}

// Create a dropdown list
void lv_dropdown_weapon(void) {
   lv_obj_t * weapon_dropdown = lv_dropdown_create(lv_scr_act());
   lv_obj_align(weapon_dropdown, LV_ALIGN_TOP_LEFT, 10, 10);
   lv_dropdown_set_options(weapon_dropdown, "None\n"
                                      "Assault Rifle\n"
                                      "LR-300\n"
                                      "MP5\n"
                                      "Thompson\n"
                                      "Custom\n"
                                      "M249\n"
                                      "M39\n"
                                      "M92\n"
                                      "Python\n"
                                      "Revolver\n"
                                      "P2\n"
                                      "SAR");

   //Set a fixed text to display on the button of the drop-down list
   lv_dropdown_set_text(weapon_dropdown, "Weapon");

   if (strcmp(recoil.getWeapon().c_str(), "None") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 0);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "Assualt Rifle") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 1);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "LR300 Assault-Rifle") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 2);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "MP5") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 3);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "Thompson") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 4);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "Custom") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 5);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "M249") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 6);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "M39") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 7);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "M92") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 8);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "Python") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 9);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "Revolver") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 10);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "P2") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 11);
   }
   else if (strcmp(recoil.getWeapon().c_str(), "SAR") == 0){
      lv_dropdown_set_selected(weapon_dropdown, 12);
   }
   else {
      LV_LOG_USER("Error - no matching string");
   }

   lv_obj_add_style(weapon_dropdown, lv_button_style(), LV_PART_MAIN);
   lv_obj_add_event_cb(weapon_dropdown, dropdown_weapon_cb, LV_EVENT_VALUE_CHANGED, NULL);
   lv_obj_add_event_cb(weapon_dropdown, dropdown_weapon_refresh_cb, LV_EVENT_REFRESH, NULL);
   lv_obj_add_event_cb(weapon_dropdown, dropdown_weapon_init_style_cb, LV_EVENT_RELEASED, NULL);
}

#if LV_USE_LOG != 0
// Serial debugging
void my_print(const char* buffer){
   Serial.println(buffer);
}
#endif

// Display flushing
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p ) {
   uint32_t w = ( area->x2 - area->x1 + 1 );
   uint32_t h = ( area->y2 - area->y1 + 1 );

   tft.writeRect(area->x1,area->y1,w,h,(uint16_t*)color_p);

   lv_disp_flush_ready( disp );
}

// Read the touchscreen
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data ) {
   bool touched = ts.touched();

   if(!touched) {
      data->state = LV_INDEV_STATE_REL;
   }
   else {
      data->state = LV_INDEV_STATE_PR;
      TS_Point p = ts.getPoint();
      // Scale from ~0->4000 to tft.width using the calibration #'s
      // and set the coordinates
      data->point.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
      data->point.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

#if LV_USE_LOG != 0
      Serial.print("Touchscreen X: ");
      Serial.print(data->point.x);
      Serial.print("   Y: ");
      Serial.print(data->point.y);
      Serial.println();
#endif
   }
}

// update the gui timer
void guiInc() {
  lv_tick_inc(5);  
}

// initialize the main menu screen
void iniMainMenu(void){
   if (!screen1){
      screen1 = lv_obj_create(NULL);
   }
   lv_scr_load(screen1);
   lv_obj_add_style(lv_scr_act(), lv_desktop_style(), LV_PART_MAIN); // set the current screen background style as a gradient
   lv_label_current_equipment(1); // send any positive value to initialize the label
   lv_dropdown_weapon();
   lv_dropdown_sight();
   lv_toggle_silencer(); // index 5 of screen1
   lv_btn_fov();
   lv_btn_sens();
   lv_btn_reset();
}

// initialize the fov settings menu screen
void iniFOVMenu(void){
   if (!screen2){
      screen2 = lv_obj_create(NULL);
   }
   lv_scr_load(screen2);
   lv_obj_add_style(lv_scr_act(), lv_desktop_style(), LV_PART_MAIN); // set the current screen background style as a gradient
   lv_btn_home();
   lv_arc_fov();
}

// initialize the sensitivity menu screen
void iniSensMenu(void){
   if (!screen2){
      screen2 = lv_obj_create(NULL);
   }
   lv_scr_load(screen2);
   lv_obj_add_style(lv_scr_act(), lv_desktop_style(), LV_PART_MAIN); // set the current screen background style as a gradient
   lv_btn_home();
   lv_kb_number();
}

void report_inject_to_serial(){
  Serial.print("Mouse injected X: ");
  Serial.print(x);
  Serial.print("\t");
  Serial.print("Y: ");
  Serial.print(y);
  Serial.println();
}

void injectMouse() {
   usb_mouse_move(x, y, 0, 0);
#if LV_USE_LOG != 0
   report_inject_to_serial();
#endif
}

void updateMouse() {
   usb_mouse_buttons_state = mouse.getButtons();
   usb_mouse_move(mouse.getMouseX(), mouse.getMouseY(), mouse.getWheel(), mouse.getWheelH());
}

void handleRecoil() {
   recoil.fireBullet(x, y);
   injectMouse();
   sinceInject = 0;
}
