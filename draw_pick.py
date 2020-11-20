import os
import cv2
from skimage import measure

ldrDir = './compare/rough_d/meet/input'
roughDrawDir = "./compare/rough_d/meet/draw"

roughList = ["0.1", "0.2", "0.4", "0.8"]
targetList = [0.82, 0.84, 0.86, 0.87]
# roughList = ["0.2"]
# targetList = [0.84]

all_prefix = []
all_files = os.listdir(ldrDir)
for one in all_files:
    if one.endswith('.jpg'):
        all_prefix.append(one[:-4])
all_prefix.sort()


def ssim(imfil1, imfil2):
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


def check(fn):
    prefix = fn[:-5]
    rowId = int(fn[-3])
    colId = int(fn[-1])
    for index in range(len(roughList)):
        drawGtImgPath = "{}/{}_i{}_{}_gt_{}.jpg".format(roughDrawDir, prefix, rowId, colId, roughList[index])
        drawPreImgPath = "{}/{}_i{}_{}_pre_{}.jpg".format(roughDrawDir, prefix, rowId, colId, roughList[index])
        print('name: {} rough: {} ssim: {}'.format(fn, roughList[index], ssim(drawGtImgPath, drawPreImgPath)))
        if ssim(drawGtImgPath, drawPreImgPath) < targetList[index]:
            return False
    return True


if __name__ == "__main__":
    ansPrefix = []
    for fn in all_prefix:
        check(fn)


# def myMove(fn):
#     prefix = fn[:-5]
#     rowId = int(fn[-3])
#     colId = int(fn[-1])
#     for index in range(len(roughList)):
#         srcDrawGtImgPath = "{}/rough{}/{}_i{}_{}_gt.jpg".format(roughDrawDir, roughList[index], prefix, rowId, colId)
#         srcDrawPreImgPath = "{}/rough{}/{}_i{}_{}_pre.jpg".format(roughDrawDir, roughList[index], prefix, rowId, colId)
#         dstDrawGtImgPath = "{}/meet/{}_i{}_{}_gt_{}.jpg".format(roughDrawDir, prefix, rowId, colId, roughList[index])
#         dstDrawPreImgPath = "{}/meet/{}_i{}_{}_pre_{}.jpg".format(roughDrawDir, prefix, rowId, colId, roughList[index])
#         shutil.copy2(srcDrawGtImgPath, dstDrawGtImgPath)
#         shutil.copy2(srcDrawPreImgPath, dstDrawPreImgPath)