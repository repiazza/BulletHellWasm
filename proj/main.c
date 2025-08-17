#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <emscripten.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "game_config.h"
#include "main.h"
#include "game_context.h"

/* ====== CONTEXTO ====== */
STRUCT_GAMECTX gstCtx;           /* vida inteira do programa */
STRUCT_GAMECTX *gpstCtx = NULL;  /* ponte global para facilitar o acesso */

/* ===== Utils ===== */
int bStrIsEmpty(const char *kpszStr) {
  if (kpszStr == NULL) return TRUE;
  if (kpszStr[0] == '\0') return TRUE;
  if (!strcmp(kpszStr, "\n")) return TRUE;
  return FALSE;
}

void vDrawStaminaBar() {
  const int iBarWidth = 20;
  const int iBarHeight = 100;
  const int iX = WIDTH - 60; /* à esquerda da barra de blink */
  const int iY = HEIGHT - 30 - iBarHeight;

  SDL_Rect stBackground;
  int iFillHeight;
  SDL_Rect stFill;

  stBackground.x = iX;
  stBackground.y = iY;
  stBackground.w = iBarWidth;
  stBackground.h = iBarHeight;

  SDL_SetRenderDrawColor(pRenderer, 80,80,80,255);
  SDL_RenderFillRect(pRenderer, &stBackground);

  iFillHeight = (int)((iBarHeight * giStamina) / (float)STAMINA_MAX);
  stFill.x = iX;
  stFill.y = iY + iBarHeight - iFillHeight;
  stFill.w = iBarWidth;
  stFill.h = iFillHeight;

  SDL_SetRenderDrawColor(pRenderer, 255,255,0,255); /* amarelo */
  SDL_RenderFillRect(pRenderer, &stFill);

  SDL_SetRenderDrawColor(pRenderer, 255,255,255,255);
  SDL_RenderDrawRect(pRenderer, &stBackground);
}


/* ===== Texto imediato (para rótulos dinâmicos) ===== */
void vDrawText(const char *pszText, int iX, int iY, SDL_Color stColor) {
  SDL_Surface *pSurface;
  SDL_Texture *pTexture;
  SDL_Rect stDst;

  pSurface = TTF_RenderText_Blended(pFont, pszText, stColor);
  if (!pSurface) return;

  pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
  if (!pTexture) { SDL_FreeSurface(pSurface); return; }

  stDst.x = iX;
  stDst.y = iY;
  stDst.w = pSurface->w;
  stDst.h = pSurface->h;

  SDL_RenderCopy(pRenderer, pTexture, NULL, &stDst);

  SDL_FreeSurface(pSurface);
  SDL_DestroyTexture(pTexture);
}

/* captura o estado atual como “home/spawn” de cada bullet inimigo */
static void vBakeBulletHomes(void) {
  int i;
  for (i = 0; i < giBulletCount; i++) {
    afBulletHomeX[i] = agstBullets[i].fX;
    afBulletHomeY[i] = agstBullets[i].fY;
    afBulletHomeVx[i] = agstBullets[i].fVx;
    afBulletHomeVy[i] = agstBullets[i].fVy;
    afBulletHomeAngle[i] = agstBullets[i].fAngle;
    aeBulletHomeRoom[i] = agstBullets[i].eRoom;
  }
}
/* reseta um bullet inimigo para o seu estado de fábrica */
static void vResetBulletToHome(int iIdx) {
  agstBullets[iIdx].fX = afBulletHomeX[iIdx];
  agstBullets[iIdx].fY = afBulletHomeY[iIdx];
  agstBullets[iIdx].fVx = afBulletHomeVx[iIdx];
  agstBullets[iIdx].fVy = afBulletHomeVy[iIdx];
  agstBullets[iIdx].fAngle = afBulletHomeAngle[iIdx];
  agstBullets[iIdx].eRoom = aeBulletHomeRoom[iIdx];
}

/* ===== Círculo cacheado ===== */
static void vEnsureCircleTexture(int iRadius) {
  int iW;
  int iH;
  SDL_Surface *pSurf;
  Uint32 *pPix;
  int iR2;
  int y;
  int x;
  SDL_Texture *pNewTex;

  if (gpCircleTex != NULL && giCircleR == iRadius) return;

  if (gpCircleTex != NULL) {
    SDL_DestroyTexture(gpCircleTex);
    gpCircleTex = NULL;
  }

  giCircleR = iRadius;
  iW = giCircleR * 2;
  iH = giCircleR * 2;

  pSurf = SDL_CreateRGBSurfaceWithFormat(0, iW, iH, 32, SDL_PIXELFORMAT_RGBA32);
  if (!pSurf) return;

  pPix = (Uint32 *)pSurf->pixels;
  iR2 = giCircleR * giCircleR;

  for (y = 0; y < iH; y++) {
    for (x = 0; x < iW; x++) {
      int iDx;
      int iDy;
      iDx = giCircleR - x;
      iDy = giCircleR - y;
      pPix[y * iW + x] = ((iDx * iDx + iDy * iDy) <= iR2) ? 0xFF0000FFu : 0x00000000u;
    }
  }

  pNewTex = SDL_CreateTextureFromSurface(pRenderer, pSurf);
  SDL_FreeSurface(pSurf);

  if (pNewTex) {
    SDL_SetTextureBlendMode(pNewTex, SDL_BLENDMODE_BLEND);
    gpCircleTex = pNewTex;
  }
}

