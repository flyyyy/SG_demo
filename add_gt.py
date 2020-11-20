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
import generate_photo

dataDir = './input'
matterportDir = './matterport'
saveDir = './save'
poseDir = '/root/indoor_data/matterport/v1/undistorted_camera_pose'
hdrDir = '/root/indoor_data/matterport/v1/matterport_exr_images'

allPrefix = []
allFiles = os.listdir(dataDir)
for one in allFiles:
    if 'jpg' in one:
        allPrefix.append(one[:-4])
allPrefix.sort()
allPrefix = list(set(allPrefix))


def rotate_pano(depthPath, offElevation, offAzimuth, rowId, colId, outPath):
    generate_photo.take_photo_ex(depthPath, offElevation, -offAzimuth, outPath)


if __name__ == "__main__":
    # for fn in allPrefix:
    #     srcPath = '{}/{}.jpg'.format(matterportDir, fn)
    #     dstPath = '{}/{}.jpg'.format(saveDir, fn)
    #     shutil.copy2(srcPath, dstPath)
    print(len(allPrefix))
    for fn in allPrefix:
        prefix = fn[:-5]
        rowId = int(fn[-3])
        colId = int(fn[-1])

        # load camera pose txt to get vx and vy
        camera_path = '{}/{}.txt'.format(poseDir, prefix)
        with open(camera_path) as f:
            line = f.readline()
            vxVy = line.split(' ')

            index = rowId*6 + colId
            offAzimuth = float(vxVy[index])
            offElevation = 0

            # rotate hdr pano
            hdrPath = '{}/{}.exr'.format(hdrDir, prefix)
            savePath = '{}/{}_{}_{}.exr'.format(saveDir, prefix, rowId, colId)
            hdrPano = rotate_pano(hdrPath, offElevation, offAzimuth, rowId, colId, savePath)
