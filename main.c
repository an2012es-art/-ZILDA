#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SW 256
#define SH 192
#define WW 800
#define WH 600
#define NTREES 110
#define MAXE 20
#define COL(r,g,b) (RGB15(r,g,b)|BIT(15))

static u16 buf[SW*SH];
static u16 *gfx;

typedef struct { int x,y; } Tree;
static Tree trees[NTREES];

typedef struct { int x,y,type,hp,acc,stamp,alive; } Enemy;
static Enemy en[MAXE];
/* 0 = spider, 1 = zombie, 2 = jinn */
static const int EW[3]  = {10,12,12};
static const int EH[3]  = {8,14,14};
static const int ESPD[3]= {20,8,12};
static const int EHP[3] = {1,3,2};

static int px,py,face,hp,kills,swing,atk,cool,inv,frame,spawnT,over,camx,camy;
static int lastHp=-1,lastKills=-1,lastOver=-1;

static void rect(int x,int y,int w,int h,u16 c){
  int x0=x<0?0:x, y0=y<0?0:y;
  int x1=x+w>SW?SW:x+w, y1=y+h>SH?SH:y+h;
  for(int j=y0;j<y1;j++) for(int i=x0;i<x1;i++) buf[j*SW+i]=c;
}
static void wrect(int x,int y,int w,int h,u16 c){ rect(x-camx,y-camy,w,h,c); }

static int overlap(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh){
  return ax<bx+bw && bx<ax+aw && ay<by+bh && by<ay+ah;
}

static int blocked(int x,int y,int w,int h){
  if(x<0||y<0||x+w>WW||y+h>WH) return 1;
  for(int i=0;i<NTREES;i++)
    if(overlap(x,y,w,h,trees[i].x,trees[i].y,16,16)) return 1;
  return 0;
}

static void reset(void){
  px=400; py=300; face=1; hp=5; kills=0; swing=0;
  atk=0; cool=0; inv=0; spawnT=60; over=0;
  for(int i=0;i<MAXE;i++) en[i].alive=0;
  lastHp=-1;
}

static void spawn(void){
  int slot=-1;
  for(int i=0;i<MAXE;i++) if(!en[i].alive){slot=i;break;}
  if(slot<0) return;
  for(int tries=0;tries<20;tries++){
    int r=rand()%10;
    int t = r<4 ? 0 : (r<8 ? 1 : 2);
    int x=rand()%(WW-20), y=rand()%(WH-20);
    if(abs(x-px)<140 && abs(y-py)<110) continue;
    if(t!=2 && blocked(x,y,EW[t],EH[t])) continue;
    Enemy *e=&en[slot];
    e->x=x; e->y=y; e->type=t; e->hp=EHP[t];
    e->acc=0; e->stamp=-1; e->alive=1;
    return;
  }
}

static void swordRect(int *sx,int *sy,int *sw,int *sh){
  switch(face){
    case 0: *sx=px+3;  *sy=py-18; *sw=6;  *sh=18; break;
    case 1: *sx=px+3;  *sy=py+12; *sw=6;  *sh=18; break;
    case 2: *sx=px-18; *sy=py+3;  *sw=18; *sh=6;  break;
    default:*sx=px+12; *sy=py+3;  *sw=18; *sh=6;  break;
  }
}

static void update(u32 held,u32 down){
  if(over){ if(down&KEY_START) reset(); return; }

  int dx=0,dy=0;
  if(held&KEY_LEFT){dx=-2;face=2;}
  if(held&KEY_RIGHT){dx=2;face=3;}
  if(held&KEY_UP){dy=-2;face=0;}
  if(held&KEY_DOWN){dy=2;face=1;}
  if(dx && !blocked(px+dx,py,12,12)) px+=dx;
  if(dy && !blocked(px,py+dy,12,12)) py+=dy;

  if((down&(KEY_A|KEY_B)) && cool==0){ swing++; atk=10; cool=22; }
  if(atk>0) atk--;
  if(cool>0) cool--;
  if(inv>0) inv--;

  int sx=0,sy=0,sw=0,sh=0;
  swordRect(&sx,&sy,&sw,&sh);

  for(int i=0;i<MAXE;i++){
    Enemy *e=&en[i];
    if(!e->alive) continue;
    int t=e->type;

    e->acc+=ESPD[t];
    int steps=e->acc/16; e->acc%=16;
    while(steps--){
      int ex=e->x+EW[t]/2, ey=e->y+EH[t]/2;
      int tx=px+6, ty=py+6;
      int mx=0,my=0;
      if(tx-ex>2) mx=1; else if(tx-ex<-2) mx=-1;
      if(ty-ey>2) my=1; else if(ty-ey<-2) my=-1;
      if(mx && (t==2 || !blocked(e->x+mx,e->y,EW[t],EH[t]))) e->x+=mx;
      if(my && (t==2 || !blocked(e->x,e->y+my,EW[t],EH[t]))) e->y+=my;
    }

    if(atk>0 && e->stamp!=swing &&
       overlap(sx,sy,sw,sh,e->x,e->y,EW[t],EH[t])){
      e->stamp=swing;
      e->hp--;
      if(e->hp<=0){ e->alive=0; kills++; continue; }
    }

    if(inv==0 && overlap(px,py,12,12,e->x,e->y,EW[t],EH[t])){
      hp--; inv=60;
      if(hp<=0){ hp=0; over=1; }
    }
  }

  if(--spawnT<=0){
    spawn();
    spawnT = 100-kills*2;
    if(spawnT<30) spawnT=30;
  }
}