void vDrawCircle(float fCx, float fCy, float fRadius) {
  SDL_Rect stDst;
  int iR;

  iR = (int)fRadius;
  if (iR <= 0) return;

  vEnsureCircleTexture(iR);
  if (!gpCircleTex) return;

  stDst.x = (int)(fCx - iR);
  stDst.y = (int)(fCy - iR);
  stDst.w = iR * 2;
  stDst.h = iR * 2;

  SDL_RenderCopy(pRenderer, gpCircleTex, NULL, &stDst);
}

/* ===== Gameplay: projéteis do player ===== */
void vFirePlayerBullet() {
  int i;
  float fCenterX;
  float fCenterY;
  float fNorm;
  float fDx;
  float fDy;

  if (giLookDirX == 0 && giLookDirY == 0) return;

  for (i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (!astPlayerBullets[i].iActive) {
      fCenterX = stPlayer.x + PLAYER_SIZE / 2.0f;
      fCenterY = stPlayer.y + PLAYER_SIZE / 2.0f;

      fNorm = sqrtf((float)(giLookDirX * giLookDirX + giLookDirY * giLookDirY));
      if (fNorm == 0.0f) return;
      fDx = (float)giLookDirX / fNorm;
      fDy = (float)giLookDirY / fNorm;

      astPlayerBullets[i].fX = fCenterX;
      astPlayerBullets[i].fY = fCenterY;
      astPlayerBullets[i].fVx = fDx * PLAYER_BULLET_SPEED;
      astPlayerBullets[i].fVy = fDy * PLAYER_BULLET_SPEED;
      astPlayerBullets[i].fRadius = PLAYER_SIZE / 3.0f;
      astPlayerBullets[i].iActive = 1;
      astPlayerBullets[i].eRoom = eCurrentRoom; /* associar sala ao tiro */
      break;
    }
  }
}

void vDrawPlayerBullets() {
  int i;
  for (i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (astPlayerBullets[i].iActive && astPlayerBullets[i].eRoom == eCurrentRoom) {
      vDrawCircle(astPlayerBullets[i].fX, astPlayerBullets[i].fY, astPlayerBullets[i].fRadius);
    }
  }
}

void vDrawEnemyBullets() {
  int i;
  for (i = 0; i < giBulletCount; i++) {
    STRUCT_BULLET *pstB;
    pstB = &agstBullets[i];
    if (pstB->eRoom == eCurrentRoom) {
      vDrawCircle(pstB->fX, pstB->fY, pstB->fRadius);
    }
  }
}

void vDrawHealthBar() {
  const int iBarWidth = 180;
  const int iBarHeight = 20;
  const int iX = 10;
  const int iY = 10;

  SDL_Rect stBackground;
  float fUnitWidth;
  int iRedWidth;
  SDL_Rect stForeground;

  stBackground.x = iX;
  stBackground.y = iY;
  stBackground.w = iBarWidth;
  stBackground.h = iBarHeight;

  SDL_SetRenderDrawColor(pRenderer, 100,100,100,255);
  SDL_RenderFillRect(pRenderer, &stBackground);

  fUnitWidth = iBarWidth / (float)MAX_HEALTH;
  iRedWidth = (int)(fUnitWidth * giPlayerHealth);
  stForeground.x = iX;
  stForeground.y = iY;
  stForeground.w = iRedWidth;
  stForeground.h = iBarHeight;
  SDL_SetRenderDrawColor(pRenderer, 200,0,0,255);
  SDL_RenderFillRect(pRenderer, &stForeground);

  SDL_SetRenderDrawColor(pRenderer, 255,255,255,255);
  SDL_RenderDrawRect(pRenderer, &stBackground);
}

/* ===== HUD ===== */
static void vHudEnsureSala(void) {
  char szBuffer[128];
  if (giLastRoomId != (int)eCurrentRoom) {
    snprintf(szBuffer, sizeof(szBuffer), "Sala: %s", apszRoomNames[eCurrentRoom]);
    vTextcacheUpdate(pRenderer, pFont, &gstTxtSala, szBuffer);
    giLastRoomId = (int)eCurrentRoom;
  }
}

static void vHudEnsureModo(void) {
  char szBuffer[64];
  const char *pszMode;

  if (giLastSpeedMode == giSpeedMode) return;

  pszMode = (giSpeedMode == 1) ? "LENTO" :
            (giSpeedMode == 2) ? "CORRENDO" : "NORMAL";

  snprintf(szBuffer, sizeof(szBuffer), "Modo: %s", pszMode);
  vTextcacheUpdate(pRenderer, pFont, &gstTxtModo, szBuffer);
  giLastSpeedMode = giSpeedMode;
}

/* Coord e Direção usam caches independentes para não se “atrapalharem” */
static void vHudEnsureCoord(void) {
  static int iLastLookXCoord = 9999;
  static int iLastLookYCoord = 9999;
  char szBuffer[128];

  if (iLastLookXCoord == giLookDirX && iLastLookYCoord == giLookDirY) return;

  snprintf(szBuffer, sizeof(szBuffer),
    "At/Ant:(%d,%d)/(%d,%d)",
    giLookDirX, giLookDirY, 
    iLastLookXCoord == 9999 ? 0 : iLastLookXCoord,
    iLastLookYCoord == 9999 ? 0 : iLastLookYCoord
  );
  vTextcacheUpdate(pRenderer, pFont, &gstTxtCoord, szBuffer);
  iLastLookXCoord = giLookDirX;
  iLastLookYCoord = giLookDirY;
}

