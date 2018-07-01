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

#ifndef GUI_ENGINE_H
#define GUI_ENGINE_H

#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <string.h>

/* gui engine defines */
/* changing this values can make the windows and widgets look shifted */
#define G_MARGIN 8
#define G_CHAR_W 8
#define G_CHAR_H 12

struct g_color_struct{
  Uint8 r, g, b, a;
};

/* event types */
typedef enum{
  G_UNDEFINED,
  
  G_WINDOW_CLOSE,
  G_WINDOW_MOVE,
  G_WINDOW_RESIZE,
  
  G_CLICK_LEFT,
  G_CLICK_MIDDLE,
  G_CLICK_RIGHT,
  G_WHEEL_UP,
  G_WHEEL_DOWN,
  G_MOUSE_MOTION,
  
  G_KEYDOWN,
  G_KEYUP
}g_event_type;

typedef struct{
  g_event_type type;
  
  /* if this event is a window move or resize event, the difference between previous */
  /* position/size and new position/size is stored in x/y */
  
  /* in case this event is caused by a mouse event inside a widget, the position of the mouse is stored in x and y */
  /* this contains only the position inside the widget and can differ from the original mouse position on the video surface */
  /* to get the original mouse position, you have to add window and widget position to this value */
  int x, y;
  
  /* key pressed inside a input box */
  /* it is usefull if you want to leave the input box when the user hits a special key */
  SDLKey key;
}g_event;

/* widget defines */
#define G_TEXT_LENGTH 4096
#define G_CHECK_BOX_SIZE 15
#define G_DROP_DOWN_LIST_SIZE 20

/* height for single line input box */
#define G_INPUT_BOX_H 22

/* slider w and h are swapped on vertical slider */
#define G_SLIDER_W 15
#define G_SLIDER_H 10
#define G_SLIDER_THICKNESS 2
#define G_SLIDER_MOUSEWHEEL_STEP 0.05

/* widget types */
typedef enum{
  G_TYPE_TEXT,
  G_TYPE_INPUT_BOX,
  G_TYPE_BUTTON,
  G_TYPE_CHECK_BOX,
  G_TYPE_SURFACE,
  G_TYPE_SLIDER_H,
  G_TYPE_SLIDER_V,
  G_TYPE_DROP_DOWN_LIST
}g_widget_type;

struct g_widget_text{
  char text[G_TEXT_LENGTH];
  
  struct{
    struct g_color_struct text;
  }color;
};

struct g_widget_input_box{
  char text[G_TEXT_LENGTH];
  
  /* you can set a maximum of characters in the input box */
  /* if this value is smaller then 0, or bigger then G_TEXT_LENGTH, 'limit' it is ignored */
  int limit;
  
  /* this character replaces all other characters when 'hide_text' is enabled */
  /* it is usefull if you want to have a input box for passwords */
  /* this character equals '*' by default, so you only need to change */
  /* it if you prefer other characters then '*' */
  char replace_character;
  
  /* special key repeat settings for input box */
  /* they are only used when key_repeat is enabled */
  /* this sets SDL key_repeat when entering input box, and */
  /* restore previous key_repeat settings after leaving it */
  struct{
    int delay, interval;
  }key_repeat;
  
  /* some flags you can enable/disable */
  struct{
    /* enable this flag to hide the text, i.e. to create a password input box */
    int hide_text:1;
    
    /* disable this flags to prevent input of numbers, letters or special characters like '$', '#', '{', ... */
    int numbers:1;
    int letters:1;
    int uppercase:1;
    int lowercase:1;
    int special_chars:1;
    
    /* disable/enable key repeat; its enabled by default */
    int key_repeat:1;
  }flags;
  
  struct{
    struct g_color_struct text;
    struct g_color_struct frame;
    struct g_color_struct background;
    struct g_color_struct background_active;
    struct g_color_struct cursor;
  }color;
  
  /* the following variables are only used by the widget itself */
  /* dont change/access them, unless you know what you are doing */
  
  /* position of first visible character in 'text' */
  int first_character;
  
  /* position of cursor in input box */
  int cursor_pos;
  
  /* amount of characters which can be displayed in the input box at the same time */
  int char_amount;
  
  /* width of input box in characters */
  int char_amount_w;
};

struct g_widget_button{
  char text[G_TEXT_LENGTH];
  
  struct{
    struct g_color_struct text;
    struct g_color_struct frame;
    struct g_color_struct top;
    struct g_color_struct bottom;
  }color;
};

struct g_widget_check_box{
  int state:1;
  
  struct{
    struct g_color_struct mark;
    struct g_color_struct frame;
    struct g_color_struct background;
  }color;
};

struct g_widget_surface{
  int src_x, src_y;
  
  SDL_Surface *surface;
};

struct g_widget_slider{
  double value;
  double max_value;
  
  struct{
    int invert:1;
    
