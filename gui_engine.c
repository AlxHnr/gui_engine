/*
  Copyright (c) 2018 Alexander Heinrich

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "gui_engine.h"

static g_window *first_window = NULL;
static g_window *last_window = NULL;

static g_window *moving_window = NULL;
static int temp_window_x, temp_window_y;

static g_window *resizing_window = NULL;
static int temp_window_w, temp_window_h;

static g_window *selected_pop_up_window = NULL;
static Uint32 selected_pop_up_timestop, selected_pop_up_delay;

static g_widget *clicked_button = NULL;
static g_widget *clicked_slider = NULL;
static g_widget *active_drop_down_list = NULL;
static g_widget *active_input_box = NULL;

static int key_repeat_delay, key_repeat_interval;

static g_window *grab_keyboard_window = NULL;
static g_widget *grab_keyboard_widget = NULL;

/* gui defaults */
static g_setting_struct g_defaults;

/* core functions start */
static g_widget *g_attach_raw_widged(g_window *window)
{
  if(!window)
    return NULL;
  
  /* allocate memory and set new widget to window->last_widget */
  if(!window->first_widget)
  {
    window->first_widget = malloc(sizeof(struct g_widget));
    if(!window->first_widget)
      return NULL;
    
    window->first_widget->next = NULL;
    window->first_widget->prev = NULL;
    window->last_widget = window->first_widget;
  }
  else
  {
    window->last_widget->next = malloc(sizeof(struct g_widget));
    if(!window->last_widget->next)
      return NULL;
    
    window->last_widget->next->prev = window->last_widget;
    window->last_widget->next->next = NULL;
    window->last_widget = window->last_widget->next;
  }
  
  window->last_widget->window = window;
  
  window->last_widget->pop_up_delay = g_defaults.pop_up_delay;
  window->last_widget->pop_up = NULL;
  
  window->last_widget->event_function = NULL;
  window->last_widget->event_data = NULL;
  
  return window->last_widget;
}
static g_widget *g_attach_slider_raw(g_window *window, const double value, const double max_value)
{
  g_widget *widget = g_attach_raw_widged(window);
  
  if(!widget)
    return NULL;
  
  /* widget specific stuff */
  if(value < 0)
    widget->slider.value = 0;
  else if(value > max_value)
    widget->slider.value = max_value;
  else
    widget->slider.value = value;
  
  widget->slider.max_value = max_value;
  
  widget->slider.flags.invert = g_defaults.slider.flags.invert;
  widget->slider.flags.mouse_wheel = g_defaults.slider.flags.mouse_wheel;
  
  /* set colors to default */
  widget->slider.color.slider.r = g_defaults.slider.color.slider.r;
  widget->slider.color.slider.g = g_defaults.slider.color.slider.g;
  widget->slider.color.slider.b = g_defaults.slider.color.slider.b;
  widget->slider.color.slider.a = g_defaults.slider.color.slider.a;
  
  widget->slider.color.frame.r = g_defaults.slider.color.frame.r;
  widget->slider.color.frame.g = g_defaults.slider.color.frame.g;
  widget->slider.color.frame.b = g_defaults.slider.color.frame.b;
  widget->slider.color.frame.a = g_defaults.slider.color.frame.a;
  
  widget->slider.color.line.r = g_defaults.slider.color.line.r;
  widget->slider.color.line.g = g_defaults.slider.color.line.g;
  widget->slider.color.line.b = g_defaults.slider.color.line.b;
  widget->slider.color.line.a = g_defaults.slider.color.line.a;
  
  return widget;
}
static void g_draw_text(SDL_Surface *dst, const char *text,
                        const int x, const int y, const int w, const int h,
                        const Uint8 r, const Uint8 g, const Uint8 b, const Uint8 a)
{
  int dest_x, dest_y;
  int counter;
  
  /* draw text loop */
  dest_x = x;
  dest_y = y;
  for(counter = 0; text[counter] != '\0' && dest_y < y + h; counter++)
  {
    if(text[counter] != '\n')
      characterRGBA(dst, dest_x, dest_y, text[counter], r, g, b, a);
    
    if(dest_x + G_CHAR_W < x + w && text[counter] != '\n')
    {
      dest_x += G_CHAR_W;
    }
    else
    {
      dest_x = x;
      dest_y += G_CHAR_H;
    }
  }
}
static void g_mark_window_as_selected_pop_up(g_window *window, const Uint32 delay)
{
  selected_pop_up_window = window;
  selected_pop_up_timestop = SDL_GetTicks();
  selected_pop_up_delay = delay;
}

/* returns adjustet position in window, not on video surface */
static void g_adjust_widget_position(const g_widget *widget, int *x, int *y, int *w, int *h)
{
  g_window *window = widget->window;
  
  /* widget position in window cant be smaller than window->margin */
  if(widget->x < window->margin)
    *x = window->margin;
  else
    *x = widget->x;
  
  if(widget->y < window->margin)
    *y = window->margin;
  else
    *y = widget->y;
  
  /* widget cant be bigger then window */
  if(w)
  {
    if((widget->w <= 0) || (*x + widget->w > window->w - window->margin))
      *w = window->w - window->margin - *x;
    else
      *w = widget->w;
  }
  
  if(h)
  {
    if((widget->h <= 0) || (*y + widget->h > window->h - window->margin))
      *h = window->h - window->margin - *y;
    else
      *h = widget->h;
  }
}
static void g_adjust_widget_position_button(const g_widget *widget, int *x, int *y, int *w, int *h)
{
  g_window *window = widget->window;
  int counter, cache_w;
  int temp_w, temp_h;
  
  g_adjust_widget_position(widget, x, y, NULL, NULL);
  
  /* caclucate text width and height in characters */
  counter = 0;
  cache_w = 1;
  temp_w = temp_h = 1;
  while(widget->button.text[counter] != '\0')
  {
    if((widget->button.text[counter] == '\n') || (cache_w * G_CHAR_W > window->w - G_MARGIN * 2 - window->margin * 2))
    {
      cache_w = 1;
      temp_h++;
    }
    else
    {
      if(cache_w > temp_w)
        temp_w = cache_w;
      cache_w++;
    }
    
    counter++;
  }
  
  /* calculate button width and height from text width and height */
  *w = temp_w * G_CHAR_W + G_MARGIN * 2;
  *h = temp_h * G_CHAR_H + G_MARGIN;
  
  /* correct position */
  if(*x > window->w - window->margin - *w)
    *x = window->w - window->margin - *w;
  if(*y > window->h - window->margin - *h)
    *y = window->h - window->margin - *h;
  
  /* check if button was moved out of window while correcting position */
  if(*x < window->margin)
  {
    *x = window->margin;
    *w = window->w - window->margin * 2;
  }
  if(*y < window->margin)
  {
    *y = window->margin;
    *h = window->h - window->margin * 2;
  }
}
static void g_adjust_widget_position_check_box(const g_widget *widget, int *x, int *y)
{
  g_window *window = widget->window;
  
  g_adjust_widget_position(widget, x, y, NULL, NULL);
  
  if(*x > window->w - window->margin - G_CHECK_BOX_SIZE)
    *x = window->w - window->margin - G_CHECK_BOX_SIZE;
  if(*y > window->h - window->margin - G_CHECK_BOX_SIZE)
    *y = window->h - window->margin - G_CHECK_BOX_SIZE;
}
static void g_adjust_widget_position_surface(const g_widget *widget, int *x, int *y, int *w, int *h)
{
  g_window *window = widget->window;
  
  g_adjust_widget_position(widget, x, y, NULL, NULL);
  
  /* widget x/y + w/h cant be outside surface */
  if((widget->w <= 0) || (widget->surface.src_x + widget->w > widget->surface.surface->w))
    *w = widget->surface.surface->w - widget->surface.src_x;
  else
    *w = widget->w;
  
  if((widget->h <= 0)  || (widget->surface.src_y + widget->h > widget->surface.surface->h))
    *h = widget->surface.surface->h - widget->surface.src_y;
  else
    *h = widget->h;
  
  /* w/h cant be outside the window */
  if(*x + *w > window->w - window->margin)
    *w = window->w - window->margin - *x;
  
  if(*y + *h > window->h - window->margin)
    *h = window->h - window->margin - *y;
}
static void g_adjust_widget_position_slider(const g_widget *widget, int *x, int *y, int *w, int *h)
{
  g_window *window = widget->window;
  
  g_adjust_widget_position(widget, x, y, NULL, NULL);
  
  /* adjust x/y */
  if(widget->type == G_TYPE_SLIDER_H)
  {
    if(*y + G_SLIDER_H > window->h - window->margin)
      *y = window->h - window->margin - G_SLIDER_H;
    
    /* adjust widget width */
    if(w)
    {
      if((widget->w <= 0) || (*x + widget->w > window->w - window->margin))
        *w = window->w - window->margin - *x;
      else
        *w = widget->w;
    }
  }
  else if(widget->type == G_TYPE_SLIDER_V)
  {
    if(*x + G_SLIDER_H > window->w - window->margin)
      *x = window->w - window->margin - G_SLIDER_H;
    
    /* adjust widget height */
    if(h)
    {
      if((widget->h <= 0) || (*y + widget->h > window->h - window->margin))
        *h = window->h - window->margin - *y;
      else
        *h = widget->h;
    }
  }
}
static void g_adjust_widget_position_drop_down_list(const g_widget *widget, int *x, int *y, int *w)
{
  g_window *window = widget->window;
  
  g_adjust_widget_position(widget, x, y, w, NULL);
  
  /* correct y */
  if(*y + G_DROP_DOWN_LIST_SIZE > window->h - window->margin)
    *y = window->h - window->margin - G_DROP_DOWN_LIST_SIZE;
  else
    *y = widget->y;
}
static void g_adjust_widget_position_drop_down_list_size(const g_widget *widget, int *x, int *y, int *w, int *h)
{
  g_window *window = widget->window;
  SDL_Surface *dst = SDL_GetVideoSurface();
  int counter, cache_w;
  int temp_x, temp_y;
  int temp_w, temp_h;
  
  /* caclucate text width and height in characters */
  counter = 0;
  cache_w = 1;
  temp_w = temp_h = 1;
  while(widget->button.text[counter] != '\0')
  {
    if(widget->button.text[counter] == '\n')
    {
      cache_w = 1;
      temp_h++;
    }
    else
    {
      if(cache_w > temp_w)
        temp_w = cache_w;
      cache_w++;
    }
    
    counter++;
  }
  
  /* calculate box width and height from text width and height */
  *w = temp_w * G_CHAR_W + G_MARGIN * 2;
  *h = temp_h * G_CHAR_H;
  
  /* set x/y */
  g_adjust_widget_position_drop_down_list(widget, &temp_x, &temp_y, &temp_w);
  if(*w < temp_w)
    *w = temp_w;
  
  if(window->x + temp_x < 1)
    *x = 1;
  else if(window->x + temp_x + *w > dst->w)
    *x = dst->w - *w - 1;
  else
    *x = window->x + temp_x;
  
  if(window->y + temp_y + G_DROP_DOWN_LIST_SIZE + *h > dst->h)
    *y = window->y + temp_y - *h - 1;
  else
    *y = window->y + temp_y + G_DROP_DOWN_LIST_SIZE + 1;
}

