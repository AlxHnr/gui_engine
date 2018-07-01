#include "gui_engine.h"

void setExampleTheme(void)
{
  /* Get struct containing global defaults. This values will only apply to
     windows/widgets created afterwards. */
  g_setting_struct *defaults = g_get_setting_struct();

  /* window colors */
  defaults->window.color.frame.r = 133;
  defaults->window.color.frame.g = 170;
  defaults->window.color.frame.b = 217;
  defaults->window.color.frame.a = 255;

  defaults->window.color.background.r = 237;
  defaults->window.color.background.g = 236;
  defaults->window.color.background.b = 235;
  defaults->window.color.background.a = 255;

  defaults->window.color.resize_mark.r = 200;
  defaults->window.color.resize_mark.g = 200;
  defaults->window.color.resize_mark.b = 200;
  defaults->window.color.resize_mark.a = 255;

  defaults->window.color.title_text.r = 255;
  defaults->window.color.title_text.g = 255;
  defaults->window.color.title_text.b = 255;
  defaults->window.color.title_text.a = 255;

  defaults->window.color.title_bar_top.r = 144;
  defaults->window.color.title_bar_top.g = 179;
  defaults->window.color.title_bar_top.b = 222;
  defaults->window.color.title_bar_top.a = 255;

  defaults->window.color.title_bar_bottom.r = 121;
  defaults->window.color.title_bar_bottom.g = 160;
  defaults->window.color.title_bar_bottom.b = 209;
  defaults->window.color.title_bar_bottom.a = 255;

  defaults->window.color.close_button_text.r = 255;
  defaults->window.color.close_button_text.g = 255;
  defaults->window.color.close_button_text.b = 255;
  defaults->window.color.close_button_text.a = 255;

  defaults->window.color.close_button_top.r = 144;
  defaults->window.color.close_button_top.g = 179;
  defaults->window.color.close_button_top.b = 222;
  defaults->window.color.close_button_top.a = 255;

  defaults->window.color.close_button_bottom.r = 121;
  defaults->window.color.close_button_bottom.g = 160;
  defaults->window.color.close_button_bottom.b = 209;
  defaults->window.color.close_button_bottom.a = 255;

  /* widget defaults */
  /* text field */
  defaults->text.color.text.r = 40;
  defaults->text.color.text.g = 40;
  defaults->text.color.text.b = 40;
  defaults->text.color.text.a = 255;

  /* input box */
  defaults->input.color.text.r = 40;
  defaults->input.color.text.g = 40;
  defaults->input.color.text.b = 40;
  defaults->input.color.text.a = 255;

  defaults->input.color.frame.r = 176;
  defaults->input.color.frame.g = 173;
  defaults->input.color.frame.b = 170;
  defaults->input.color.frame.a = 255;

  defaults->input.color.background.r = 255;
  defaults->input.color.background.g = 255;
  defaults->input.color.background.b = 255;
  defaults->input.color.background.a = 255;

  defaults->input.color.background_active.r = 255;
  defaults->input.color.background_active.g = 255;
  defaults->input.color.background_active.b = 255;
  defaults->input.color.background_active.a = 255;

  defaults->input.color.cursor.r = 40;
  defaults->input.color.cursor.g = 40;
  defaults->input.color.cursor.b = 40;
  defaults->input.color.cursor.a = 255;

  /* button */
  defaults->button.color.text.r = 40;
  defaults->button.color.text.g = 40;
  defaults->button.color.text.b = 40;
  defaults->button.color.text.a = 255;

  defaults->button.color.frame.r = 142;
  defaults->button.color.frame.g = 142;
  defaults->button.color.frame.b = 142;
  defaults->button.color.frame.a = 255;

  defaults->button.color.top.r = 250;
  defaults->button.color.top.g = 250;
  defaults->button.color.top.b = 250;
  defaults->button.color.top.a = 255;

  defaults->button.color.bottom.r = 230;
  defaults->button.color.bottom.g = 230;
  defaults->button.color.bottom.b = 230;
  defaults->button.color.bottom.a = 255;

  /* check box */
  defaults->check.color.mark.r = 40;
  defaults->check.color.mark.g = 40;
  defaults->check.color.mark.b = 40;
  defaults->check.color.mark.a = 255;

  defaults->check.color.frame.r = 142;
  defaults->check.color.frame.g = 142;
  defaults->check.color.frame.b = 142;
  defaults->check.color.frame.a = 255;

  defaults->check.color.background.r = 255;
  defaults->check.color.background.g = 255;
  defaults->check.color.background.b = 255;
  defaults->check.color.background.a = 255;

  /* slider */
  defaults->slider.color.slider.r = 243;
  defaults->slider.color.slider.g = 243;
  defaults->slider.color.slider.b = 243;
  defaults->slider.color.slider.a = 255;

  defaults->slider.color.frame.r = 142;
  defaults->slider.color.frame.g = 142;
  defaults->slider.color.frame.b = 142;
  defaults->slider.color.frame.a = 255;

  defaults->slider.color.line.r = 190;
  defaults->slider.color.line.g = 190;
  defaults->slider.color.line.b = 190;
  defaults->slider.color.line.a = 255;

  /* drop down list */
  defaults->drop_down.color.text.r = 40;
  defaults->drop_down.color.text.g = 40;
  defaults->drop_down.color.text.b = 40;
  defaults->drop_down.color.text.a = 255;

  defaults->drop_down.color.frame.r = 142;
  defaults->drop_down.color.frame.g = 142;
  defaults->drop_down.color.frame.b = 142;
  defaults->drop_down.color.frame.a = 255;

  defaults->drop_down.color.background.r = 255;
  defaults->drop_down.color.background.g = 255;
  defaults->drop_down.color.background.b = 255;
  defaults->drop_down.color.background.a = 255;

  defaults->drop_down.color.background_list.r = 235;
  defaults->drop_down.color.background_list.g = 235;
  defaults->drop_down.color.background_list.b = 235;
  defaults->drop_down.color.background_list.a = 255;

  defaults->drop_down.color.highlight.r = 255;
  defaults->drop_down.color.highlight.g = 255;
  defaults->drop_down.color.highlight.b = 255;
  defaults->drop_down.color.highlight.a = 255;

  defaults->drop_down.color.arrow.r = 40;
  defaults->drop_down.color.arrow.g = 40;
  defaults->drop_down.color.arrow.b = 40;
  defaults->drop_down.color.arrow.a = 255;

  /* pop_up */
  defaults->pop_up.color.frame.r = 142;
  defaults->pop_up.color.frame.g = 142;
  defaults->pop_up.color.frame.b = 142;
  defaults->pop_up.color.frame.a = 255;

  defaults->pop_up.color.background.r = 255;
  defaults->pop_up.color.background.g = 255;
  defaults->pop_up.color.background.b = 185;
  defaults->pop_up.color.background.a = 255;
}
