/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * koala/turtle: Logo-style turtle graphics over SDL2.
 *
 * The Turtle object holds the pen state (position, heading, colour, size)
 * and a growable list of line segments. Moving with the pen down appends a
 * segment; nothing is drawn to a window until `done()`/`update()` renders
 * the accumulated segments. This keeps the state machine headless-testable.
 *
 * Coordinates follow CPython's turtle: origin at the window centre, x to the
 * right, y UP, heading 0 = east and counter-clockwise positive. SDL's y-down
 * pixel coordinates are converted only at render time.
 */

#include "turtle_shim.h"

typedef struct _Segment {
    double x1, y1, x2, y2;
    Uint8 r, g, b;
    int size;
} Segment;

typedef struct _Dot {
    double x, y;
    Uint8 r, g, b;
    int size;
} Dot;

typedef struct _TurtleObject {
    OBJECT_HEAD
    double x, y;        /* turtle coordinates, centre origin, y up */
    double heading;     /* degrees, 0 = east, CCW positive */
    int pen_down;
    Uint8 r, g, b;
    int pen_size;
    Segment *segs;
    int nsegs, cap;
    Dot *dots;
    int ndots, dcap;
    int w, h;           /* window size */
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *canvas;
    int quit;
    int destroyed;
} TurtleObject;

static TypeObject turtle_type;

#define TUR(self) ((TurtleObject *)SELF_AS(turtle_type))

static double norm_angle(double a)
{
    a = fmod(a, 360.0);
    if (a < 0) a += 360.0;
    return a;
}

static void add_segment(TurtleObject *t, double x1, double y1, double x2, double y2)
{
    Segment *s;

    if (t->nsegs >= t->cap) {
        t->cap = t->cap ? t->cap * 2 : 64;
        t->segs = realloc(t->segs, (size_t)t->cap * sizeof(Segment));
        if (!t->segs) {
            t->cap = 0;
            t->nsegs = 0;
            raise_exc_str("turtle: out of memory");
            return;
        }
    }
    s = &t->segs[t->nsegs++];
    s->x1 = x1; s->y1 = y1; s->x2 = x2; s->y2 = y2;
    s->r = t->r; s->g = t->g; s->b = t->b;
    s->size = t->pen_size;
}

static void add_dot(TurtleObject *t, int size)
{
    Dot *d;

    if (t->ndots >= t->dcap) {
        t->dcap = t->dcap ? t->dcap * 2 : 64;
        t->dots = realloc(t->dots, (size_t)t->dcap * sizeof(Dot));
        if (!t->dots) {
            t->dcap = 0;
            t->ndots = 0;
            raise_exc_str("turtle: out of memory");
            return;
        }
    }
    d = &t->dots[t->ndots++];
    d->x = t->x; d->y = t->y;
    d->r = t->r; d->g = t->g; d->b = t->b;
    d->size = size < 1 ? 1 : size;
}

/* Internal move/turn shared by the public methods and circle(). */
static void move(TurtleObject *t, double dist)
{
    double ox = t->x, oy = t->y;
    double rad = t->heading * T_PI / 180.0;

    t->x += dist * cos(rad);
    t->y += dist * sin(rad);
    if (t->pen_down) add_segment(t, ox, oy, t->x, t->y);
}

static void turn(TurtleObject *t, double angle)
{
    t->heading = norm_angle(t->heading + angle);
}

