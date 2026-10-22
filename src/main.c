#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 1000
#define SCREEN_H 650
#define MAX_TRAFFIC 14
#define MAX_PARTICLES 180
#define MAX_PICKUPS 6
#define SAVE_FILE "redline_save.dat"
#define PI 3.14159265358979323846f

typedef enum {
    SCREEN_TITLE, SCREEN_TRACKS, SCREEN_GARAGE, SCREEN_MODES,
    SCREEN_DRIVE, SCREEN_PAUSE, SCREEN_CRASH, SCREEN_WIN
} ScreenState;
typedef enum { PICKUP_NITRO, PICKUP_SCORE, PICKUP_REPAIR } PickupType;
typedef enum { MODE_ENDLESS, MODE_TIME_ATTACK, MODE_OVERTAKE, MODE_COUNT } RaceMode;
typedef enum {
    TRACK_MIDNIGHT, TRACK_NEON, TRACK_CANYON,
    TRACK_ALPINE, TRACK_COASTAL, TRACK_INDUSTRIAL, TRACK_COUNT
} TrackId;

typedef struct {
    const char *name, *subtitle;
    Color skyTop, skyBottom, grassA, grassB, asphalt, roadEdge, accent;
    float roadWidth, trafficMultiplier, curveMultiplier;
    int unlockScore;
} TrackDefinition;

static const TrackDefinition tracks[TRACK_COUNT] = {
    {"MIDNIGHT RUN", "THE ORIGINAL NIGHT HIGHWAY",
     {17,28,34,255},{22,47,38,255},{25,65,50,255},{21,56,44,255},
     {43,47,55,255},{213,202,174,255},{255,185,78,255},470,1.0f,1.0f,0},
    {"NEON DISTRICT", "THE CITY NEVER SLEEPS",
     {14,12,39,255},{34,21,55,255},{25,24,48,255},{19,19,38,255},
     {38,39,55,255},{95,83,143,255},{82,222,255,255},470,1.2f,1.1f,1500},
    {"CANYON RUSH", "HEAT. DUST. SPEED.",
     {91,50,38,255},{209,137,76,255},{139,89,49,255},{116,72,42,255},
     {57,52,48,255},{230,190,127,255},{255,199,111,255},430,1.0f,1.2f,3000},
    {"ALPINE PASS", "KEEP YOUR LINE",
     {28,45,58,255},{106,137,147,255},{43,74,62,255},{34,62,53,255},
     {49,55,62,255},{217,225,224,255},{159,218,238,255},450,1.1f,1.3f,5000},
    {"COASTAL SPRINT", "THE LONG WAY HOME",
     {30,92,127,255},{139,194,201,255},{48,112,88,255},{39,96,77,255},
     {54,58,61,255},{235,224,194,255},{109,221,233,255},490,0.95f,0.8f,8000},
    {"INDUSTRIAL RUN", "NO ROOM FOR MISTAKES",
     {30,34,43,255},{79,82,83,255},{46,50,47,255},{35,39,37,255},
     {40,43,47,255},{199,171,118,255},{255,169,79,255},440,1.3f,1.1f,12000}
};

typedef struct {
    const char *name, *className;
    Color paint;
    float acceleration, maxSpeed, braking, handling, nitroEfficiency;
    int unlockScore;
    float widthScale, lengthScale;
} CarDefinition;

static const CarDefinition cars[] = {
    {"R-01", "STREET COUPE", {226,64,60,255}, 82,145,112,1.00f,1.00f,0,1.00f,1.00f},
    {"VORTEX GT", "SUPERCAR", {69,158,225,255}, 100,165,118,1.08f,1.00f,1500,0.96f,1.08f},
    {"THUNDER V8", "MUSCLE", {244,184,70,255}, 108,155,105,0.78f,1.05f,3000,1.16f,1.03f},
    {"RALLY X", "RALLY", {156,105,219,255}, 88,140,125,1.25f,0.95f,4500,1.04f,1.02f},
    {"METRO RS", "COMPACT", {222,231,231,255}, 84,132,118,1.30f,1.00f,6000,0.88f,0.91f},
    {"APEX E", "ELECTRIC", {81,222,230,255}, 115,160,125,1.15f,0.90f,8500,0.98f,1.00f},
    {"TITAN S", "GRAND TOURER", {103,224,151,255}, 90,158,120,0.82f,1.15f,11000,1.20f,1.08f},
    {"CLASSIC 77", "VINTAGE", {233,157,119,255}, 76,138,108,1.05f,0.90f,14000,0.94f,1.05f}
};
#define CAR_COUNT ((int)(sizeof(cars)/sizeof(cars[0])))

typedef struct { Vector2 pos, vel; float life, maxLife, size; Color color; bool active; } Particle;
typedef struct { float y, lane, speed; int type, paint; bool active, nearMissCounted; } Traffic;
typedef struct { float y, lane; PickupType type; bool active; float spin; } Pickup;

static ScreenState screenState = SCREEN_TITLE;
static Traffic traffic[MAX_TRAFFIC];
static Pickup pickups[MAX_PICKUPS];
static Particle particles[MAX_PARTICLES];
static int currentTrack = 0, currentCar = 0, currentMode = MODE_ENDLESS;
static int bestScore = 0, totalRuns = 0, totalOvertakes = 0, paintIndex = 0;
static int overtakes = 0, nearMisses = 0, combo = 0, lastLaneSpawn = 1;
static float bestDistance = 0, bestTrackScores[TRACK_COUNT] = {0};
static float playerX=0, playerVelX=0, speed=0, roadScroll=0, roadPhase=0;
static float roadCenter=SCREEN_W*0.5f, spawnTimer=.6f, pickupTimer=7;
static float nitro=55, score=0, distanceTravelled=0, timeLeft=90;
static float shake=0, flash=0, comboTimer=0;
static bool newBest=false, nitroActive=false;
static bool missionDone[3]={false,false,false};
static float missionProgress[3]={0,0,0};