static void vHudEnsureDirecao(void) {
  static int iLastLookXDir = 9999;
  static int iLastLookYDir = 9999;
  char szBuffer[64];

  if (iLastLookXDir == giLookDirX && iLastLookYDir == giLookDirY) return;

  snprintf(szBuffer, sizeof(szBuffer), "Direcao: %s",
    pszGetXYDirectionName(giLookDirX, giLookDirY)
  );
  vTextcacheUpdate(pRenderer, pFont, &gstTxtDir, szBuffer);
  iLastLookXDir = giLookDirX;
  iLastLookYDir = giLookDirY;
}

void vDrawHUD() {
  SDL_Color stTextColor = {255,255,255,255};
  SDL_Rect stHudBg = { WIDTH - 250, 6, 240, 140 };
  char szBuffer[128];
  int i;
  int iFound;

  SDL_SetRenderDrawColor(pRenderer, 0,0,0,100);
  SDL_RenderFillRect(pRenderer, &stHudBg);
  SDL_SetRenderDrawColor(pRenderer, 200,200,200,255);
  SDL_RenderDrawRect(pRenderer, &stHudBg);

  vHudEnsureSala();
  vHudEnsureModo();
  vHudEnsureDirecao();
  vHudEnsureCoord();

  if (gstTxtSala.pTex) {
    SDL_RenderCopy(pRenderer, gstTxtSala.pTex, NULL,
      &(SDL_Rect){ stHudBg.x + 10, stHudBg.y + 10, gstTxtSala.iW, gstTxtSala.iH });
  }

  snprintf(szBuffer, sizeof(szBuffer), "Player: (%d, %d)", stPlayer.x, stPlayer.y);
  vDrawText(szBuffer, stHudBg.x + 10, stHudBg.y + 30, stTextColor);

  iFound = 0;
  for (i = 0; i < giBulletCount; i++) {
    if (agstBullets[i].eRoom == eCurrentRoom) {
      snprintf(szBuffer, sizeof(szBuffer), "Bullet: (%d, %d)", (int)agstBullets[i].fX, (int)agstBullets[i].fY);
      vDrawText(szBuffer, stHudBg.x + 10, stHudBg.y + 50, stTextColor);
      iFound = 1;
      break;
    }
  }
  if (!iFound) vDrawText("Bullet: N/A", stHudBg.x + 10, stHudBg.y + 50, stTextColor);

  if (gstTxtModo.pTex) {
    SDL_RenderCopy(pRenderer, gstTxtModo.pTex, NULL,
      &(SDL_Rect){ stHudBg.x + 40, stHudBg.y + 80, gstTxtModo.iW, gstTxtModo.iH });
  }
  if (gstTxtDir.pTex) {
    SDL_RenderCopy(pRenderer, gstTxtDir.pTex, NULL,
      &(SDL_Rect){ stHudBg.x + 40, stHudBg.y + 100, gstTxtDir.iW, gstTxtDir.iH });
  }
  if (gstTxtCoord.pTex) {
    SDL_RenderCopy(pRenderer, gstTxtCoord.pTex, NULL,
      &(SDL_Rect){ stHudBg.x + 20, stHudBg.y + 120, gstTxtCoord.iW, gstTxtCoord.iH });
  }
}

/* ===== Input ===== */

/* Movimento: lê teclas *seguradas* (WASD e setas) e calcula Vx/Vy */
static void vUpdateMoveFromKeysHeld(void) {
  const Uint8 *k;
  int iLeft;
  int iRight;
  int iUp;
  int iDown;

  k = SDL_GetKeyboardState(NULL);

  iLeft  = k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_A];
  iRight = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
  iUp    = k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W];
  iDown  = k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S];

  giPlayerVx = (iRight ? 1 : 0) - (iLeft ? 1 : 0);
  giPlayerVy = (iDown  ? 1 : 0) - (iUp   ? 1 : 0);
}

/* Mira: setas -> mira; se nenhuma seta, mira segue o movimento atual */
static void vUpdateLookDirFromKeysHeld(void) {
  const Uint8 *ui8Keys;
  int iLeft;
  int iRight;
  int iUp;
  int iDown;
  int iNewLookX;
  int iNewLookY;

  ui8Keys = SDL_GetKeyboardState(NULL);

  iLeft  = ui8Keys[SDL_SCANCODE_LEFT]  ? 1 : 0;
  iRight = ui8Keys[SDL_SCANCODE_RIGHT] ? 1 : 0;
  iUp    = ui8Keys[SDL_SCANCODE_UP]    ? 1 : 0;
  iDown  = ui8Keys[SDL_SCANCODE_DOWN]  ? 1 : 0;

  iNewLookX = (iRight ? 1 : 0) - (iLeft ? 1 : 0);
  iNewLookY = (iDown  ? 1 : 0) - (iUp   ? 1 : 0);

  if (iNewLookX != 0 || iNewLookY != 0) {
    giLookDirX = iNewLookX;
    giLookDirY = iNewLookY;
  } else if (giPlayerVx != 0 || giPlayerVy != 0) {
    giLookDirX = giPlayerVx;
    giLookDirY = giPlayerVy;
  }
}

