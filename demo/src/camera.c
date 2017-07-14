#include "camera.h"
#include <string.h>
#include <math.h>

void camera_defaults(struct camera* cam)
{
    cam->pos = vec3_new(0.0f, 0.0f, 0.0f);
    cam->prev_pos = cam->pos;
    cam->rot = quat_id();
    cam->prev_rot = cam->rot;
    cam->yaw = -90.0f;
    cam->pitch = 0.0f;
    cam->move_speed = 0.05f;
    cam->sensitivity = 0.05f;
    cam->pitch_lim = 89.0f;
    cam->view_mat = mat4_id();
}

void camera_move(struct camera* cam, int move_directions)
{
    mat4 v = mat4_rotation_quat(cam->rot);
    vec3 forward = vec3_new(v.m2[0][2], v.m2[1][2], v.m2[2][2]);
    vec3 strafe  = vec3_new(v.m2[0][0], v.m2[1][0], v.m2[2][0]);
    float dx = 0.0f, dz = 0.0f;
    for (int mask = (1 << 3); mask != 0; mask >>= 1) {
        switch (move_directions & mask) {
            case cmd_forward:
                dz += cam->move_speed;
                break;
            case cmd_left:
                dx += -cam->move_speed;
                break;
            case cmd_backward:
                dz += -cam->move_speed;
                break;
            case cmd_right:
                dx += cam->move_speed;
                break;
            default:
                break;
        }
    }
    cam->pos = vec3_add(cam->pos, vec3_add(vec3_mul(forward, -dz), vec3_mul(strafe, dx)));
}

void camera_look(struct camera* cam, float offx, float offy)
{
    offx *= cam->sensitivity;
    offy *= cam->sensitivity;

    cam->yaw += offx;
    cam->pitch += offy;

    if (cam->pitch > cam->pitch_lim)
        cam->pitch = cam->pitch_lim;
    if (cam->pitch < -cam->pitch_lim)
        cam->pitch = -cam->pitch_lim;

    quat pq = quat_rotation_x(radians(cam->pitch));
    quat yq = quat_rotation_y(radians(cam->yaw));
    cam->rot = quat_mul_quat(pq, yq);
}

void camera_setdir(struct camera* cam, vec3 dir)
{
    dir = vec3_normalize(vec3_mul(dir, -1));
    cam->pitch = degrees(asin(dir.y));
    cam->yaw   = degrees(atan2f(dir.x, dir.z));

    camera_look(cam, 0.0f, 0.0f);
}

static inline mat4 camera_view(vec3 pos, quat rot)
{
    return mat4_mul_mat4(
        mat4_rotation_quat(rot),
        mat4_translation(vec3_neg(pos))
    );
}

void camera_update(struct camera* cam)
{
    cam->prev_rot = cam->rot;
    cam->prev_pos = cam->pos;
    cam->view_mat = camera_view(cam->pos, cam->rot);
}

mat4 camera_interpolated_view(struct camera* cam, float interpolation)
{
    vec3 ipos = vec3_lerp(cam->prev_pos, cam->pos, interpolation);
    quat irot = quat_slerp(cam->prev_rot, cam->rot, interpolation);
    return camera_view(ipos, irot);
}