static const Color C_WHITE={238,241,233,255}, C_MUTED={145,160,171,255};
static const Color C_AMBER={255,185,78,255}, C_RED={226,64,60,255};
static const Color C_CYAN={81,222,230,255}, C_GREEN={103,224,151,255};
static const Color C_DARK={7,12,18,225};
static const Color PAINT_PALETTE[5]={{226,64,60,255},{69,158,225,255},{244,184,70,255},{156,105,219,255},{222,231,231,255}};
static Color SelectedPaint(void){return PAINT_PALETTE[paintIndex];}

static float ClampF(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}
static float LerpF(float a,float b,float t){return a+(b-a)*t;}
static float RandF(float lo,float hi){return lo+(hi-lo)*(float)GetRandomValue(0,10000)/10000.0f;}
static Color Alpha(Color c,unsigned char a){c.a=a;return c;}
static const TrackDefinition *Track(void){return &tracks[currentTrack];}
static const CarDefinition *Car(void){return &cars[currentCar];}
static float RoadWidth(void){return Track()->roadWidth;}
static float LaneWidth(void){return RoadWidth()/3.0f;}
static float RoadCenterAt(float y){
    float t=y/(float)SCREEN_H;
    float k=Track()->curveMultiplier;
    return roadCenter+sinf(roadPhase+t*2.15f)*30.0f*t*k+sinf(roadPhase*.47f+t*4.2f)*13.0f*t*k;
}
static float LaneX(float lane,float y){return RoadCenterAt(y)+lane*LaneWidth();}

static bool TrackUnlocked(int i){return bestScore>=tracks[i].unlockScore;}
static bool CarUnlocked(int i){return bestScore>=cars[i].unlockScore;}

static void LoadSave(void){
    FILE *f=fopen(SAVE_FILE,"rb"); if(!f)return;
    int magic=0,version=0,bs=0,runs=0,overs=0,car=0,track=0,mode=0; float bd=0,trackScores[TRACK_COUNT]={0};
    if(fread(&magic,sizeof(int),1,f)==1 && fread(&version,sizeof(int),1,f)==1 &&
       magic==0x52444C4E && version==3){
        fread(&bs,sizeof(int),1,f); fread(&runs,sizeof(int),1,f); fread(&overs,sizeof(int),1,f);
        fread(&bd,sizeof(float),1,f); fread(&car,sizeof(int),1,f); fread(&track,sizeof(int),1,f);
        fread(&mode,sizeof(int),1,f); fread(trackScores,sizeof(float),TRACK_COUNT,f);
        bestScore=bs>0?bs:0; totalRuns=runs>0?runs:0; totalOvertakes=overs>0?overs:0;
        bestDistance=bd>0?bd:0; currentCar=(car>=0&&car<CAR_COUNT)?car:0;
        currentTrack=(track>=0&&track<TRACK_COUNT)?track:0;
        currentMode=(mode>=0&&mode<MODE_COUNT)?mode:0;
        memcpy(bestTrackScores,trackScores,sizeof(trackScores));
    }
    fclose(f);
}
static void SaveGame(void){
    FILE *f=fopen(SAVE_FILE,"wb"); if(!f)return;
    int magic=0x52444C4E,version=3;
    fwrite(&magic,sizeof(int),1,f); fwrite(&version,sizeof(int),1,f);
    fwrite(&bestScore,sizeof(int),1,f); fwrite(&totalRuns,sizeof(int),1,f);
    fwrite(&totalOvertakes,sizeof(int),1,f); fwrite(&bestDistance,sizeof(float),1,f);
    fwrite(&currentCar,sizeof(int),1,f); fwrite(&currentTrack,sizeof(int),1,f);
    fwrite(&currentMode,sizeof(int),1,f); fwrite(bestTrackScores,sizeof(float),TRACK_COUNT,f);
    fclose(f);
}
static void AddParticle(float x,float y,float vx,float vy,float life,float size,Color color){
    for(int i=0;i<MAX_PARTICLES;i++)if(!particles[i].active){
        particles[i]=(Particle){{x,y},{vx,vy},life,life,size,color,true};return;
    }
}
static void Burst(float x,float y,int count,Color color,float power){
    for(int i=0;i<count;i++){float a=RandF(0,2*PI),v=RandF(power*.25f,power);
        AddParticle(x,y,cosf(a)*v,sinf(a)*v,RandF(.22f,.7f),RandF(2,5.5f),color);}
}
static void UpdateParticles(float dt){
    for(int i=0;i<MAX_PARTICLES;i++){Particle*p=&particles[i];if(!p->active)continue;
        p->life-=dt;if(p->life<=0){p->active=false;continue;}
        p->pos.x+=p->vel.x*dt;p->pos.y+=p->vel.y*dt;p->vel.x*=.985f;p->vel.y*=.985f;}
}
static void DrawParticles(void){
    for(int i=0;i<MAX_PARTICLES;i++){Particle*p=&particles[i];if(!p->active)continue;
        float a=ClampF(p->life/p->maxLife,0,1);
        DrawCircleV(p->pos,p->size*a,Alpha(p->color,(unsigned char)(p->color.a*a)));}
}

