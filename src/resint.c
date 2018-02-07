#include "resint.h"
#include <string.h>
#include <stdio.h>
#include <glad/glad.h>
#include <energycore/asset.h>
#include "txtpp.h"

static const struct shdr_info {
    const char* name;
    const char* vs_loc;
    const char* gs_loc;
    const char* fs_loc;
} shdr_infos[] = {
    {
        .name = "geom_pass",
        .vs_loc = "static_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "geom_pass_fs.glsl"
    },
    {
        .name = "dir_light",
        .vs_loc = "passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "dir_light_fs.glsl"
    },
    {
        .name = "env_light",
        .vs_loc = "passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "env_light_fs.glsl"
    },
    {
        .name = "sky_prth",
        .vs_loc = "sky_prth_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "sky_prth_fs.glsl"
    },
    {
        .name = "irr_conv",
        .vs_loc = "ibl/cubemap_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "ibl/convolution_fs.glsl"
    },
    {
        .name = "brdf_lut",
        .vs_loc = "passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "ibl/brdf_lut_fs.glsl"
    },
    {
        .name = "prefilter",
        .vs_loc = "ibl/cubemap_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "ibl/prefilter_fs.glsl"
    },
    {
        .name = "probe_vis",
        .vs_loc = "static_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "vis/probe_fs.glsl"
    },
    {
        .name = "norm_vis",
        .vs_loc = "vis/normal_vs.glsl",
        .gs_loc = "vis/normal_gs.glsl",
        .fs_loc = "vis/normal_fs.glsl"
    },
    {
        .name = "tonemap_fx",
        .vs_loc = "passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "fx/tonemap.glsl"
    },
    {
        .name = "gamma_fx",
        .vs_loc = "passthrough_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "fx/gamma.glsl"
    },
    {
        .name = "smaa_fx",
        .vs_loc = "fx/smaa_pass_vs.glsl",
        .gs_loc = 0,
        .fs_loc = "fx/smaa_pass_fs.glsl"
    }
};

static unsigned int shdrs[sizeof(shdr_infos)] = {};

static int txtpp_custom_load(void* ud, const char* fpath, unsigned char** buf)
{
    (void)ud;
    void* data; size_t size;
    int r = embedded_file(&data, &size, fpath);
    if (!r)
        return 0;
    *buf = calloc(1, size + 1);
    memcpy(*buf, data, size);
    return 1;
}

static void txtpp_err_cb(void* userdata, const char* err)
{
    (void) userdata;
    fprintf(stderr, "%s\n", err);
}

static const char* shader_load(const char* fpath)
{
    if (!fpath)
        return 0;

    /* Construct shader filepath */
    const char* shader_folder = "shaders/";
    char* rpath = calloc(1, 8 + strlen(fpath) + 1);
    strcat(rpath, shader_folder);
    strcat(rpath, fpath);

    /* Make a preprocessed load for included files */
    struct txtpp_settings settings;
    settings.load_type   = TXTPP_LOAD_CUSTOM;
    settings.load_fn     = txtpp_custom_load;
    settings.error_cb    = txtpp_err_cb;
    const char* shdr_src = txtpp_load(rpath, &settings);

    free(rpath);
    return shdr_src;
}

void resint_init()
{
    for (unsigned int i = 0; i < sizeof(shdr_infos)/sizeof(shdr_infos[0]); ++i) {
        const struct shdr_info* si = shdr_infos + i;
        const char* vs_src = shader_load(si->vs_loc);
        const char* gs_src = shader_load(si->gs_loc);
        const char* fs_src = shader_load(si->fs_loc);
        shdrs[i] = shader_from_srcs(vs_src, gs_src, fs_src);
        free((void*)vs_src);
        if (gs_src)
            free((void*)gs_src);
        if (fs_src)
            free((void*)fs_src);
    }
}

unsigned int resint_shdr_fetch(const char* shdr_name)
{
    for (unsigned int i = 0; i < sizeof(shdr_infos)/sizeof(shdr_infos[0]); ++i) {
        const struct shdr_info* si = shdr_infos + i;
        if (strcmp(shdr_name, si->name) == 0)
            return shdrs[i];
    }
    return 0;
}

void resint_destroy()
{
    for (unsigned int i = 0; i < sizeof(shdr_infos)/sizeof(shdr_infos[0]); ++i) {
        glDeleteProgram(shdrs[i]);
        shdrs[i] = 0;
    }
}
