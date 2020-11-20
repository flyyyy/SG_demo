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
from skimage import measure

ldrDir = './matterport'
configGtDir = './config/config_gt'
configPreDir = './config/config_pre'
drawDir = './compare/draw'

all_prefix = []
all_files = os.listdir(ldrDir)
for one in all_files:
    if one.endswith('.jpg'):
        all_prefix.append(one[:-4])
all_prefix.sort()


def match(imfil1, imfil2):
    img1 = cv2.imread(imfil1)
    img1 = img1[380:610, 450:860, :]
    (h, w) = img1.shape[:2]
    img2 = cv2.imread(imfil2)
    img2 = img2[380:610, 450:860, :]
    resized = cv2.resize(img2, (w, h))
    (h1, w1) = resized.shape[:2]
    img1 = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
    img2 = cv2.cvtColor(resized, cv2.COLOR_BGR2GRAY)
    return measure.compare_ssim(img1, img2)


if __name__ == '__main__':
    cnt = 0
    for fn in all_prefix:
        gtDrawPath = drawDir + '/' + fn + '_gt.jpg'
        preDrawPath = drawDir + '/' + fn + '_pre.jpg'
        # print(fn, match(gtDrawPath, preDrawPath))
        img = cv2.imread(gtDrawPath)
        img = img[380:610, 450:860, :]
        cv2.imwrite('./test.jpg', img)
