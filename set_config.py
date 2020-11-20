import numpy as np
import cv2
import os
import shutil
import torch

root_dir = './get_test_sample'
ldr_dir = root_dir + '/testsample_ldr'
SG_param_dir = root_dir + '/testsample_result'
config_dir = root_dir + '/draw_config'
draw_dir = root_dir + '/draw'
depth_dir = root_dir + '/testsample_depth'

objRotMat = [
    -0.34904227, 0.02939312, 0.93664604, 0.00000000, 0.93182892, -0.09504716, 0.35022992, 0.00000000, 0.09931970, 0.99503869, 0.00578615, 0.00000000,
    0.00000000, 0.00000000, 0.00000000, 1.00000000
]
objMat = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -0.2, 0, 0.2, 1]
sfMat = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -1.4, 0, 1]

# bg_path           background path
# config_path       config save path
# draw_save_path    draw_pic save path
# weight            SG rgb(128*3)
# alpha             SG alpha(128)


def config_print(bg_path, config_path, draw_save_path, weight, alpha, SG_pos, SH, SG_depth, depth_path):
    weight = weight.reshape(-1)
    alpha = alpha.reshape(-1)
    with open(config_path, 'w') as f:
        f.write('model:\n')
        f.write('1\n')
        f.write('./teapot.obj\n')
        f.write('background:\n')
        f.write('%s\n' % bg_path)
        f.write('depth:\n')
        f.write('%s\n' % depth_path)
        f.write('elevation:\n')
        f.write('-20\n')
        f.write('fov:\n')
        f.write('45\n')
        f.write('SG:\n')
        for w in weight:
            f.write(str(round(w, 2)) + ' ')
        f.write('\n')
        f.write('alpha:\n')
        for a in alpha:
            f.write(str(round(a, 2)) + ' ')
        f.write('\n')
        f.write('center:\n')
        for i in range(128):
            f.write(str(round(SG_pos[i][0], 2) * SG_depth[i]) + ' ' + str(round(SG_pos[i][1], 2) * SG_depth[i]) + ' ' + str(round(SG_pos[i][2], 2) * SG_depth[i]) + ' ')
        f.write('\n')
        f.write('SH:\n')
        for i in range(16):
            f.write(str(round(SH[i][0], 2)) + ' ' + str(round(SH[i][1], 2)) + ' ' + str(round(SH[i][2], 2)) + '\n')
        f.write('objRotMat:\n')
        for i in range(16):
            f.write(str(objRotMat[i]) + ' ')
        f.write('\n')
        f.write('objMat:\n')
        for i in range(16):
            f.write(str(objMat[i]) + ' ')
        f.write('\n')
        f.write('sfMat:\n')
        for i in range(16):
            f.write(str(sfMat[i]) + ' ')
        f.write('\n')
        f.write('sfY:\n')
        f.write('-1.4\n')
        f.write('save_path:\n')
        f.write('%s\n' % draw_save_path)


def get_center(n):
    angle = np.pi * (3.0 - np.sqrt(5.0))
    theta = angle * np.arange(n, dtype=np.float32)
    theta += np.pi/2
    y = np.linspace(1.0 - 1.0 / n, 1.0 / n - 1.0, n)
    r = np.sqrt(1.0 - y * y)
    center = np.zeros((n, 3), dtype=np.float32)
    center[:, 0] = r * np.cos(theta)
    center[:, 1] = y
    center[:, 2] = r * np.sin(theta)
    return center


def config2img(weight, depth, pos):
    img = np.zeros((1, 384, 3), dtype=np.float32)
    for index in range(128):
        img[0][index] = weight[index]
        img[0][128+index] = pos[index]
        img[0][128*2+index][0] = depth[index]
    cv2.imwrite('config.exr', img)


def get_prefix():
    all_file = os.listdir(ldr_dir)
    all_prefix = []
    for file in all_file:
        all_prefix.append(file[:-4])
    return all_prefix


