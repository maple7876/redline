#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * REDLINE — Ashwick Prototype
 * Custom-coded in C + raylib (no full game engine).
 * Current slice: title menu -> opening cutscene -> first playable night-road scene.
 * Fictional setting inspired by McHenry County, Illinois.
 */

#define INTERNAL_W 480
#define INTERNAL_H 270
#define SCREEN_W  960
#define SCREEN_H  540
#define MAX_DIALOGUE 6

typedef enum {
    STATE_TITLE,
    STATE_CUTSCENE,
    STATE_DRIVING,
    STATE_PAUSE
} GameState;

typedef struct {
    Vector2 pos;
    float angle;      /* radians; 0 points right */
    float speed;
    float maxSpeed;
    float acceleration;
    float turnRate;
} Car;

typedef struct {
    const char *speaker;
    const char *line;
} DialogueLine;

static const DialogueLine opening[MAX_DIALOGUE] = {
    {"ASHWICK, ILLINOIS  |  FRIDAY, 9:47 PM", "The last commuter train has gone north."},
    {"NARRATION", "Beyond the old depot, the town is settling into another cold night."},
    {"NARRATION", "A water tower rises over dark rooftops. Freight cars groan somewhere west."},
    {"PHONE", "JULES: Meet us at the station on Route 14. Bring the car."},
    {"RIVAL", "Haven't seen that car around here before."},
    {"YOU", "Then I guess you'll remember it."}
};

static RenderTexture2D world;
static GameState state = STATE_TITLE;
static int dialogueIndex = 0;
static float cutsceneTimer = 0.0f;
static float menuPulse = 0.0f;
static Car player = {{190.0f, 203.0f}, -2.20f, 0.0f, 115.0f, 78.0f, 2.7f};
static float raceTime = 0.0f;
static int roadHits = 0;
static bool showHelp = true;

static Color C_BG       = {11, 17, 28, 255};
static Color C_ASPHALT  = {37, 43, 51, 255};
static Color C_ROADEDGE = {82, 86, 84, 255};
static Color C_AMBER    = {240, 173, 78, 255};
static Color C_PALE     = {204, 221, 222, 255};
static Color C_RED      = {220, 72, 64, 255};
static Color C_MUTED    = {129, 151, 161, 255};
static Color C_GREEN    = {91, 139, 105, 255};