void vHandleInput(const SDL_Event *pstEvent) {
  if (pstEvent->type == SDL_KEYDOWN && !pstEvent->key.repeat) {
    switch (pstEvent->key.keysym.sym) {
      case SDLK_TAB:      giMapOpen = !giMapOpen; break;
      case SDLK_c:        giSpeedMode = (giSpeedMode == 1 ? 0 : 1); break;
      case SDLK_LSHIFT:
      case SDLK_RSHIFT:
        if (giStamina > STAMINA_MIN_TO_SPRINT) 
          giSpeedMode = 2;
        break;
      case SDLK_DELETE:   
        vFirePlayerBullet(); 
        break;
      case SDLK_SPACE: {
        if (giCanBlink && (giPlayerVx != 0 || giPlayerVy != 0)) {
          int iNewX;
          int iNewY;

          iNewX = stPlayer.x + giPlayerVx * BLINK_DISTANCE;
          iNewY = stPlayer.y + giPlayerVy * BLINK_DISTANCE;
          if (iNewX < WALL_THICKNESS) iNewX = WALL_THICKNESS + 1;
          if (iNewX + PLAYER_SIZE > WIDTH - WALL_THICKNESS)
            iNewX = WIDTH - WALL_THICKNESS - PLAYER_SIZE - 1;
          if (iNewY < WALL_THICKNESS) iNewY = WALL_THICKNESS + 1;
          if (iNewY + PLAYER_SIZE > HEIGHT - WALL_THICKNESS)
            iNewY = HEIGHT - WALL_THICKNESS - PLAYER_SIZE - 1;

          stPlayer.x = iNewX; stPlayer.y = iNewY;

          {
            int i;
            for (i = 0; i < MAX_TRAILS; i++) astPlayerTrails[i].iAlpha = 0;
          }
          giCanBlink = 0; uiLastBlinkTime = SDL_GetTicks();
        }
      } break;
    }
  } else if (pstEvent->type == SDL_KEYUP) {
    switch (pstEvent->key.keysym.sym) {
      case SDLK_LSHIFT:
      case SDLK_RSHIFT:
        if (giSpeedMode == 2) giSpeedMode = 0;
        break;
    }
  }
}

/* ===== Minimap / Sala / Colisões / Mundo ===== */
void vDrawMinimap() {
  int iBoxSize = 40;
  int iSpacing = 10;
  int iStartX = WIDTH / 2 - (MAP_COLS * (iBoxSize + iSpacing)) / 2;
  int iStartY = HEIGHT / 2 - (MAP_ROWS * (iBoxSize + iSpacing)) / 2;

  SDL_Rect stScreen = {0,0,WIDTH,HEIGHT};
  int i;
  int j;
  int iCx;
  int iCy;

  SDL_SetRenderDrawColor(pRenderer, 0,0,0,200);
  SDL_RenderFillRect(pRenderer, &stScreen);

  iCx = -1; iCy = -1;
  for (i = 0; i < MAP_ROWS; i++) {
    for (j = 0; j < MAP_COLS; j++) {
      STRUCT_ROOM *pstRoom;
      SDL_Rect stBox;

      pstRoom = &agstMap[i][j];
      if (!pstRoom->iExists || !pstRoom->iVisited) continue;

      stBox.x = iStartX + j * (iBoxSize + iSpacing);
      stBox.y = iStartY + i * (iBoxSize + iSpacing);
      stBox.w = iBoxSize;
      stBox.h = iBoxSize;

      if (pstRoom->eId == eCurrentRoom) {
        SDL_SetRenderDrawColor(pRenderer, 200,200,200,255);
        SDL_RenderFillRect(pRenderer, &stBox);
        SDL_SetRenderDrawColor(pRenderer, 255,50,50,255);
        SDL_RenderDrawRect(pRenderer, &stBox);
        SDL_RenderDrawRect(pRenderer, &stBox);
        iCx = stBox.x + stBox.w/2;
        iCy = stBox.y + stBox.h/2;
      } else {
        SDL_SetRenderDrawColor(pRenderer, 100,100,100,255);
        SDL_RenderFillRect(pRenderer, &stBox);
      }
    }
  }

  if (iCx != -1 && iCy != -1) {
    if ((SDL_GetTicks() / 300) % 2 == 0) {
      SDL_SetRenderDrawColor(pRenderer, 255,0,0,255);
      SDL_Rect stBox = { iCx-3, iCy-3, 6, 6 };
      SDL_RenderFillRect(pRenderer, &stBox);
    }
  }
}

