#include <stdio.h>
#include <stdbool.h>

#include <SDL.h>
#include "gui_engine.h"
#include "example-theme.h"

static void die(const char *message)
{
  fprintf(stderr, "error: %s\n", message);
  exit(EXIT_FAILURE);
}

static SDL_Surface *initSDL(void)
{
  if(SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    die("failed to initialize SDL");
  }
  atexit(SDL_Quit);

  SDL_Surface *screen = SDL_SetVideoMode(800, 600, 16, SDL_SWSURFACE);
  if(screen == NULL)
  {
    die("failed to open SDL window");
  }
  SDL_WM_SetCaption("GUI engine demo", NULL);

  return screen;
}

static void buttonCallback(const g_event *event,
                           g_widget *widget,
                           void *user_data)
{
  g_widget *input_box = user_data;

  if(event->type == G_CLICK_LEFT)
  {
    strcpy(widget->window->title, input_box->input.text);
  }
}

static void checkboxCallback(const g_event *event,
                             g_widget *widget,
                             void *user_data)
{
  g_widget *input_box = user_data;

  if(event->type == G_CLICK_LEFT)
  {
    input_box->input.flags.hide_text = widget->check.state;
  }
}

static g_window *createDemoWindow(int x, int y)
{
  g_window *window = g_create_window(x, y, 400, 300, "Test Window");
  window->flags.close_button = 0;

  /* Attach text input elements. A width/height of 0 will cause the widget
     to fit by expanding to the right/bottom. */
  g_attach_text(window, 0, 0, 0, 0, "Insert window title:");
  g_widget *input_box = g_attach_input_box(window, 0, 25, 0, G_INPUT_BOX_H);

  /* Impose limits to input. */
  input_box->input.limit = 20;
  /* input_box->input.flags.numbers = 0; */
  /* input_box->input.flags.special_chars = 0; */

  g_widget *button = g_attach_button(window, 0, 60, "set title");
  button->event_function = buttonCallback;
  button->event_data = input_box;

  /* Attach checkboxes. */
  g_attach_text(window, 30, 100, 0, 0, "Hide input");
  g_widget *checkbox = g_attach_check_box(window, 0, 95);
  checkbox->event_function = checkboxCallback;
  checkbox->event_data = input_box;

  g_attach_text(window, 30, 125, 0, 0, "Checkbox 2");
  g_attach_check_box(window, 0, 120);

  g_attach_text(window, 30, 150, 0, 0, "Checkbox 3");
  g_attach_check_box(window, 0, 145);

  /* Attach drop-down menu. */
  g_attach_text(window, window->w - 210, 63, 0, 0, "Select item:");
  g_attach_drop_down_list(window, window->w - 100, 55, 100,
                          "Item 1\nItem 2\nItem 3");

  /* Attach label with pop-up window. */
  g_widget *label = g_attach_text(window, window->w - 140, 250, 0,
                                  G_CHAR_H, "move mouse here");
  label->pop_up = g_create_pop_up_window(165, 80);
  g_attach_text(label->pop_up, 25, 15, 0, 0, "example pop-up");
  g_attach_input_box(label->pop_up, 0, 35, 0, 0);

  /* Attach slider. */
  g_attach_slider_h(window, 0, window->h - 20, 0, 0.5, 1.0);

  return window;
}

int main(void)
{
  SDL_Surface *screen = initSDL();

  /* Initialize GUI. */
  if(g_init_everything() == 0)
  {
    SDL_FreeSurface(screen);
    die("failed to initialize GUI engine");
  }

  g_window *first_window = createDemoWindow(250, 200);

  /* Set the global default theme. This will only affect windows/widgets
     created after this call. */
  setExampleTheme();

  createDemoWindow(50, 50);

  g_raise_window(first_window);

  /* Main loop. */
  bool running = true;
  while(running)
  {
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
      if(event.type == SDL_QUIT)
      {
        running = false;
      }
    }

    /* Clear screen. */
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));

    /* Draw GUI. */
    g_draw_everything(screen);

    /* Update screen and keep framerate. */
    SDL_Flip(screen);
    SDL_Delay(16);
  }

  SDL_FreeSurface(screen);

  /* The GUI engine will cleanup itself atexit. */
}
