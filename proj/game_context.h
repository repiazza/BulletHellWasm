/* game_context.h */
#ifndef GAME_CONTEXT_H
  #define GAME_CONTEXT_H

  #include <SDL2/SDL.h>
  #include <SDL2/SDL_ttf.h>
  #include "game_config.h"

  #ifndef TRUE
    #define TRUE 1
  #endif
  #ifndef FALSE
    #define FALSE 0
  #endif

  typedef enum ENUM_ROOMID {
    CENTER,
    NORTH,
    SOUTH,
    EAST,
    WEST
  } ENUM_ROOMID;

  typedef struct STRUCT_PLAYERBULLET {
    float fX;
    float fY;
    float fVx;
    float fVy;
    float fRadius;
    int iActive;
    ENUM_ROOMID eRoom; /* sala onde o tiro foi criado */
  } STRUCT_PLAYERBULLET;


  typedef struct STRUCT_TRAIL{
    int iX; 
    int iY;
    int iAlpha;
  } STRUCT_TRAIL;

  typedef struct STRUCT_ROOM{
    int iExists;
    int iVisited;
    ENUM_ROOMID eId;
  } STRUCT_ROOM;

  typedef struct STRUCT_BULLET{
    int iType;
    float fX;
    float fY;
    float fVx;
    float fVy;
    float fAngle;
    float fRadius;
    ENUM_ROOMID eRoom;
  } STRUCT_BULLET;

  typedef struct STRUCT_TEXTCACHE {
    SDL_Texture *pTex;
    int iW;
    int iH;
  } STRUCT_TEXTCACHE;

  typedef struct STRUCT_HUDCACHE {
    STRUCT_TEXTCACHE stSala;
    STRUCT_TEXTCACHE stModo;
    STRUCT_TEXTCACHE stDirecao;
    STRUCT_TEXTCACHE stCoord;
    int iLastRoomId;
    int iLastSpeedMode;
    int iLastLookX;
    int iLastLookY;
  } STRUCT_HUDCACHE;

  typedef struct STRUCT_RENDERCTX {
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    TTF_Font *pFont;
    int iWidth;
    int iHeight;
  } STRUCT_RENDERCTX;

  typedef struct STRUCT_RUNTIMESTATE {
    int iMapOpen;
    int iSpeedMode;
    int iLookDirX;
    int iLookDirY;
    int iPlayerHealth;
    int iIsInvulnerable;
    int iTrailIndex;
    int iCanBlink;
    int iPlayerVx;
    int iPlayerVy;
    int iBulletCount;
    Uint32 uiInvulnerableStart;
    Uint32 uiLastBlinkTime;
    int iStamina;
    Uint32 uiLastStaminaTick;
    int iStamDrainAcc;
    int iStamRegenAcc;
    ENUM_ROOMID eCurrentRoom;
    SDL_Rect stPlayer;
  } STRUCT_RUNTIMESTATE;

  typedef struct STRUCT_WORLD {
    STRUCT_ROOM astMap[MAP_ROWS][MAP_COLS];
    STRUCT_TRAIL astPlayerTrails[MAX_TRAILS];
    STRUCT_PLAYERBULLET astPlayerBullets[MAX_PLAYER_BULLETS];
    STRUCT_BULLET astEnemyBullets[MAX_BULLETS];
    float afBulletHomeX[MAX_BULLETS];
    float afBulletHomeY[MAX_BULLETS];
    float afBulletHomeVx[MAX_BULLETS];
    float afBulletHomeVy[MAX_BULLETS];
    float afBulletHomeAngle[MAX_BULLETS];
    ENUM_ROOMID aeBulletHomeRoom[MAX_BULLETS];
  } STRUCT_WORLD;

  typedef struct STRUCT_RESOURCESCACHE {
    SDL_Texture *pCircleTex;
    int iCircleRadius;
  } STRUCT_RESOURCESCACHE;

  typedef struct STRUCT_GAMECTX {
    const char **apszRoomNames;
    STRUCT_RENDERCTX stRender;
    STRUCT_RUNTIMESTATE stState;
    STRUCT_WORLD stWorld;
    STRUCT_RESOURCESCACHE stRes;
    STRUCT_HUDCACHE stHud;
  } STRUCT_GAMECTX;

  /* ciclo de vida */
  int iGameCtxInit(STRUCT_GAMECTX *pstCtx);
  void vGameCtxShutdown(STRUCT_GAMECTX *pstCtx);

  /* util de texto */
  void vTextcacheUpdate(SDL_Renderer *pRenderer, TTF_Font *pFont, STRUCT_TEXTCACHE *pst, const char *pszNew);
  void vTextcacheFree(STRUCT_TEXTCACHE *pst);
  const char *pszGetXYDirectionName(int iX, int iY);

  extern STRUCT_GAMECTX gstCtx; 
  extern STRUCT_GAMECTX *gpstCtx; 
#endif /* GAME_CONTEXT_H */