static int ensure_window(TurtleObject *t)
{
    if (t->win) return 1;
    if (t->destroyed) {
        raise_exc_str("turtle: turtle is destroyed");
        return 0;
    }
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        raise_exc_fmt("turtle: SDL_Init failed: %s", SDL_GetError());
        return 0;
    }
    t->win = SDL_CreateWindow("Koala Turtle", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, t->w, t->h, SDL_WINDOW_SHOWN);
    if (!t->win) {
        raise_exc_fmt("turtle: SDL_CreateWindow failed: %s", SDL_GetError());
        return 0;
    }
    t->ren = SDL_CreateRenderer(t->win, -1, SDL_RENDERER_ACCELERATED);
    if (!t->ren) t->ren = SDL_CreateRenderer(t->win, -1, SDL_RENDERER_SOFTWARE);
    if (!t->ren) {
        raise_exc_fmt("turtle: SDL_CreateRenderer failed: %s", SDL_GetError());
        return 0;
    }
    t->canvas = SDL_CreateTexture(t->ren, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_TARGET, t->w, t->h);
    if (!t->canvas) {
        raise_exc_fmt("turtle: SDL_CreateTexture failed: %s", SDL_GetError());
        return 0;
    }
    return 1;
}

/* Draw a line of the given thickness as parallel offset strokes. */
static void draw_thick(SDL_Renderer *ren, int x1, int y1, int x2, int y2,
                       int size, Uint8 r, Uint8 g, Uint8 b)
{
    double dx = x2 - x1, dy = y2 - y1;
    double len = sqrt(dx * dx + dy * dy);
    double px = 0, py = 0;
    int k;

    if (len > 0) { px = -dy / len; py = dx / len; }
    SDL_SetRenderDrawColor(ren, r, g, b, 255);
    if (size < 1) size = 1;
    for (k = 0; k < size; k++) {
        double d = k - (size - 1) / 2.0;
        SDL_RenderDrawLine(ren, (int)(x1 + px * d), (int)(y1 + py * d),
                           (int)(x2 + px * d), (int)(y2 + py * d));
    }
}

/* Filled circle by horizontal scanlines (SDL2 has no filled-circle primitive). */
static void draw_dot(SDL_Renderer *ren, int cx, int cy, int size, Uint8 r, Uint8 g, Uint8 b)
{
    int radius = size / 2;
    int dy;

    if (radius < 1) radius = 1;
    SDL_SetRenderDrawColor(ren, r, g, b, 255);
    for (dy = -radius; dy <= radius; dy++) {
        int half = (int)sqrt((double)(radius * radius - dy * dy));
        SDL_RenderDrawLine(ren, cx - half, cy + dy, cx + half, cy + dy);
    }
}

static SDL_Scancode scancode_of(const char *name)
{
    if (str_equal(name, "up")) return SDL_SCANCODE_UP;
    if (str_equal(name, "down")) return SDL_SCANCODE_DOWN;
    if (str_equal(name, "left")) return SDL_SCANCODE_LEFT;
    if (str_equal(name, "right")) return SDL_SCANCODE_RIGHT;
    if (str_equal(name, "space")) return SDL_SCANCODE_SPACE;
    if (str_equal(name, "escape")) return SDL_SCANCODE_ESCAPE;
    if (str_equal(name, "return")) return SDL_SCANCODE_RETURN;
    if (name[0] >= 'a' && name[0] <= 'z' && name[1] == '\0')
        return (SDL_Scancode)(SDL_SCANCODE_A + (name[0] - 'a'));
    return SDL_SCANCODE_UNKNOWN;
}

static int render(TurtleObject *t)
{
    int i;

    if (!ensure_window(t)) return 0;
    SDL_SetRenderTarget(t->ren, t->canvas);
    SDL_SetRenderDrawColor(t->ren, 255, 255, 255, 255);
    SDL_RenderClear(t->ren);
    for (i = 0; i < t->nsegs; i++) {
        Segment *s = &t->segs[i];
        int sx1 = (int)(t->w / 2.0 + s->x1), sy1 = (int)(t->h / 2.0 - s->y1);
        int sx2 = (int)(t->w / 2.0 + s->x2), sy2 = (int)(t->h / 2.0 - s->y2);
        draw_thick(t->ren, sx1, sy1, sx2, sy2, s->size, s->r, s->g, s->b);
    }
    for (i = 0; i < t->ndots; i++) {
        Dot *d = &t->dots[i];
        int cx = (int)(t->w / 2.0 + d->x), cy = (int)(t->h / 2.0 - d->y);
        draw_dot(t->ren, cx, cy, d->size, d->r, d->g, d->b);
    }
    SDL_SetRenderTarget(t->ren, NULL);
    SDL_RenderCopy(t->ren, t->canvas, NULL, NULL);
    SDL_RenderPresent(t->ren);
    return 1;
}

