from gym import Env, spaces
import numpy as np
import time
import sys
import subprocess
import json
import os
import random


PLAYER_CHAR = ['G', 'S', 'H', 'M', 'D']
SEA_ABBRS = ['AS', 'AO', 'BB', 'BS', 'EC', 'IO', 'IR', 'MS', 'NS', 'TS']
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir))
LOCATIONS = {0: "AS", 1: "AL", 2: "AM", 3: "AT", 4: "AO", 5: "BA", 6: "BI", 7: "BB", 8: "BE", 9: "BR", 10: "BS", 11: "BO", 12: "BU", 13: "BC", 14: "BD", 15: "CA", 16: "CG", 17: "CD", 18: "CF", 19: "CO", 20: "CN", 21: "DU", 22: "ED", 23: "EC", 24: "FL", 25: "FR", 26: "GA", 27: "GW", 28: "GE", 29: "GO", 30: "GR", 31: "HA", 32: "IO", 33: "IR", 34: "KL", 35: "LE", 36: "LI", 37: "LS", 38: "LV", 39: "LO", 40: "MA", 41: "MN", 42: "MR", 43: "MS", 44: "MI", 45: "MU", 46: "NA", 47: "NP", 48: "NS", 49: "NU", 50: "PA", 51: "PL", 52: "PR", 53: "RO", 54: "SA", 55: "SN", 56: "SR", 57: "SJ", 58: "SO", 59: "JM", 60: "ST", 61: "SW", 62: "SZ", 63: "TO", 64: "TS", 65: "VA", 66: "VR", 67: "VE", 68: "VI", 69: "ZA", 70: "ZU", 102: "HI", 103: "D1", 104: "D2", 105: "D3", 106: "D4", 107: "D5", 108: "TP"}

class FuryOfDraculaEnv(Env):
    def __init__(self):
        self.reset()
    def step(self, action):
        if isinstance(action, (int, long)):
            action = LOCATIONS[action]
        self.past_plays_dracula += PLAYER_CHAR[self.player] + action + "...."
        process = subprocess.Popen([os.path.join(ROOT_DIR, "nn_features"), '1'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        # print(self.past_plays_dracula)
        stdoutdata,_ = process.communicate(input = self.past_plays_dracula)
        # print(stdoutdata)
        result = json.loads(stdoutdata)
        self.past_plays_dracula = self.past_plays_dracula[:-4] + result['move']
        if self.player == 4:
            if action in SEA_ABBRS:
                self.past_plays_hunter += PLAYER_CHAR[self.player] + 'S?' + result['move']
            else:
                self.past_plays_hunter += PLAYER_CHAR[self.player] + 'C?' + result['move']
        else:
            self.past_plays_hunter += PLAYER_CHAR[self.player] + action + result['move']
        for revealed in result['revealed']:
            self.past_plays_hunter = self.past_plays_hunter[:35 * revealed + 28] + self.past_plays_dracula[35 * revealed + 28:35 * (revealed + 1)] + self.past_plays_hunter[35 * (revealed + 1):]
        self.action_space = np.asarray(result['actions'])

        # print("past plays (dracula_view): " + self.past_plays_dracula)
        # print("past plays (hunter view): " + self.past_plays_hunter)
        # print(stdoutdata)
        # print(self.player)

        process = subprocess.Popen([os.path.join(ROOT_DIR, "nn_features"), '0'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

        is_dracula = (self.player == 4)
        if is_dracula:
            stdoutdata,_ = process.communicate(input = self.past_plays_dracula)
            self.player = 0
        else:
            stdoutdata,_ = process.communicate(input = self.past_plays_hunter)
            self.player += 1

        # print(stdoutdata)
        result = json.loads(stdoutdata)

        # print("features: " + json.dumps(self.features))
        #TODO: different for hunter
        reward = 0
        done = False
        if self.features[-2] <= 0:
            reward = -100
            done = True
        elif self.features[-1] <= 0:
            self.win = 1
            reward = 100
            done = True
        else:
            reward = self.features[-1] - result['features'][-1] + 9 * (result['features'][-2] - self.features[-2])

        # print('score: %d (%d-%d)' % (self.features[-1] - result['features'][-1], result['features'][-1], self.features[-1]))
        # print('health: %d (%d-%d)' % (9 * (result['features'][-2] - self.features[-2]), result['features'][-2], self.features[-2]))
        self.features = np.asarray(result['features'])
        if done:
            print("done")
            if is_dracula:
                self.rewards += reward
                # print("reward: %d" % reward)
            return self.features, reward, done, {}
        if self.player == 4:
            return self.features, reward, done, {}
        # return self.step(self.getRandomMove())
        _, nxtrwd, done, _ = self.step(self.getRandomMove())
        reward += nxtrwd
        if is_dracula:
            self.rewards += reward
            # print("reward: %d" % reward)
        return self.features, reward, done, {}
    def reset(self):
        self.action_space = np.asarray([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70])
        self.player = 0
        self.rewards = 0
        self.win = 0
        self.past_plays_dracula = ""
        self.past_plays_hunter = ""
        self.features = np.asarray([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 9, 9, 9, 9, 40, 366])
        ob,_,_,_ = self.step(self.getRandomMove())
        return ob
    def getRandomMove(self):
        return self.action_space[random.randint(0, len(self.action_space) - 1)]
    def getHunterAIMove(self):
        process = subprocess.Popen(os.path.join(ROOT_DIR, "hunter_ai"), stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        instr =  "{\"past_plays\": \"%s\", \"messages\": []}" % self.past_plays_hunter
        stdoutdata,_ = process.communicate(input = instr)
        result = json.loads(stdoutdata)
        return result['move']
