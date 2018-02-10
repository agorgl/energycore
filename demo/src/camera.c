#include "camera.h"
#include <string.h>
#include <math.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

void camera_defaults(struct camera* cam)
{
    cam->pos = cam->prev_pos = vec3_new(0.0f, 0.0f, 0.0f);
    cam->rot = cam->prev_rot = quat_id();
    cam->eul = vec3_new(-90.0f, 0.0f, 0.0f);
    cam->move_speed = 3.0f;
    cam->sensitivity = 3.0f;
    cam->pitch_lim = 89.0f;
    cam->view_mat = mat4_id();
}

void camera_move(struct camera* cam, int move_directions, float dt)
{
    mat4 v = mat4_rotation_quat(cam->rot);
    vec3 forward = vec3_new(v.m2[0][2], v.m2[1][2], v.m2[2][2]);
    vec3 strafe  = vec3_new(v.m2[0][0], v.m2[1][0], v.m2[2][0]);
    float dx = 0.0f, dz = 0.0f;
    for (int mask = (1 << 3); mask != 0; mask >>= 1) {
        switch (move_directions & mask) {
            case cmd_forward:
                ++dz;
                break;
            case cmd_left:
                --dx;
                break;
            case cmd_backward:
                --dz;
                break;
            case cmd_right:
                ++dx;
                break;
            default:
                break;
        }
    }
    dx *= cam->move_speed * dt;
    dz *= cam->move_speed * dt;
    cam->pos = vec3_add(cam->pos, vec3_add(vec3_mul(forward, -dz), vec3_mul(strafe, dx)));
}

void camera_look(struct camera* cam, float offx, float offy, float dt)
{
    offx *= cam->sensitivity * dt;
    offy *= cam->sensitivity * dt;

    cam->eul.x += offy;
    cam->eul.y += offx;
    cam->eul.x = CLAMP(cam->eul.x, -cam->pitch_lim, cam->pitch_lim);

    quat pq = quat_rotation_x(radians(cam->eul.x));
    quat yq = quat_rotation_y(radians(cam->eul.y));
    cam->rot = quat_mul_quat(pq, yq);
}

void camera_setdir(struct camera* cam, vec3 dir)
{
    dir = vec3_normalize(vec3_mul(dir, -1));
    cam->eul.x = degrees(asin(dir.y));
    cam->eul.y = degrees(atan2f(dir.x, dir.z));
    camera_look(cam, 0.0f, 0.0f, 0.0f);
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
