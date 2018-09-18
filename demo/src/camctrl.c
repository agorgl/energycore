#include "camctrl.h"
#include <string.h>
#include <math.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

void camctrl_defaults(struct camctrl* cam)
{
    cam->pos = cam->prev_pos = vec3_new(0.0f, 0.0f, 0.0f);
    cam->rot = cam->prev_rot = quat_id();
    cam->eul = vec3_new(-90.0f, 0.0f, 0.0f);
    cam->pitch_lim = 89.0f;
    cam->vel = cam->angvel = vec3_zero();
    cam->accel = 100.0;
    cam->ang_accel = 60.0;
    cam->max_vel = 3.0;
    cam->max_angvel = 720;
    cam->view_mat = mat4_id();
}

void camctrl_move(struct camctrl* cam, int move_directions, float dt)
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
    dx *= cam->accel * dt;
    dz *= cam->accel * dt;
    cam->vel = vec3_add(cam->vel, vec3_add(vec3_mul(forward, -dz), vec3_mul(strafe, dx)));
}

void camctrl_look(struct camctrl* cam, float offx, float offy, float dt)
{
    cam->angvel.x += offy * cam->ang_accel * dt;
    cam->angvel.y += offx * cam->ang_accel * dt;
}

void camctrl_setpos(struct camctrl* cam, vec3 pos)
{
    cam->pos = pos;
}

void camctrl_setdir(struct camctrl* cam, vec3 dir)
{
    dir = vec3_normalize(vec3_mul(dir, -1));
    cam->eul.x = degrees(asin(dir.y));
    cam->eul.y = degrees(atan2f(dir.x, dir.z));
}

static inline mat4 camctrl_view(vec3 pos, quat rot)
{
    return mat4_mul_mat4(
        mat4_rotation_quat(rot),
        mat4_translation(vec3_neg(pos))
    );
}

void camctrl_update(struct camctrl* cam, float dt)
{
    /* Store previous state for interpolation */
    cam->prev_rot = cam->rot;
    cam->prev_pos = cam->pos;

    /* Movement */
    float vspeed = CLAMP(vec3_length(cam->vel), 0.0f, cam->max_vel);
    if (vspeed)
        cam->vel = vec3_mul(vec3_normalize(cam->vel), vspeed);
    cam->pos = vec3_add(cam->pos, vec3_mul(cam->vel, dt));
    cam->vel = vec3_mul(cam->vel, pow(0.001, dt));

    /* Rotation */
    float aspeed = CLAMP(vec3_length(cam->angvel), 0.0, cam->max_angvel);
    if (aspeed)
        cam->angvel = vec3_mul(vec3_normalize(cam->angvel), aspeed);
    cam->eul = vec3_add(cam->eul, vec3_mul(cam->angvel, dt));
    cam->angvel = vec3_mul(cam->angvel, pow(0.000000005, dt));

    /* Limit pitch and construct rotation quaternion */
    cam->eul.x = CLAMP(cam->eul.x, -cam->pitch_lim, cam->pitch_lim);
    quat pq = quat_rotation_x(radians(cam->eul.x));
    quat yq = quat_rotation_y(radians(cam->eul.y));
    cam->rot = quat_mul_quat(pq, yq);

    /* Construct view matrix */
    cam->view_mat = camctrl_view(cam->pos, cam->rot);
}

mat4 camctrl_interpolated_view(struct camctrl* cam, float interpolation)
{
    vec3 ipos = vec3_lerp(cam->prev_pos, cam->pos, interpolation);
    quat irot = quat_slerp(cam->prev_rot, cam->rot, interpolation);
    return camctrl_view(ipos, irot);
}
