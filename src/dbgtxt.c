#include "dbgtxt.h"
#include <stdlib.h>
#include <string.h>
#include <linalgb.h>
#include "opengl.h"
#include "glutils.h"

/* Embedded Fonts */
#include "slkscrfnt.h"
#include "gohufnt.h"

static struct fntdata {
    struct font_map_entry {
        char c;
        int width;
        int x, y;
        int w, h;
        int ox, oy;
    }* map;
    struct font_metrics {
        unsigned int ascender;
        unsigned int descender;
        unsigned int height;
    }* metrics;
    struct font_atlas {
        unsigned int width;
        unsigned int height;
    }* atlas;
    unsigned char* data;
} embedded_fonts[] = {
    {
        .map     = (struct font_map_entry*) gohu_font_map,
        .metrics = (struct font_metrics*) &gohu_font_metrics,
        .atlas   = (struct font_atlas*) &gohu_font_atlas,
        .data    = (unsigned char*) gohu_font_data_bits
    }, {
        .map     = (struct font_map_entry*) slkscr_font_map,
        .metrics = (struct font_metrics*) &slkscr_font_metrics,
        .atlas   = (struct font_atlas*) &slkscr_font_atlas,
        .data    = (unsigned char*) slkscr_font_data_bits
    }
};
#define NUM_EFONTS (sizeof(embedded_fonts) / sizeof (embedded_fonts[0]))

static unsigned int font_index(enum font fnt)
{
    switch (fnt) {
        case FNT_GOHU:
            return 0;
        case FNT_SLKSCR:
            return 1;
        default:
            break;
    }
    return 0;
}

static const char* vshader = "\
#version 330 core                            \n\
layout (location = 0) in vec3 position;      \n\
layout (location = 1) in vec2 uv;            \n\
layout (location = 2) in vec4 color;         \n\
                                             \n\
out VS_OUT {                                 \n\
    vec4 col;                                \n\
    vec2 uv;                                 \n\
} vs_out;                                    \n\
uniform mat4 mvp;                            \n\
                                             \n\
void main()                                  \n\
{                                            \n\
    vs_out.uv = uv;                          \n\
    vs_out.col = color;                      \n\
    gl_Position = mvp * vec4(position, 1.0); \n\
}";

static const char* fshader = "\
#version 330 core                                 \n\
out vec4 color;                                   \n\
                                                  \n\
in VS_OUT {                                       \n\
    vec4 col;                                     \n\
    vec2 uv;                                      \n\
} fs_in;                                          \n\
uniform sampler2D fntatlas;                       \n\
                                                  \n\
void main()                                       \n\
{                                                 \n\
    float a = texture(fntatlas, fs_in.uv.xy).r;   \n\
    color = vec4(fs_in.col.rgb, fs_in.col.a * a); \n\
}";

typedef struct {
    float x, y, z;    /* position */
    float s, t;       /* uv */
    float r, g, b, a; /* color */
} vertex_t;

static struct {
    GLuint shdr;
    GLuint vao, vbo, ebo;
    struct {
        GLuint tex;
        unsigned int width, height;
    } fntatlas[NUM_EFONTS];
    enum font active_fnt;
} st;

static void dbgtxt_uncompress_font(unsigned char** buf, int* w, int* h, struct fntdata* fnt)
{
    /* Fetch font data */
    *w = fnt->atlas->width; *h = fnt->atlas->height;
    unsigned char* font_data_bits = fnt->data;
    /* Align width to 8 byte boundary */
    unsigned int extra_bits = *w % 8;
    *w += extra_bits ? (8 - extra_bits) : 0;
    unsigned int num_pixels = (*w) * (*h);
    *buf = malloc(num_pixels * sizeof(char));
    memset(*buf, 0, num_pixels * sizeof(char));
    for (int i = 0; i < *h; ++i) {
        for (int j = 0; j < *w; ++j) {
            unsigned int cur = i * (*w) + j;
            (*buf)[cur] = (font_data_bits[cur / 8] & (1 << (cur % 8))) ? 255 : 0;
        }
    }
}

static void dbgtxt_add_text(vertex_t* verts, GLuint* indcs, const char* text, size_t num_chars, int x, int y, float col[4])
{
    struct fntdata* fnt = embedded_fonts + st.active_fnt;
    struct font_map_entry* font_map = fnt->map;
    unsigned int atlas_width = st.fntatlas[st.active_fnt].width, atlas_height = st.fntatlas[st.active_fnt].height;
    int pen_x = x, pen_y = y - (fnt->metrics->ascender + fnt->metrics->descender);
    float r = col[0], g = col[1], b = col[2], a = col[3];
    /* Iterate through each character */
    for (size_t i = 0; i < num_chars ; ++i) {
        /* Current character */
        const char cc = text[i];
        /* Handle line breaks */
        if (cc == '\n') {
            pen_y -= atlas_height;
            pen_x = x;
            continue;
        }
        /* Get corresponding glyph for current char */
        struct font_map_entry glph;
        if (cc != '\t')
            glph = font_map[cc - 32]; /* TODO: Replace with font_map.c mapping */
        else {
            glph = font_map[' ' - 32];
            glph.width *= 4; /* Tab is 4 spaces */
        }
        /* Calculate glyph render triangles */
        int x0 = pen_x + glph.ox;
        int y0 = pen_y + glph.oy;
        int x1 = x0 + glph.w;
        int y1 = y0 - glph.h;
        float s0 = (float)(glph.x) / atlas_width;
        float t0 = (float)(glph.y) / atlas_height;
        float s1 = s0 + (float)(glph.w) / atlas_width;
        float t1 = t0 + (float)(glph.h) / atlas_height;
        vertex_t vertices[4] =
            {{x0, y0, 0, s0, t0, r, g, b, a},
             {x0, y1, 0, s0, t1, r, g, b, a},
             {x1, y1, 0, s1, t1, r, g, b, a},
             {x1, y0, 0, s1, t0, r, g, b, a}};
        /* Construct indice list */
        GLuint indices[6] = {0, 1, 2, 0, 2, 3};
        for (int j = 0; j < 6; ++j)
            indices[j] += i * 4;
        /* Append vertices and indices to buffers */
        memcpy(indcs + i * 6, indices, sizeof(indices));
        memcpy(verts + i * 4, vertices, sizeof(vertices));
        /* Advance cursor (make it act like monospaced fonts) */
        /* pen_x += glph.width */
        pen_x += abs(glph.width - 12) <= 2 ? 12 : glph.width;
    }
}

