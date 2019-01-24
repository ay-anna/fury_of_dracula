////////////////////////////////////////////////////////////////////////
// COMP2521 19t0 ... the Fury of Dracula
// hunter.c: your "Fury of Dracula" hunter AI.
//
// Adam Yi <i@adamyi.com>, Simon Hanly-Jones <simon.hanly.jones@gmail.com>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #include <Python.h>

#include "game.h"
#include "hunter.h"
#include "hunter_view.h"
#include "internal_game_view.h"
#include "myplayer.h"
#include "places.h"
#include "rollingarray.h"

#include "ac_log.h"
#include "ac_memory.h"

#define MAX_SCENARIOS 1000000

#define LONGEST_PATH 10

typedef struct scenario {
  player_t *player;
  struct scenario *next;
} scenario_t;

static inline bool isValidLoc(location_t loc) {
  return loc >= MIN_MAP_LOCATION && loc <= MAX_MAP_LOCATION;
}

int probabilities[NUM_MAP_LOCATIONS];

bool getPossibleDraculaLocations(player_t *players[], round_t round) {
  memset(probabilities, 0, sizeof(probabilities));
  round_t last_known_round;
  location_t last_known_location = NOWHERE;
  for (last_known_round = round - 1;
       (!isValidLoc(last_known_location)) && last_known_round > -1;
       last_known_round--) {
    last_known_location =
        players[PLAYER_DRACULA]->all_location_history[last_known_round];
    if (last_known_location == TELEPORT) last_known_location = CASTLE_DRACULA;
  }
  ac_log(AC_LOG_DEBUG, "last_known_round %d last_known_location %s",
         last_known_round, location_get_abbrev(last_known_location));
  if (!isValidLoc(last_known_location)) return false;
  scenario_t *start = ac_malloc(sizeof(scenario_t), "scenario");
  start->player = new_player(PLAYER_DRACULA, false);
  player_move_to(start->player, last_known_location, last_known_location);
  start->next = NULL;
  scenario_t *end = start;
  int scount = 1;
  for (last_known_round += 2; last_known_round < round; last_known_round++) {
    location_t loc =
        players[PLAYER_DRACULA]->all_location_history[last_known_round];
    scenario_t **l = &start;
    bool cont = true;
    if (scount == 0) {
      ac_log(AC_LOG_ERROR, "scount = 0");
      return false;
    }
    for (scenario_t *oend = end, *i = start; cont && i != oend->next;) {
      if (loc == HIDE || (loc >= DOUBLE_BACK_1 && loc <= DOUBLE_BACK_5))
        player_move_to(i->player, loc,
                       player_resolve_move_location(i->player, loc));
      else {
        size_t n_locations = 0;
        location_t *moves;
        /*
location_t *_gv_do_get_connections(player_t *pobj, size_t *n_locations,
                                   location_t from, enum player player,
                                   round_t round, bool road, bool rail,
                                   bool sea, bool trail, bool stay, bool hide);
                                   */
        if (loc == SEA_UNKNOWN) {
          moves = _gv_do_get_connections(
              i->player, &n_locations, i->player->location, PLAYER_DRACULA,
              last_known_round, false, false, true, true, false, false);
        } else if (loc == CITY_UNKNOWN) {
          // NOTES: boat is set to true (go to city from sea via boat)
          moves = _gv_do_get_connections(
              i->player, &n_locations, i->player->location, PLAYER_DRACULA,
              last_known_round, true, false, true, true, false, false);
        } else {
          ac_log(AC_LOG_FATAL, "wrong loc: %d (%s)", loc,
                 location_get_name(loc));
        }
        ac_log(AC_LOG_DEBUG, "_gv_do_get_connections ret: %d", n_locations);
        int plays = 0;
        for (int j = 0; j < n_locations; j++) {
          ac_log(AC_LOG_DEBUG, "Consider %s", location_get_abbrev(moves[j]));
          if ((loc == players[0]->all_location_history[last_known_round]) ||
              (loc == players[1]->all_location_history[last_known_round]) ||
              (loc == players[2]->all_location_history[last_known_round]) ||
              (loc == players[3]->all_location_history[last_known_round]) ||
              (loc == CITY_UNKNOWN && location_get_type(moves[j]) == SEA) ||
              (loc == SEA_UNKNOWN && location_get_type(moves[j]) == LAND))
            continue;
          if (plays == 0) {
            ac_log(AC_LOG_DEBUG, "play scenario %p to %s", i,
                   location_get_abbrev(moves[j]));
            player_move_to(i->player, moves[j], moves[j]);
          } else {
            scenario_t *s = ac_malloc(sizeof(scenario_t), "scenario");
            s->player = clone_player(i->player);
            player_move_to(s->player, moves[j], moves[j]);
            s->next = NULL;
            end->next = s;
            end = s;
            scount++;
            ac_log(AC_LOG_DEBUG, "clone scenario %p to %s", s,
                   location_get_abbrev(moves[j]));
          }
          plays++;
        }
        if (plays == 0) {
          ac_log(AC_LOG_DEBUG, "destroy scenario %p", i);
          ac_log(AC_LOG_DEBUG, "scenario count: %d", scount);
          scenario_t *nxt = i->next;
          if (i == oend) {
            printf("nocont!");
            cont = false;
          }
          destroy_player(i->player);
          free(i);
          i = nxt;
          *l = i;
          scount--;
        } else {
          l = &(i->next);
          i = *l;
        }
        free(moves);
        ac_log(AC_LOG_DEBUG, "l %p\n", l);
      }
    }
  }
  if (start == NULL) return false;
  if (scount == 0) return false;
  scenario_t *n;
  for (; start != NULL; start = n) {
    n = start->next;
    probabilities[start->player->location]++;
    destroy_player(start->player);
    free(start);
  }
  return true;
}

