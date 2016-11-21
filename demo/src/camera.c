#include "camera.h"
#include <string.h>
#include <math.h>

void camera_defaults(struct camera* cam)
{
    cam->pos = vec3_new(0, 0, 0);
    cam->front = vec3_new(0, 0, -1);
    cam->up = vec3_new(0, 1, 0);
    cam->yaw = -90.0f;
    cam->pitch = 0.0f;
    cam->move_speed = 0.05f;
    cam->sensitivity = 0.05f;
    cam->pitch_lim = 89.0f;
    cam->prev_view_mat = mat4_id();
    cam->view_mat = mat4_id();
}

void camera_move(struct camera* cam, int move_directions)
{
    for (int mask = (1 << 3); mask != 0; mask >>= 1) {
        switch (move_directions & mask) {
            case cmd_forward:
                cam->pos = vec3_add(cam->pos, vec3_mul(cam->front, cam->move_speed));
                break;
            case cmd_left:
                cam->pos = vec3_sub(cam->pos, (vec3_mul(vec3_normalize(vec3_cross(cam->front, cam->up)), cam->move_speed)));
                break;
            case cmd_backward:
                cam->pos = vec3_sub(cam->pos, vec3_mul(cam->front, cam->move_speed));
                break;
            case cmd_right:
                cam->pos = vec3_add(cam->pos, (vec3_mul(vec3_normalize(vec3_cross(cam->front, cam->up)), cam->move_speed)));
                break;
            default:
                break;
        }
    }
}

void camera_look(struct camera* cam, float offx, float offy)
{
    offx *= cam->sensitivity;
    offy *= cam->sensitivity;

    cam->yaw += offx;
    cam->pitch -= offy;

    if (cam->pitch > cam->pitch_lim)
        cam->pitch = cam->pitch_lim;
    if (cam->pitch < -cam->pitch_lim)
        cam->pitch = -cam->pitch_lim;

    vec3 direction = vec3_new(
        cos(radians(cam->pitch)) * cos(radians(cam->yaw)),
        sin(radians(cam->pitch)),
        cos(radians(cam->pitch)) * sin(radians(cam->yaw))
    );
    cam->front = vec3_normalize(direction);
}

void camera_update(struct camera* cam)
{
    memcpy(&cam->prev_view_mat, &cam->view_mat, sizeof(mat4));
    cam->view_mat = mat4_view_look_at(cam->pos, vec3_add(cam->pos, cam->front), cam->up);
}

mat4 camera_interpolated_view(struct camera* cam, float interpolation)
{
    float prev[4][4], new[4][4], out[4][4];
    mat4_to_array_trans(cam->prev_view_mat, (float*)prev);
    mat4_to_array_trans(cam->view_mat, (float*)new);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            out[i][j] = prev[i][j] + (new[i][j] - prev[i][j]) * interpolation;
    return *(mat4*)out;
}
