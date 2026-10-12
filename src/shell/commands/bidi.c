/**
 * The Mojibake shell
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */

#include "../shell.h"

static const char *bidi_dir_name(mjb_direction dir) {
    switch(dir) {
        case MJB_DIRECTION_LTR:
            return "Left-to-right";
        case MJB_DIRECTION_RTL:
            return "Right-to-left";
        default:
            return "Auto";
    }
}

static const char *bidi_dir_json(mjb_direction dir) {
    switch(dir) {
        case MJB_DIRECTION_LTR:
            return "ltr";
        case MJB_DIRECTION_RTL:
            return "rtl";
        default:
            return "auto";
    }
}

static int mjbsh_bidi_revolve(const char *input) {
    size_t input_size = strlen(input);
    bool is_json = (cmd_output_mode == OUTPUT_MODE_JSON);

    mjb_bidi_paragraph para;
    mjb_status status = mjb_bidi_resolve(input, input_size, MJB_ENC_UTF_8, cmd_direction, &para);

    if(status != MJB_STATUS_OK) {
        return mjbsh_error("%s", mjb_status_message(status));
    }

    size_t *visual_order = para.count > 0 ? (size_t *)malloc(para.count * sizeof(size_t)) : NULL;

    if(para.count > 0 && visual_order == NULL) {
        mjb_bidi_paragraph_free(&para);

        return mjbsh_error("bidi: could not allocate visual order");
    }

    if(para.count > 0 &&
        mjb_bidi_reorder_line(&para, 0, para.count, visual_order) != MJB_STATUS_OK) {
        free(visual_order);
        mjb_bidi_paragraph_free(&para);

        return mjbsh_error("bidi: line reordering failed");
    }

    // Visual level runs: count first, then fill.
    size_t run_count = 0;
    mjb_bidi_run *runs = NULL;

    if(para.count > 0) {
        if(mjb_bidi_line_runs(&para, visual_order, para.count, NULL, &run_count) != MJB_STATUS_OK) {
            free(visual_order);
            mjb_bidi_paragraph_free(&para);

            return mjbsh_error("bidi: line runs failed");
        }

        runs = run_count > 0 ? (mjb_bidi_run *)malloc(run_count * sizeof(mjb_bidi_run)) : NULL;

        if(run_count > 0 && runs == NULL) {
            free(visual_order);
            mjb_bidi_paragraph_free(&para);

            return mjbsh_error("bidi: could not allocate runs");
        }

        if(run_count > 0 &&
            mjb_bidi_line_runs(&para, visual_order, para.count, runs, &run_count) !=
                MJB_STATUS_OK) {
            free(runs);
            free(visual_order);
            mjb_bidi_paragraph_free(&para);

            return mjbsh_error("bidi: line runs failed");
        }
    }

    if(is_json) {
        printf("{%s", mjbsh_jnl());
    }

    mjbsh_value("Base direction", 1, "%s",
        is_json ? bidi_dir_json(cmd_direction) : bidi_dir_name(cmd_direction));
    mjbsh_numeric("Paragraph level", 1, para.paragraph_level);
    mjbsh_value("Direction", 1, "%s",
        is_json ? bidi_dir_json(para.direction) : bidi_dir_name(para.direction));

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
            printf("%s%s}%s%s", mjbsh_ji(), mjbsh_ji(),
                para.count > 1 && i != para.count - 1 ? "," : "", mjbsh_jnl());
        }
    }

    if(is_json) {
        printf("%s%s],%s", mjbsh_ji(), mjbsh_ji(), mjbsh_jnl());
        printf("%s%s\"visual_order\":%s[%s", mjbsh_ji(), mjbsh_ji(),
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
        printf(",%s%s%s\"runs\":%s[%s", mjbsh_jnl(), mjbsh_ji(), mjbsh_ji(),
            cmd_json_indent == 0 ? "" : " ", run_count > 0 ? mjbsh_jnl() : "");
    } else {
        puts("");
    }

    for(size_t i = 0; i < run_count; ++i) {
        if(is_json) {
            printf("%s%s{%s", mjbsh_ji(), mjbsh_ji(), mjbsh_jnl());
        } else {
            puts("");
        }

        unsigned int previous_indent = cmd_json_indent;
        cmd_json_indent *= 2;

        mjbsh_numeric("Run", 1, (unsigned int)i);
        mjbsh_numeric("Start", 1, (unsigned int)runs[i].start);
        mjbsh_numeric("End", 1, (unsigned int)runs[i].end);
        mjbsh_numeric("Level", 1, runs[i].level);
        mjbsh_value("Direction", 0, "%s",
            is_json ? bidi_dir_json(runs[i].direction) : bidi_dir_name(runs[i].direction));

        cmd_json_indent = previous_indent;

        if(is_json) {
            printf("%s%s}%s%s", mjbsh_ji(), mjbsh_ji(), i != run_count - 1 ? "," : "", mjbsh_jnl());
        }
    }

    if(is_json) {
        printf("%s%s]%s}", mjbsh_ji(), mjbsh_ji(), mjbsh_jnl());
    }

    free(runs);
    free(visual_order);
    mjb_bidi_paragraph_free(&para);

    return 0;
}

int mjbsh_bidi_command(int argc, char *const argv[], unsigned int flags) {
    (void)argc;
    (void)flags;

    return mjbsh_bidi_revolve(argv[0]);
}
