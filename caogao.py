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

img_path = './get_test_sample/testsample_depth/pano_aadmuaxyxouqic_dep.png'

mask = cv2.imread(img_path)

mask = 255 - mask

# 360 480

cv2.imwrite('mask.png', mask)