static float ClampF(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static void DrawPixelText(const char *text, int x, int y, int size, Color color) {
    DrawText(text, x, y, size, color);
}

static void DrawCar(Vector2 p, float angle, Color body, bool headlights) {
    /* Draw in world-space using chunky geometry, then rotate around car center. */
    Vector2 forward = { cosf(angle), sinf(angle) };
    Vector2 right = { -sinf(angle), cosf(angle) };
    Vector2 nose = { p.x + forward.x * 8.0f, p.y + forward.y * 8.0f };
    Vector2 tailL = { p.x - forward.x * 7.0f - right.x * 5.0f,
                      p.y - forward.y * 7.0f - right.y * 5.0f };
    Vector2 tailR = { p.x - forward.x * 7.0f + right.x * 5.0f,
                      p.y - forward.y * 7.0f + right.y * 5.0f };
    Vector2 sideL = { p.x + forward.x * 2.0f - right.x * 5.0f,
                      p.y + forward.y * 2.0f - right.y * 5.0f };
    Vector2 sideR = { p.x + forward.x * 2.0f + right.x * 5.0f,
                      p.y + forward.y * 2.0f + right.y * 5.0f };

    DrawTriangle(nose, sideL, tailL, body);
    DrawTriangle(nose, tailR, sideR, body);
    DrawTriangle(sideL, sideR, tailL, body);
    DrawTriangle(sideR, tailR, tailL, body);

    /* Windshield and rear lamps. */
    Vector2 glass = { p.x + forward.x * 1.5f, p.y + forward.y * 1.5f };
    DrawCircleV(glass, 2.7f, (Color){ 75, 103, 119, 255 });
    Vector2 lampL = { p.x - forward.x * 5.5f - right.x * 3.2f,
                      p.y - forward.y * 5.5f - right.y * 3.2f };
    Vector2 lampR = { p.x - forward.x * 5.5f + right.x * 3.2f,
                      p.y - forward.y * 5.5f + right.y * 3.2f };
    DrawCircleV(lampL, 1.4f, C_RED);
    DrawCircleV(lampR, 1.4f, C_RED);

    if (headlights) {
        Vector2 beamEnd = { p.x + forward.x * 26.0f, p.y + forward.y * 26.0f };
        Vector2 beamA = { beamEnd.x - right.x * 8.0f, beamEnd.y - right.y * 8.0f };
        Vector2 beamB = { beamEnd.x + right.x * 8.0f, beamEnd.y + right.y * 8.0f };
        DrawTriangle(nose, beamA, beamB, (Color){ 245, 223, 157, 24 });
        DrawCircleV((Vector2){p.x + forward.x * 8.5f, p.y + forward.y * 8.5f},
                    1.4f, (Color){255, 239, 180, 255});
    }
}


/* Smooth centerline for the first Ashwick county-road section. */
static const Vector2 roadAnchors[] = {
    {215, 286}, {215, 250}, {208, 224}, {190, 203},
    {174, 181}, {169, 157}, {174, 137}, {190, 119},
    {214, 106}, {242, 99}, {274, 96}, {303, 87},
    {326, 69}, {344, 43}, {354, 12}, {359, -18}
};

static Vector2 RoadPoint(float t) {
    const int count = (int)(sizeof(roadAnchors) / sizeof(roadAnchors[0]));
    t = ClampF(t, 0.0f, 1.0f);

    float scaled = t * (float)(count - 1);
    int i = (int)scaled;
    float u = scaled - (float)i;

    if (i >= count - 1) {
        i = count - 2;
        u = 1.0f;
    }

    Vector2 p0 = roadAnchors[i > 0 ? i - 1 : i];
    Vector2 p1 = roadAnchors[i];
    Vector2 p2 = roadAnchors[i + 1];
    Vector2 p3 = roadAnchors[i + 2 < count ? i + 2 : i + 1];

    float u2 = u * u;
    float u3 = u2 * u;

    Vector2 p = {
        0.5f * ((2*p1.x) + (-p0.x+p2.x)*u
        + (2*p0.x-5*p1.x+4*p2.x-p3.x)*u2
        + (-p0.x+3*p1.x-3*p2.x+p3.x)*u3),
        0.5f * ((2*p1.y) + (-p0.y+p2.y)*u
        + (2*p0.y-5*p1.y+4*p2.y-p3.y)*u2
        + (-p0.y+3*p1.y-3*p2.y+p3.y)*u3)
    };
    return p;
}

static float RoadDistance(Vector2 point) {
    float best = 1000000.0f;
    Vector2 a = RoadPoint(0.0f);

    for (int i = 1; i <= 240; ++i) {
        Vector2 b = RoadPoint((float)i / 240.0f);
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float len2 = dx*dx + dy*dy;
        float u = 0.0f;

        if (len2 > 0.0001f) {
            u = ((point.x-a.x)*dx + (point.y-a.y)*dy) / len2;
            u = ClampF(u, 0.0f, 1.0f);
        }

        float ex = point.x - (a.x + u*dx);
        float ey = point.y - (a.y + u*dy);
        float d = sqrtf(ex*ex + ey*ey);
        if (d < best) best = d;
        a = b;
    }

    return best;
}

static void DrawRoadMarkings(void) {
    const int segments = 220;
    Vector2 previous = RoadPoint(0.0f);

    /* Draw edge first, then asphalt, so the border stays consistent. */
    for (int i = 1; i <= segments; ++i) {
        Vector2 next = RoadPoint((float)i / (float)segments);
        DrawLineEx(previous, next, 40.0f, C_ROADEDGE);
        DrawLineEx(previous, next, 35.0f, C_ASPHALT);
        previous = next;
    }

    /* Short dashed centerline follows the curve rather than the screen. */
    for (float t = 0.015f; t < 0.98f; t += 0.065f) {
        Vector2 a = RoadPoint(t);
        Vector2 b = RoadPoint(fminf(t + 0.027f, 1.0f));
        DrawLineEx(a, b, 1.25f, (Color){191, 179, 132, 235});
    }

    /* Subtle damp reflections, also aligned to the road direction. */
    for (float t = 0.08f; t < 0.96f; t += 0.105f) {
        Vector2 a = RoadPoint(t);
        Vector2 b = RoadPoint(fminf(t + 0.018f, 1.0f));
        DrawLineEx(a, b, 1.0f, (Color){103, 132, 145, 95});
    }
}

static void DrawTown(void) {
    ClearBackground(C_BG);

    /* Distant fields and tree line. */
    DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, (Color){14, 25, 31, 255});
    for (int i = 0; i < 52; ++i) {
        int x = (i * 47 + 19) % INTERNAL_W;
        int y = (i * 31 + 11) % INTERNAL_H;
        DrawRectangle(x, y, 2, 2, (i % 3 == 0) ? (Color){43, 63, 48, 255} : (Color){26, 43, 40, 255});
    }

    /* Industrial sheds and old town buildings: deliberately chunky silhouettes. */
    DrawRectangle(16, 19, 71, 38, (Color){38, 45, 51, 255});
    DrawRectangle(20, 23, 25, 8, (Color){73, 68, 59, 255});
    DrawRectangle(51, 23, 29, 8, (Color){73, 68, 59, 255});
    DrawRectangle(26, 39, 6, 12, (Color){15, 21, 28, 255});
    DrawRectangle(59, 39, 6, 12, (Color){15, 21, 28, 255});
    DrawText("ASHWICK WORKS", 19, 59, 6, C_MUTED);

    /* Depot / railway spur. */
    DrawRectangle(99, 15, 3, 74, (Color){91, 91, 83, 255});
    DrawRectangle(108, 15, 3, 74, (Color){91, 91, 83, 255});
    for (int y = 17; y < 88; y += 8) DrawRectangle(97, y, 16, 2, (Color){64, 68, 68, 255});
    DrawRectangle(80, 73, 42, 21, (Color){58, 45, 39, 255});
    DrawRectangle(84, 77, 34, 13, (Color){25, 29, 34, 255});
    DrawText("DEPOT", 88, 82, 6, C_AMBER);

    /* Trees / preserve edge. */
    for (int i = 0; i < 21; ++i) {
        int x = 389 + (i % 4) * 19;
        int y = (i * 37) % 260;
        DrawCircle(x, y, 7, (Color){23, 54, 43, 255});
        DrawCircle(x-2, y-2, 3, (Color){38, 75, 53, 255});
    }

    /* A roadside gas station: a key recurring story location. */
    DrawRectangle(291, 173, 58, 35, (Color){48, 49, 52, 255});
    DrawRectangle(296, 177, 48, 27, (Color){20, 28, 36, 255});
    DrawRectangle(299, 179, 42, 5, (Color){184, 70, 57, 255});
    DrawText("MILE 14", 304, 180, 6, (Color){255, 224, 183, 255});
    DrawRectangle(298, 211, 8, 6, (Color){110, 117, 115, 255});
    DrawRectangle(315, 211, 8, 6, (Color){110, 117, 115, 255});
    DrawRectangle(332, 211, 8, 6, (Color){110, 117, 115, 255});
    DrawCircle(320, 192, 22, (Color){255, 173, 76, 9});
    DrawCircle(320, 192, 13, (Color){255, 173, 76, 9});

    /* Residential blocks, little pools of amber window light. */
    for (int i = 0; i < 7; ++i) {
        int x = 20 + i * 20;
        int y = 118 + (i % 2) * 15;
        DrawRectangle(x, y, 13, 11, (Color){48, 49, 53, 255});
        DrawRectangle(x+3, y+3, 3, 3, (i % 3 == 0) ? C_AMBER : (Color){90, 107, 111, 255});
        DrawRectangle(x+8, y+3, 2, 3, (Color){18, 25, 30, 255});
    }

    /* County road and parking apron. */
    DrawRoadMarkings();
    DrawRectangle(280, 222, 82, 28, (Color){50, 55, 56, 255});
    for (int x = 286; x < 357; x += 13) DrawLine(x, 226, x, 246, (Color){91, 95, 90, 255});

    /* Roadside reflector posts. */
    for (int i = 0; i < 5; ++i) {
        int x = 242 + i * 4;
        int y = 251 - i * 23;
        DrawRectangle(x, y, 2, 4, C_PALE);
    }

    /* Water tower: unmistakably Midwestern skyline detail. */
    DrawLineEx((Vector2){ 412, 30 }, (Vector2){ 412, 56 }, 2, (Color){ 94, 106, 108, 255 });
    DrawLineEx((Vector2){ 430, 30 }, (Vector2){ 430, 56 }, 2, (Color){ 94, 106, 108, 255 });
    DrawRectangle(407, 21, 28, 11, (Color){ 66, 77, 81, 255 });
    DrawRectangle(411, 24, 20, 2, (Color){ 111, 126, 124, 255 });
    DrawText("A W", 414, 27, 5, C_PALE);

    /* Damp-road highlights. */
    for (int i = 0; i < 20; ++i) {
        int x = 187 + ((i * 13) % 54);
        int y = 171 + ((i * 19) % 74);
        DrawRectangle(x, y, 4 + (i % 3), 1, (Color){87, 111, 123, 90});
    }
}

