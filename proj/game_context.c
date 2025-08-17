/* game_context.c */
#include <string.h>
#include <stdio.h>
#include "game_context.h"

static const char *gaszRoomNames[] = {
  "SALA_CENTRAL",
  "SALA_NORTE",
  "SALA_SUL",
  "SALA_LESTE",
  "SALA_OESTE"
};

static SDL_Texture *pBakeText(SDL_Renderer *pRenderer, TTF_Font *pFont, const char *psz, int *piW, int *piH)
{
  SDL_Surface *pSurf;
  SDL_Texture *pTex;

  pSurf = TTF_RenderText_Blended(pFont, psz, (SDL_Color){255, 255, 255, 255});
  if (pSurf == NULL)
    return NULL;
  pTex = SDL_CreateTextureFromSurface(pRenderer, pSurf);
  if (pTex != NULL)
  {
    *piW = pSurf->w;
    *piH = pSurf->h;
  }
  SDL_FreeSurface(pSurf);
  return pTex;
}

void vTextcacheUpdate(SDL_Renderer *pRenderer, TTF_Font *pFont, STRUCT_TEXTCACHE *pst, const char *pszNew)
{
  SDL_Texture *pNew;
  int iW;
  int iH;

  if (pst->pTex != NULL)
  {
    SDL_DestroyTexture(pst->pTex);
    pst->pTex = NULL;
  }
  pNew = pBakeText(pRenderer, pFont, pszNew, &iW, &iH);
  if (pNew != NULL)
  {
    pst->pTex = pNew;
    pst->iW = iW;
    pst->iH = iH;
  }
}

void vTextcacheFree(STRUCT_TEXTCACHE *pst)
{
  if (pst->pTex != NULL)
  {
    SDL_DestroyTexture(pst->pTex);
    pst->pTex = NULL;
  }
  pst->iW = 0;
  pst->iH = 0;
}

static void vHudcacheInit(STRUCT_HUDCACHE *pstHud)
{
  pstHud->stSala.pTex = NULL;
  pstHud->stSala.iW = 0;
  pstHud->stSala.iH = 0;
  pstHud->stModo.pTex = NULL;
  pstHud->stModo.iW = 0;
  pstHud->stModo.iH = 0;
  pstHud->stDirecao.pTex = NULL;
  pstHud->stDirecao.iW = 0;
  pstHud->stDirecao.iH = 0;
  pstHud->iLastRoomId = -1;
  pstHud->iLastSpeedMode = -1;
  pstHud->iLastLookX = 9999;
  pstHud->iLastLookY = 9999;
}

static void vHudcacheFree(STRUCT_HUDCACHE *pstHud)
{
  vTextcacheFree(&pstHud->stSala);
  vTextcacheFree(&pstHud->stModo);
  vTextcacheFree(&pstHud->stDirecao);
}

static void vRenderInitDefaults(STRUCT_RENDERCTX *pstRend)
{
  pstRend->pWindow = NULL;
  pstRend->pRenderer = NULL;
  pstRend->pFont = NULL;
  pstRend->iWidth = WIDTH;
  pstRend->iHeight = HEIGHT;
}

static void vRuntimeInitDefaults(STRUCT_RUNTIMESTATE *pstState)
{
  pstState->iMapOpen = 0;
  pstState->iSpeedMode = 0;
  pstState->iLookDirX = 1;
  pstState->iLookDirY = 0;

  pstState->iPlayerHealth = MAX_HEALTH;
  pstState->iIsInvulnerable = 0;
  pstState->iTrailIndex = 0;
  pstState->iCanBlink = 1;

  pstState->iPlayerVx = 0;
  pstState->iPlayerVy = 0;

  pstState->iBulletCount = 0;

  pstState->uiInvulnerableStart = 0;
  pstState->uiLastBlinkTime = 0;
  pstState->iStamina = STAMINA_MAX;
  pstState->uiLastStaminaTick = 0;
  pstState->iStamDrainAcc = 0;
  pstState->iStamRegenAcc = 0;
 
  pstState->eCurrentRoom = CENTER;

  pstState->stPlayer.x = WIDTH / 2 - PLAYER_SIZE / 2;
  pstState->stPlayer.y = HEIGHT / 2 - PLAYER_SIZE / 2;
  pstState->stPlayer.w = PLAYER_SIZE;
  pstState->stPlayer.h = PLAYER_SIZE;
}

