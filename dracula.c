////////////////////////////////////////////////////////////////////////
// COMP2521 19t0 ... the Fury of Dracula
// dracula.c: your "Fury of Dracula" Dracula AI
//
// Adam Yi <i@adamyi.com>, Simon Hanly-Jones <simon.hanly.jones@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// #include <Python.h>

#include "ac_log.h"

#include "dracula.h"
#include "dracula_view.h"
#include "game.h"
#include "internal_game_view.h"
#include "mapdata.h"
#include "myplayer.h"

static inline int weighted_spdist(int spdist) {
  if (spdist == 0) return -100;
  if (spdist == 1) return -10;
  if (spdist == 2) return -3;
  if (spdist == 3) return -1;
  if (spdist > 6) return 6;
  return spdist;
}

static inline int apply_weight(int x, double weight) {
  if (x > 0) return (int)(x * weight);
  return (int)(x / weight);
}

void decide_dracula_move(DraculaView dv) {
  // TODO(unassigned): Replace this with something better!
  struct timeval t1;
  gettimeofday(&t1, NULL);
  unsigned int ts = (t1.tv_usec % 5000) * (t1.tv_sec % 5000);
  srand(ts);
  round_t round = dv_get_round(dv);
  location_t ret = NOWHERE;
  size_t num = 0;
  location_t *possible;
  possible = dv_get_possible_moves(dv, &num);
  int dist[NUM_MAP_LOCATIONS];
  bool cango[NUM_MAP_LOCATIONS];
  memset(dist, 0, sizeof(dist));
  memset(cango, 0, sizeof(cango));
  location_t rev[110];
  player_t *dracp = dv_get_player_class(dv, PLAYER_DRACULA);
  int health = dv_get_health(dv, PLAYER_DRACULA);
  for (size_t i = 0; i < num; i++) {
    if (possible[i] >= HIDE) {
      location_t res = player_resolve_move_location(dracp, possible[i]);
      rev[res] = possible[i];
      possible[i] = res;
    } else
      rev[possible[i]] = possible[i];
    cango[possible[i]] = true;
  }
  for (int i = 0; i < PLAYER_DRACULA; i++) {
    location_t loc = dv_get_location(dv, i);
    if (round == 0) {
      for (int j = MIN_MAP_LOCATION; j <= MAX_MAP_LOCATION; j++) {
        if (j == HOSPITAL_LOCATION) continue;
        dist[j] += weighted_spdist(SPDIST[loc][j]);
      }
    } else {
      for (int j = 0; j < num; j++) {
        if (possible[j] >= MIN_MAP_LOCATION &&
            possible[j] <= MAX_MAP_LOCATION) {
          if (rev[possible[j]] != possible[j])
            dist[possible[j]] +=
                apply_weight(weighted_spdist(SPDIST[loc][possible[j]]), 0.75);
          else
            dist[possible[j]] += weighted_spdist(SPDIST[loc][possible[j]]);
        }
      }
    }
  }
  int maxdist = -10000;
  for (int i = MIN_MAP_LOCATION; i <= MAX_MAP_LOCATION; i++) {
    if (location_get_type(i) == SEA) {
      if (dist[i] < 0)
        dist[i] = 1;
      else if (round % 13 == 0 || round % 13 == 12)
        dist[i] = apply_weight(dist[i], 0.25);
      else
        dist[i] = apply_weight(dist[i], 0.65);
      if (health <= 2)
        dist[i] = -100;
    }
    if (dist[i] != 0)
        ac_log(AC_LOG_INFO, "%s: %d", location_get_abbrev(i), dist[i]);
    if (cango[i] && dist[i] > maxdist) {
      maxdist = dist[i];
      ret = i;
    }
  }
  if (ret == NOWHERE) {
    ac_log(AC_LOG_ERROR, "random move");
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.rand)
    ret = possible[rand() % num];
  }
  free(possible);
  char name[3];
  strncpy(name, location_get_abbrev(rev[ret]), 3);
  register_best_play(name, "random location!");
}