/* widget draw functions */
static void g_draw_widget_text(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  int x, y, w, h;
  
  g_adjust_widget_position(widget, &x, &y, &w, &h);
  
  /* draw widget text */
  g_draw_text(dst, widget->text.text, window->x + x, window->y + y, w, h,
              widget->text.color.text.r, widget->text.color.text.g,
              widget->text.color.text.b, widget->text.color.text.a);
}
static void g_draw_widget_input_box(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  int x, y, w, h;
  int dest_x, dest_y;
  int counter;
  int text_length;
  
  g_adjust_widget_position(widget, &x, &y, &w, &h);
  
  /* draw background */
  if(widget == active_input_box)
  {
    boxRGBA(dst, window->x + x + 1, window->y + y + 1,
            window->x + x + w - 1, window->y + y + h - 1,
            widget->input.color.background_active.r, widget->input.color.background_active.g,
            widget->input.color.background_active.b, widget->input.color.background_active.a);
  }
  else
  {
    boxRGBA(dst, window->x + x + 1, window->y + y + 1,
            window->x + x + w - 1, window->y + y + h - 1,
            widget->input.color.background.r, widget->input.color.background.g,
            widget->input.color.background.b, widget->input.color.background.a);
  }
  
  /* draw frame */
  rectangleRGBA(dst, window->x + x, window->y + y, window->x + x + w, window->y + y + h,
                widget->input.color.frame.r, widget->input.color.frame.g,
                widget->input.color.frame.b, widget->input.color.frame.a);
  
  /* adjust position */
  x += G_MARGIN;
  y += G_MARGIN;
  w -= G_MARGIN * 2;
  h -= G_MARGIN * 2;
  
  /* draw text loop */
  dest_x = window->x + x;
  dest_y = window->y + y;
  counter = widget->input.first_character;
  text_length = strlen(widget->input.text);
  while(counter <= text_length && dest_y < window->y + y + h + G_CHAR_H)
  {
    /* write character */
    if(widget->input.text[counter] != '\0' && dest_y < window->y + y + h)
    {
      if(widget->input.flags.hide_text)
      {
        characterRGBA(dst, dest_x, dest_y, widget->input.replace_character,
                      widget->input.color.text.r, widget->input.color.text.g,
                      widget->input.color.text.b, widget->input.color.text.a);
      }
      else
      {
        characterRGBA(dst, dest_x, dest_y, widget->input.text[counter],
                      widget->input.color.text.r, widget->input.color.text.g,
                      widget->input.color.text.b, widget->input.color.text.a);
      }
    }
    
    /* draw cursor */
    if(widget == active_input_box &&
       counter == active_input_box->input.cursor_pos)
    {
      /* draw cursor in upper right corner */
      if(dest_y >= window->y + y + G_CHAR_H && dest_x == window->x + x)
      {
        vlineRGBA(dst, window->x + x + (active_input_box->input.char_amount_w * G_CHAR_W),
                  dest_y - 2 - G_CHAR_H, dest_y + 10 - G_CHAR_H,
                  widget->input.color.cursor.r, widget->input.color.cursor.g,
                  widget->input.color.cursor.b, widget->input.color.cursor.a);
      }
      else
      {
        vlineRGBA(dst, dest_x, dest_y - 2, dest_y + 10,
                  widget->input.color.cursor.r, widget->input.color.cursor.g,
                  widget->input.color.cursor.b, widget->input.color.cursor.a);
      }
    }
    
    if(dest_x + G_CHAR_W < window->x + x + w)
    {
      dest_x += G_CHAR_W;
    }
    else
    {
      dest_x = window->x + x;
      dest_y += G_CHAR_H;
    }
    
    counter++;
  }
}
static void g_draw_widget_button(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  int x, y, w, h;
  
  int counter;
  float r, g, b, a;
  float r_step, g_step, b_step, a_step;
  
  g_adjust_widget_position_button(widget, &x, &y, &w, &h);
  
  /* reset colors and calculate color steps */
  if(widget == clicked_button)
  {
    r = widget->button.color.bottom.r;
    g = widget->button.color.bottom.g;
    b = widget->button.color.bottom.b;
    a = widget->button.color.bottom.a;
    
    r_step = ((float)widget->button.color.bottom.r - (float)widget->button.color.top.r)/(float)h;
    g_step = ((float)widget->button.color.bottom.g - (float)widget->button.color.top.g)/(float)h;
    b_step = ((float)widget->button.color.bottom.b - (float)widget->button.color.top.b)/(float)h;
    a_step = ((float)widget->button.color.bottom.a - (float)widget->button.color.top.a)/(float)h;
  }
  else
  {
    r = widget->button.color.top.r;
    g = widget->button.color.top.g;
    b = widget->button.color.top.b;
    a = widget->button.color.top.a;
    
    r_step = ((float)widget->button.color.top.r - (float)widget->button.color.bottom.r)/(float)h;
    g_step = ((float)widget->button.color.top.g - (float)widget->button.color.bottom.g)/(float)h;
    b_step = ((float)widget->button.color.top.b - (float)widget->button.color.bottom.b)/(float)h;
    a_step = ((float)widget->button.color.top.a - (float)widget->button.color.bottom.a)/(float)h;
  }
  
  /* draw button background */
  for(counter = window->y + y; counter < window->y + y + h; counter++)
  {
    hlineRGBA(dst, window->x + x, window->x + x + w - 1, counter, r, g, b, a);
    
    r -= r_step;
    g -= g_step;
    b -= b_step;
    a -= a_step;
  }
  
  /* draw button frame */
  rectangleRGBA(dst, window->x + x - 1, window->y + y - 1, window->x + x + w, window->y + y + h,
                widget->button.color.frame.r, widget->button.color.frame.g,
                widget->button.color.frame.b, widget->button.color.frame.a);
  
  /* adjust position */
  x += G_MARGIN;
  y += G_MARGIN;
  w -= G_MARGIN * 2;
  h -= G_MARGIN * 2;
  
  /* draw widget text */
  g_draw_text(dst, widget->button.text, window->x + x, window->y + y, w, h,
              widget->button.color.text.r, widget->button.color.text.g,
              widget->button.color.text.b, widget->button.color.text.a);
}
static void g_draw_widget_check_box(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  int x, y;
  
  g_adjust_widget_position_check_box(widget, &x, &y);
  
  /* draw background */
  boxRGBA(dst, window->x + x + 1, window->y + y + 1,
          window->x + x + G_CHECK_BOX_SIZE - 1, window->y + y + G_CHECK_BOX_SIZE - 1,
          widget->check.color.background.r, widget->check.color.background.g,
          widget->check.color.background.b, widget->check.color.background.a);
  
  /* draw frame */
  rectangleRGBA(dst, window->x + x, window->y + y,
          window->x + x + G_CHECK_BOX_SIZE, window->y + y + G_CHECK_BOX_SIZE,
          widget->check.color.frame.r, widget->check.color.frame.g,
          widget->check.color.frame.b, widget->check.color.frame.a);
  
  /* draw 'X' if check box is enabled */
  if(widget->check.state)
  {
    characterRGBA(dst, window->x + x + 5, window->y + y + 5, 'X',
    widget->check.color.mark.r, widget->check.color.mark.g,
    widget->check.color.mark.b, widget->check.color.mark.a);
  }
}
static void g_draw_widget_surface(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  SDL_Rect temp_src, temp_dst;
  int x, y, w, h;
  
  g_adjust_widget_position_surface(widget, &x, &y, &w, &h);
  
  /* set temp_src x and y */
  temp_src.x = widget->surface.src_x;
  temp_src.y = widget->surface.src_y;
  temp_src.w = w;
  temp_src.h = h;
  
  /* correct temp_dst */
  temp_dst.x = window->x + x;
  temp_dst.y = window->y + y;
  
  /* blit surface */
  SDL_BlitSurface(widget->surface.surface, &temp_src, dst, &temp_dst);
}
static void g_draw_widget_slider_h(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  int x, y, w;
  int slider_value;
  
  g_adjust_widget_position_slider(widget, &x, &y, &w, NULL);
  
  /* draw line */
  thickLineRGBA(dst, window->x + x, window->y + y + G_SLIDER_H/2,
                window->x + x + w, window->y + y + G_SLIDER_H/2, G_SLIDER_THICKNESS,
                widget->slider.color.line.r, widget->slider.color.line.g,
                widget->slider.color.line.b, widget->slider.color.line.a);
  
  if(widget->slider.flags.invert)
    slider_value = (1 - widget->slider.value/widget->slider.max_value) * (w - G_SLIDER_W);
  else
    slider_value = (widget->slider.value/widget->slider.max_value) * (w - G_SLIDER_W);
  
  /* draw slider */
  boxRGBA(dst, window->x + x + slider_value, window->y + y,
          window->x + x + slider_value + G_SLIDER_W - 1, window->y + y + G_SLIDER_H - 1,
          widget->slider.color.slider.r, widget->slider.color.slider.g,
          widget->slider.color.slider.b, widget->slider.color.slider.a);
  
  /* draw frame */
  rectangleRGBA(dst, window->x + x + slider_value - 1, window->y + y - 1,
                window->x + x + slider_value + G_SLIDER_W, window->y + y + G_SLIDER_H,
                widget->slider.color.frame.r, widget->slider.color.frame.g,
                widget->slider.color.frame.b, widget->slider.color.frame.a);
}
static void g_draw_widget_slider_v(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  int x, y, h;
  int slider_value;
  
  g_adjust_widget_position_slider(widget, &x, &y, NULL, &h);
  
  /* draw line */
  thickLineRGBA(dst, window->x + x + G_SLIDER_H/2, window->y + y,
                window->x + x + G_SLIDER_H/2, window->y + y + h, G_SLIDER_THICKNESS,
                widget->slider.color.line.r, widget->slider.color.line.g,
                widget->slider.color.line.b, widget->slider.color.line.a);
  
  if(widget->slider.flags.invert)
    slider_value = (widget->slider.value/widget->slider.max_value) * (h - G_SLIDER_W);
  else
    slider_value = (1 - widget->slider.value/widget->slider.max_value) * (h - G_SLIDER_W);
  
  /* draw slider */
  boxRGBA(dst, window->x + x, window->y + y + slider_value,
          window->x + x + G_SLIDER_H - 1, window->y + y + slider_value + G_SLIDER_W - 1,
          widget->slider.color.slider.r, widget->slider.color.slider.g,
          widget->slider.color.slider.b, widget->slider.color.slider.a);
  
  /* draw frame */
  rectangleRGBA(dst, window->x + x - 1, window->y + y + slider_value - 1,
                window->x + x + G_SLIDER_H, window->y + y + slider_value + G_SLIDER_W,
                widget->slider.color.frame.r, widget->slider.color.frame.g,
                widget->slider.color.frame.b, widget->slider.color.frame.a);
}
static void g_draw_widget_drop_down_list(SDL_Surface *dst, const g_widget *widget)
{
  g_window *window = widget->window;
  int x, y, w;
  int counter;
  char *text;
  char temp_string[G_TEXT_LENGTH] = {0};
  
  g_adjust_widget_position_drop_down_list(widget, &x, &y, &w);
  
  /* draw background */
  boxRGBA(dst, window->x + x, window->y + y, window->x + x + w - 1, window->y + y + G_DROP_DOWN_LIST_SIZE - 1,
          widget->drop_down.color.background.r, widget->drop_down.color.background.g,
          widget->drop_down.color.background.b, widget->drop_down.color.background.a);
  
  /* draw frame */
  rectangleRGBA(dst, window->x + x - 1, window->y + y - 1, window->x + x + w,
          window->y + y + G_DROP_DOWN_LIST_SIZE,
          widget->drop_down.color.frame.r, widget->drop_down.color.frame.g,
          widget->drop_down.color.frame.b, widget->drop_down.color.frame.a);
  vlineRGBA(dst, window->x + x + w - G_DROP_DOWN_LIST_SIZE, window->y + y,
            window->y + y + G_DROP_DOWN_LIST_SIZE - 1,
            widget->drop_down.color.frame.r, widget->drop_down.color.frame.g,
            widget->drop_down.color.frame.b, widget->drop_down.color.frame.a);
  
  /* draw arrow pointing down; character number '31' in "SDL_gfxPrimitivesfont.h" */
  characterRGBA(dst, window->x + x + w - G_DROP_DOWN_LIST_SIZE + 7, window->y + y + 8, 31,
                widget->drop_down.color.arrow.r, widget->drop_down.color.arrow.g,
                widget->drop_down.color.arrow.b, widget->drop_down.color.arrow.a);
  
  /* check position of current text item */
  counter = g_get_line(widget->drop_down.text, widget->drop_down.current_item);
  if(counter >= 0)
  {
    strcpy(temp_string, &widget->drop_down.text[counter]);
    
    /* cut off other list items */
    if((text = strchr(temp_string, '\n')))
      temp_string[strlen(temp_string) - strlen(text)] = '\0';
    
    /* adjust position */
    x += G_MARGIN;
    y += G_MARGIN;
    w -= G_MARGIN * 2 + G_DROP_DOWN_LIST_SIZE;
    
    g_draw_text(dst, temp_string, window->x + x,
                window->y + y, w, G_CHAR_H,
                widget->drop_down.color.text.r, widget->drop_down.color.text.g,
                widget->drop_down.color.text.b, widget->drop_down.color.text.a);
  }
}

