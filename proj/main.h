#ifndef MAIN_H
  #define MAIN_H

  #include <SDL2/SDL.h>
  #include <SDL2/SDL_ttf.h>
  #include "game_context.h"
  

  
  /* Render / janela / fonte */
  #define pRenderer           (gpstCtx->stRender.pRenderer)
  #define pWindow             (gpstCtx->stRender.pWindow)
  #define pFont               (gpstCtx->stRender.pFont)

  /* Estado de runtime */
  #define giMapOpen           (gpstCtx->stState.iMapOpen)
  #define giSpeedMode         (gpstCtx->stState.iSpeedMode)
  #define giLookDirX          (gpstCtx->stState.iLookDirX)
  #define giLookDirY          (gpstCtx->stState.iLookDirY)
  #define giPlayerHealth      (gpstCtx->stState.iPlayerHealth)
  #define giIsInvulnerable    (gpstCtx->stState.iIsInvulnerable)
  #define giTrailIndex        (gpstCtx->stState.iTrailIndex)
  #define giCanBlink          (gpstCtx->stState.iCanBlink)
  #define giPlayerVx          (gpstCtx->stState.iPlayerVx)
  #define giPlayerVy          (gpstCtx->stState.iPlayerVy)
  #define giBulletCount       (gpstCtx->stState.iBulletCount)
  #define uiInvulnerableStart (gpstCtx->stState.uiInvulnerableStart)
  /* Stamina */
  #define giStamina           (gpstCtx->stState.iStamina)
  #define uiLastStaminaTick   (gpstCtx->stState.uiLastStaminaTick)
  #define uiLastBlinkTime     (gpstCtx->stState.uiLastBlinkTime) 
  #define giStamDrainAcc      (gpstCtx->stState.iStamDrainAcc)
  #define giStamRegenAcc      (gpstCtx->stState.iStamRegenAcc)

  /* Homes dos inimigos */
  #define afBulletHomeX       (gpstCtx->stWorld.afBulletHomeX)
  #define afBulletHomeY       (gpstCtx->stWorld.afBulletHomeY)
  #define afBulletHomeVx      (gpstCtx->stWorld.afBulletHomeVx)
  #define afBulletHomeVy      (gpstCtx->stWorld.afBulletHomeVy)
  #define afBulletHomeAngle   (gpstCtx->stWorld.afBulletHomeAngle)
  #define aeBulletHomeRoom    (gpstCtx->stWorld.aeBulletHomeRoom)

  #define eCurrentRoom        (gpstCtx->stState.eCurrentRoom)
  #define stPlayer            (gpstCtx->stState.stPlayer)

  /* Mundo */
  #define agstMap             (gpstCtx->stWorld.astMap)
  #define astPlayerTrails     (gpstCtx->stWorld.astPlayerTrails)
  #define astPlayerBullets    (gpstCtx->stWorld.astPlayerBullets)
  #define agstBullets         (gpstCtx->stWorld.astEnemyBullets)

  /* Recursos (caches) */
  #define gpCircleTex         (gpstCtx->stRes.pCircleTex)
  #define giCircleR           (gpstCtx->stRes.iCircleRadius)

  /* HUD */
  #define gstTxtSala          (gpstCtx->stHud.stSala)
  #define gstTxtModo          (gpstCtx->stHud.stModo)
  #define gstTxtDir           (gpstCtx->stHud.stDirecao)
  #define gstTxtCoord         (gpstCtx->stHud.stCoord)
  #define giLastRoomId        (gpstCtx->stHud.iLastRoomId)
  #define giLastSpeedMode     (gpstCtx->stHud.iLastSpeedMode)
  #define giLastLookX         (gpstCtx->stHud.iLastLookX)
  #define giLastLookY         (gpstCtx->stHud.iLastLookY)

  /* Dados estáticos */
  #define apszRoomNames       (gpstCtx->apszRoomNames)

  void vLoop();
  void vInitMap();
  void vSpawnBullets();
  void vHandleInput(const SDL_Event *pstEvent);
  void vApplyWallCollision();
  void vCheckRoomTransition();
  void vSetRoomVisited(ENUM_ROOMID eRoomId);
  void vDrawRoom(ENUM_ROOMID eRoom);
  void vDrawMinimap();
  void vDrawHUD();
  void vDrawText(const char *pszText, int iX, int iY, SDL_Color stColor);
  void vDrawHealthBar();
  void vDrawBlinkBar();
  void vDrawCircle(float fCx, float fCy, float fRadius);
  void vDrawPlayerBullets();
  void vDrawEnemyBullets();
  void vDrawTrails();
  void vFirePlayerBullet();
  void vUpdateBullets();
  int iCheckCollision(STRUCT_BULLET *pstBullet);
  void vTeleportToCenter();
  int iHasExit(ENUM_ROOMID eRoom, const char *pszDirection);
  int bStrIsEmpty(const char *kpszStr);

#endif 



