import os
import random
import time
import math
import cv2
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.autograd import Variable
from PIL import Image
from skimage import io, transform, filters
from scipy.optimize import fmin_l_bfgs_b
import shutil
import subprocess

# global configuration
resize_width = 128
resize_height = 64
ori_width = 1024
ori_height = 512
kernel_count = 3
bin_r = 4
bin_c = 16

hdr_dir = './hdr'
ldr_dir = './ldr'
axis_dir = './axis'
result_dir = './result'
data_dir = '/root/indoor_optim/sample_data'


def take_photo(image_path, elevation, azimuth):
    out_path = './hdr/temp.exr'
    y = math.sin(elevation)
    x = math.cos(elevation) * math.sin(azimuth)
    z = math.cos(elevation) * math.cos(azimuth)
    cmd = 'mitsuba -Dsrc="%s" -Dtarget="%f %f %f" -q -o %s pano_rotate.xml' % (image_path, x, y, z, out_path)
    #print(cmd)
    os.system(cmd)


def take_photo_ex(image_path, elevation, azimuth, out_path):
    y = math.sin(elevation)
    x = math.cos(elevation) * math.sin(azimuth)
    z = math.cos(elevation) * math.cos(azimuth)

    uy = math.cos(elevation)
    ux = -math.sin(elevation) * math.sin(azimuth)
    uz = -math.sin(elevation) * math.cos(azimuth)

    cmd = 'mitsuba -Dsrc="%s" -Dtarget="%f %f %f" -Dup="%f %f %f" -q -o %s pano_rotate.xml' % (image_path, x, y, z, ux, uy, uz, out_path)
    #print(cmd)
    # os.system(cmd)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    (output, err) = p.communicate()
    p_status = p.wait()


def take_photo_ex_ldr(image_path, elevation, azimuth, out_path):
    y = math.sin(elevation)
    x = math.cos(elevation) * math.sin(azimuth)
    z = math.cos(elevation) * math.cos(azimuth)

    uy = math.cos(elevation)
    ux = -math.sin(elevation) * math.sin(azimuth)
    uz = -math.sin(elevation) * math.cos(azimuth)

    cmd = 'mitsuba -Dsrc="%s" -Dtarget="%f %f %f" -Dup="%f %f %f" -q -o %s pano_rotate_ldr.xml' % (image_path, x, y, z, ux, uy, uz, out_path)
    #print(cmd)
    # os.system(cmd)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    (output, err) = p.communicate()
    p_status = p.wait()


if __name__ == "__main__":
    all_prefix = []
    all_files = os.listdir(data_dir)
    for one in all_files:
        if one.endswith('.txt'):
            all_prefix.append(one[:-17])
    all_prefix.sort()

    fn = 'pano_aaaapkoyhqmkdh'
    camera_path = data_dir + '/' + fn + '_camera_param.txt'
    with open(camera_path) as f:
        for i in range(7):
            line = f.readline()
            line = line[9:]
            off_elevation, off_azimuth, fov = line.split(',')
            off_elevation = 0
            off_azimuth = float(off_azimuth) / 180 * math.pi

            image_path = hdr_dir + '/' + fn + '.exr'
            out_path = result_dir + '/' + fn + '%d.exr' % i
            take_photo_ex(image_path, off_elevation, -off_azimuth, out_path)