void vCheckRoomTransition() {
  if (eCurrentRoom == CENTER) {
    if (stPlayer.y <= 0) {
      eCurrentRoom = NORTH;
      stPlayer.y = HEIGHT - WALL_THICKNESS - PLAYER_SIZE - 1;
    } else if (stPlayer.y + PLAYER_SIZE >= HEIGHT) {
      eCurrentRoom = SOUTH;
      stPlayer.y = WALL_THICKNESS + 1;
    } else if (stPlayer.x <= 0) {
      eCurrentRoom = WEST;
      stPlayer.x = WIDTH - WALL_THICKNESS - PLAYER_SIZE - 1;
    } else if (stPlayer.x + PLAYER_SIZE >= WIDTH) {
      eCurrentRoom = EAST;
      stPlayer.x = WALL_THICKNESS + 1;
    }
    vSetRoomVisited(eCurrentRoom);
  } else {
    if (eCurrentRoom == NORTH && stPlayer.y + PLAYER_SIZE >= HEIGHT) {
      eCurrentRoom = CENTER;
      stPlayer.y = WALL_THICKNESS + 1;
    } else if (eCurrentRoom == SOUTH && stPlayer.y <= 0) {
      eCurrentRoom = CENTER;
      stPlayer.y = HEIGHT - WALL_THICKNESS - PLAYER_SIZE - 1;
    } else if (eCurrentRoom == WEST && stPlayer.x + PLAYER_SIZE >= WIDTH) {
      eCurrentRoom = CENTER;
      stPlayer.x = WALL_THICKNESS + 1;
    } else if (eCurrentRoom == EAST && stPlayer.x <= 0) {
      eCurrentRoom = CENTER;
      stPlayer.x = WIDTH - WALL_THICKNESS - PLAYER_SIZE - 1;
    }
  }
}

void vApplyWallCollision() {
  int iDoorX1;
  int iDoorX2;
  int iDoorY1;
  int iDoorY2;

  iDoorX1 = (WIDTH - DOOR_WIDTH) / 2;
  iDoorX2 = (WIDTH + DOOR_WIDTH) / 2;
  iDoorY1 = (HEIGHT - DOOR_WIDTH) / 2;
  iDoorY2 = (HEIGHT + DOOR_WIDTH) / 2;

  if (!iHasExit(eCurrentRoom, "N") && stPlayer.y < WALL_THICKNESS) {
    stPlayer.y = WALL_THICKNESS;
  } else if (stPlayer.y < WALL_THICKNESS) {
    if (stPlayer.x + PLAYER_SIZE < iDoorX1 || stPlayer.x > iDoorX2) stPlayer.y = WALL_THICKNESS;
  }

  if (!iHasExit(eCurrentRoom, "S") && stPlayer.y + PLAYER_SIZE > HEIGHT - WALL_THICKNESS) {
    stPlayer.y = HEIGHT - WALL_THICKNESS - PLAYER_SIZE;
  } else if (stPlayer.y + PLAYER_SIZE > HEIGHT - WALL_THICKNESS) {
    if (stPlayer.x + PLAYER_SIZE < iDoorX1 || stPlayer.x > iDoorX2)
      stPlayer.y = HEIGHT - WALL_THICKNESS - PLAYER_SIZE;
  }

  if (!iHasExit(eCurrentRoom, "W") && stPlayer.x < WALL_THICKNESS) {
    stPlayer.x = WALL_THICKNESS;
  } else if (stPlayer.x < WALL_THICKNESS) {
    if (stPlayer.y + PLAYER_SIZE < iDoorY1 || stPlayer.y > iDoorY2) stPlayer.x = WALL_THICKNESS;
  }

  if (!iHasExit(eCurrentRoom, "E") && stPlayer.x + PLAYER_SIZE > WIDTH - WALL_THICKNESS) {
    stPlayer.x = WIDTH - WALL_THICKNESS - PLAYER_SIZE;
  } else if (stPlayer.x + PLAYER_SIZE > WIDTH - WALL_THICKNESS) {
    if (stPlayer.y + PLAYER_SIZE < iDoorY1 || stPlayer.y > iDoorY2)
      stPlayer.x = WIDTH - WALL_THICKNESS - PLAYER_SIZE;
  }
}

void vUpdateBullets() {
  int i;

  for (i = 0; i < giBulletCount; i++) {
    STRUCT_BULLET *pstBullet;
    pstBullet = &agstBullets[i];

    if (pstBullet->eRoom != eCurrentRoom) continue;

    switch (pstBullet->iType) {
      case 0:
        pstBullet->fX += pstBullet->fVx;
        pstBullet->fY += pstBullet->fVy;
        if (pstBullet->fX < 0 || pstBullet->fX > WIDTH ||
            pstBullet->fY < 0 || pstBullet->fY > HEIGHT) {
          pstBullet->fX = WALL_THICKNESS + pstBullet->fRadius;
          pstBullet->fY = WALL_THICKNESS + pstBullet->fRadius;
        }
        break;

      case 1:
        pstBullet->fY += pstBullet->fVy;
        if (pstBullet->fY < WALL_THICKNESS + pstBullet->fRadius ||
            pstBullet->fY > HEIGHT - WALL_THICKNESS - pstBullet->fRadius) {
          pstBullet->fVy *= -1;
        }
        break;

      case 2:
        pstBullet->fAngle += 0.05f;
        pstBullet->fX = WIDTH / 2 + cosf(pstBullet->fAngle) * (WIDTH / 10);
        pstBullet->fY = HEIGHT / 2 + sinf(pstBullet->fAngle) * (HEIGHT / 10);
        break;

      case 3:
        pstBullet->fY += pstBullet->fVy;
        if (pstBullet->fY < WALL_THICKNESS + 30 ||
            pstBullet->fY > HEIGHT - WALL_THICKNESS - 30) {
          pstBullet->fVy *= -1;
        }
        break;

      case 4:
        pstBullet->fX += pstBullet->fVx;
        if (pstBullet->fX < WALL_THICKNESS + 30 ||
            pstBullet->fX > WIDTH - WALL_THICKNESS - 30) {
          pstBullet->fVx *= -1;
        }
        break;
    }

    if (!giIsInvulnerable && iCheckCollision(pstBullet)) {
      giPlayerHealth -= 1;
      giIsInvulnerable = 1;
      uiInvulnerableStart = SDL_GetTicks();

      if (giPlayerHealth <= 0) {
        vTeleportToCenter();
        giPlayerHealth = MAX_HEALTH;
      }
    }
  }
}

