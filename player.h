#include "game.h"
#include "places.h"
#include "rollingarray.h"

#ifndef FOD__PLAYER_H_
#define FOD__PLAYER_H_

typedef struct player_t {
  enum player id;  // who is it
  int health;
  location_t location;
  rollingarray_t *trail;
} player_t;

player_t *new_player(enum player id);
void destroy_player(player_t *player);
int player_get_health(player_t *player);
bool player_lose_health(player_t *player, int lose);
location_t player_get_location(player_t *player);
void player_get_trail(player_t *player, location_t trail[TRAIL_SIZE]);
void player_move_to(player_t *player, location_t location);
enum player player_id_from_char(char player);

#endif  // !defined (FOD__PLAYER_H_)