static void DrawTree(float x,float y,float s,bool snowy){
    DrawEllipse((int)x,(int)(y+8*s),14*s,6*s,(Color){0,0,0,42});
    DrawRectangle((int)(x-2*s),(int)(y-2*s),(int)(4*s),(int)(13*s),(Color){83,62,43,255});
    Color leaf=snowy?(Color){104,133,145,255}:(currentTrack==TRACK_CANYON?(Color){133,78,42,255}:
        (currentTrack==TRACK_NEON?(Color){60,46,100,255}:(Color){35,100,64,255}));
    DrawCircle((int)x,(int)(y-8*s),11*s,leaf);
    DrawCircle((int)(x-6*s),(int)(y-3*s),7*s,leaf);
    DrawCircle((int)(x+6*s),(int)(y-2*s),7*s,leaf);
    if(snowy)DrawCircle((int)x,(int)(y-12*s),5*s,(Color){224,235,239,255});
}
static void DrawTrackScenery(void){
    int offset=(int)fmodf(roadScroll*.8f,118);
    for(int y=-118+offset,i=0;y<SCREEN_H+118;y+=118,i++){
        float yy=(float)y,c=RoadCenterAt(yy);
        for(int side=-1;side<=1;side+=2){
            float edge=c+(float)side*(RoadWidth()*.5f+30);
            DrawRectangle((int)(edge-3),y+8,6,22,(Color){11,24,24,255});
            DrawRectangle((int)(edge-2),y+8,4,9,Track()->accent);
            float outside=RoadWidth()*.5f+75;
            if(currentTrack==TRACK_COASTAL){
                DrawRectangle((int)(c+side*(RoadWidth()*.5f+12)),y,26,118,(Color){44,130,159,255});
                DrawLine((int)(c+side*(RoadWidth()*.5f+20)),y+15,(int)(c+side*(RoadWidth()*.5f+20)),y+65,(Color){134,220,230,180});
            }
            if(currentTrack==TRACK_INDUSTRIAL){
                DrawRectangle((int)(c+side*outside)-18,y+12,36,64,(i%2)?(Color){97,76,61,255}:(Color){76,91,99,255});
                DrawLine((int)(c+side*outside)-18,y+30,(int)(c+side*outside)+18,y+30,Track()->accent);
            }else if(currentTrack==TRACK_NEON){
                DrawRectangle((int)(c+side*outside)-4,y+10,8,55,Track()->accent);
                DrawRectangle((int)(c+side*outside)-12,y+15,24,4,C_CYAN);
            }else if((i+side)%2==0){
                DrawTree(c+side*outside,yy+28,.72f,currentTrack==TRACK_ALPINE);
            }
        }
    }
}
static void DrawWorld(void){
    const TrackDefinition*t=Track();
    ClearBackground(t->skyTop);
    DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H,t->skyTop,t->skyBottom);
    int stripe=(int)fmodf(roadScroll*.65f,100);
    for(int y=-100+stripe;y<SCREEN_H+100;y+=100){
        DrawRectangle(0,y,SCREEN_W,49,t->grassA);DrawRectangle(0,y+49,SCREEN_W,51,t->grassB);}
    float half=RoadWidth()*.5f;
    Vector2 l[4]={{RoadCenterAt(0)-half,0},{RoadCenterAt(220)-half,220},
        {RoadCenterAt(430)-half,430},{RoadCenterAt(SCREEN_H)-half,SCREEN_H}};
    Vector2 r[4]={{RoadCenterAt(0)+half,0},{RoadCenterAt(220)+half,220},
        {RoadCenterAt(430)+half,430},{RoadCenterAt(SCREEN_H)+half,SCREEN_H}};
    for(int i=0;i<3;i++){DrawTriangle(l[i],l[i+1],r[i],t->asphalt);DrawTriangle(r[i],l[i+1],r[i+1],t->asphalt);}
    float roadL=RoadCenterAt(0)-half,roadR=RoadCenterAt(0)+half;
    DrawRectangle((int)roadL,0,(int)(roadR-roadL),SCREEN_H,t->asphalt);
    for(int y=-30;y<SCREEN_H+30;y+=16){
        float c=RoadCenterAt((float)y);int b=((int)((roadScroll+y)/32))&1;
        Color rumble=b?t->roadEdge:C_RED;
        DrawRectangle((int)(c-half-9),y,9,16,rumble);DrawRectangle((int)(c+half),y,9,16,rumble);}
    int dash=(int)fmodf(roadScroll,88);
    for(int y=-88+dash;y<SCREEN_H+88;y+=88)for(int d=-1;d<=1;d+=2){
        float x1=RoadCenterAt((float)y)+d*LaneWidth()*.5f;
        float x2=RoadCenterAt((float)y+48)+d*LaneWidth()*.5f;
        DrawLineEx((Vector2){x1,(float)y},(Vector2){x2,(float)y+48},4,(Color){211,210,189,210});}
    DrawTrackScenery();
}
static Rectangle CarRect(float x,float y,float scale){
    const CarDefinition*c=Car();
    float w=36*c->widthScale*scale,h=62*c->lengthScale*scale;
    return (Rectangle){x-w*.5f,y-h*.5f,w,h};
}
static void DrawCar(float x,float y,float scale,Color paint,bool player,float lean){
    float w=36*scale,h=62*scale;
    if(player){w*=Car()->widthScale;h*=Car()->lengthScale;}
    DrawRectangleRounded((Rectangle){x-w*.52f+3,y-h*.45f+5,w*1.04f,h*.94f},.18f,5,(Color){0,0,0,85});
    Color tire=(Color){13,16,19,255};
    DrawRectangleRounded((Rectangle){x-w*.57f+lean,y-h*.31f,w*.19f,h*.24f},.2f,4,tire);
    DrawRectangleRounded((Rectangle){x+w*.38f+lean,y-h*.31f,w*.19f,h*.24f},.2f,4,tire);
    DrawRectangleRounded((Rectangle){x-w*.57f+lean,y+h*.13f,w*.19f,h*.24f},.2f,4,tire);
    DrawRectangleRounded((Rectangle){x+w*.38f+lean,y+h*.13f,w*.19f,h*.24f},.2f,4,tire);
    DrawRectangleRounded((Rectangle){x-w*.5f+lean,y-h*.5f,w,h},.2f,7,paint);
    DrawRectangleRounded((Rectangle){x-w*.36f+lean,y-h*.43f,w*.72f,h*.23f},.16f,5,(Color){25,38,48,255});
    DrawRectangleRounded((Rectangle){x-w*.32f+lean,y-h*.18f,w*.64f,h*.31f},.12f,4,(Color){57,78,88,255});
    DrawLineEx((Vector2){x-w*.29f+lean,y-h*.02f},(Vector2){x+w*.29f+lean,y-h*.02f},1.5f,(Color){155,185,192,180});
    DrawRectangleRounded((Rectangle){x-w*.31f+lean,y+h*.20f,w*.62f,h*.12f},.15f,4,(Color){28,38,43,255});
    DrawRectangleRounded((Rectangle){x-w*.38f+lean,y-h*.47f,w*.22f,h*.07f},.3f,3,(Color){255,241,190,255});
    DrawRectangleRounded((Rectangle){x+w*.16f+lean,y-h*.47f,w*.22f,h*.07f},.3f,3,(Color){255,241,190,255});
    DrawRectangleRounded((Rectangle){x-w*.38f+lean,y+h*.40f,w*.22f,h*.06f},.3f,3,C_RED);
    DrawRectangleRounded((Rectangle){x+w*.16f+lean,y+h*.40f,w*.22f,h*.06f},.3f,3,C_RED);
    if(player&&nitroActive){
        DrawTriangle((Vector2){x-w*.22f,y+h*.47f},(Vector2){x-w*.08f,y+h*.47f},
            (Vector2){x-w*.15f,y+h*.47f+RandF(18,35)},C_CYAN);
        DrawTriangle((Vector2){x+w*.08f,y+h*.47f},(Vector2){x+w*.22f,y+h*.47f},
            (Vector2){x+w*.15f,y+h*.47f+RandF(18,35)},C_AMBER);}
}
static void DrawTraffic(void){
    for(int i=0;i<MAX_TRAFFIC;i++)if(traffic[i].active){
        Color p=traffic[i].paint==0?(Color){77,157,184,255}:traffic[i].paint==1?(Color){191,188,168,255}:
            traffic[i].paint==2?(Color){100,123,105,255}:traffic[i].paint==3?(Color){150,105,177,255}:(Color){194,92,72,255};
        float s=traffic[i].type==2?1.12f:1;
        DrawCar(LaneX(traffic[i].lane,traffic[i].y),traffic[i].y,s,p,false,0);}
}
static void DrawPickups(void){
    for(int i=0;i<MAX_PICKUPS;i++){Pickup*p=&pickups[i];if(!p->active)continue;
        float x=LaneX(p->lane,p->y),pulse=1+.1f*sinf(p->spin*5);
        Color c=p->type==PICKUP_NITRO?C_CYAN:(p->type==PICKUP_SCORE?C_AMBER:C_GREEN);
        DrawCircle((int)x,(int)p->y,20*pulse,Alpha(c,45));DrawCircleLines((int)x,(int)p->y,15*pulse,c);
        DrawText(p->type==PICKUP_NITRO?"N":(p->type==PICKUP_SCORE?"$":"+"),(int)x-6,(int)p->y-10,20,C_WHITE);}
}