static void drawEnemy(Enemy *e){
  int x=e->x,y=e->y;
  if(e->type==0){
    wrect(x,y+1,10,1,COL(2,2,2));
    wrect(x,y+5,10,1,COL(2,2,2));
    wrect(x+2,y+2,6,5,COL(3,3,3));
    wrect(x+3,y+3,1,1,COL(31,0,0));
    wrect(x+6,y+3,1,1,COL(31,0,0));
  } else if(e->type==1){
    wrect(x+1,y+5,10,9,COL(8,14,8));
    wrect(x,y+6,12,2,COL(10,16,10));
    wrect(x+2,y,8,6,COL(12,20,12));
    wrect(x+3,y+2,2,2,COL(31,0,0));
    wrect(x+7,y+2,2,2,COL(31,0,0));
  } else {
    wrect(x,y+2,12,10,COL(18,4,24));
    wrect(x+3,y+12,6,2,COL(14,3,20));
    wrect(x+2,y,8,6,COL(24,8,28));
    wrect(x+3,y+2,2,2,COL(31,31,0));
    wrect(x+7,y+2,2,2,COL(31,31,0));
  }
}

static void draw(void){
  rect(0,0,SW,SH,COL(1,5,2));
  for(int ty=camy/32; ty*32<camy+SH; ty++)
    for(int tx=camx/32; tx*32<camx+SW; tx++)
      if((tx*7+ty*13)%3==0) wrect(tx*32,ty*32,32,32,COL(2,7,3));

  for(int i=0;i<NTREES;i++){
    int x=trees[i].x,y=trees[i].y;
    wrect(x+6,y+10,4,6,COL(10,5,2));
    wrect(x,y+2,16,10,COL(0,7,1));
    wrect(x+3,y-3,10,6,COL(1,9,2));
  }

  for(int i=0;i<MAXE;i++) if(en[i].alive) drawEnemy(&en[i]);

  if(!(inv>0 && (frame&4))){
    wrect(px,py+4,12,8,COL(4,8,26));
    wrect(px+2,py,8,5,COL(26,20,14));
    wrect(px+2,py,8,2,COL(3,2,1));
  }
  if(atk>0){
    int sx,sy,sw,sh; swordRect(&sx,&sy,&sw,&sh);
    wrect(sx,sy,sw,sh,COL(28,28,31));
  }

  for(int i=0;i<hp;i++) rect(4+i*10,4,8,8,COL(31,3,3));
  if(over) rect(40,80,176,32,COL(10,0,0));
}

int main(void){
  videoSetMode(MODE_5_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  int bg=bgInit(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
  gfx=bgGetGfxPtr(bg);
  consoleDemoInit();

  srand(1234);
  for(int i=0;i<NTREES;i++){
    int x,y;
    do { x=rand()%(WW-16); y=rand()%(WH-16); }
    while(abs(x-400)<70 && abs(y-300)<70);
    trees[i].x=x; trees[i].y=y;
  }
  srand((unsigned)time(NULL));
  reset();

  while(1){
    scanKeys();
    update(keysHeld(),keysDown());
    frame++;

    camx=px-SW/2+6; camy=py-SH/2+6;
    if(camx<0) camx=0; if(camx>WW-SW) camx=WW-SW;
    if(camy<0) camy=0; if(camy>WH-SH) camy=WH-SH;
    draw();

    if(hp!=lastHp||kills!=lastKills||over!=lastOver){
      iprintf("\x1b[2J");
      iprintf("Forest Hunt\n\nD-pad: move\nA/B: sword\n\nHP: %d  Kills: %d\n",hp,kills);
      if(over) iprintf("\nGAME OVER\nPress START\n");
      lastHp=hp; lastKills=kills; lastOver=over;
    }

    swiWaitForVBlank();
    dmaCopy(buf,gfx,sizeof(buf));
  }
  return 0;
}