static void vWorldInitDefaults(STRUCT_WORLD *pstWorld)
{
  memset(pstWorld->astMap, 0, sizeof(pstWorld->astMap));
  memset(pstWorld->astPlayerTrails, 0, sizeof(pstWorld->astPlayerTrails));
  memset(pstWorld->astPlayerBullets, 0, sizeof(pstWorld->astPlayerBullets));
  memset(pstWorld->astEnemyBullets, 0, sizeof(pstWorld->astEnemyBullets));

  pstWorld->astMap[2][2].iExists = 1;
  pstWorld->astMap[2][2].iVisited = 1;
  pstWorld->astMap[2][2].eId = CENTER;
  pstWorld->astMap[1][2].iExists = 1;
  pstWorld->astMap[1][2].eId = NORTH;
  pstWorld->astMap[3][2].iExists = 1;
  pstWorld->astMap[3][2].eId = SOUTH;
  pstWorld->astMap[2][1].iExists = 1;
  pstWorld->astMap[2][1].eId = WEST;
  pstWorld->astMap[2][3].iExists = 1;
  pstWorld->astMap[2][3].eId = EAST;
}

static void vResInitDefaults(STRUCT_RESOURCESCACHE *pstRes)
{
  pstRes->pCircleTex = NULL;
  pstRes->iCircleRadius = 0;
}

int iGameCtxInit(STRUCT_GAMECTX *pstCtx)
{
  int iOk;

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");

  vRenderInitDefaults(&pstCtx->stRender);
  vRuntimeInitDefaults(&pstCtx->stState);
  vWorldInitDefaults(&pstCtx->stWorld);
  vResInitDefaults(&pstCtx->stRes);
  vHudcacheInit(&pstCtx->stHud);

  pstCtx->apszRoomNames = gaszRoomNames;

  iOk = SDL_Init(SDL_INIT_VIDEO);
  if (iOk != 0)
    return -1;

  iOk = TTF_Init();
  if (iOk != 0)
    return -2;

  pstCtx->stRender.pWindow = SDL_CreateWindow("Bullet Hell Lab",
                                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                              pstCtx->stRender.iWidth, pstCtx->stRender.iHeight,
                                              SDL_WINDOW_SHOWN);
  if (pstCtx->stRender.pWindow == NULL)
    return -3;

  pstCtx->stRender.pRenderer = SDL_CreateRenderer(pstCtx->stRender.pWindow, -1, SDL_RENDERER_ACCELERATED);
  if (pstCtx->stRender.pRenderer == NULL)
    return -4;

  SDL_SetRenderDrawBlendMode(pstCtx->stRender.pRenderer, SDL_BLENDMODE_BLEND);

  pstCtx->stRender.pFont = TTF_OpenFont("FiraCode.ttf", 16);
  if (pstCtx->stRender.pFont == NULL)
    return -5;

  return 0;
}

const char *pszGetXYDirectionName(int iX, int iY) {
  if (iX == -1) {
    if (iY == -1) return "ESQ-CIMA";
    if (iY ==  1) return "ESQ-BAIXO";
    if (iY ==  0) return "ESQUERDA";
  } else if (iX == 1) {
    if (iY == -1) return "DIR-CIMA";
    if (iY ==  1) return "DIR-BAIXO";
    if (iY ==  0) return "DIREITA";
  } else if (iX == 0) {
    if (iY == -1) return "CIMA";
    if (iY ==  1) return "BAIXO";
    if (iY ==  0) return "PARADO";
  }
  return "PARADO";
}

void vGameCtxShutdown(STRUCT_GAMECTX *pstCtx)
{
  vHudcacheFree(&pstCtx->stHud);

  if (pstCtx->stRes.pCircleTex != NULL)
  {
    SDL_DestroyTexture(pstCtx->stRes.pCircleTex);
    pstCtx->stRes.pCircleTex = NULL;
  }

  if (pstCtx->stRender.pFont)
  {
    TTF_CloseFont(pstCtx->stRender.pFont);
    pstCtx->stRender.pFont = NULL;
  }

  if (pstCtx->stRender.pRenderer)
  {
    SDL_DestroyRenderer(pstCtx->stRender.pRenderer);
    pstCtx->stRender.pRenderer = NULL;
  }

  if (pstCtx->stRender.pWindow)
  {
    SDL_DestroyWindow(pstCtx->stRender.pWindow);
    pstCtx->stRender.pWindow = NULL;
  }

  TTF_Quit();
  SDL_Quit();
}