static void g_draw_widget(SDL_Surface *dst, const g_widget *widget)
{
  /* return if widget is outside the window */
  if((widget->x > widget->window->w - widget->window->margin) ||
    (widget->y > widget->window->h - widget->window->margin))
    return;
  
  switch(widget->type)
  {
    case G_TYPE_TEXT:
      g_draw_widget_text(dst, widget);
      break;
    case G_TYPE_INPUT_BOX:
      g_draw_widget_input_box(dst, widget);
      break;
    case G_TYPE_BUTTON:
      g_draw_widget_button(dst, widget);
      break;
    case G_TYPE_CHECK_BOX:
      g_draw_widget_check_box(dst, widget);
      break;
    case G_TYPE_SURFACE:
      g_draw_widget_surface(dst, widget);
      break;
    case G_TYPE_SLIDER_H:
      g_draw_widget_slider_h(dst, widget);
      break;
    case G_TYPE_SLIDER_V:
      g_draw_widget_slider_v(dst, widget);
      break;
    case G_TYPE_DROP_DOWN_LIST:
      g_draw_widget_drop_down_list(dst, widget);
      break;
  }
}

/* window draw function */
static void g_draw_window(SDL_Surface *dst, const g_window *window)
{
  g_widget *widget = NULL;
  char temp_string[G_TEXT_LENGTH] = {0};
  
  int counter;
  float r, g, b, a;
  float r_step, g_step, b_step, a_step;
  
  /* return if window is not visible */
  if(!window->flags.visible)
    return;
  
  /* draw title bar start */
  if(window->flags.title_bar)
  {
    /* reset colors */
    r = window->color.title_bar_top.r;
    g = window->color.title_bar_top.g;
    b = window->color.title_bar_top.b;
    a = window->color.title_bar_top.a;
    
    /* calculate color steps */
    r_step = ((float)window->color.title_bar_top.r - (float)window->color.title_bar_bottom.r)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
    g_step = ((float)window->color.title_bar_top.g - (float)window->color.title_bar_bottom.g)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
    b_step = ((float)window->color.title_bar_top.b - (float)window->color.title_bar_bottom.b)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
    a_step = ((float)window->color.title_bar_top.a - (float)window->color.title_bar_bottom.a)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
    
    /* draw tile bar background */
    for(counter = window->y - G_WINDOW_TITLE_BAR_HEIGHT; counter < window->y - 1; counter++)
    {
      if(window->flags.close_button)
        hlineRGBA(dst, window->x, window->x + window->w - G_WINDOW_CLOSE_BUTTON_WIDTH - 1, counter, r, g, b, a);
      else
        hlineRGBA(dst, window->x, window->x + window->w - 1, counter, r, g, b, a);
      
      r -= r_step;
      g -= g_step;
      b -= b_step;
      a -= a_step;
    }
    
    /* draw frame */
    hlineRGBA(dst, window->x - 1, window->x + window->w, window->y - G_WINDOW_TITLE_BAR_HEIGHT - 1,
              window->color.frame.r, window->color.frame.g, window->color.frame.b, window->color.frame.a);
    vlineRGBA(dst, window->x - 1, window->y - 2, window->y - G_WINDOW_TITLE_BAR_HEIGHT,
              window->color.frame.r, window->color.frame.g, window->color.frame.b, window->color.frame.a);
    vlineRGBA(dst, window->x + window->w, window->y - G_WINDOW_TITLE_BAR_HEIGHT, window->y - 2,
              window->color.frame.r, window->color.frame.g, window->color.frame.b, window->color.frame.a);
    
    /* draw close button */
    if(window->flags.close_button)
    {
      /* reset colors */
      r = window->color.close_button_top.r;
      g = window->color.close_button_top.g;
      b = window->color.close_button_top.b;
      a = window->color.close_button_top.a;
      
      /* calculate color steps */
      r_step = ((float)window->color.close_button_top.r - (float)window->color.close_button_bottom.r)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
      g_step = ((float)window->color.close_button_top.g - (float)window->color.close_button_bottom.g)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
      b_step = ((float)window->color.close_button_top.b - (float)window->color.close_button_bottom.b)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
      a_step = ((float)window->color.close_button_top.a - (float)window->color.close_button_bottom.a)/(float)G_WINDOW_TITLE_BAR_HEIGHT;
      
      /* draw close button background */
      for(counter = window->y - G_WINDOW_TITLE_BAR_HEIGHT; counter < window->y - 1; counter++)
      {
        hlineRGBA(dst, window->x + window->w - G_WINDOW_CLOSE_BUTTON_WIDTH + 1,
                  window->x + window->w - 1, counter, r, g, b, a);
        
        r -= r_step;
        g -= g_step;
        b -= b_step;
        a -= a_step;
      }
      
      /* draw frame */
      vlineRGBA(dst, window->x + window->w - G_WINDOW_CLOSE_BUTTON_WIDTH, window->y - G_WINDOW_TITLE_BAR_HEIGHT,
                window->y - 2, window->color.frame.r, window->color.frame.g,
                window->color.frame.b, window->color.frame.a);
      
      /* write close button character */
      characterRGBA(dst, window->x + window->w - G_WINDOW_CLOSE_BUTTON_WIDTH + G_MARGIN,
                    window->y - G_WINDOW_TITLE_BAR_HEIGHT + G_MARGIN, window->close_button_character,
                    window->color.close_button_text.r, window->color.close_button_text.g,
                    window->color.close_button_text.b, window->color.close_button_text.a);
      
      /* calculate how much characters fit in title bar */
      counter = (window->w - G_WINDOW_CLOSE_BUTTON_WIDTH - 2 * G_MARGIN)/G_CHAR_W;
    }
    else
      counter = (window->w - 2 * G_MARGIN)/G_CHAR_W;
    
    /* text should fit in title bar */
    if(strlen(window->title) > counter)
    {
      /* shorten text */
      strncpy(temp_string, window->title, counter);
      temp_string[counter - 3] = '.';
      temp_string[counter - 2] = '.';
      temp_string[counter - 1] = '.';
      temp_string[counter] = '\0';
    }
    else
      strcpy(temp_string, window->title);
    
    /* write text in center of title bar */
    if(window->flags.close_button)
    {
      stringRGBA(dst, window->x + (window->w - G_WINDOW_CLOSE_BUTTON_WIDTH)/2 - (strlen(temp_string) * G_CHAR_W)/2,
                 window->y - G_WINDOW_TITLE_BAR_HEIGHT + G_MARGIN, temp_string,
                 window->color.title_text.r, window->color.title_text.g,
                 window->color.title_text.b, window->color.title_text.a);
    }
    else
    {
      stringRGBA(dst, window->x + window->w/2 - (strlen(temp_string) * G_CHAR_W)/2,
                 window->y - G_WINDOW_TITLE_BAR_HEIGHT + G_MARGIN, temp_string,
                 window->color.title_text.r, window->color.title_text.g,
                 window->color.title_text.b, window->color.title_text.a);
    }
  }
  /* draw title bar end */
  
  /* draw window body with frame */
  boxRGBA(dst, window->x, window->y, window->x + window->w - 1, window->y + window->h - 1,
          window->color.background.r, window->color.background.g,
          window->color.background.b, window->color.background.a);
  rectangleRGBA(dst, window->x - 1, window->y - 1, window->x + window->w, window->y + window->h, window->color.frame.r, window->color.frame.g, window->color.frame.b, window->color.frame.a);
  
  /* draw all widgets */
  for(widget = window->first_widget; widget; widget = widget->next)
    g_draw_widget(dst, widget);
  
  /* draw resziable mark */
  if(window->flags.resizable)
  {
    pixelRGBA(dst, window->x + window->w - 2, window->y + window->h - 2, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    pixelRGBA(dst, window->x + window->w - 4, window->y + window->h - 4, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    
    pixelRGBA(dst, window->x + window->w - 4, window->y + window->h - 2, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    pixelRGBA(dst, window->x + window->w - 6, window->y + window->h - 2, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    pixelRGBA(dst, window->x + window->w - 6, window->y + window->h - 4, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    pixelRGBA(dst, window->x + window->w - 8, window->y + window->h - 2, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    
    pixelRGBA(dst, window->x + window->w - 2, window->y + window->h - 4, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    pixelRGBA(dst, window->x + window->w - 2, window->y + window->h - 6, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    pixelRGBA(dst, window->x + window->w - 4, window->y + window->h - 6, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
    pixelRGBA(dst, window->x + window->w - 2, window->y + window->h - 8, window->color.resize_mark.r, window->color.resize_mark.g, window->color.resize_mark.b, window->color.resize_mark.a);
  }
}
static int g_SDL_EventFilter(const SDL_Event *event)
{
  g_window *window = NULL;
  g_widget *widget = NULL;
  
  /* set event type to undefined and x to -1 */
  g_event gui_event;
  
  int x, y, w, h;
  char buffer[G_TEXT_LENGTH] = {0};
  
  /* hide pop_up if cursor leaves it */
  if(selected_pop_up_window && event->type == SDL_MOUSEMOTION &&
     !(event->motion.x > selected_pop_up_window->x &&
     event->motion.x < selected_pop_up_window->x + selected_pop_up_window->w &&
     event->motion.y > selected_pop_up_window->y &&
     event->motion.y < selected_pop_up_window->y + selected_pop_up_window->h))
  {
    g_close_pop_up();
  }
  
  /* handle moving_window, clicked button and current input_box */
  if(event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT)
  {
    if(moving_window)
    {
      if(moving_window->event_function &&
         !(moving_window->x == temp_window_x &&
         moving_window->y == temp_window_y))
      {
        gui_event.type = G_WINDOW_MOVE;
        gui_event.x = moving_window->x - temp_window_x;
        gui_event.y = moving_window->y - temp_window_y;
        
        moving_window->event_function(&gui_event, moving_window, moving_window->event_data);
      }
      
      moving_window = NULL;
      
      return 0;
    }
    else if(resizing_window)
    {
      if(resizing_window->event_function &&
         !(resizing_window->w == temp_window_w &&
         resizing_window->h == temp_window_h))
      {
        gui_event.type = G_WINDOW_RESIZE;
        gui_event.x = resizing_window->w - temp_window_w;
        gui_event.y = resizing_window->h - temp_window_h;
        
        resizing_window->event_function(&gui_event, resizing_window, resizing_window->event_data);
      }
      
      resizing_window = NULL;
      
      return 0;
    }
    else if(clicked_button)
    {
      clicked_button = NULL;
      return 0;
    }
    else if(clicked_slider)
    {
      clicked_slider = NULL;
      return 0;
    }
  }
  else if(grab_keyboard_window)
  {
    if((event->type == SDL_KEYDOWN) || (event->type == SDL_KEYUP))
    {
      if(grab_keyboard_window->event_function)
      {
        if(event->type == SDL_KEYDOWN)
          gui_event.type = G_KEYDOWN;
        else if(event->type == SDL_KEYUP)
          gui_event.type = G_KEYUP;
        
        gui_event.key = event->key.keysym.sym;
        
        grab_keyboard_window->event_function(&gui_event, grab_keyboard_window, grab_keyboard_window->event_data);
      }
      
      return 0;
    }
    else if(event->type == SDL_MOUSEBUTTONDOWN &&
            event->button.button != SDL_BUTTON_WHEELUP &&
            event->button.button != SDL_BUTTON_WHEELDOWN)
    {
      grab_keyboard_window = NULL;
    }
  }
  else if(grab_keyboard_widget)
  {
    if((event->type == SDL_KEYDOWN) || (event->type == SDL_KEYUP))
    {
      if(grab_keyboard_widget->event_function)
      {
        if(event->type == SDL_KEYDOWN)
          gui_event.type = G_KEYDOWN;
        else if(event->type == SDL_KEYUP)
          gui_event.type = G_KEYUP;
        
        gui_event.key = event->key.keysym.sym;
        
        grab_keyboard_widget->event_function(&gui_event, grab_keyboard_widget, grab_keyboard_widget->event_data);
      }
      
      return 0;
    }
    else if(event->type == SDL_MOUSEBUTTONDOWN &&
            event->button.button != SDL_BUTTON_WHEELUP &&
            event->button.button != SDL_BUTTON_WHEELDOWN)
    {
      grab_keyboard_widget = NULL;
    }
  }
  else if(active_input_box)
  {
    if((event->type == SDL_KEYDOWN) || (event->type == SDL_KEYUP))
    {
      if(event->type == SDL_KEYDOWN)
      {
        if(event->key.keysym.sym == SDLK_BACKSPACE && active_input_box->input.cursor_pos > 0)
        {
          strcpy(buffer, active_input_box->input.text);
          
          active_input_box->input.text[active_input_box->input.cursor_pos - 1] = '\0';
          
          strcat(active_input_box->input.text, &buffer[active_input_box->input.cursor_pos]);
          
          active_input_box->input.cursor_pos--;
          if(active_input_box->input.cursor_pos < active_input_box->input.first_character)
            active_input_box->input.first_character--;
        }
        else if(event->key.keysym.sym == SDLK_DELETE && active_input_box->input.cursor_pos < strlen(active_input_box->input.text))
        {
          strcpy(buffer, active_input_box->input.text);
          
          active_input_box->input.text[active_input_box->input.cursor_pos] = '\0';
          
          strcat(active_input_box->input.text, &buffer[active_input_box->input.cursor_pos + 1]);
        }
        else if(event->key.keysym.sym == SDLK_LEFT && active_input_box->input.cursor_pos > 0)
        {
          active_input_box->input.cursor_pos--;
          if(active_input_box->input.cursor_pos < active_input_box->input.first_character)
            active_input_box->input.first_character--;
        }
        else if(event->key.keysym.sym == SDLK_RIGHT &&	active_input_box->input.cursor_pos < strlen(active_input_box->input.text))
        {
          active_input_box->input.cursor_pos++;
          if(active_input_box->input.cursor_pos > active_input_box->input.first_character + active_input_box->input.char_amount)
            active_input_box->input.first_character++;
        }
        else if(event->key.keysym.sym == SDLK_UP && active_input_box->input.cursor_pos > 0 &&
                active_input_box->input.char_amount != active_input_box->input.char_amount_w)
        {
          active_input_box->input.cursor_pos -= active_input_box->input.char_amount_w;
          
          if(active_input_box->input.cursor_pos < 0)
            active_input_box->input.cursor_pos = 0;
          if(active_input_box->input.cursor_pos < active_input_box->input.first_character)
          {
            active_input_box->input.first_character -= active_input_box->input.char_amount_w;
            if(active_input_box->input.first_character < 0)
              active_input_box->input.first_character = 0;
          }
        }
        else if(event->key.keysym.sym == SDLK_DOWN && active_input_box->input.cursor_pos < strlen(active_input_box->input.text) &&
                active_input_box->input.char_amount != active_input_box->input.char_amount_w)
        {
          active_input_box->input.cursor_pos += active_input_box->input.char_amount_w;
          
          if(active_input_box->input.cursor_pos > strlen(active_input_box->input.text))
            active_input_box->input.cursor_pos = strlen(active_input_box->input.text);
          if(active_input_box->input.cursor_pos > active_input_box->input.first_character + active_input_box->input.char_amount)
            active_input_box->input.first_character += active_input_box->input.char_amount_w;
        }
        else if(strlen(active_input_box->input.text) < G_TEXT_LENGTH - 1 &&
                !(active_input_box->input.limit > 0 && strlen(active_input_box->input.text) > active_input_box->input.limit) &&
                ((active_input_box->input.flags.numbers && event->key.keysym.unicode >= '0' && event->key.keysym.unicode <= '9') ||
                (active_input_box->input.flags.letters &&
                  ((active_input_box->input.flags.uppercase && event->key.keysym.unicode >= 'A' && event->key.keysym.unicode <= 'Z') ||
                  (active_input_box->input.flags.lowercase && event->key.keysym.unicode >= 'a' && event->key.keysym.unicode <= 'z'))) ||
                (active_input_box->input.flags.special_chars &&
                  ((event->key.keysym.unicode >= ' ' && event->key.keysym.unicode <= '/') ||
                  (event->key.keysym.unicode >= ':' && event->key.keysym.unicode <= '@') ||
                  (event->key.keysym.unicode >= '[' && event->key.keysym.unicode <= '`') ||
                  (event->key.keysym.unicode >= '{' && event->key.keysym.unicode <= '~')))))
        {
          strcpy(buffer, active_input_box->input.text);
          
          active_input_box->input.text[active_input_box->input.cursor_pos] = event->key.keysym.unicode;
          active_input_box->input.text[active_input_box->input.cursor_pos + 1] = '\0';
          
          strcat(active_input_box->input.text, &buffer[active_input_box->input.cursor_pos]);
          
          active_input_box->input.cursor_pos++;
          if(active_input_box->input.cursor_pos > active_input_box->input.first_character + active_input_box->input.char_amount)
            active_input_box->input.first_character++;
        }
      }
      
      if(active_input_box->event_function)
      {
        if(event->type == SDL_KEYDOWN)
          gui_event.type = G_KEYDOWN;
        else if(event->type == SDL_KEYUP)
          gui_event.type = G_KEYUP;
        
        gui_event.key = event->key.keysym.sym;
        
        active_input_box->event_function(&gui_event, active_input_box, active_input_box->event_data);
      }
      
      return 0;
    }
    else if(event->type == SDL_MOUSEBUTTONDOWN &&
            event->button.button != SDL_BUTTON_WHEELUP &&
            event->button.button != SDL_BUTTON_WHEELDOWN)
    {
      /* restore key repeat settings */
      if(active_input_box->input.flags.key_repeat)
        SDL_EnableKeyRepeat(key_repeat_delay, key_repeat_interval);
      
      active_input_box = NULL;
    }
  }
  else if(active_drop_down_list)
  {
    g_adjust_widget_position_drop_down_list_size(active_drop_down_list, &x, &y, &w, &h);
    
    /* check if event occurs inside the drop down list */
    if(((event->type == SDL_MOUSEMOTION) ||
        (event->type == SDL_MOUSEBUTTONDOWN)) &&
        event->button.x > + x &&
        event->button.x < + x + w &&
        event->button.y > + y &&
        event->button.y < + y + h)
    {
      /* choose drop down list item and close it */
      if(event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT)
      {
        active_drop_down_list->drop_down.current_item = (event->button.y - y)/G_CHAR_H;
        
        active_drop_down_list = NULL;
      }
      
      return 0;
    }
    else if(event->type == SDL_MOUSEBUTTONDOWN)
    {
      active_drop_down_list = NULL;
      
      return 0;
    }
  }
  else if(event->type == SDL_MOUSEMOTION)
  {
    if(moving_window)
    {
      moving_window->x += event->motion.xrel;
      moving_window->y += event->motion.yrel;
      
      return 0;
    }
    else if(resizing_window)
    {
      if(resizing_window->flags.keep_ratio)
      {
        resizing_window->w += (event->motion.xrel + event->motion.yrel)/2;
        resizing_window->h += (event->motion.xrel + event->motion.yrel)/2;
      }
      else
      {
        resizing_window->w += event->motion.xrel;
        resizing_window->h += event->motion.yrel;
      }
      
      /* correct window size */
      if(resizing_window->w < resizing_window->min_w)
        resizing_window->w = resizing_window->min_w;
      else if(resizing_window->max_w > 0 && resizing_window->w > resizing_window->max_w)
        resizing_window->w = resizing_window->max_w;
      
      if(resizing_window->h < resizing_window->min_h)
        resizing_window->h = resizing_window->min_h;
      else if(resizing_window->max_h > 0 && resizing_window->h > resizing_window->max_h)
        resizing_window->h = resizing_window->max_h;
      
      return 0;
    }
    else if(clicked_slider)
    {
      if(clicked_slider->type == G_TYPE_SLIDER_H)
      {
        g_adjust_widget_position_slider(clicked_slider, &x, &y, &w, NULL);
        
        if(clicked_slider->slider.flags.invert)
          clicked_slider->slider.value -= event->motion.xrel * (clicked_slider->slider.max_value/w);
        else
          clicked_slider->slider.value += event->motion.xrel * (clicked_slider->slider.max_value/w);
      }
      else if(clicked_slider->type == G_TYPE_SLIDER_V)
      {
        g_adjust_widget_position_slider(clicked_slider, &x, &y, NULL, &h);
        
        if(clicked_slider->slider.flags.invert)
          clicked_slider->slider.value += event->motion.yrel * (clicked_slider->slider.max_value/h);
        else
          clicked_slider->slider.value -= event->motion.yrel * (clicked_slider->slider.max_value/h);
      }
      
      /* correct slider value */
      if(clicked_slider->slider.value < 0)
        clicked_slider->slider.value = 0;
      else if(clicked_slider->slider.value > clicked_slider->slider.max_value)
        clicked_slider->slider.value = clicked_slider->slider.max_value;
      
      return 0;
    }
  }
  
  /* check all windows */
  for(window = last_window; window; window = window->prev)
  {
    /* check if event is inside a window */
    if(window->flags.visible &&
       ((event->type == SDL_MOUSEBUTTONDOWN) || (event->type == SDL_MOUSEMOTION))  &&
       event->button.x > window->x &&
       event->button.x < window->x + window->w &&
       event->button.y < window->y + window->h &&
       ((!window->flags.title_bar && event->button.y > window->y) ||
        (window->flags.title_bar && event->button.y > window->y - G_WINDOW_TITLE_BAR_HEIGHT)))
    {
      /* raise window */
      if(event->type == SDL_MOUSEBUTTONDOWN && 
         ((event->button.button == SDL_BUTTON_LEFT) || (event->button.button == SDL_BUTTON_RIGHT)))
        g_raise_window(window);
      
      /* check if event match to title bar */
      if(event->button.y < window->y)
      {
        if(event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT)
        {
          /* check if close button was clicked */
          if(window->flags.close_button && event->button.x > window->x + window->w - G_WINDOW_CLOSE_BUTTON_WIDTH)
          {
            /* call window event function */
            if(window->event_function)
            {
              gui_event.type = G_WINDOW_CLOSE;
              
              window->event_function(&gui_event, window, window->event_data);
            }
          }
          else if(window->flags.moveable)
          {
            temp_window_x = window->x;
            temp_window_y = window->y;
            
            /* set window to moving_window */
            moving_window = window;
          }
        }
        
        return 0;
      }
      
      /* check if resize mark was clicked */
      if(window->flags.resizable &&
         event->button.button == SDL_BUTTON_LEFT &&
         event->button.x < window->x + window->w &&
         event->button.x > window->x + window->w - G_MARGIN &&
         event->button.y < window->y + window->h &&
         event->button.y > window->y + window->h - G_MARGIN)
      {
        temp_window_w = window->w;
        temp_window_h = window->h;
        
        resizing_window = window;
        
        return 0;
      }
      
      /* set mouse event type */
      /* if this function has not returned already, */
      /*the event will be checked if it matchs to any widget inside this window */
      if(event->type == SDL_MOUSEMOTION)
      {
        gui_event.type = G_MOUSE_MOTION;
      }
      else
      {
        switch(event->button.button)
        {
          case SDL_BUTTON_LEFT:
            gui_event.type = G_CLICK_LEFT;
            break;
          case SDL_BUTTON_MIDDLE:
            gui_event.type = G_CLICK_MIDDLE;
            break;
          case SDL_BUTTON_RIGHT:
            gui_event.type = G_CLICK_RIGHT;
            break;
          case SDL_BUTTON_WHEELUP:
            gui_event.type = G_WHEEL_UP;
            break;
          case SDL_BUTTON_WHEELDOWN:
            gui_event.type = G_WHEEL_DOWN;
            break;
          default:
            gui_event.type = G_UNDEFINED;
        }
      }
      
      /* check each widget for collision */
      for(widget = window->last_widget; widget; widget = widget->prev)
      {
        /* continue if widget is not in window w/h range */
        if((widget->x > window->w - window->margin) || (widget->y > window->h - window->margin))
          continue;
        
        if(widget->type == G_TYPE_TEXT)
        {
          g_adjust_widget_position(widget, &x, &y, &w, &h);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + w &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + h)
          {
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
        else if(widget->type == G_TYPE_INPUT_BOX)
        {
          g_adjust_widget_position(widget, &x, &y, &w, &h);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + w &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + h)
          {
            if(event->type == SDL_MOUSEBUTTONDOWN &&
               event->button.button == SDL_BUTTON_LEFT)
            {
              g_enter_input_box(widget);
              
              /* set horizontal cursor position */
              active_input_box->input.cursor_pos = (event->button.x - window->x - x)/G_CHAR_W;
              
              if(active_input_box->input.cursor_pos > active_input_box->input.char_amount_w)
                active_input_box->input.cursor_pos = active_input_box->input.char_amount_w;
              
              /* if input box has more then 1 lines */
              if(h > G_INPUT_BOX_H)
              {
                active_input_box->input.cursor_pos += (event->button.y - window->y - y - G_CHAR_H)/G_CHAR_H * active_input_box->input.char_amount_w;
                
                if(event->button.y > window->y + y + G_MARGIN + G_CHAR_H &&
                   event->button.x < window->x + x + G_MARGIN)
                  active_input_box->input.cursor_pos++;
                
                if(active_input_box->input.cursor_pos > active_input_box->input.char_amount)
                  active_input_box->input.cursor_pos -= active_input_box->input.char_amount_w;
              }
              
              active_input_box->input.cursor_pos += widget->input.first_character;
              if(active_input_box->input.cursor_pos > strlen(widget->input.text))
                active_input_box->input.cursor_pos = strlen(widget->input.text);
            }
            
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up && widget != active_input_box)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
        else if(widget->type == G_TYPE_BUTTON)
        {
          g_adjust_widget_position_button(widget, &x, &y, &w, &h);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + w &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + h)
          {
            if(event->type == SDL_MOUSEBUTTONDOWN &&
               event->button.button == SDL_BUTTON_LEFT)
            {
              clicked_button = widget;
            }
            
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
        else if(widget->type == G_TYPE_CHECK_BOX)
        {
          g_adjust_widget_position_check_box(widget, &x, &y);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + G_CHECK_BOX_SIZE &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + G_CHECK_BOX_SIZE)
          {
            if(event->type == SDL_MOUSEBUTTONDOWN &&
               event->button.button == SDL_BUTTON_LEFT)
            {
              if(widget->check.state)
                widget->check.state = 0;
              else
                widget->check.state = 1;
            }
            
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
        else if(widget->type == G_TYPE_SURFACE)
        {
          g_adjust_widget_position_surface(widget, &x, &y, &w, &h);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + w &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + h)
          {
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
        else if(widget->type == G_TYPE_SLIDER_H)
        {
          g_adjust_widget_position_slider(widget, &x, &y, &w, NULL);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + w &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + G_SLIDER_H)
          {
            if(event->type == SDL_MOUSEBUTTONDOWN)
            {
              if(event->button.button == SDL_BUTTON_LEFT)
              {
                if(widget->slider.flags.invert)
                  widget->slider.value = widget->slider.max_value * ((float)(w - (event->button.x - window->x - x))/(float)w);
                else
                  widget->slider.value = widget->slider.max_value * ((float)(event->button.x - window->x - x)/(float)w);
                
                clicked_slider = widget;
              }
              else if(widget->slider.flags.mouse_wheel)
              {
                if(event->button.button == SDL_BUTTON_WHEELUP)
                {
                  if(widget->slider.flags.invert)
                    widget->slider.value -= widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  else
                    widget->slider.value += widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  
                  if(widget->slider.value < 0)
                    widget->slider.value = 0;
                  else if(widget->slider.value > widget->slider.max_value)
                    widget->slider.value = widget->slider.max_value;
                }
                else if(event->button.button == SDL_BUTTON_WHEELDOWN)
                {
                  if(widget->slider.flags.invert)
                    widget->slider.value += widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  else
                    widget->slider.value -= widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  
                  if(widget->slider.value < 0)
                    widget->slider.value = 0;
                  else if(widget->slider.value > widget->slider.max_value)
                    widget->slider.value = widget->slider.max_value;
                }
              }
            }
            
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
        else if(widget->type == G_TYPE_SLIDER_V)
        {
          g_adjust_widget_position_slider(widget, &x, &y, NULL, &h);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + G_SLIDER_H &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + h)
          {
            if(event->type == SDL_MOUSEBUTTONDOWN)
            {
              if(event->button.button == SDL_BUTTON_LEFT)
              {
                if(widget->slider.flags.invert)
                  widget->slider.value = widget->slider.max_value * ((float)(event->button.y - window->y - y)/(float)h);
                else
                  widget->slider.value = widget->slider.max_value * ((float)(h - (event->button.y - window->y - y))/(float)h);
                
                clicked_slider = widget;
              }
              else if(widget->slider.flags.mouse_wheel)
              {
                if(event->button.button == SDL_BUTTON_WHEELUP)
                {
                  if(widget->slider.flags.invert)
                    widget->slider.value -= widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  else
                    widget->slider.value += widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  
                  if(widget->slider.value < 0)
                    widget->slider.value = 0;
                  else if(widget->slider.value > widget->slider.max_value)
                    widget->slider.value = widget->slider.max_value;
                }
                else if(event->button.button == SDL_BUTTON_WHEELDOWN)
                {
                  if(widget->slider.flags.invert)
                    widget->slider.value += widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  else
                    widget->slider.value -= widget->slider.max_value * G_SLIDER_MOUSEWHEEL_STEP;
                  
                  if(widget->slider.value < 0)
                    widget->slider.value = 0;
                  else if(widget->slider.value > widget->slider.max_value)
                    widget->slider.value = widget->slider.max_value;
                }
              }
            }
            
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
        else if(widget->type == G_TYPE_DROP_DOWN_LIST && widget != active_drop_down_list)
        {
          g_adjust_widget_position_drop_down_list(widget, &x, &y, &w);
          
          /* check collision */
          if(event->button.x > window->x + x &&
             event->button.x < window->x + x + w &&
             event->button.y > window->y + y &&
             event->button.y < window->y + y + G_DROP_DOWN_LIST_SIZE)
          {
            if(event->type == SDL_MOUSEBUTTONDOWN)
            {
              if(event->button.button == SDL_BUTTON_LEFT &&
                 event->button.x > window->x + x + w - G_DROP_DOWN_LIST_SIZE)
              {
                active_drop_down_list = widget;
              }
              else if(widget->drop_down.flags.mouse_wheel &&
                      event->button.x < window->x + x + w - G_DROP_DOWN_LIST_SIZE)
              {
                if(event->button.button == SDL_BUTTON_WHEELUP &&
                  widget->drop_down.current_item > 0)
                {
                  widget->drop_down.current_item--;
                }
                else if(event->button.button == SDL_BUTTON_WHEELDOWN &&
                        widget->drop_down.current_item < widget->drop_down.max_item)
                {
                  widget->drop_down.current_item++;
                }
              }
            }
            
            /* mark widget pop_up as pop_up window */
            if(widget->pop_up)
            {
              if(event->type == SDL_MOUSEMOTION)
                g_mark_window_as_selected_pop_up(widget->pop_up, widget->pop_up_delay);
              else
                g_close_pop_up();
            }
            
            /* call widget event function */
            if(widget->event_function)
            {
              gui_event.x = event->button.x - window->x - x;
              gui_event.y = event->button.y - window->y - y;
              
              widget->event_function(&gui_event, widget, widget->event_data);
            }
            
            /* break out of widget loop, because event was processed */
            break;
          }
        }
      }
      
      /* if this function has not returned already due to a title bar or resize event */
      /* an widget is pointing to NULL, it does mean that 'event' has been compared with all widgets */
      /* without finding any match. If the window has a event function, the event is passed to it */
      if(!widget && window->event_function)
      {
        /* set gui_event x/y to mouse position inside the window */
        gui_event.x = event->button.x - window->x;
        gui_event.y = event->button.y - window->y;
        
        window->event_function(&gui_event, window, window->event_data);
      }
      
      return 0;
    }
  }
  
  /* return 1 if event is not matching to any window/widget */
  /* and function has reached end */
  return 1;
}
/* core functions end */

/* widget functions start */
g_widget *g_attach_text(g_window *window, const int x, const int y, const int w, const int h, const char *text)
{
  g_widget *widget = g_attach_raw_widged(window);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_TEXT;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  widget->w = w;
  widget->h = h;
  
  /* copy text */
  if(text)
  {
    strncpy(widget->text.text, text, G_TEXT_LENGTH);
    widget->text.text[G_TEXT_LENGTH - 1] = '\0';
  }
  else
    widget->text.text[0] = '\0';
  
  /* set colors to default */
  widget->text.color.text.r = g_defaults.text.color.text.r;
  widget->text.color.text.g = g_defaults.text.color.text.g;
  widget->text.color.text.b = g_defaults.text.color.text.b;
  widget->text.color.text.a = g_defaults.text.color.text.a;
  
  return widget;
}
g_widget *g_attach_input_box(g_window *window, const int x, const int y, const int w, const int h)
{
  g_widget *widget = g_attach_raw_widged(window);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_INPUT_BOX;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  widget->w = w;
  widget->h = h;
  
  /* set widget specific stuff */
  widget->input.text[0] = '\0';
  widget->input.first_character = 0;
  widget->input.limit = 0;
  
  widget->input.replace_character = g_defaults.input.replace_character;
  
  widget->input.key_repeat.delay = g_defaults.input.key_repeat.delay;
  widget->input.key_repeat.interval = g_defaults.input.key_repeat.interval;
  
  widget->input.flags.hide_text = g_defaults.input.flags.hide_text;
  widget->input.flags.numbers = g_defaults.input.flags.numbers;
  widget->input.flags.letters = g_defaults.input.flags.letters;
  widget->input.flags.uppercase = g_defaults.input.flags.uppercase;
  widget->input.flags.lowercase = g_defaults.input.flags.lowercase;
  widget->input.flags.special_chars = g_defaults.input.flags.special_chars;
  widget->input.flags.key_repeat = g_defaults.input.flags.key_repeat;
  
  /* set colors to default */
  widget->input.color.text.r = g_defaults.input.color.text.r;
  widget->input.color.text.g = g_defaults.input.color.text.g;
  widget->input.color.text.b = g_defaults.input.color.text.b;
  widget->input.color.text.a = g_defaults.input.color.text.a;
  
  widget->input.color.frame.r = g_defaults.input.color.frame.r;
  widget->input.color.frame.g = g_defaults.input.color.frame.g;
  widget->input.color.frame.b = g_defaults.input.color.frame.b;
  widget->input.color.frame.a = g_defaults.input.color.frame.a;
  
  widget->input.color.background.r = g_defaults.input.color.background.r;
  widget->input.color.background.g = g_defaults.input.color.background.g;
  widget->input.color.background.b = g_defaults.input.color.background.b;
  widget->input.color.background.a = g_defaults.input.color.background.a;
  
  widget->input.color.background_active.r = g_defaults.input.color.background_active.r;
  widget->input.color.background_active.g = g_defaults.input.color.background_active.g;
  widget->input.color.background_active.b = g_defaults.input.color.background_active.b;
  widget->input.color.background_active.a = g_defaults.input.color.background_active.a;
  
  widget->input.color.cursor.r = g_defaults.input.color.cursor.r;
  widget->input.color.cursor.g = g_defaults.input.color.cursor.g;
  widget->input.color.cursor.b = g_defaults.input.color.cursor.b;
  widget->input.color.cursor.a = g_defaults.input.color.cursor.a;
  
  return widget;
}
g_widget *g_attach_button(g_window *window, const int x, const int y, const char *text)
{
  g_widget *widget = g_attach_raw_widged(window);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_BUTTON;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  
  /* copy text */
  if(text)
  {
    strncpy(widget->button.text, text, G_TEXT_LENGTH);
    widget->button.text[G_TEXT_LENGTH - 1] = '\0';
  }
  else
    widget->button.text[0] = '\0';
  
  /* set colors to default */
  widget->button.color.text.r = g_defaults.button.color.text.r;
  widget->button.color.text.g = g_defaults.button.color.text.g;
  widget->button.color.text.b = g_defaults.button.color.text.b;
  widget->button.color.text.a = g_defaults.button.color.text.a;
  
  widget->button.color.frame.r = g_defaults.button.color.frame.r;
  widget->button.color.frame.g = g_defaults.button.color.frame.g;
  widget->button.color.frame.b = g_defaults.button.color.frame.b;
  widget->button.color.frame.a = g_defaults.button.color.frame.a;
  
  widget->button.color.top.r = g_defaults.button.color.top.r;
  widget->button.color.top.g = g_defaults.button.color.top.g;
  widget->button.color.top.b = g_defaults.button.color.top.b;
  widget->button.color.top.a = g_defaults.button.color.top.a;
  
  widget->button.color.bottom.r = g_defaults.button.color.bottom.r;
  widget->button.color.bottom.g = g_defaults.button.color.bottom.g;
  widget->button.color.bottom.b = g_defaults.button.color.bottom.b;
  widget->button.color.bottom.a = g_defaults.button.color.bottom.a;
  
  return widget;
}
g_widget *g_attach_check_box(g_window *window, const int x, const int y)
{
  g_widget *widget = g_attach_raw_widged(window);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_CHECK_BOX;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  
  /* widget specific stuff */
  widget->check.state = 0;
  
  /* set colors to default */
  widget->check.color.mark.r = g_defaults.check.color.mark.r;
  widget->check.color.mark.g = g_defaults.check.color.mark.g;
  widget->check.color.mark.b = g_defaults.check.color.mark.b;
  widget->check.color.mark.a = g_defaults.check.color.mark.a;
  
  widget->check.color.frame.r = g_defaults.check.color.frame.r;
  widget->check.color.frame.g = g_defaults.check.color.frame.g;
  widget->check.color.frame.b = g_defaults.check.color.frame.b;
  widget->check.color.frame.a = g_defaults.check.color.frame.a;
  
  widget->check.color.background.r = g_defaults.check.color.background.r;
  widget->check.color.background.g = g_defaults.check.color.background.g;
  widget->check.color.background.b = g_defaults.check.color.background.b;
  widget->check.color.background.a = g_defaults.check.color.background.a;
  
  return widget;
}
g_widget *g_attach_surface(g_window *window, const int x, const int y, const int w, const int h, SDL_Surface *surface)
{
  g_widget *widget = g_attach_raw_widged(window);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_SURFACE;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  widget->w = w;
  widget->h = h;
  
  /* widget specific stuff */
  widget->surface.src_x = 0;
  widget->surface.src_y = 0;
  widget->surface.surface = surface;
  
  return widget;
}
g_widget *g_attach_slider_h(g_window *window, const int x, const int y, const int w, const double value, const double max_value)
{
  g_widget *widget = g_attach_slider_raw(window, value, max_value);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_SLIDER_H;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  widget->w = w;
  
  return widget;
}
g_widget *g_attach_slider_v(g_window *window, const int x, const int y, const int h, const double value, const double max_value)
{
  g_widget *widget = g_attach_slider_raw(window, value, max_value);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_SLIDER_V;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  widget->h = h;
  
  return widget;
}
g_widget *g_attach_drop_down_list(g_window *window, const int x, const int y, const int w, const char *text)
{
  g_widget *widget = g_attach_raw_widged(window);
  
  if(!widget)
    return NULL;
  
  /* set widget type */
  widget->type = G_TYPE_DROP_DOWN_LIST;
  
  /* set coordinates */
  widget->x = x;
  widget->y = y;
  widget->w = w;
  
  /* copy text */
  if(text)
  {
    strncpy(widget->drop_down.text, text, G_TEXT_LENGTH);
    widget->drop_down.text[G_TEXT_LENGTH - 1] = '\0';
  }
  else
    widget->drop_down.text[0] = '\0';
  
  /* widget specific stuff */
  widget->drop_down.flags.mouse_wheel = g_defaults.drop_down.flags.mouse_wheel;
  
  widget->drop_down.current_item = 0;
  widget->drop_down.max_item = g_count_lines(widget->drop_down.text) - 1;
  
  /* set colors to default */
  widget->drop_down.color.text.r = g_defaults.drop_down.color.text.r;
  widget->drop_down.color.text.g = g_defaults.drop_down.color.text.g;
  widget->drop_down.color.text.b = g_defaults.drop_down.color.text.b;
  widget->drop_down.color.text.a = g_defaults.drop_down.color.text.a;
  
  widget->drop_down.color.frame.r = g_defaults.drop_down.color.frame.r;
  widget->drop_down.color.frame.g = g_defaults.drop_down.color.frame.g;
  widget->drop_down.color.frame.b = g_defaults.drop_down.color.frame.b;
  widget->drop_down.color.frame.a = g_defaults.drop_down.color.frame.a;
  
  widget->drop_down.color.background.r = g_defaults.drop_down.color.background.r;
  widget->drop_down.color.background.g = g_defaults.drop_down.color.background.g;
  widget->drop_down.color.background.b = g_defaults.drop_down.color.background.b;
  widget->drop_down.color.background.a = g_defaults.drop_down.color.background.a;
  
  widget->drop_down.color.background_list.r = g_defaults.drop_down.color.background_list.r;
  widget->drop_down.color.background_list.g = g_defaults.drop_down.color.background_list.g;
  widget->drop_down.color.background_list.b = g_defaults.drop_down.color.background_list.b;
  widget->drop_down.color.background_list.a = g_defaults.drop_down.color.background_list.a;
  
  widget->drop_down.color.highlight.r = g_defaults.drop_down.color.highlight.r;
  widget->drop_down.color.highlight.g = g_defaults.drop_down.color.highlight.g;
  widget->drop_down.color.highlight.b = g_defaults.drop_down.color.highlight.b;
  widget->drop_down.color.highlight.a = g_defaults.drop_down.color.highlight.a;
  
  widget->drop_down.color.arrow.r = g_defaults.drop_down.color.arrow.r;
  widget->drop_down.color.arrow.g = g_defaults.drop_down.color.arrow.g;
  widget->drop_down.color.arrow.b = g_defaults.drop_down.color.arrow.b;
  widget->drop_down.color.arrow.a = g_defaults.drop_down.color.arrow.a;
  
  return widget;
}
void g_destroy_widget(g_widget *widget)
{
  if(!widget)
    return;
  
  g_window *window = widget->window;
  
  if(widget == window->first_widget)
  {
    window->first_widget = widget->next;
    
    if(widget->next)
      widget->next->prev = NULL;
    else
      window->last_widget = NULL;
  }
  else if(widget == window->last_widget)
  {
    widget->prev->next = NULL;
    window->last_widget = widget->prev;
  }
  else
  {
    widget->next->prev = widget->prev;
    widget->prev->next = widget->next;
  }
  
  free(widget);
}

void g_enter_input_box(g_widget *widget)
{
  if(!(widget && widget->type == G_TYPE_INPUT_BOX))
    return;
  
  int x, y, w, h;
  
  /* mark widget as current text box */
  active_input_box = widget;
  
  /* adjust x, y, w and h */
  g_adjust_widget_position(widget, &x, &y, &w, &h);
  
  x += G_MARGIN;
  y += G_MARGIN;
  w -= G_MARGIN * 2;
  h -= G_MARGIN;
  
  /* caclulate how many characters fit in the input box */
  if(h/G_CHAR_H > 0)
    active_input_box->input.char_amount = (w/G_CHAR_W + 1) * (h/G_CHAR_H);
  else
    active_input_box->input.char_amount = w/G_CHAR_W + 1;
  
  active_input_box->input.char_amount_w = w/G_CHAR_W + 1;
  
  /* enable key repeat */
  if(widget->input.flags.key_repeat)
  {
    /* save old settings */
    SDL_GetKeyRepeat(&key_repeat_delay, &key_repeat_interval);
    
    SDL_EnableKeyRepeat(widget->input.key_repeat.delay, widget->input.key_repeat.interval);
  }
}
void g_leave_input_box(void)
{
  if(active_input_box)
    active_input_box = NULL;
}
/* widget functions end */

/* window functions start */
g_window *g_create_window(const int x, const int y, const int w, const int h, const char *title)
{
  /* allocate memory and set new window to last window */
  if(!first_window)
  {
    first_window = malloc(sizeof(struct g_window));
    if(!first_window)
      return NULL;
    
    first_window->next = NULL;
    first_window->prev = NULL;
    last_window = first_window;
  }
  else
  {
    last_window->next = malloc(sizeof(struct g_window));
    if(!last_window->next)
      return NULL;
    
    last_window->next->prev = last_window;
    last_window->next->next = NULL;
    last_window = last_window->next;
  }
  
  /* set window properties */
  last_window->x = x;
  last_window->y = y;
  last_window->w = w;
  last_window->h = h;
  
  last_window->min_w = g_defaults.window.min_w;
  last_window->min_h = g_defaults.window.min_h;
  
  last_window->max_w = g_defaults.window.max_w;
  last_window->max_h = g_defaults.window.max_h;
  
  /* copy title, shorten if needed */
  if(title)
  {
    strncpy(last_window->title, title, G_WINDOW_TITLE_LENGTH);
    last_window->title[G_WINDOW_TITLE_LENGTH - 1] = '\0';
  }
  else
    last_window->title[0] = '\0';
  
  /* set window flags */
  last_window->flags.title_bar = g_defaults.window.flags.title_bar;
  last_window->flags.close_button = g_defaults.window.flags.close_button;
  last_window->flags.visible = g_defaults.window.flags.visible;
  last_window->flags.moveable = g_defaults.window.flags.moveable;
  last_window->flags.resizable = g_defaults.window.flags.resizable;
  last_window->flags.keep_ratio = g_defaults.window.flags.keep_ratio;
  
  last_window->close_button_character = g_defaults.window.close_button_character;
  last_window->margin = g_defaults.window.margin;
  
  /* set window colors */
  last_window->color.frame.r = g_defaults.window.color.frame.r;
  last_window->color.frame.g = g_defaults.window.color.frame.g;
  last_window->color.frame.b = g_defaults.window.color.frame.b;
  last_window->color.frame.a = g_defaults.window.color.frame.a;
  
  last_window->color.background.r = g_defaults.window.color.background.r;
  last_window->color.background.g = g_defaults.window.color.background.g;
  last_window->color.background.b = g_defaults.window.color.background.b;
  last_window->color.background.a = g_defaults.window.color.background.a;
  
  last_window->color.resize_mark.r = g_defaults.window.color.resize_mark.r;
  last_window->color.resize_mark.g = g_defaults.window.color.resize_mark.g;
  last_window->color.resize_mark.b = g_defaults.window.color.resize_mark.b;
  last_window->color.resize_mark.a = g_defaults.window.color.resize_mark.a;
  
  last_window->color.title_text.r = g_defaults.window.color.title_text.r;
  last_window->color.title_text.g = g_defaults.window.color.title_text.g;
  last_window->color.title_text.b = g_defaults.window.color.title_text.b;
  last_window->color.title_text.a = g_defaults.window.color.title_text.a;
  
  last_window->color.title_bar_top.r = g_defaults.window.color.title_bar_top.r;
  last_window->color.title_bar_top.g = g_defaults.window.color.title_bar_top.g;
  last_window->color.title_bar_top.b = g_defaults.window.color.title_bar_top.b;
  last_window->color.title_bar_top.a = g_defaults.window.color.title_bar_top.a;
  
  last_window->color.title_bar_bottom.r = g_defaults.window.color.title_bar_bottom.r;
  last_window->color.title_bar_bottom.g = g_defaults.window.color.title_bar_bottom.g;
  last_window->color.title_bar_bottom.b = g_defaults.window.color.title_bar_bottom.b;
  last_window->color.title_bar_bottom.a = g_defaults.window.color.title_bar_bottom.a;
  
  last_window->color.close_button_text.r = g_defaults.window.color.close_button_text.r;
  last_window->color.close_button_text.g = g_defaults.window.color.close_button_text.g;
  last_window->color.close_button_text.b = g_defaults.window.color.close_button_text.b;
  last_window->color.close_button_text.a = g_defaults.window.color.close_button_text.a;
  
  last_window->color.close_button_top.r = g_defaults.window.color.close_button_top.r;
  last_window->color.close_button_top.g = g_defaults.window.color.close_button_top.g;
  last_window->color.close_button_top.b = g_defaults.window.color.close_button_top.b;
  last_window->color.close_button_top.a = g_defaults.window.color.close_button_top.a;
  
  last_window->color.close_button_bottom.r = g_defaults.window.color.close_button_bottom.r;
  last_window->color.close_button_bottom.g = g_defaults.window.color.close_button_bottom.g;
  last_window->color.close_button_bottom.b = g_defaults.window.color.close_button_bottom.b;
  last_window->color.close_button_bottom.a = g_defaults.window.color.close_button_bottom.a;
  
  /* widgets */
  last_window->first_widget = NULL;
  last_window->last_widget = NULL;
  
  last_window->event_function = NULL;
  last_window->event_data = NULL;
  
  return last_window;
}
void g_destroy_window(g_window *window)
{
  if(!window)
    return;
  
  if(window == first_window)
  {
    first_window = window->next;
    
    if(window->next)
      window->next->prev = NULL;
    else
      last_window = NULL;
  }
  else if(window == last_window)
  {
    window->prev->next = NULL;
    last_window = window->prev;
  }
  else
  {
    window->next->prev = window->prev;
    window->prev->next = window->next;
  }
  
  /* destroy all widgets */
  while(window->first_widget)
    g_destroy_widget(window->first_widget);
  free(window);
}
void g_raise_window(g_window *window)
{
  if(!window)
    return;
  
  /* return if window is already on top */
  if(window == last_window)
    return;
  
  if(window->prev != NULL)
    window->prev->next = window->next;
  window->next->prev = window->prev;
  
  if(window == first_window)
    first_window = window->next;
  
  last_window->next = window;
  window->prev = last_window;
  window->next = NULL;
  last_window = window;
}
void g_maximize_window(g_window *window)
{
  g_maximize_window_h(window);
  g_maximize_window_v(window);
}
void g_maximize_window_h(g_window *window)
{
  if(!window)
    return;
  
  SDL_Surface *dst = SDL_GetVideoSurface();
  
  /* set window w */
  window->w = dst->w;
  
  /* cweck if window width was a limit */
  if(window->max_w > 0 && window->w > window->max_w)
    window->w = window->max_w;
  
  /* adjust x position */
  if(window->x < 0)
    window->x = 0;
  else if(window->x + window->w > dst->w)
    window->x = dst->w - window->w;
}
void g_maximize_window_v(g_window *window)
{
  if(!window)
    return;
  
  SDL_Surface *dst = SDL_GetVideoSurface();
  
  /* set window h */
  window->h = dst->h;
  if(window->flags.title_bar)
    window->h -= G_WINDOW_TITLE_BAR_HEIGHT;
  
  /* check if window height has a limit */
  if(window->max_h > 0 && window->h > window->max_h)
    window->h = window->max_h;
  
  /* adjust y position */
  if(window->flags.title_bar && window->y < G_WINDOW_TITLE_BAR_HEIGHT)
    window->y = G_WINDOW_TITLE_BAR_HEIGHT;
  else if(!window->flags.title_bar && window->y < 0)
    window->y = 0;
  else if(window->y + window->h > dst->h)
    window->y = dst->h - window->h;
}

/* pop_up window functions */
g_window *g_create_pop_up_window(const int w, const int h)
{
  g_window *window = g_create_window(0, 0, w, h, NULL);
  
  if(!window)
    return NULL;
  
  /* set pop_up flags */
  window->flags.title_bar = 0;
  window->flags.close_button = 0;
  window->flags.visible = 0;
  window->flags.moveable = 0;
  window->flags.resizable = 0;
  window->flags.keep_ratio = 0;
  
  /* set colors */
  window->color.frame.r = g_defaults.pop_up.color.frame.r;
  window->color.frame.g = g_defaults.pop_up.color.frame.g;
  window->color.frame.b = g_defaults.pop_up.color.frame.b;
  window->color.frame.a = g_defaults.pop_up.color.frame.a;
  
  window->color.background.r = g_defaults.pop_up.color.background.r;
  window->color.background.g = g_defaults.pop_up.color.background.g;
  window->color.background.b = g_defaults.pop_up.color.background.b;
  window->color.background.a = g_defaults.pop_up.color.background.a;
  
  return window;
}
void g_open_pop_up(g_window *window)
{
  SDL_Surface *dst = SDL_GetVideoSurface();
  int x, y;
  
  if(!window)
    return;
  
  /* deactivate old pop_up */
  g_close_pop_up();
  
  /* "pop_up" window */
  selected_pop_up_window = window;
  window->flags.visible = 1;
  g_raise_window(window);
  
  /* get mouse position */
  SDL_GetMouseState(&x, &y);
  
  /* set position, pop_up cant be outside video surface */
  if(x - G_MARGIN + window->w > dst->w)
    window->x = x + G_MARGIN - window->w;
  else
    window->x = x - G_MARGIN;
  
  if(y - G_MARGIN + window->h > dst->h)
    window->y = y + G_MARGIN - window->h;
  else
    window->y = y - G_MARGIN;
}
void g_close_pop_up(void)
{
  if(selected_pop_up_window)
  {
    selected_pop_up_window->flags.visible = 0;
    selected_pop_up_window = NULL;
  }
}
/* window functions end */

/* engine functions */
int g_init_everything(void)
{
  /* set defaults for the first time */
  g_reset_settings();
  
  /* SDL settings */
  SDL_EnableUNICODE(SDL_ENABLE);
  SDL_SetEventFilter(g_SDL_EventFilter);
  
  atexit(g_destroy_everything);
  return 1;
}
void g_destroy_everything(void)
{
  while(first_window)
    g_destroy_window(first_window);
}
void g_draw_everything(SDL_Surface *dst)
{
  g_window *window = NULL;
  
  int x, y, w, h;
  int temp_x, temp_y;
  
  /* return if dest surface is NULL */
  if(!dst)
    return;
  
  /* activate pop_up window */
  if(selected_pop_up_window && !selected_pop_up_window->flags.visible &&
     SDL_GetTicks() - selected_pop_up_timestop > selected_pop_up_delay)
    g_open_pop_up(selected_pop_up_window);
  
  /* draw each window */
  for(window = first_window; window; window = window->next)
    g_draw_window(dst, window);
  
  /* draw active drop down list */
  if(active_drop_down_list)
  {
    g_adjust_widget_position_drop_down_list_size(active_drop_down_list, &x, &y, &w, &h);
    
    /* draw background */
    boxRGBA(dst, x, y, x + w - 1, y + h,
            active_drop_down_list->drop_down.color.background_list.r, active_drop_down_list->drop_down.color.background_list.g,
            active_drop_down_list->drop_down.color.background_list.b, active_drop_down_list->drop_down.color.background_list.a);
    
    /* draw frame */
    rectangleRGBA(dst, x - 1, y, x + w, y + h + 1,
                  active_drop_down_list->drop_down.color.frame.r, active_drop_down_list->drop_down.color.frame.g,
                  active_drop_down_list->drop_down.color.frame.b, active_drop_down_list->drop_down.color.frame.a);
    
    /* highlight current item */
    SDL_GetMouseState(&temp_x, &temp_y);
    if(temp_x > x && temp_x < x + w && temp_y > y && temp_y < y + h)
    {
      boxRGBA(dst, x, y + ((temp_y - y)/G_CHAR_H) * G_CHAR_H + 1,
              x + w - 1, y + ((temp_y - y)/G_CHAR_H) * G_CHAR_H + G_CHAR_H,
              active_drop_down_list->drop_down.color.highlight.r, active_drop_down_list->drop_down.color.highlight.g,
              active_drop_down_list->drop_down.color.highlight.b, active_drop_down_list->drop_down.color.highlight.a);
    }
    
    /* adjust position */
    x += G_MARGIN;
    y += G_MARGIN/2;
    w -= G_MARGIN;
    h += G_MARGIN/2;
    
    /* draw text */
    g_draw_text(dst, active_drop_down_list->drop_down.text, x, y, w, h,
                active_drop_down_list->drop_down.color.text.r, active_drop_down_list->drop_down.color.text.g,
                active_drop_down_list->drop_down.color.text.b, active_drop_down_list->drop_down.color.text.a);
  }
}
void g_reset_settings(void)
{
  /* minimal window size */
  g_defaults.window.min_w = 80;
  g_defaults.window.min_h = 50;
  
  g_defaults.window.max_w = 0;
  g_defaults.window.max_h = 0;
  
  /* window flags */
  g_defaults.window.flags.title_bar = 1;
  g_defaults.window.flags.close_button = 1;
  g_defaults.window.flags.visible = 1;
  g_defaults.window.flags.moveable = 1;
  g_defaults.window.flags.resizable = 0;
  g_defaults.window.flags.keep_ratio = 0;
  
  /* window properties */
  g_defaults.window.close_button_character = 'X';
  g_defaults.window.margin = 8;
  
  /* window colors */
  g_defaults.window.color.frame.r = 80;
  g_defaults.window.color.frame.g = 80;
  g_defaults.window.color.frame.b = 80;
  g_defaults.window.color.frame.a = 200;
  
  g_defaults.window.color.background.r = 50;
  g_defaults.window.color.background.g = 50;
  g_defaults.window.color.background.b = 50;
  g_defaults.window.color.background.a = 150;
  
  g_defaults.window.color.resize_mark.r = 80;
  g_defaults.window.color.resize_mark.g = 80;
  g_defaults.window.color.resize_mark.b = 80;
  g_defaults.window.color.resize_mark.a = 255;
  
  g_defaults.window.color.title_text.r = 180;
  g_defaults.window.color.title_text.g = 180;
  g_defaults.window.color.title_text.b = 180;
  g_defaults.window.color.title_text.a = 255;
  
  g_defaults.window.color.title_bar_top.r = 81;
  g_defaults.window.color.title_bar_top.g = 81;
  g_defaults.window.color.title_bar_top.b = 81;
  g_defaults.window.color.title_bar_top.a = 255;
  
  g_defaults.window.color.title_bar_bottom.r = 50;
  g_defaults.window.color.title_bar_bottom.g = 50;
  g_defaults.window.color.title_bar_bottom.b = 50;
  g_defaults.window.color.title_bar_bottom.a = 255;
  
  g_defaults.window.color.close_button_text.r = 180;
  g_defaults.window.color.close_button_text.g = 180;
  g_defaults.window.color.close_button_text.b = 180;
  g_defaults.window.color.close_button_text.a = 255;
  
  g_defaults.window.color.close_button_top.r = 81;
  g_defaults.window.color.close_button_top.g = 81;
  g_defaults.window.color.close_button_top.b = 81;
  g_defaults.window.color.close_button_top.a = 255;
  
  g_defaults.window.color.close_button_bottom.r = 50;
  g_defaults.window.color.close_button_bottom.g = 50;
  g_defaults.window.color.close_button_bottom.b = 50;
  g_defaults.window.color.close_button_bottom.a = 255;
  
  /* widget defaults */
  /* text field */
  g_defaults.text.color.text.r = 180;
  g_defaults.text.color.text.g = 180;
  g_defaults.text.color.text.b = 180;
  g_defaults.text.color.text.a = 255;
  
  /* input box */
  g_defaults.input.replace_character = '*';
  
  g_defaults.input.key_repeat.delay = 350;
  g_defaults.input.key_repeat.interval = 30;
  
  g_defaults.input.flags.hide_text = 0;
  g_defaults.input.flags.numbers = 1;
  g_defaults.input.flags.letters = 1;
  g_defaults.input.flags.uppercase = 1;
  g_defaults.input.flags.lowercase = 1;
  g_defaults.input.flags.special_chars = 1;
  g_defaults.input.flags.key_repeat = 1;
  
  g_defaults.input.color.text.r = 180;
  g_defaults.input.color.text.g = 180;
  g_defaults.input.color.text.b = 180;
  g_defaults.input.color.text.a = 255;
  
  g_defaults.input.color.frame.r = 80;
  g_defaults.input.color.frame.g = 80;
  g_defaults.input.color.frame.b = 80;
  g_defaults.input.color.frame.a = 200;
  
  g_defaults.input.color.background.r = 50;
  g_defaults.input.color.background.g = 50;
  g_defaults.input.color.background.b = 50;
  g_defaults.input.color.background.a = 150;
  
  g_defaults.input.color.background_active.r = 20;
  g_defaults.input.color.background_active.g = 20;
  g_defaults.input.color.background_active.b = 20;
  g_defaults.input.color.background_active.a = 150;
  
  g_defaults.input.color.cursor.r = 200;
  g_defaults.input.color.cursor.g = 200;
  g_defaults.input.color.cursor.b = 200;
  g_defaults.input.color.cursor.a = 255;
  
  /* button */
  g_defaults.button.color.text.r = 180;
  g_defaults.button.color.text.g = 180;
  g_defaults.button.color.text.b = 180;
  g_defaults.button.color.text.a = 255;
  
  g_defaults.button.color.frame.r = 80;
  g_defaults.button.color.frame.g = 80;
  g_defaults.button.color.frame.b = 80;
  g_defaults.button.color.frame.a = 200;
  
  g_defaults.button.color.top.r = 100;
  g_defaults.button.color.top.g = 100;
  g_defaults.button.color.top.b = 100;
  g_defaults.button.color.top.a = 255;
  
  g_defaults.button.color.bottom.r = 80;
  g_defaults.button.color.bottom.g = 80;
  g_defaults.button.color.bottom.b = 80;
  g_defaults.button.color.bottom.a = 255;
  
  /* check box */
  g_defaults.check.color.mark.r = 180;
  g_defaults.check.color.mark.g = 180;
  g_defaults.check.color.mark.b = 180;
  g_defaults.check.color.mark.a = 255;
  
  g_defaults.check.color.frame.r = 80;
  g_defaults.check.color.frame.g = 80;
  g_defaults.check.color.frame.b = 80;
  g_defaults.check.color.frame.a = 200;
  
  g_defaults.check.color.background.r = 20;
  g_defaults.check.color.background.g = 20;
  g_defaults.check.color.background.b = 20;
  g_defaults.check.color.background.a = 150;
  
  /* slider */
  g_defaults.slider.flags.invert = 0;
  g_defaults.slider.flags.mouse_wheel = 1;
  
  g_defaults.slider.color.slider.r = 20;
  g_defaults.slider.color.slider.g = 20;
  g_defaults.slider.color.slider.b = 20;
  g_defaults.slider.color.slider.a = 150;
  
  g_defaults.slider.color.frame.r = 80;
  g_defaults.slider.color.frame.g = 80;
  g_defaults.slider.color.frame.b = 80;
  g_defaults.slider.color.frame.a = 200;
  
  g_defaults.slider.color.line.r = 80;
  g_defaults.slider.color.line.g = 80;
  g_defaults.slider.color.line.b = 80;
  g_defaults.slider.color.line.a = 200;
  
  /* drop down list */
  g_defaults.drop_down.flags.mouse_wheel = 1;
  
  g_defaults.drop_down.color.text.r = 180;
  g_defaults.drop_down.color.text.g = 180;
  g_defaults.drop_down.color.text.b = 180;
  g_defaults.drop_down.color.text.a = 255;
  
  g_defaults.drop_down.color.frame.r = 80;
  g_defaults.drop_down.color.frame.g = 80;
  g_defaults.drop_down.color.frame.b = 80;
  g_defaults.drop_down.color.frame.a = 200;
  
  g_defaults.drop_down.color.background.r = 50;
  g_defaults.drop_down.color.background.g = 50;
  g_defaults.drop_down.color.background.b = 50;
  g_defaults.drop_down.color.background.a = 150;
  
  g_defaults.drop_down.color.background_list.r = 20;
  g_defaults.drop_down.color.background_list.g = 20;
  g_defaults.drop_down.color.background_list.b = 20;
  g_defaults.drop_down.color.background_list.a = 150;
  
  g_defaults.drop_down.color.highlight.r = 80;
  g_defaults.drop_down.color.highlight.g = 80;
  g_defaults.drop_down.color.highlight.b = 80;
  g_defaults.drop_down.color.highlight.a = 200;
  
  g_defaults.drop_down.color.arrow.r = 180;
  g_defaults.drop_down.color.arrow.g = 180;
  g_defaults.drop_down.color.arrow.b = 180;
  g_defaults.drop_down.color.arrow.a = 255;
  
  /* pop_up */
  g_defaults.pop_up_delay = 900;
  
  g_defaults.pop_up.color.frame.r = 80;
  g_defaults.pop_up.color.frame.g = 80;
  g_defaults.pop_up.color.frame.b = 80;
  g_defaults.pop_up.color.frame.a = 200;
  
  g_defaults.pop_up.color.background.r = 50;
  g_defaults.pop_up.color.background.g = 50;
  g_defaults.pop_up.color.background.b = 50;
  g_defaults.pop_up.color.background.a = 150;
}
g_setting_struct *g_get_setting_struct(void)
{
  return &g_defaults;
}

/* special functions */
/* to read in a text, use g_enter_input_bux() instead */
void g_window_grab_keyboard(g_window *window)
{
  grab_keyboard_window = window;
}
void g_widget_grab_keyboard(g_widget *widget)
{
  grab_keyboard_widget = widget;
}

int g_count_lines(const char *text)
{
  if(!text)
    return 0;
  
  int counter, lines;
  counter = 0;
  lines = 1;
  
  while(text[counter] != '\0')
  {
    if(text[counter] == '\n')
      lines++;
    
    counter++;
  }
  
  return lines;
}
int g_get_line(const char *text, const int line)
{
  if((!text) || (line < 0))
    return -1;
  
  int counter, counter2;
  counter = counter2 = 0;
  
  while(text[counter] != '\0' && counter2 < line)
  {
    if(text[counter] == '\n')
      counter2++;
    
    counter++;
  }
  
  /* return -1 if counter reached end without finding line */
  if(counter2 < line)
    return -1;
  else
    return counter;
}