static void DrawHUD(void) {
    DrawRectangle(7, 7, 139, 36, (Color){8, 13, 22, 220});
    DrawText("ASHWICK / COUNTY LINE", 14, 13, 7, C_AMBER);
    DrawText("FRI  9:47 PM  |  DRY / COLD", 14, 25, 6, C_PALE);

    DrawRectangle(334, 7, 139, 36, (Color){8, 13, 22, 220});
    DrawText("REDLINE  //  FREE DRIVE", 341, 13, 7, C_PALE);
    char speed[32];
    snprintf(speed, sizeof(speed), "%03d MPH", (int)(fabsf(player.speed) * 0.55f));
    DrawText(speed, 341, 25, 9, C_AMBER);

    if (showHelp) {
        DrawRectangle(8, 235, 225, 27, (Color){8, 13, 22, 220});
        DrawText("WASD / ARROWS  DRIVE     ESC  PAUSE", 14, 242, 7, C_PALE);
        DrawText("ENTER  DISMISS HELP", 14, 252, 6, C_MUTED);
    }
}

static void UpdateCar(float dt) {
    float throttle = 0.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) throttle += 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) throttle -= 1.15f;

    float steer = 0.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) steer -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) steer += 1.0f;

    player.speed += throttle * player.acceleration * dt;
    player.speed *= (1.0f - 0.42f * dt);
    player.speed = ClampF(player.speed, -32.0f, player.maxSpeed);

    float speedFactor = 0.18f + 0.82f *
        ClampF(fabsf(player.speed) / player.maxSpeed, 0.0f, 1.0f);
    player.angle += steer * player.turnRate * speedFactor * dt *
        (player.speed >= 0 ? 1.0f : -1.0f);

    Vector2 old = player.pos;
    player.pos.x += cosf(player.angle) * player.speed * dt * 0.54f;
    player.pos.y += sinf(player.angle) * player.speed * dt * 0.54f;

    /* Keep the car in the viewport. */
    if (player.pos.x < 8 || player.pos.x > INTERNAL_W-8 ||
        player.pos.y < 8 || player.pos.y > INTERNAL_H-8) {
        player.pos = old;
        player.speed *= -0.24f;
        roadHits++;
        return;
    }

    /* The asphalt is 35 pixels wide. Slow the car near its edge and
       reject movement that would carry its center off the road. */
    float distance = RoadDistance(player.pos);
    if (distance > 14.0f) {
        player.pos = old;
        player.speed *= -0.18f;
        roadHits++;
    } else if (distance > 10.5f) {
        player.speed *= (1.0f - 0.8f * dt);
    }
}