static location_t sp_go_to(player_t *p, location_t dest, int round) {
  size_t n_locations = 0;
  size_t count = 1;
  location_t moves[LONGEST_PATH];
  location_t loc[LONGEST_PATH];
  round_t rounds[LONGEST_PATH];
  moves[0] = NOWHERE;
  loc[0] = p->location;
  rounds[0] = round;

  int i;
  for (i = 0; loc[i] != dest; i++) {
    location_t *ds =
        _gv_do_get_connections(p, &n_locations, loc[i], p->id, rounds[i], true,
                               true, true, false, false, false);
    for (int j = 0; j < n_locations; j++) {
      if (moves[i] == NOWHERE)
        moves[count] = ds[j];
      else
        moves[count] = moves[i];
      loc[count] = ds[j];
      rounds[count] = rounds[i] + 1;
      count++;
    }
  }
  return moves[i];
}

void decide_hunter_move(HunterView hv) {
  // TODO(unassigned): Replace this with something better!
  srand(time(0));
  round_t round = hv_get_round(hv);
  location_t ret;
  enum player cp = hv_get_player(hv);
  player_t *players[NUM_PLAYERS];
  if (round == 0) {
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.rand)
    ret = rand() % MAX_MAP_LOCATION;
  } else {
    for (int i = 0; i < NUM_PLAYERS; i++) {
      players[i] = hv_get_player_class(hv, i);
    }
    // size_t num = 0;
    // location_t *possible = hv_get_dests(hv, &num, true, true, true);
    bool guessDracula = getPossibleDraculaLocations(players, round);
    ac_log(AC_LOG_DEBUG, "getprob: %d", guessDracula);
    int maxprob = 0, actionSpaceSize = 0;
    location_t maxprobl = NOWHERE;
    for (int i = MIN_MAP_LOCATION; i <= MAX_MAP_LOCATION; i++) {
      if (probabilities[i] > 0) {
        actionSpaceSize += probabilities[i];
        ac_log(AC_LOG_DEBUG, "%s: %d", location_get_abbrev(i),
               probabilities[i]);
        if (probabilities[i] > maxprob) {
          maxprob = probabilities[i];
          maxprobl = i;
        }
      }
    }
    if (maxprob * 2 < actionSpaceSize)  // if we are not 50% sure where Drac is
      guessDracula = false;
    if (hv_get_location(hv, PLAYER_DRACULA) == hv_get_location(hv, cp) ||
        hv_get_health(hv, cp) <= 4 || (!guessDracula)) {
      ret = hv_get_location(hv, cp);
    } else {
      // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.rand)
      // ret = possible[rand() % num];
      ret = sp_go_to(players[cp], maxprobl, round);
    }
  }
  char name[3];
  strncpy(name, location_get_abbrev(ret), 3);
  register_best_play(name, "");
}