    /* allows usage of mouse wheel to change slider value */
    int mouse_wheel:1;
  }flags;
  
  struct{
    struct g_color_struct slider;
    struct g_color_struct frame;
    struct g_color_struct line;
  }color;
};

struct g_widget_drop_down_list{
  /* items are seperated by '\n' in 'text' */
  char text[G_TEXT_LENGTH];
  
  int current_item;
  int max_item;
  
  struct{
    /* allows usage of mouse wheel to change 'current_item' */
    int mouse_wheel:1;
  }flags;
  
  struct{
    struct g_color_struct text;
    struct g_color_struct frame;
    struct g_color_struct background;
    struct g_color_struct background_list;
    struct g_color_struct highlight;
    struct g_color_struct arrow;
  }color;
};

typedef struct g_widget{
  g_widget_type type;
  
  /* every widget has a position inside the window */
  /* this position can differ from the position on the video surface */
  /* if you set w and/or h to 0, the widget will be as big as the possible, but not bigger than the window */
  int x, y, w, h;
  
  /* widgets */
  union{
    struct g_widget_text text;
    struct g_widget_input_box input;
    struct g_widget_button button;
    struct g_widget_check_box check;
    struct g_widget_surface surface;
    struct g_widget_slider slider;
    struct g_widget_drop_down_list drop_down;
  };
  
  /* this is something like a tooltip, which can be filled with widgets, but disappears if mouse leave pop_up */
  struct g_window *pop_up;
  
  /* the time mouse must hover over this widget to open pop_up */
  Uint32 pop_up_delay;
  
  /* this function is called, if any event occurs on the widget. function example: */
  /* void example_function(const g_event *event, struct g_widget *widget, void *data); */
  void (*event_function)(const g_event *, struct g_widget *, void *);
  
  /* pointer to some data. this data is passed to the event function if any event occurs */
  void *event_data;
  
  /* do not assign this pointers, unless you know what you are doing */
  /* in most cases the engine will do this for you */
  struct g_window *window;
  
  struct g_widget *next;
  struct g_widget *prev;
}g_widget;

/* window defines */
#define G_WINDOW_TITLE_LENGTH 128
#define G_WINDOW_TITLE_BAR_HEIGHT 20
#define G_WINDOW_CLOSE_BUTTON_WIDTH 20

struct g_window_flags{
  /* show title bar */
  int title_bar:1;
  
  /* show close button */
  int close_button:1;
  
  /* you can disable this flag to hide window */
  int visible:1;
  
  /* disable, to prevent moving window while title bar is pressed */
  int moveable:1;
  
  /* allow resizing of window with the mouse */
  int resizable:1;
  
  /* keep window ratio while resizing */
  int keep_ratio:1;
};

typedef struct g_window{
  /* y is the position of the window body */
  /* the title bar position equals y - G_WINDOW_TITLE_BAR_HEIGHT */
  int x, y, w, h;
  int min_w, min_h;
  
  /* max_w/h are ignored if they are 0 or less */
  int max_w, max_h;
  
  char title[G_WINDOW_TITLE_LENGTH];
  
  /* window flags to enable/disable */
  struct g_window_flags flags;
  
  /* the character inside the close button; it is set to 'X' by default */
  char close_button_character;
  
  /* this margin effects only the gap between window frame and widget */
  int margin;
  
  /* colors */
  struct{
    struct g_color_struct frame;
    struct g_color_struct background;
    struct g_color_struct resize_mark;
    
    struct g_color_struct title_text;
    struct g_color_struct title_bar_top;
    struct g_color_struct title_bar_bottom;
    
    struct g_color_struct close_button_text;
    struct g_color_struct close_button_top;
    struct g_color_struct close_button_bottom;
  }color;
  
  /* this function is called, if any event occurs inside the window */
  void (*event_function)(const g_event *, struct g_window *, void *);
  
  /* same as in g_widget */
  void *event_data;
  
  /* do not assign this pointers, unless you know what you are doing */
  /* in most cases the engine will do this for you */
  struct g_widget *first_widget;
  struct g_widget *last_widget;
  
  struct g_window *next;
  struct g_window *prev;
}g_window;