/* colisão projétil player vs inimigo (somente sala atual) */
static void vResolvePlayerVsEnemyBullets(void) {
  int i;
  int j;

  for (i = 0; i < MAX_PLAYER_BULLETS; i++) {
    if (!astPlayerBullets[i].iActive) continue;
    if (astPlayerBullets[i].eRoom != eCurrentRoom) continue;

    for (j = 0; j < giBulletCount; j++) {
      float fDx;
      float fDy;
      float fSumR;
      float fDist2;

      if (agstBullets[j].eRoom != eCurrentRoom) continue;

      fDx = astPlayerBullets[i].fX - agstBullets[j].fX;
      fDy = astPlayerBullets[i].fY - agstBullets[j].fY;
      fSumR = astPlayerBullets[i].fRadius + agstBullets[j].fRadius;
      fDist2 = fDx * fDx + fDy * fDy;

      if (fDist2 < (fSumR * fSumR)) {
        astPlayerBullets[i].iActive = 0;
        vResetBulletToHome(j);
        break;
      }
    }
  }
}

void vDrawRoom(ENUM_ROOMID eRoom) {
  int iDx;
  int iDy;
  SDL_Rect stR;

  SDL_SetRenderDrawColor(pRenderer, 50,50,50,255);
  SDL_RenderClear(pRenderer);
  SDL_SetRenderDrawColor(pRenderer, 200,200,200,255);

  iDx = (WIDTH - DOOR_WIDTH) / 2;
  iDy = (HEIGHT - DOOR_WIDTH) / 2;

  if (!iHasExit(eRoom, "N")) {
    stR = (SDL_Rect){0,0,WIDTH,WALL_THICKNESS};
    SDL_RenderFillRect(pRenderer, &stR);
  } else {
    stR = (SDL_Rect){0,0,iDx,WALL_THICKNESS};
    SDL_RenderFillRect(pRenderer, &stR);
    stR = (SDL_Rect){iDx + DOOR_WIDTH,0,iDx,WALL_THICKNESS};
    SDL_RenderFillRect(pRenderer, &stR);
  }

  if (!iHasExit(eRoom, "S")) {
    stR = (SDL_Rect){0,HEIGHT - WALL_THICKNESS,WIDTH,WALL_THICKNESS};
    SDL_RenderFillRect(pRenderer, &stR);
  } else {
    stR = (SDL_Rect){0,HEIGHT - WALL_THICKNESS,iDx,WALL_THICKNESS};
    SDL_RenderFillRect(pRenderer, &stR);
    stR = (SDL_Rect){iDx + DOOR_WIDTH,HEIGHT - WALL_THICKNESS,iDx,WALL_THICKNESS};
    SDL_RenderFillRect(pRenderer, &stR);
  }

  if (!iHasExit(eRoom, "W")) {
    stR = (SDL_Rect){0,0,WALL_THICKNESS,HEIGHT};
    SDL_RenderFillRect(pRenderer, &stR);
  } else {
    stR = (SDL_Rect){0,0,WALL_THICKNESS,iDy};
    SDL_RenderFillRect(pRenderer, &stR);
    stR = (SDL_Rect){0,iDy + DOOR_WIDTH,WALL_THICKNESS,iDy};
    SDL_RenderFillRect(pRenderer, &stR);
  }

  if (!iHasExit(eRoom, "E")) {
    stR = (SDL_Rect){WIDTH - WALL_THICKNESS,0,WALL_THICKNESS,HEIGHT};
    SDL_RenderFillRect(pRenderer, &stR);
  } else {
    stR = (SDL_Rect){WIDTH - WALL_THICKNESS,0,WALL_THICKNESS,iDy};
    SDL_RenderFillRect(pRenderer, &stR);
    stR = (SDL_Rect){WIDTH - WALL_THICKNESS,iDy + DOOR_WIDTH,WALL_THICKNESS,iDy};
    SDL_RenderFillRect(pRenderer, &stR);
  }
}

void vDrawTrails() {
  int i;
  for (i = 0; i < MAX_TRAILS; i++) {
    if (astPlayerTrails[i].iAlpha <= 0) continue;

    SDL_SetRenderDrawColor(pRenderer, 0,0,0, astPlayerTrails[i].iAlpha);
    {
      SDL_Rect stGhost;
      stGhost.x = astPlayerTrails[i].iX;
      stGhost.y = astPlayerTrails[i].iY;
      stGhost.w = PLAYER_SIZE;
      stGhost.h = PLAYER_SIZE;
      SDL_RenderFillRect(pRenderer, &stGhost);
    }

    astPlayerTrails[i].iAlpha -= 8;
    if (astPlayerTrails[i].iAlpha < 0) astPlayerTrails[i].iAlpha = 0;
  }
}