static void DrawStatBar(int x,int y,const char*label,float value,float maxValue,Color c){
    DrawText(label,x,y,10,C_MUTED);DrawRectangle(x,y+15,220,7,(Color){29,38,46,255});
    DrawRectangle(x,y+15,(int)(220*ClampF(value/maxValue,0,1)),7,c);
}
static void DrawTitle(void){
    DrawRectangle(0,0,SCREEN_W,SCREEN_H,(Color){3,7,12,155});
    DrawRectangleRounded((Rectangle){82,52,836,548},.025f,8,(Color){5,10,17,244});
    DrawRectangle(82,52,836,4,C_AMBER);
    DrawText("ARCADE RACING // BUILD YOUR RUN",118,80,13,C_MUTED);
    DrawText("REDLINE",111,108,66,C_WHITE);
    DrawText("HIGHWAY EDITION",118,181,22,Track()->accent);
    DrawLine(118,220,880,220,(Color){56,70,78,255});
    DrawText(TextFormat("TRACK  %s",Track()->name),118,242,17,C_WHITE);
    DrawText(TextFormat("CAR  %s  /  %s",Car()->name,Car()->className),118,272,15,SelectedPaint());
    DrawText(TextFormat("MODE  %s",currentMode==MODE_ENDLESS?"ENDLESS":currentMode==MODE_TIME_ATTACK?"TIME ATTACK":"OVERTAKE CHALLENGE"),118,301,15,C_AMBER);
    DrawText("T  TRACK SELECT     G  GARAGE     M  RACE MODE",118,350,14,C_MUTED);
    DrawText("LEFT / RIGHT + ENTER TO CHOOSE IN MENUS",118,378,12,C_MUTED);
    DrawText(TextFormat("BEST SCORE %08d",bestScore),118,427,17,C_AMBER);
    DrawText(TextFormat("CAREER RUNS %d     TOTAL OVERTAKES %d",totalRuns,totalOvertakes),118,455,12,C_MUTED);
    DrawRectangleRounded((Rectangle){118,500,242,43},.12f,8,C_AMBER);
    DrawText("ENTER  START RACE",139,513,15,(Color){17,22,25,255});
    DrawText("C  CHANGE PAINT",390,514,13,C_MUTED);
    DrawRectangleRounded((Rectangle){820,492,54,64},.2f,8,SelectedPaint());
    DrawRectangleRounded((Rectangle){829,500,36,22},.15f,5,(Color){35,50,59,255});
    DrawText("CAR PREVIEW",792,567,9,C_MUTED);
}
static void DrawSelectionScreen(bool garage){
    DrawRectangle(0,0,SCREEN_W,SCREEN_H,(Color){3,7,12,190});
    DrawRectangleRounded((Rectangle){95,65,810,520},.025f,8,(Color){6,11,18,247});
    DrawRectangle(95,65,810,4,C_AMBER);
    if(garage){
        DrawText("GARAGE",132,92,34,C_WHITE);
        DrawText("CHOOSE YOUR MACHINE",134,133,13,C_MUTED);
        DrawText(Car()->name,132,177,31,Car()->paint);
        DrawText(Car()->className,134,216,14,C_AMBER);
        DrawRectangleRounded((Rectangle){660,155,110,190},.12f,9,(Color){17,25,32,255});
        DrawCar(715,247,1.5f,SelectedPaint(),true,0);
        DrawStatBar(135,260,"ACCELERATION",Car()->acceleration,120,C_AMBER);
        DrawStatBar(135,306,"TOP SPEED",Car()->maxSpeed,175,C_CYAN);
        DrawStatBar(135,352,"BRAKING",Car()->braking,140,C_GREEN);
        DrawStatBar(135,398,"HANDLING",Car()->handling,1.4f,C_AMBER);
        DrawStatBar(135,444,"NITRO EFFICIENCY",Car()->nitroEfficiency,1.2f,C_CYAN);
        if(CarUnlocked(currentCar))DrawText("UNLOCKED",665,373,13,C_GREEN);
        else DrawText(TextFormat("LOCKS AT %d SCORE",Car()->unlockScore),635,373,12,C_RED);
        DrawText("LEFT / RIGHT  CHANGE CAR     C  PAINT     ENTER  EQUIP",134,525,12,C_WHITE);
        DrawText("ESC  BACK",134,551,12,C_MUTED);
    }else{
        DrawText("TRACK SELECT",132,92,34,C_WHITE);
        DrawText("SIX ENVIRONMENTS. SIX DIFFERENT VIBES.",134,133,13,C_MUTED);
        for(int i=0;i<TRACK_COUNT;i++){
            int col=i%2,row=i/2,x=135+col*380,y=170+row*108;
            bool selected=i==currentTrack,unlocked=TrackUnlocked(i);
            DrawRectangleRounded((Rectangle){x,y,340,82},.08f,7,selected?(Color){43,48,52,255}:(Color){17,24,31,255});
            DrawRectangle(x,y,5,82,selected?tracks[i].accent:(Color){52,64,72,255});
            DrawText(tracks[i].name,x+18,y+12,16,unlocked?C_WHITE:C_MUTED);
            DrawText(tracks[i].subtitle,x+18,y+39,10,tracks[i].accent);
            DrawText(unlocked?"UNLOCKED":TextFormat("%d PTS",tracks[i].unlockScore),x+230,y+57,9,unlocked?C_GREEN:C_RED);
        }
        DrawText("LEFT / RIGHT  CHANGE TRACK     ENTER  SELECT",134,525,13,C_WHITE);
        DrawText("ESC  BACK",134,551,12,C_MUTED);
    }
}
static void DrawModeScreen(void){
    DrawRectangle(0,0,SCREEN_W,SCREEN_H,(Color){3,7,12,190});
    DrawRectangleRounded((Rectangle){150,95,700,460},.03f,8,(Color){6,11,18,247});
    DrawRectangle(150,95,700,4,C_AMBER);
    DrawText("CHOOSE RACE MODE",190,130,32,C_WHITE);
    const char*names[MODE_COUNT]={"ENDLESS","TIME ATTACK","OVERTAKE CHALLENGE"};
    const char*desc[MODE_COUNT]={"Survive as long as you can. Difficulty keeps rising.",
        "Score as much as possible before the 90-second clock expires.",
        "Pass 25 traffic cars without crashing to complete the challenge."};
    for(int i=0;i<MODE_COUNT;i++){
        int y=200+i*88;
        DrawRectangleRounded((Rectangle){190,y,620,65},.08f,7,currentMode==i?(Color){48,53,57,255}:(Color){17,24,31,255});
        DrawText(names[i],210,y+9,18,currentMode==i?C_AMBER:C_WHITE);
        DrawText(desc[i],210,y+35,11,C_MUTED);
    }
    DrawText("LEFT / RIGHT  SELECT     ENTER  CONFIRM     ESC  BACK",190,490,13,C_WHITE);
}
static void DrawHUD(void){
    DrawRectangleRounded((Rectangle){16,16,250,105},.08f,8,C_DARK);
    DrawRectangle(16,16,4,105,C_AMBER);
    DrawText(TextFormat("%s  /  %s",Track()->name,Car()->name),31,27,10,C_MUTED);
    DrawText(TextFormat("%08d",(int)score),30,45,30,C_WHITE);
    DrawText("SCORE",32,82,10,C_AMBER);
    DrawText(TextFormat("%.2f KM",distanceTravelled/1000),132,82,12,C_MUTED);
    if(combo>1&&comboTimer>0)DrawText(TextFormat("x%d COMBO",combo),32,101,10,C_CYAN);
    DrawRectangleRounded((Rectangle){SCREEN_W-220,16,204,105},.08f,8,C_DARK);
    DrawText("SPEED",SCREEN_W-201,26,10,C_MUTED);
    DrawText(TextFormat("%03d",(int)(speed*1.65f)),SCREEN_W-202,41,31,C_WHITE);
    DrawText("MPH",SCREEN_W-132,58,10,C_AMBER);
    DrawRectangle(SCREEN_W-201,78,168,7,(Color){29,38,46,255});
    DrawRectangle(SCREEN_W-201,78,(int)(168*speed/(Car()->maxSpeed*1.2f)),7,C_AMBER);
    DrawText("NITRO",SCREEN_W-201,92,9,C_CYAN);
    DrawRectangle(SCREEN_W-151,94,118,7,(Color){29,38,46,255});
    DrawRectangle(SCREEN_W-151,94,(int)(118*nitro/100),7,C_CYAN);
    DrawRectangleRounded((Rectangle){16,SCREEN_H-72,205,54},.08f,7,C_DARK);
    DrawText(TextFormat("OVERTAKES %03d",overtakes),29,SCREEN_H-60,11,C_WHITE);
    DrawText(TextFormat("NEAR MISSES %03d",nearMisses),29,SCREEN_H-39,10,C_CYAN);
    if(currentMode==MODE_TIME_ATTACK)DrawText(TextFormat("TIME  %05.1f",timeLeft),SCREEN_W/2-55,22,20,timeLeft<15?C_RED:C_WHITE);
    if(currentMode==MODE_OVERTAKE)DrawText(TextFormat("OVERTAKES  %d / 25",overtakes),SCREEN_W/2-75,22,17,C_WHITE);
    if(speed>80){
        float a=ClampF((speed-80)/55,0,.75f);
        for(int i=0;i<13;i++){float x=(float)((i*83+(int)(roadScroll*4))%SCREEN_W);
            float y=(float)((i*61+(int)(roadScroll*7))%SCREEN_H);
            DrawLineEx((Vector2){x,y},(Vector2){x+(x<SCREEN_W/2?-13:13),y+22},1.5f,Alpha(C_WHITE,(unsigned char)(a*150)));}}
}
static void DrawOverlay(const char*heading,const char*line1,const char*line2){
    DrawRectangle(0,0,SCREEN_W,SCREEN_H,(Color){2,5,9,175});
    DrawRectangleRounded((Rectangle){235,175,530,275},.06f,10,(Color){7,12,19,247});
    DrawRectangle(235,175,530,4,C_AMBER);
    DrawText(heading,275,207,30,C_WHITE);DrawText(line1,275,270,16,C_AMBER);
    DrawText(line2,275,301,13,C_MUTED);DrawText("ENTER  CONTINUE",275,365,14,C_WHITE);
    DrawText("ESC  TITLE SCREEN",275,397,12,C_MUTED);
}