static void DrawTitle(void) {
    DrawTown();
    DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, (Color){3, 7, 15, 155});
    DrawRectangle(24, 25, 432, 220, (Color){8, 13, 22, 218});
    DrawRectangle(24, 25, 432, 2, C_AMBER);
    DrawText("A GAME ABOUT THE ROADS BACK HOME", 42, 43, 7, C_MUTED);
    DrawText("R E D L I N E", 39, 76, 30, C_PALE);
    DrawText("ASHWICK, ILLINOIS", 43, 111, 10, C_AMBER);
    DrawText("A MIDWEST STREET-RACING STORY", 43, 128, 8, C_MUTED);

    float blink = (sinf(menuPulse * 4.0f) > -0.1f) ? 1.0f : 0.45f;
    Color prompt = (Color){255, 215, 150, (unsigned char)(255 * blink)};
    DrawText("PRESS ENTER TO BEGIN", 43, 166, 10, prompt);
    DrawText("A fictional town inspired by the Fox River, rail lines,", 43, 200, 6, C_PALE);
    DrawText("prairie edges, old downtowns, and late-night gas stations.", 43, 210, 6, C_PALE);
    DrawText("C + RAYLIB  //  PROTOTYPE BUILD  0.1", 43, 231, 6, C_MUTED);
}