def set_config(all_prefix):
    for prefix in all_prefix:
        bg_path = '{}/{}.png'.format(ldr_dir, prefix)
        config_path = '{}/{}.txt'.format(config_dir, prefix)
        draw_save_path = '{}/{}.jpg'.format(draw_dir, prefix)
        SG_param_path = '{}/{}_illu.npy'.format(SG_param_dir, prefix)
        depth_path = '{}/{}.png'.format(depth_dir, prefix)
        weight = np.load(SG_param_path)
        weight[:, [0, 1, 2]] = weight[:, [2, 1, 0]]
        alpha = np.zeros(128, dtype=np.float)
        alpha[:] = 45.4
        sg_pos = get_center(128)
        SH = np.zeros((16, 3))
        SG_depth = np.ones(128)

        # config2img(weight, SG_depth, sg_pos)
        config_print(bg_path, config_path, draw_save_path, weight, alpha, sg_pos, SH, SG_depth, depth_path)


def draw_gt(config_dir, all_prefix, save_dir):
    for prefix in all_prefix:
        srcConfigPath = '{}/{}.txt'.format(config_dir, prefix)
        dstConfigPath = './scene_config.txt'
        shutil.copy2(srcConfigPath, dstConfigPath)
        os.system('IndoorDemo.exe')


def xy_to_omega(x, y, height, width):
    half_width = width / 2
    half_height = height / 2
    x = (x - half_width + 0.5) / half_width
    y = (y - half_height + 0.5) / half_height
    return (y * np.pi * 0.5, x * np.pi)


def get_pixel_map(height, width):
    data_idx = 0
    pixel_map = np.zeros((width * height, 2), dtype=np.float32)
    for y in range(height):
        for x in range(width):
            omega = xy_to_omega(x, height - 1 - y, height, width)
            pixel_map[data_idx] = (omega[0], omega[1])
            data_idx += 1
    return pixel_map


def cal_I(x, alpha, elevation, azimuth, weight):
    x = torch.from_numpy(x).cuda().float()
    alpha = torch.from_numpy(alpha).cuda().float()
    elevation = torch.from_numpy(elevation).cuda().float()
    azimuth = torch.from_numpy(azimuth).cuda().float()
    weight = torch.from_numpy(weight).cuda().float()

    mi = torch.sin(x[:, 0]).view(-1, 1) * torch.sin(elevation) + torch.cos(x[:, 0]).view(
        -1, 1) * torch.cos(elevation) * torch.cos(x[:, 1].view(-1, 1).expand(-1, 128) - azimuth) - 1
    exp = torch.exp(mi * alpha)
    I = torch.stack((exp, exp, exp), dim=-1) * weight
    I = torch.sum(I, dim=1)
    return I


# TODO: azimuth diff pi/2
def get_center_ea(n):
    angle = np.pi * (3.0 - np.sqrt(5.0))
    theta = angle * np.arange(n, dtype=np.float32)
    theta += np.pi/2
    y = np.linspace(1.0 - 1.0 / n, 1.0 / n - 1.0, n)

    return np.arcsin(y), theta


def get_SGEnv(all_prefix):
    elevation, azimuth = get_center_ea(128)
    pixel_map = get_pixel_map(512, 1024)
    alpha = np.zeros(128, dtype=np.float)
    alpha[:] = 45.4
    for prefix in all_prefix:
        SG_param_path = '{}/{}_illu.npy'.format(SG_param_dir, prefix)
        weight = np.load(SG_param_path)

        I = cal_I(pixel_map, alpha, elevation, azimuth, weight)

        I = I.reshape(512, 1024, 3).cpu().data.numpy()
        save_path = '{}/{}_gt_sg_env.exr'.format(draw_dir, prefix)
        cv2.imwrite(save_path, I)


if __name__ == "__main__":
    all_prefix = get_prefix()
    all_prefix = all_prefix[: 1]

    # set config
    set_config(all_prefix)

    # draw obj to draw_dir
    # draw_gt(config_dir, all_prefix, draw_dir)

    # get_SGEnv(all_prefix)