static void poll_events(TurtleObject *t)
{
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) t->quit = 1;
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) t->quit = 1;
    }
}

static void release(TurtleObject *t)
{
    if (t->canvas) { SDL_DestroyTexture(t->canvas); t->canvas = NULL; }
    if (t->ren) { SDL_DestroyRenderer(t->ren); t->ren = NULL; }
    if (t->win) { SDL_DestroyWindow(t->win); t->win = NULL; }
    if (t->segs) { free(t->segs); t->segs = NULL; }
    t->nsegs = t->cap = 0;
    if (t->dots) { free(t->dots); t->dots = NULL; }
    t->ndots = t->dcap = 0;
}

// __init__()
static TValue _turtle_init(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);

    ASSERT(nargs == 0);
    t->x = t->y = 0;
    t->heading = 0;
    t->pen_down = 1;
    t->r = t->g = t->b = 0;   /* black pen, like CPython */
    t->pen_size = 1;
    t->segs = NULL;
    t->nsegs = t->cap = 0;
    t->dots = NULL;
    t->ndots = t->dcap = 0;
    t->w = 800; t->h = 600;
    t->win = NULL; t->ren = NULL; t->canvas = NULL;
    t->quit = 0;
    t->destroyed = 0;
    return no_value;
}

// forward(dist float)
static TValue _turtle_forward(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    move(TUR(self), kl_arg_float(0));
    return no_value;
}

// backward(dist float)
static TValue _turtle_backward(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    move(TUR(self), -kl_arg_float(0));
    return no_value;
}

// left(angle float)
static TValue _turtle_left(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    turn(TUR(self), kl_arg_float(0));
    return no_value;
}

// right(angle float)
static TValue _turtle_right(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    turn(TUR(self), -kl_arg_float(0));
    return no_value;
}

// goto(x float, y float)
static TValue _turtle_goto(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);
    double ox, oy, nx, ny;

    ASSERT(nargs == 2);
    ox = t->x; oy = t->y;
    nx = kl_arg_float(0); ny = kl_arg_float(1);
    t->x = nx; t->y = ny;
    if (t->pen_down) add_segment(t, ox, oy, nx, ny);
    return no_value;
}

// setheading(angle float)
static TValue _turtle_setheading(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    TUR(self)->heading = norm_angle(kl_arg_float(0));
    return no_value;
}

// home()
static TValue _turtle_home(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);
    double ox = t->x, oy = t->y;

    ASSERT(nargs == 0);
    t->x = 0; t->y = 0; t->heading = 0;
    if (t->pen_down) add_segment(t, ox, oy, 0, 0);
    return no_value;
}

// penup()
static TValue _turtle_penup(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TUR(self)->pen_down = 0;
    return no_value;
}

// pendown()
static TValue _turtle_pendown(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    TUR(self)->pen_down = 1;
    return no_value;
}

// color(r int, g int, b int)
static TValue _turtle_color(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);

    ASSERT(nargs == 3);
    t->r = (Uint8)kl_arg_int(0);
    t->g = (Uint8)kl_arg_int(1);
    t->b = (Uint8)kl_arg_int(2);
    return no_value;
}

// pensize(width int)
static TValue _turtle_pensize(TValue *self, TValue *args, int nargs)
{
    int n;

    ASSERT(nargs == 1);
    n = (int)kl_arg_int(0);
    TUR(self)->pen_size = n < 1 ? 1 : n;
    return no_value;
}