void vDrawBlinkBar() {
  const int iBarWidth = 20;
  const int iBarHeight = 100;
  const int iX = WIDTH - 30;
  const int iY = HEIGHT - 30 - iBarHeight;

  SDL_Rect stBackground = { iX, iY, iBarWidth, iBarHeight };

  SDL_SetRenderDrawColor(pRenderer, 80,80,80,255);
  SDL_RenderFillRect(pRenderer, &stBackground);

  if (!giCanBlink) {
    Uint32 uiNow;
    float fProgress;
    int iFillHeight;
    SDL_Rect stFill;

    uiNow = SDL_GetTicks();
    fProgress = (uiNow - uiLastBlinkTime) / (float)BLINK_COOLDOWN;
    if (fProgress > 1.0f) fProgress = 1.0f;

    iFillHeight = (int)(iBarHeight * fProgress);
    stFill.x = iX;
    stFill.y = iY + iBarHeight - iFillHeight;
    stFill.w = iBarWidth;
    stFill.h = iFillHeight;
    SDL_SetRenderDrawColor(pRenderer, 0,100,255,255);
    SDL_RenderFillRect(pRenderer, &stFill);
  } else {
    SDL_Rect stFull = { iX, iY, iBarWidth, iBarHeight };
    SDL_SetRenderDrawColor(pRenderer, 0,100,255,255);
    SDL_RenderFillRect(pRenderer, &stFull);
  }

  SDL_SetRenderDrawColor(pRenderer, 255,255,255,255);
  SDL_RenderDrawRect(pRenderer, &stBackground);
}

void vSetRoomVisited(ENUM_ROOMID eRoomId) {
  int i;
  int j;
  for (i = 0; i < MAP_ROWS; i++) {
    for (j = 0; j < MAP_COLS; j++) {
      STRUCT_ROOM *pstRoom;
      pstRoom = &agstMap[i][j];
      if (!pstRoom->iExists) continue;
      if (pstRoom->eId == eRoomId) { pstRoom->iVisited = 1; return; }
    }
  }
}

void vInitMap() {
  memset(agstMap, 0, sizeof(agstMap));

  agstMap[2][2] = (STRUCT_ROOM){1,1,CENTER};
  agstMap[1][2] = (STRUCT_ROOM){1,0,NORTH};
  agstMap[3][2] = (STRUCT_ROOM){1,0,SOUTH};
  agstMap[2][1] = (STRUCT_ROOM){1,0,WEST};
  agstMap[2][3] = (STRUCT_ROOM){1,0,EAST};
}

void vSpawnBullets() {
  giBulletCount = 0;
  agstBullets[giBulletCount++] = (STRUCT_BULLET){0, WALL_THICKNESS + 10, WALL_THICKNESS + 10, 2.0f, 2.0f, 0, 10, CENTER};
  agstBullets[giBulletCount++] = (STRUCT_BULLET){1, WIDTH / 2, HEIGHT / 2, 0, 1.5f, 0, 10, WEST};
  agstBullets[giBulletCount++] = (STRUCT_BULLET){2, WIDTH / 2, HEIGHT / 2, 0, 0, 0, 10, SOUTH};
  agstBullets[giBulletCount++] = (STRUCT_BULLET){3, WIDTH / 2, HEIGHT / 2, 0, 1.5f, 0, 10, EAST};
  agstBullets[giBulletCount++] = (STRUCT_BULLET){4, WIDTH / 2, HEIGHT / 2, 1.5f, 0, 0, 10, NORTH};
  vBakeBulletHomes();
}

int iHasExit(ENUM_ROOMID eRoom, const char *pszDirection) {
  char cD;
  if (bStrIsEmpty(pszDirection)) return 0;
  cD = pszDirection[0];
  switch (eRoom) {
    case CENTER: return 1;
    case NORTH:  return cD == 'S';
    case SOUTH:  return cD == 'N';
    case EAST:   return cD == 'W';
    case WEST:   return cD == 'E';
    default:     return 0;
  }
}

void vTeleportToCenter() {
  eCurrentRoom = CENTER;
  stPlayer.x = WIDTH / 2 - PLAYER_SIZE / 2;
  stPlayer.y = HEIGHT / 2 - PLAYER_SIZE / 2;
}

int iCheckCollision(STRUCT_BULLET *pstBullet) {
  float fDx;
  float fDy;
  float fDist2;
  float fSumR;
  float fSumR2;

  fDx = pstBullet->fX - (stPlayer.x + PLAYER_SIZE / 2.0f);
  fDy = pstBullet->fY - (stPlayer.y + PLAYER_SIZE / 2.0f);
  fDist2 = fDx * fDx + fDy * fDy;
  fSumR = pstBullet->fRadius + (PLAYER_SIZE / 2.0f);
  fSumR2 = fSumR * fSumR;
  return fDist2 < fSumR2;
}

