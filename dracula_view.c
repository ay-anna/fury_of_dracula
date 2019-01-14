////////////////////////////////////////////////////////////////////////
// COMP2521 19t0 ... the Fury of Dracula
// dracula_view.c: the DraculaView ADT implementation
//
// Adam Yi <i@adamyi.com>, Simon Hanly-Jones <simon.hanly.jones@gmail.com>

#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sysexits.h>

#include "dracula_view.h"
#include "game.h"
#include "game_view.h"
#include "map.h"
#include "mapdata.h"

typedef struct dracula_view {
  GameView gv;
} dracula_view;

dracula_view *dv_new(char *past_plays, player_message messages[]) {
  dracula_view *new = malloc(sizeof *new);
  if (new == NULL) err(EX_OSERR, "couldn't allocate DraculaView");
  new->gv = gv_new(past_plays, messages);

  return new;
}

void dv_drop(dracula_view *dv) {
  gv_drop(dv->gv);
  free(dv);
}

round_t dv_get_round(dracula_view *dv) { return gv_get_round(dv->gv); }

int dv_get_score(dracula_view *dv) { return gv_get_score(dv->gv); }

int dv_get_health(dracula_view *dv, enum player player) {
  return gv_get_health(dv->gv, player);
}

location_t dv_get_location(dracula_view *dv, enum player player) {
  return gv_get_location(dv->gv, player);
}

void dv_get_player_move(dracula_view *dv, enum player player, location_t *start,
                        location_t *end) {
  *end = gv_get_location(dv->gv, player);

  location_t trail[TRAIL_SIZE];
  gv_get_history(dv->gv, player, trail);
  round_t round = gv_get_round(dv->gv);

  if (round == 0) {
    *start = UNKNOWN_LOCATION;

  } else {
    *start = trail[0];
  }
}

// function to take the index of a trail move and determine the round that move
// was made in
static int get_round_trail_move_made(dracula_view *dv, int index) {
  int round = gv_get_round(dv->gv);
  return round - index - 1;
}

void dv_get_locale_info(
    dracula_view *dv, location_t where, int *n_traps,
    int *n_vamps) {  // NOTES: traps and vampire tracking can be done in
                     // gameview in second part of assignment
  int last_hunter_visit = -1;
  *n_traps = 0;
  *n_vamps = 0;

  location_t trail[TRAIL_SIZE] = {-1};

  for (enum player hunter = 0; hunter < PLAYER_DRACULA; hunter++) {
    gv_get_history(dv->gv, hunter, trail);
    for (int i = TRAIL_SIZE; i > last_hunter_visit; i--) {
      if (trail[i] == where) last_hunter_visit = i;
    }
  }

  gv_get_history(dv->gv, PLAYER_DRACULA, trail);

  for (int i = last_hunter_visit == -1 ? TRAIL_SIZE - 1 : last_hunter_visit;
       i > -1; i--) {
    if (trail[i] == where) {
      if (get_round_trail_move_made(dv, i) % 13 == 0) {
        (*n_vamps)++;
      } else {
        (*n_traps)++;
      }
    }
  }
}

void dv_get_trail(dracula_view *dv, enum player player,
                  location_t trail[TRAIL_SIZE]) {
  gv_get_history(dv->gv, player, trail);
}

location_t *dv_get_dests(dracula_view *dv, size_t *n_locations, bool road,
                         bool sea) {
  return dv_get_dests_player(dv, n_locations, PLAYER_DRACULA, road, false, sea);
}

location_t *dv_get_dests_player(dracula_view *dv, size_t *n_locations,
                                enum player player, bool road, bool rail,
                                bool sea) {
  // TODO(adamyi): dracula trail/hide/doubleback
  return gv_get_connections(dv->gv, n_locations, dv_get_location(dv, player),
                            player, dv_get_round(dv), road, rail, sea);
}
