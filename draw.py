import os
import random
import time
import math
import cv2
import copy
import numpy as np
import threading
import argparse
import shutil

ldrDir = './matterport'
configGtDir = './config/config_gt'
configPreDir = './config/config_pre'

all_prefix = []
all_files = os.listdir(ldrDir)
for one in all_files:
    if one.endswith('.jpg'):
        all_prefix.append(one[:-4])
all_prefix.sort()
random.shuffle(all_prefix)

# trans multi 10
scale_list = [1, 2, 4, 6, 8, 12]
trans_list = [0, 1, 2, 3, 4]


def modify_config(filename, scale, trans):
    f = open(filename, 'r')
    lines = f.readlines()
    lines[-1] = '{}_s{}_t{}.jpg\n'.format(lines[-1][:-5], scale, trans)
    f.close()
    trans /= 10

    f = open(filename, 'w')
    for line in lines:
        f.write('{}'.format(line))
    f.write('SG_scale:\n')
    f.write('{}\n'.format(str(scale)))
    f.write('SG_trans:\n')
    f.write('{}\n'.format(str(trans)))
    f.close()


def draw_gt(fn, scale, trans):
    prefix = fn[:-5]
    row_id = int(fn[-3])
    col_id = int(fn[-1])
    configGtPath = configGtDir + '/' + prefix + '_%d_%d_config.txt' % (row_id, col_id)
    configDrawPath = './scene_config.txt'
    shutil.copy2(configGtPath, configDrawPath)

    modify_config(configDrawPath, scale, trans)
    os.system('IndoorDemo.exe')


def draw_pre(fn, scale, trans):
    prefix = fn[:-5]
    row_id = int(fn[-3])
    col_id = int(fn[-1])
    configPrePath = configPreDir + '/' + prefix + '_%d_%d_config.txt' % (row_id, col_id)
    configDrawPath = './scene_config.txt'
    shutil.copy2(configPrePath, configDrawPath)

    modify_config(configDrawPath, scale, trans)
    os.system('IndoorDemo.exe')


if __name__ == '__main__':
    print(len(all_prefix))
    cnt = 0
    for fn in all_prefix:
        if cnt >= 100:
            break
        cnt += 1
        for scale in scale_list:
            for trans in trans_list:
                # draw_gt(fn, scale, trans)
                draw_pre(fn, scale, trans)
