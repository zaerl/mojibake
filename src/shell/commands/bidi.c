/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../mojibake.h"
#include "../maps.h"
#include "../shell.h"
#include "commands.h"

static const char *bidi_dir_name(mjb_direction dir) {
    switch(dir) {
        case MJB_DIRECTION_LTR: return "Left-to-right";
        case MJB_DIRECTION_RTL: return "Right-to-left";
        default: return "Auto";
    }
}

static const char *bidi_dir_json(mjb_direction dir) {
    switch(dir) {
        case MJB_DIRECTION_LTR: return "ltr";
        case MJB_DIRECTION_RTL: return "rtl";
        default: return "auto";
    }
}

static void mjbsh_bidi_revolve(const char *input) {
    size_t input_size = strlen(input);
    bool is_json = (cmd_output_mode == OUTPUT_MODE_JSON);

    mjb_bidi_paragraph para;

    if(!mjb_bidi_resolve(input, input_size, MJB_ENCODING_UTF_8, MJB_DIRECTION_AUTO, &para)) {
        fprintf(stderr, "bidi: resolution failed\n");
        return;
    }

    size_t *visual_order = para.count > 0
        ? (size_t *)malloc(para.count * sizeof(size_t))
        : NULL;

    if(visual_order) {
        mjb_bidi_reorder_line(&para, 0, para.count, visual_order);
    }

    if(is_json) {
        printf("{%s", mjbsh_jnl());
    }

    mjbsh_numeric("Paragraph level", 1, para.paragraph_level);
    mjbsh_value("Direction", 1, "%s", is_json ? bidi_dir_json(para.direction) : bidi_dir_name(para.direction));

    if(is_json) {
        printf("%s%s\"chars\":%s[%s", mjbsh_ji(), mjbsh_ji(), cmd_json_indent == 0 ? "" : " ",
            para.count > 0 ? mjbsh_jnl() : "");
    }

    for(size_t i = 0; i < para.count; ++i) {
        mjb_bidi_char c = para.chars[i];

        if(is_json) {
            printf("%s%s{%s", mjbsh_ji(), mjbsh_ji(), mjbsh_jnl());
        }

        unsigned int previous_indent = cmd_json_indent;
        cmd_json_indent *= 2;

        if(!is_json) {
            puts("");
        }

        mjbsh_value("Codepoint", 1, "U+%04X", (unsigned int)c.codepoint);
        mjbsh_numeric("Level", 1, c.level);
        mjbsh_numeric("Resolved class", 1, c.resolved_class);
        mjbsh_value("Mirror glyph", 0, "U+%04X", (unsigned int)c.mirroring_glyph);

        cmd_json_indent = previous_indent;

        if(is_json) {
            printf("%s%s}%s%s", mjbsh_ji(), mjbsh_ji(), para.count > 1 && i != para.count - 1 ? "," : "", mjbsh_jnl());
        }
    }

    if(is_json) {
        printf("%s%s],%s", mjbsh_ji(), mjbsh_ji(), mjbsh_jnl());
    }

    if(is_json) {
        printf(
            "%s%s\"visual_order\":%s[%s",
            mjbsh_ji(), mjbsh_ji(),
            cmd_json_indent == 0 ? "" : " ", mjbsh_green());
    } else {
        printf("\nVisual order: %s", mjbsh_green());
    }

    for(size_t i = 0; i < para.count; ++i) {
        if(is_json) {
            printf("%zu%s", visual_order[i], i == para.count - 1 ? "" : ", ");
        } else {
            printf("%zu%s", visual_order[i], i == para.count - 1 ? "" : " ");
        }
    }

    printf("%s%s", mjbsh_reset(), is_json ? "]" : "");

    if(is_json) {
        printf("%s}", mjbsh_jnl());
    }

    free(visual_order);
    mjb_bidi_free(&para);
}

int mjbsh_bidi_command(int argc, char * const argv[], unsigned int flags) {
    mjbsh_bidi_revolve(argv[0]);

    return 0;
}