static void DrawCutscene(void) {
    DrawTown();
    DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, (Color){2, 5, 12, 125});
    /* Cinematic letterbox bars. */
    DrawRectangle(0, 0, INTERNAL_W, 24, (Color){3, 5, 10, 255});
    DrawRectangle(0, INTERNAL_H-25, INTERNAL_W, 25, (Color){3, 5, 10, 255});
    DrawText("REDLINE  /  CHAPTER 01: UNKNOWN", 12, 9, 7, C_AMBER);

    /* Slow cinematic push-in represented with a framed scene. */
    DrawRectangle(66, 35, 348, 105, (Color){9, 15, 22, 220});
    DrawRectangle(66, 35, 348, 1, C_MUTED);
    DrawText("NIGHT SHIFT", 78, 46, 7, C_MUTED);
    DrawText("MILE 14  //  ROUTE 14", 78, 61, 12, C_PALE);
    DrawText("THE LAST TRAIN HAS GONE NORTH.", 78, 86, 7, C_AMBER);
    DrawText("THE TOWN IS STILL AWAKE.", 78, 98, 7, C_PALE);
    DrawText("A PHONE VIBRATES ON THE PASSENGER SEAT.", 78, 119, 6, C_MUTED);

    DrawRectangle(16, 177, 448, 71, (Color){5, 9, 17, 242});
    DrawRectangle(16, 177, 3, 71, C_AMBER);
    DrawText(opening[dialogueIndex].speaker, 28, 187, 7, C_AMBER);

    /* Typewriter reveal, driven by cutsceneTimer. */
    const char *line = opening[dialogueIndex].line;
    int visible = (int)(cutsceneTimer * 38.0f);
    int len = (int)strlen(line);
    if (visible > len) visible = len;
    char partial[256];
    memcpy(partial, line, (size_t)visible);
    partial[visible] = '\0';
    DrawText(partial, 28, 205, 8, C_PALE);
    DrawText("ENTER / SPACE  CONTINUE", 322, 235, 6, C_MUTED);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W, SCREEN_H, "REDLINE — Ashwick Prototype");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    world = LoadRenderTexture(INTERNAL_W, INTERNAL_H);
    SetTextureFilter(world.texture, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        menuPulse += dt;

        if (state == STATE_TITLE) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                state = STATE_CUTSCENE;
                dialogueIndex = 0;
                cutsceneTimer = 0.0f;
            }
        } else if (state == STATE_CUTSCENE) {
            cutsceneTimer += dt;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (dialogueIndex < MAX_DIALOGUE - 1) {
                    dialogueIndex++;
                    cutsceneTimer = 0.0f;
                } else {
                    state = STATE_DRIVING;
                    raceTime = 0.0f;
                }
            }
        } else if (state == STATE_DRIVING) {
            raceTime += dt;
            UpdateCar(dt);
            if (IsKeyPressed(KEY_ENTER)) showHelp = false;
            if (IsKeyPressed(KEY_ESCAPE)) state = STATE_PAUSE;
        } else if (state == STATE_PAUSE) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) state = STATE_DRIVING;
            if (IsKeyPressed(KEY_BACKSPACE)) state = STATE_TITLE;
        }

        BeginTextureMode(world);
            if (state == STATE_TITLE) {
                DrawTitle();
            } else if (state == STATE_CUTSCENE) {
                DrawCutscene();
            } else if (state == STATE_DRIVING) {
                DrawTown();
                DrawCar(player.pos, player.angle, (Color){198, 61, 54, 255}, true);
                DrawHUD();
            } else {
                DrawTown();
                DrawCar(player.pos, player.angle, (Color){198, 61, 54, 255}, true);
                DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, (Color){2, 5, 12, 195});
                DrawRectangle(72, 72, 336, 124, (Color){8, 13, 22, 245});
                DrawRectangle(72, 72, 336, 2, C_AMBER);
                DrawText("PAUSED", 91, 89, 19, C_PALE);
                DrawText("ENTER / ESC  RESUME", 92, 129, 8, C_AMBER);
                DrawText("BACKSPACE  TITLE SCREEN", 92, 145, 7, C_MUTED);
            }
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            float scaleX = (float)GetScreenWidth() / INTERNAL_W;
            float scaleY = (float)GetScreenHeight() / INTERNAL_H;
            float scale = fminf(scaleX, scaleY);
            float w = INTERNAL_W * scale;
            float h = INTERNAL_H * scale;
            Rectangle src = {0, 0, (float)world.texture.width, -(float)world.texture.height};
            Rectangle dst = {(GetScreenWidth()-w)/2.0f, (GetScreenHeight()-h)/2.0f, w, h};
            DrawTexturePro(world.texture, src, dst, (Vector2){0,0}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(world);
    CloseWindow();
    return 0;
}
