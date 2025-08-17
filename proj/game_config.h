/* game_config.h */
#ifndef GAME_CONFIG_H
  #define GAME_CONFIG_H

  #define WIDTH 640
  #define HEIGHT 480
  #define PLAYER_SIZE 20
  #define SPEED 3
  #define DOOR_WIDTH (WIDTH / 3)
  #define WALL_THICKNESS 20
  #define MAX_BULLETS 10
  #define PI 3.14159265f
  #define MAP_ROWS 5
  #define MAP_COLS 5
  #define MAX_TRAILS 10
  #define MAX_HEALTH 6
  #define BLINK_COOLDOWN 2000
  #define BLINK_DISTANCE (3 * PLAYER_SIZE)
  #define PLAYER_BULLET_SPEED 6
  #define MAX_PLAYER_BULLETS 20

  #ifndef TRUE
    #define TRUE 1
  #endif
  #ifndef FALSE
    #define FALSE 0
  #endif
  /* Stamina (corrida) */
  #define STAMINA_MAX 100
  #define STAMINA_DRAIN_PER_SEC 40     /* drena ~40/unid por segundo correndo */
  #define STAMINA_RECOVER_PER_SEC 25   /* regenera ~25/unid por segundo parado */
  #define STAMINA_MIN_TO_SPRINT 10     /* mínimo para permitir iniciar corrida */

#endif /* GAME_CONFIG_H */