void dbgtxt_init()
{
    /* Compile shader program */
    st.shdr = shader_from_srcs(vshader, 0, fshader);

    for (unsigned int i = 0; i < NUM_EFONTS; ++i) {
        /* Uncompress embedded font */
        unsigned char* imgbuf;
        int width, height;
        dbgtxt_uncompress_font(&imgbuf, &width, &height, embedded_fonts + i);
        st.fntatlas[i].width = width;
        st.fntatlas[i].height = height;

        /* Upload font atlas */
        glGenTextures(1, &st.fntatlas[i].tex);
        glBindTexture(GL_TEXTURE_2D, st.fntatlas[i].tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, imgbuf);
        free(imgbuf);
    }

    /* Create vertex and indice buffers */
    GLuint vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    /* Create vertex attribute object and Setup vertex attributes */
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    GLuint pos_attrib = 0;
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*)offsetof(vertex_t, x));
    GLuint uv_attrib = 1;
    glEnableVertexAttribArray(uv_attrib);
    glVertexAttribPointer(uv_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*)offsetof(vertex_t, s));
    GLuint col_attrib = 2;
    glEnableVertexAttribArray(col_attrib);
    glVertexAttribPointer(col_attrib, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (GLvoid*)offsetof(vertex_t, r));
    glBindVertexArray(0);

    /* Save handles */
    st.vao = vao;
    st.vbo = vbo;
    st.ebo = ebo;
}

void dbgtxt_setfnt(enum font fnt)
{
    st.active_fnt = font_index(fnt);
}

void dbgtxt_prntnc(const char* text, size_t num_chars, float x, float y, float r, float g, float b, float a)
{
    /* Get current viewport size */
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2], height = viewport[3];

    /* Allocate and fill vertex and indice data buffers */
    unsigned int num_verts = 4 * num_chars;
    unsigned int num_indices = 6 * num_chars;
    vertex_t* verts = calloc(num_verts, sizeof(vertex_t));
    GLuint* indices = calloc(num_indices, sizeof(GLuint));
    float col[4] = {r, g, b, a};
    dbgtxt_add_text(verts, indices, text, num_chars, x, height - y, col);

    /* Update vertex and indice buffers */
    glBindBuffer(GL_ARRAY_BUFFER, st.vbo);
    glBufferData(GL_ARRAY_BUFFER, num_verts * sizeof(vertex_t), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, st.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(GLuint), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    /* Disable depth testing for text, and enable blending using the passed color alpha value */
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Setup font texture atlas */
    glUseProgram(st.shdr);
    glUniform1i(glGetUniformLocation(st.shdr, "fntatlas"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, st.fntatlas[st.active_fnt].tex);

    /* Setup and upload ortho matrix */
    mat4 model = mat4_id();
    mat4 view = mat4_id();
    mat4 proj = mat4_orthographic(0, width, 0, height, -1, 1);
    mat4 mvp = mat4_mul_mat4(mat4_mul_mat4(proj, view), model);
    glUniformMatrix4fv(glGetUniformLocation(st.shdr, "mvp"), 1, GL_FALSE, mvp.m);

    /* Render */
    glBindVertexArray(st.vao);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    /* Restore state */
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    /* Free resources */
    free(indices);
    free(verts);
}

void dbgtxt_prntc(const char* text, float x, float y, float r, float g, float b, float a)
{
    size_t num_chars = strlen(text);
    dbgtxt_prntnc(text, num_chars, x, y, r, g, b, a);
}

void dbgtxt_prnt(const char* text, float x, float y)
{
    const float cv = 0.8;
    dbgtxt_prntc(text, x, y, cv, cv, cv, 1.0);
}

void dbgtxt_destroy()
{
    glDeleteBuffers(1, &st.vbo);
    glDeleteBuffers(1, &st.ebo);
    glDeleteVertexArrays(1, &st.vao);
    for (unsigned int i = 0; i < NUM_EFONTS; ++i)
        glDeleteTextures(1, &st.fntatlas[i].tex);
    glDeleteProgram(st.shdr);
    memset(&st, 0, sizeof(st));
}