/* setting struct */
/* this struct contains the default settings, which are applied to new windows and widgets */
typedef struct{
  /* window defaults */
  struct{
    int min_w, min_h;
    int max_w, max_h;
    
    struct g_window_flags flags;
    
    char close_button_character;
    int margin;
    
    /* colors */
    struct{
      struct g_color_struct frame;
      struct g_color_struct background;
      struct g_color_struct resize_mark;
      
      struct g_color_struct title_text;
      struct g_color_struct title_bar_top;
      struct g_color_struct title_bar_bottom;
      
      struct g_color_struct close_button_text;
      struct g_color_struct close_button_top;
      struct g_color_struct close_button_bottom;
    }color;
  }window;
  
  /* widget defaults */
  /* text */
  struct{
    struct{
      struct g_color_struct text;
    }color;
  }text;
  
  /* input box */
  struct{
    char replace_character;
    
    struct{
      int delay, interval;
    }key_repeat;
    
    /* some flags */
    struct{
      int hide_text:1;
      int numbers:1;
      int letters:1;
      int uppercase:1;
      int lowercase:1;
      int special_chars:1;
      int key_repeat:1;
    }flags;
    
    struct{
      struct g_color_struct text;
      struct g_color_struct frame;
      struct g_color_struct background;
      struct g_color_struct background_active;
      struct g_color_struct cursor;
    }color;
  }input;
  
  /* button */
  struct{
    struct{
      struct g_color_struct text;
      struct g_color_struct frame;
      struct g_color_struct top;
      struct g_color_struct bottom;
    }color;
  }button;
  
  /* check box */
  struct{
    struct{
      struct g_color_struct mark;
      struct g_color_struct frame;
      struct g_color_struct background;
    }color;
  }check;
  
  /* slider */
  struct{
    struct{
      int invert:1;
      int mouse_wheel:1;
    }flags;
    
    struct{
      struct g_color_struct slider;
      struct g_color_struct frame;
      struct g_color_struct line;
    }color;
  }slider;
  
  /* drop down list */
  struct{
    struct{
      int mouse_wheel:1;
    }flags;
    
    struct{
      struct g_color_struct text;
      struct g_color_struct frame;
      struct g_color_struct background;
      struct g_color_struct background_list;
      struct g_color_struct highlight;
      struct g_color_struct arrow;
    }color;
  }drop_down;
  
  /* pop_up */
  struct{
    struct{
      struct g_color_struct frame;
      struct g_color_struct background;
    }color;
  }pop_up;
  
  Uint32 pop_up_delay;
}g_setting_struct;

/* widget functions */
/* the g_attach_... functions are used to attach a widget to a window */
/* they return NULL on failure, otherwise a pointer to the widget */
extern g_widget *g_attach_text(g_window *window, const int x, const int y, const int w, const int h, const char *text);
extern g_widget *g_attach_input_box(g_window *window, const int x, const int y, const int w, const int h);
extern g_widget *g_attach_button(g_window *window, const int x, const int y, const char *text);
extern g_widget *g_attach_check_box(g_window *window, const int x, const int y);
extern g_widget *g_attach_surface(g_window *window, const int x, const int y, const int w, const int h, SDL_Surface *surface);
extern g_widget *g_attach_slider_h(g_window *window, const int x, const int y, const int w, const double value, const double max_value);
extern g_widget *g_attach_slider_v(g_window *window, const int x, const int y, const int h, const double value, const double max_value);
extern g_widget *g_attach_drop_down_list(g_window *window, const int x, const int y, const int w, const char *text);
extern void g_destroy_widget(g_widget *widget);

extern void g_enter_input_box(g_widget *widget);
extern void g_leave_input_box(void);

/* window functions */
extern g_window *g_create_window(const int x, const int y, const int w, const int h, const char *title);
extern void g_destroy_window(g_window *window);
extern void g_raise_window(g_window *window);

/* attention: window cant get bigger then window max_w/h */
extern void g_maximize_window(g_window *window);
extern void g_maximize_window_h(g_window *window);
extern void g_maximize_window_v(g_window *window);

extern g_window *g_create_pop_up_window(const int w, const int h);

/* open pop_up window at mouse position */
extern void g_open_pop_up(g_window *window);
extern void g_close_pop_up(void);

/* engine functions */
/* you need to call this function to be able to use the engine, even before you create any window */
/* returns 1 on success, 0 on failure */
extern int g_init_everything(void);

/* this function is called at exit by default */
extern void g_destroy_everything(void);
extern void g_draw_everything(SDL_Surface *dst);

/* set all settings in the g_setting_struct to default */
extern void g_reset_settings(void);

/* returns a pointer to the default setting struct */
/* this way you can change defaults, which are applied to new windows/widgets */
extern g_setting_struct *g_get_setting_struct(void);

/* special functions */
/* this function redirects all keyboard events to the window/widget, and also to the window/widget */
/* event function if it exists. If you want to redirect all keyboard events into a input box */
/* to read in a text, use g_enter_input_bux() instead */
extern void g_window_grab_keyboard(g_window *window);
extern void g_widget_grab_keyboard(g_widget *widget);

extern int g_count_lines(const char *text);

/* searches in text for 'line'. lines are seperatet by '\n'. Return -1 on failure, */
/* otherwise the position of the first character after '\n' inside 'text' */
extern int g_get_line(const char *text, const int line);

/* some macros */
#define g_enable(object) \
        object = 1;
#define g_disable(object) \
        object = 0;
#define g_toggle(object) \
        if(object) \
          object = 0; \
        else \
          object = 1;

#endif