static void SpawnTraffic(void){
    int lane=GetRandomValue(0,2);
    if(lane==lastLaneSpawn&&GetRandomValue(0,100)<72)lane=(lane+GetRandomValue(1,2))%3;
    lastLaneSpawn=lane;
    for(int i=0;i<MAX_TRAFFIC;i++)if(traffic[i].active&&traffic[i].y<110&&fabsf(traffic[i].lane-(float)(lane-1))<.1f)return;
    for(int i=0;i<MAX_TRAFFIC;i++)if(!traffic[i].active){
        traffic[i]=(Traffic){-90,(float)(lane-1),RandF(38,82),GetRandomValue(0,2),GetRandomValue(0,4),true,false};return;}
}
static void SpawnPickup(void){
    for(int i=0;i<MAX_PICKUPS;i++)if(!pickups[i].active){
        pickups[i]=(Pickup){-35,(float)(GetRandomValue(0,2)-1),(PickupType)GetRandomValue(0,2),true,RandF(0,8)};return;}
}
static void ResetRun(void){
    memset(traffic,0,sizeof(traffic));memset(pickups,0,sizeof(pickups));memset(particles,0,sizeof(particles));
    memset(missionDone,0,sizeof(missionDone));memset(missionProgress,0,sizeof(missionProgress));
    playerX=playerVelX=speed=roadScroll=roadPhase=0;roadCenter=SCREEN_W*.5f;
    spawnTimer=.55f;pickupTimer=5;nitro=55;score=distanceTravelled=0;timeLeft=90;
    shake=flash=comboTimer=0;combo=overtakes=nearMisses=0;newBest=nitroActive=false;
    screenState=SCREEN_DRIVE;totalRuns++;SaveGame();
}
static void FinishRun(bool victory){
    screenState=victory?SCREEN_WIN:SCREEN_CRASH;nitroActive=false;speed*=.2f;shake=.35f;
    Burst(SCREEN_W*.5f+playerX*LaneWidth()*.85f,SCREEN_H-95,34,C_AMBER,185);
    int final=(int)score;if(final>bestScore){bestScore=final;newBest=true;}
    if(distanceTravelled>bestDistance)bestDistance=distanceTravelled;
    if(score>bestTrackScores[currentTrack])bestTrackScores[currentTrack]=score;
    SaveGame();
}
static void UpdateGame(float dt){
    const CarDefinition*c=Car();
    bool throttle=IsKeyDown(KEY_W)||IsKeyDown(KEY_UP),brake=IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN);
    bool left=IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT),right=IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT);
    bool boost=IsKeyDown(KEY_SPACE)||IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT);
    if(throttle)speed+=c->acceleration*dt;else speed-=16*dt;
    if(brake)speed-=c->braking*dt;
    nitroActive=boost&&nitro>.5f&&speed>22;
    if(nitroActive){speed+=112*c->nitroEfficiency*dt;nitro-=28/c->nitroEfficiency*dt;score+=4*dt;
        if(GetRandomValue(0,100)<65){float x=SCREEN_W*.5f+playerX*LaneWidth()*.82f;
            AddParticle(x+RandF(-8,8),SCREEN_H-52,RandF(-24,24),RandF(80,170),.24f,RandF(2,4),C_CYAN);}}
    else if(!boost)nitro=ClampF(nitro+3.2f*dt,0,100);
    speed=ClampF(speed,0,nitroActive?c->maxSpeed*1.2f:c->maxSpeed);
    float steer=(right?1.0f:0.0f)-(left?1.0f:0.0f);
    float target=steer*300*(.78f+speed/125)*c->handling;
    playerVelX=LerpF(playerVelX,target,ClampF(dt*7,0,1));
    if(steer==0)playerVelX=LerpF(playerVelX,0,ClampF(dt*5,0,1));
    playerX=ClampF(playerX+playerVelX*dt/LaneWidth(),-1.25f,1.25f);
    roadPhase+=dt*(.20f+speed*.004f);
    float curve=(sinf(roadPhase*.53f)*42+sinf(roadPhase*.21f)*24)*Track()->curveMultiplier;
    roadCenter=LerpF(roadCenter,SCREEN_W*.5f+curve,ClampF(dt*.4f,0,1));
    roadScroll+=speed*dt*2.25f;float movement=(55+speed*1.35f)*dt;
    distanceTravelled+=speed*dt;score+=speed*dt*(.42f+(nitroActive?.15f:0));
    if(comboTimer>0)comboTimer-=dt;else combo=0;if(flash>0)flash-=dt;if(shake>0)shake-=dt;
    if(currentMode==MODE_TIME_ATTACK){timeLeft-=dt;if(timeLeft<=0){timeLeft=0;FinishRun(true);return;}}
    if(fabsf(playerX)>.91f)speed=fmaxf(0,speed-45*dt);
    if(fabsf(playerX)>1.12f){speed=fmaxf(0,speed-72*dt);score=fmaxf(0,score-7*dt);}
    spawnTimer-=dt;float difficulty=ClampF(distanceTravelled/9000,0,1);
    if(spawnTimer<=0){SpawnTraffic();spawnTimer=(LerpF(1,.48f,difficulty))/Track()->trafficMultiplier+RandF(0,.22f);}
    pickupTimer-=dt;if(pickupTimer<=0){SpawnPickup();pickupTimer=RandF(7,11);}
    float playerScreenX=SCREEN_W*.5f+playerX*LaneWidth()*.82f;
    Rectangle playerRect=CarRect(playerScreenX,SCREEN_H-93,1);
    for(int i=0;i<MAX_TRAFFIC;i++){
        Traffic*t=&traffic[i];if(!t->active)continue;
        float relative=ClampF(speed-t->speed*.35f,28,240);
        t->y+=relative*dt*1.35f;float tx=LaneX(t->lane,t->y);
        Rectangle tr={tx-18,t->y-31,36,t->type==2?70:62};
        if(CheckCollisionRecs(playerRect,tr)){FinishRun(false);return;}
        if(!t->nearMissCounted&&t->y>SCREEN_H-160){
            float dx=fabsf(playerScreenX-tx);
            if(dx>34&&dx<70){t->nearMissCounted=true;nearMisses++;combo++;comboTimer=3.5f;
                score+=250*ClampF((float)combo,1,8);Burst(tx,t->y,7,C_CYAN,75);missionProgress[1]++;}}
        if(t->y>SCREEN_H+70){t->active=false;overtakes++;totalOvertakes++;score+=100+combo*25;
            missionProgress[0]++;if(comboTimer>0){combo++;comboTimer=3.5f;}}
    }
    for(int i=0;i<MAX_PICKUPS;i++){
        Pickup*p=&pickups[i];if(!p->active)continue;p->y+=movement;p->spin+=dt;float x=LaneX(p->lane,p->y);
        if(CheckCollisionCircleRec((Vector2){x,p->y},18,playerRect)){
            if(p->type==PICKUP_NITRO){nitro=ClampF(nitro+38,0,100);score+=75;Burst(x,p->y,15,C_CYAN,100);}
            else if(p->type==PICKUP_SCORE){score+=500;Burst(x,p->y,15,C_AMBER,100);}
            else{speed=fminf(speed+18,c->maxSpeed);score+=125;Burst(x,p->y,15,C_GREEN,100);}
            flash=.14f;p->active=false;missionProgress[2]++;
        }else if(p->y>SCREEN_H+40)p->active=false;
    }
    if(missionProgress[0]>=10)missionDone[0]=true;
    if(missionProgress[1]>=5)missionDone[1]=true;
    if(missionProgress[2]>=3)missionDone[2]=true;
    if(currentMode==MODE_OVERTAKE&&overtakes>=25){FinishRun(true);return;}
    UpdateParticles(dt);
}
static void DrawMissions(void){
    DrawRectangleRounded((Rectangle){SCREEN_W-218,137,200,94},.08f,7,(Color){7,12,18,200});
    DrawText("RUN CHALLENGES",SCREEN_W-201,149,10,C_AMBER);
    const char*labels[3]={"OVERTAKE 10","NEAR MISS 5","COLLECT 3"};
    for(int i=0;i<3;i++){int y=171+i*18;Color col=missionDone[i]?C_GREEN:C_MUTED;
        DrawText(missionDone[i]?"*":"-",SCREEN_W-201,y,13,col);DrawText(labels[i],SCREEN_W-185,y,10,col);
        DrawText(TextFormat("%d/%d",(int)missionProgress[i],i==0?10:(i==1?5:3)),SCREEN_W-64,y,10,col);}
}
int main(void){
    SetConfigFlags(FLAG_VSYNC_HINT|FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W,SCREEN_H,"REDLINE — Highway Edition");SetTargetFPS(60);SetExitKey(KEY_NULL);LoadSave();
    while(!WindowShouldClose()){
        float dt=ClampF(GetFrameTime(),0,.04f);
        if(screenState==SCREEN_TITLE){
            roadScroll+=dt*40;roadPhase+=dt*.12f;
            if(IsKeyPressed(KEY_C))paintIndex=(paintIndex+1)%5;
            if(IsKeyPressed(KEY_T))screenState=SCREEN_TRACKS;
            if(IsKeyPressed(KEY_G))screenState=SCREEN_GARAGE;
            if(IsKeyPressed(KEY_M))screenState=SCREEN_MODES;
            if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE))ResetRun();
        }else if(screenState==SCREEN_TRACKS){
            if(IsKeyPressed(KEY_ESCAPE))screenState=SCREEN_TITLE;
            if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_UP)){do{currentTrack=(currentTrack+TRACK_COUNT-1)%TRACK_COUNT;}while(!TrackUnlocked(currentTrack));}
            if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_DOWN)){do{currentTrack=(currentTrack+1)%TRACK_COUNT;}while(!TrackUnlocked(currentTrack));}
            if(IsKeyPressed(KEY_ENTER)){if(TrackUnlocked(currentTrack)){SaveGame();screenState=SCREEN_TITLE;}}
        }else if(screenState==SCREEN_GARAGE){
            if(IsKeyPressed(KEY_ESCAPE))screenState=SCREEN_TITLE;
            if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_UP)){do{currentCar=(currentCar+CAR_COUNT-1)%CAR_COUNT;}while(!CarUnlocked(currentCar));}
            if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_DOWN)){do{currentCar=(currentCar+1)%CAR_COUNT;}while(!CarUnlocked(currentCar));}
            if(IsKeyPressed(KEY_ENTER)&&CarUnlocked(currentCar)){SaveGame();screenState=SCREEN_TITLE;}
        }else if(screenState==SCREEN_MODES){
            if(IsKeyPressed(KEY_ESCAPE))screenState=SCREEN_TITLE;
            if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_UP))currentMode=(currentMode+MODE_COUNT-1)%MODE_COUNT;
            if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_DOWN))currentMode=(currentMode+1)%MODE_COUNT;
            if(IsKeyPressed(KEY_ENTER)){SaveGame();screenState=SCREEN_TITLE;}
        }else if(screenState==SCREEN_DRIVE){
            if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_P))screenState=SCREEN_PAUSE;else UpdateGame(dt);
        }else if(screenState==SCREEN_PAUSE){
            if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_P))screenState=SCREEN_DRIVE;
            if(IsKeyPressed(KEY_ENTER))screenState=SCREEN_TITLE;
        }else if(screenState==SCREEN_CRASH||screenState==SCREEN_WIN){
            UpdateParticles(dt);
            if(IsKeyPressed(KEY_ENTER))ResetRun();
            if(IsKeyPressed(KEY_ESCAPE))screenState=SCREEN_TITLE;
        }
        float sx=0,sy=0;if(shake>0){sx=RandF(-5,5)*shake/.35f;sy=RandF(-3,3)*shake/.35f;}
        BeginDrawing();BeginMode2D((Camera2D){.offset={sx,sy},.target={0,0},.rotation=0,.zoom=1});
        DrawWorld();DrawPickups();DrawTraffic();
        float px=SCREEN_W*.5f+playerX*LaneWidth()*.82f;
        if(screenState!=SCREEN_TITLE&&screenState!=SCREEN_TRACKS&&screenState!=SCREEN_GARAGE&&screenState!=SCREEN_MODES)
            DrawCar(px,SCREEN_H-93,1,SelectedPaint(),true,ClampF(playerVelX/300,-1,1)*5);
        DrawParticles();
        if(screenState==SCREEN_TITLE)DrawTitle();
        else if(screenState==SCREEN_TRACKS)DrawSelectionScreen(false);
        else if(screenState==SCREEN_GARAGE)DrawSelectionScreen(true);
        else if(screenState==SCREEN_MODES)DrawModeScreen();
        else{
            DrawHUD();DrawMissions();
            if(screenState==SCREEN_PAUSE)DrawOverlay("PAUSED","THE HIGHWAY CAN WAIT.","P / ESC resumes the run.");
            else if(screenState==SCREEN_CRASH)DrawOverlay(newBest?"NEW PERSONAL BEST":"RUN OVER",
                TextFormat("FINAL SCORE %08d",(int)score),
                TextFormat("%.2f KM  /  %d OVERTAKES  /  %d NEAR MISSES",distanceTravelled/1000,overtakes,nearMisses));
            else if(screenState==SCREEN_WIN)DrawOverlay("CHALLENGE COMPLETE",
                TextFormat("FINAL SCORE %08d",(int)score),
                currentMode==MODE_TIME_ATTACK?"90-SECOND TIME ATTACK COMPLETE":"25 OVERTAKES COMPLETE");
        }
        if(flash>0)DrawRectangle(0,0,SCREEN_W,SCREEN_H,(Color){255,255,255,30});
        EndMode2D();EndDrawing();
    }
    SaveGame();CloseWindow();return 0;
}