// circle(radius float, steps int)
static TValue _turtle_circle(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);
    double radius, step, angle;
    int steps, i;

    ASSERT(nargs == 2);
    radius = kl_arg_float(0);
    steps = (int)kl_arg_int(1);
    if (steps < 3) steps = 3;
    step = 2.0 * T_PI * radius / steps;
    angle = 360.0 / steps;
    for (i = 0; i < steps; i++) {
        move(t, step);
        turn(t, angle);
    }
    return no_value;
}

// clear()
static TValue _turtle_clear(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);

    ASSERT(nargs == 0);
    t->nsegs = 0;
    t->ndots = 0;
    return no_value;
}

// xcor() float
static TValue _turtle_xcor(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return float64_value(TUR(self)->x);
}

// ycor() float
static TValue _turtle_ycor(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return float64_value(TUR(self)->y);
}

// heading() float
static TValue _turtle_heading(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return float64_value(TUR(self)->heading);
}

// isdown() bool
static TValue _turtle_isdown(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return bool_value(TUR(self)->pen_down);
}

// update()
static TValue _turtle_update(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);

    ASSERT(nargs == 0);
    if (!render(t)) return error_value;
    poll_events(t);
    SDL_Delay(16);
    return no_value;
}

// done()
static TValue _turtle_done(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);

    ASSERT(nargs == 0);
    if (!render(t)) return error_value;
    while (!t->quit) {
        poll_events(t);
        SDL_RenderPresent(t->ren);
        SDL_Delay(16);
    }
    return no_value;
}

// bye()
static TValue _turtle_bye(TValue *self, TValue *args, int nargs)
{
    TurtleObject *t = TUR(self);

    ASSERT(nargs == 0);
    release(t);
    t->destroyed = 1;
    return no_value;
}

// dot(size int)
static TValue _turtle_dot(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    add_dot(TUR(self), (int)kl_arg_int(0));
    return no_value;
}

// key(name str) bool
static TValue _turtle_key(TValue *self, TValue *args, int nargs)
{
    const Uint8 *state;
    SDL_Scancode sc;

    ASSERT(nargs == 1);
    sc = scancode_of(kl_arg_str(0));
    state = SDL_GetKeyboardState(NULL);
    if (sc == SDL_SCANCODE_UNKNOWN || !state) return bool_value(0);
    return bool_value(state[sc]);
}

// closed() bool
static TValue _turtle_closed(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return bool_value(TUR(self)->quit);
}

static void _turtle_free(Object *self)
{
    release((TurtleObject *)self);
}

static MethodDef turtle_methods[] = {
    { "__init__", _turtle_init },
    { "forward", _turtle_forward },
    { "backward", _turtle_backward },
    { "left", _turtle_left },
    { "right", _turtle_right },
    { "goto", _turtle_goto },
    { "setheading", _turtle_setheading },
    { "home", _turtle_home },
    { "penup", _turtle_penup },
    { "pendown", _turtle_pendown },
    { "color", _turtle_color },
    { "pensize", _turtle_pensize },
    { "circle", _turtle_circle },
    { "clear", _turtle_clear },
    { "xcor", _turtle_xcor },
    { "ycor", _turtle_ycor },
    { "heading", _turtle_heading },
    { "isdown", _turtle_isdown },
    { "update", _turtle_update },
    { "done", _turtle_done },
    { "bye", _turtle_bye },
    { "dot", _turtle_dot },
    { "key", _turtle_key },
    { "closed", _turtle_closed },
    { NULL, NULL },
};

void turtle_reg_turtle(NativeLib *lib)
{
    turtle_type._type = &type_type;
    turtle_type._nfields = 0;
    turtle_type.name = (char *)"Turtle";
    turtle_type.objsize = sizeof(TurtleObject);
    turtle_type.flags = TP_FLAGS_CLASS;
    turtle_type.methdefs = turtle_methods;
    turtle_type._free = _turtle_free;
    kl_reg_type(lib, &turtle_type);
}

/* Entry point called by the runtime after dlopen("libkoala_turtle.so"). */
void turtle_native_lib_init(NativeLib *lib)
{
    turtle_reg_turtle(lib);
}