/* ===== Loop principal ===== */
void vLoop() {
  int iEffectiveSpeed;
  Uint32 uiNow;

  iEffectiveSpeed = SPEED;

  if (uiLastStaminaTick == 0) {
    uiLastStaminaTick = SDL_GetTicks();
  }
  if (giIsInvulnerable) {
    uiNow = SDL_GetTicks();
    if (uiNow - uiInvulnerableStart >= 1500) giIsInvulnerable = 0;
  }

  if (!giCanBlink) {
    uiNow = SDL_GetTicks();
    if (uiNow - uiLastBlinkTime >= BLINK_COOLDOWN) giCanBlink = 1;
  }

  /* Eventos */
  {
    SDL_Event stEvent;
    while (SDL_PollEvent(&stEvent)) {
      if (stEvent.type == SDL_QUIT)
        emscripten_cancel_main_loop();
      vHandleInput(&stEvent);
    }
  }

  SDL_PumpEvents();
  /* 1) atualiza movimento pelas teclas seguradas (WASD e setas) */
  vUpdateMoveFromKeysHeld();

  /* 2) atualiza mira: setas ou, na ausência delas, segue o movimento */
  vUpdateLookDirFromKeysHeld();

  /* ===== Stamina (corrida): drain/regener com acumuladores ===== */
  {
    Uint32 uiNowSt;
    int iDeltaMs;
    int iMoving;
    int iChange;
    int iDrain;
    int iGain;

    uiNowSt = SDL_GetTicks();
    if (uiLastStaminaTick == 0) uiLastStaminaTick = uiNowSt;
    iDeltaMs = (int)(uiNowSt - uiLastStaminaTick);
    if (iDeltaMs < 0) iDeltaMs = 0;
    uiLastStaminaTick = uiNowSt;

    iMoving = (giPlayerVx != 0 || giPlayerVy != 0) ? 1 : 0;

    if (giSpeedMode == 2 && iMoving) {
      /* drenando: acumula frações e converte para pontos de stamina */
      giStamDrainAcc += STAMINA_DRAIN_PER_SEC * iDeltaMs;   /* mili-stamina */
      iDrain = giStamDrainAcc / 1000;                       /* pontos inteiros */
      giStamDrainAcc -= iDrain * 1000;                      /* sobra fracionária */
      if (iDrain > 0) giStamRegenAcc = 0;                   /* cancela carry oposto */

      giStamina -= iDrain;
      if (giStamina <= 0) {
        giStamina = 0;
        giSpeedMode = 0; /* cansou */
        giStamDrainAcc = 0; /* zera sobra para não continuar drenando */
      }
    } else {
      /* regenerando: idem acumulador para subir suave */
      giStamRegenAcc += STAMINA_RECOVER_PER_SEC * iDeltaMs; /* mili-stamina */
      iGain = giStamRegenAcc / 1000;                        /* pontos inteiros */
      giStamRegenAcc -= iGain * 1000;                       /* sobra fracionária */
      if (iGain > 0) giStamDrainAcc = 0;                    /* cancela carry oposto */

      iChange = iGain;
      giStamina += iChange;
      if (giStamina > STAMINA_MAX) {
        giStamina = STAMINA_MAX;
        giStamRegenAcc = 0;
      }
    }
  }


  if (giMapOpen) {
    vDrawMinimap();
    SDL_RenderPresent(pRenderer);
    return;
  }

  if (giSpeedMode == 1) {
    iEffectiveSpeed = 2;
  } else if (giSpeedMode == 2) {
    iEffectiveSpeed = 5;
    if (giPlayerVx != 0 || giPlayerVy != 0) {
      astPlayerTrails[giTrailIndex].iX = stPlayer.x;
      astPlayerTrails[giTrailIndex].iY = stPlayer.y;
      astPlayerTrails[giTrailIndex].iAlpha = 120;
      giTrailIndex = (giTrailIndex + 1) % MAX_TRAILS;
    }
  }

  /* Movimento (WASD) */
  stPlayer.x += giPlayerVx * iEffectiveSpeed;
  stPlayer.y += giPlayerVy * iEffectiveSpeed;

  vCheckRoomTransition();
  vApplyWallCollision();
  vUpdateBullets();

  /* Atualiza projéteis do player apenas na sala atual */
  {
    int i;
    for (i = 0; i < MAX_PLAYER_BULLETS; i++) {
      if (!astPlayerBullets[i].iActive) continue;
      if (astPlayerBullets[i].eRoom != eCurrentRoom) continue;

      astPlayerBullets[i].fX += astPlayerBullets[i].fVx;
      astPlayerBullets[i].fY += astPlayerBullets[i].fVy;

      if (astPlayerBullets[i].fX < 0 || astPlayerBullets[i].fX > WIDTH ||
          astPlayerBullets[i].fY < 0 || astPlayerBullets[i].fY > HEIGHT) {
        astPlayerBullets[i].iActive = 0;
      }
    }
  }

  vResolvePlayerVsEnemyBullets();

  vDrawRoom(eCurrentRoom);
  vDrawEnemyBullets();
  vDrawPlayerBullets();
  vDrawTrails();

  if (!giIsInvulnerable || ((SDL_GetTicks() / 100) % 2 == 0)) {
    SDL_SetRenderDrawColor(pRenderer, 255,255,255,255);
    SDL_RenderFillRect(pRenderer, &stPlayer);
  }

  vDrawHealthBar();
  vDrawHUD();
  vDrawBlinkBar();
  vDrawStaminaBar(); /* NOVO: barra amarela */
  SDL_RenderPresent(pRenderer);

}

/* ===== Shutdown ===== */
static void vShutdown(void) {
  if (gpstCtx != NULL) {
    vGameCtxShutdown(gpstCtx);
    gpstCtx = NULL;
  }
}

/* ===== main ===== */
int main() {
  int iRc;

  iRc = iGameCtxInit(&gstCtx);
  if (iRc != 0) {
    printf("Falha ao inicializar contexto (%d)\n", iRc);
    return 1;
  }

  gpstCtx = &gstCtx;

  vInitMap();
  vSpawnBullets();

  atexit(vShutdown);
  emscripten_set_main_loop(vLoop, 0, 1);
  return 0;
}
