#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/ctrl.h>
#include <vita2d.h>
#include <stdio.h>
#include <string.h>

#define SRC_PATH "app0:/661.PBP"

static int dir_exists(const char *path) {
    SceIoStat st;
    return (sceIoGetstat(path, &st) >= 0 && (st.st_mode & SCE_S_IFDIR));
}

static int file_exists(const char *path) {
    SceIoStat st;
    return (sceIoGetstat(path, &st) >= 0);
}

static void draw_centered_text(vita2d_pgf *pgf, const char *text, int y, unsigned int color, float scale) {
    int w = vita2d_pgf_text_width(pgf, scale, text);
    int x = (960 - w) / 2;
    vita2d_pgf_draw_text(pgf, x, y, color, scale, text);
}

static int copy_file_with_progress(const char *src, const char *dst, vita2d_pgf *pgf) {
    if(file_exists(dst)) return 1; // already exists, skip copy

    int in = sceIoOpen(src, SCE_O_RDONLY, 0);
    if (in < 0) return -1;
    int out = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
    if (out < 0) { sceIoClose(in); return -2; }

    SceIoStat st; sceIoGetstat(src, &st);
    uint64_t total = (uint64_t)st.st_size;
    uint64_t done  = 0;

    char buf[16 * 1024];
    int rd;
    SceCtrlData pad;
    while ((rd = sceIoRead(in, buf, sizeof(buf))) > 0) {
        int wr = sceIoWrite(out, buf, rd);
        if (wr < 0) { sceIoClose(in); sceIoClose(out); return -3; }
        done += rd;

        // Draw progress UI
        vita2d_start_drawing();
        vita2d_clear_screen();
        char msg[128];
        int percent = (total > 0) ? (int)((done * 100) / total) : 100;
        snprintf(msg, sizeof(msg), "Copying 661.PBP... %d%%", percent);
        draw_centered_text(pgf, msg, 200, RGBA8(255,255,255,255), 1.0f);

        int full = 600;
        int fill = (total > 0) ? (int)((done * full) / total) : full;
        int x = (960 - full) / 2;
        vita2d_draw_rectangle(x, 240, full, 20, RGBA8(60,60,60,255));
        vita2d_draw_rectangle(x, 240, fill, 20, RGBA8(0,200,0,255));
        vita2d_end_drawing();
        vita2d_swap_buffers();

        sceCtrlPeekBufferPositive(0, &pad, 1);
        if (pad.buttons & SCE_CTRL_CIRCLE) {
            sceIoClose(in);
            sceIoClose(out);
            return -4; // user canceled
        }
    }

    sceIoClose(in);
    sceIoClose(out);
    return 0;
}

static void do_check_and_copy(const char *dest_dir, const char *dest_path, int *copied, int *adrenaline_missing, int *src_missing, vita2d_pgf *pgf) {
    *copied = 0; 
    *adrenaline_missing = 0; 
    *src_missing = 0;

    if (!dir_exists(dest_dir)) {
        *adrenaline_missing = 1;
        return;
    }
    if (!file_exists(SRC_PATH)) {
        *src_missing = 1;
        return;
    }
    if(!file_exists(dest_path)) { // only copy if not exists
        if (copy_file_with_progress(SRC_PATH, dest_path, pgf) == 0) {
            *copied = 1;
        }
    } else {
        *copied = 1; // already exists
    }
}

int main(void) {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0,0,0,255));
    vita2d_pgf *pgf = vita2d_load_default_pgf();

    SceCtrlData pad = {0}, old = {0};
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    const char *partition = "ux0:";
    while (1) {
        old = pad; sceCtrlPeekBufferPositive(0, &pad, 1);
        int pressed = pad.buttons & ~old.buttons;

        vita2d_start_drawing();
        vita2d_clear_screen();
        draw_centered_text(pgf, "Select destination partition", 100, RGBA8(255,255,255,255), 1.0f);
        draw_centered_text(pgf, partition, 130, RGBA8(0,255,255,255), 1.0f);
        draw_centered_text(pgf, "Left/Right to change  -  X to confirm", 170, RGBA8(200,200,200,255), 1.0f);
        vita2d_end_drawing();
        vita2d_swap_buffers();

        if (pressed & SCE_CTRL_LEFT)  partition = "ux0:";
        if (pressed & SCE_CTRL_RIGHT) partition = "uma0:";
        if (pressed & SCE_CTRL_CROSS) break;
    }

    char dest_dir[64];
    char dest_path[128];
    snprintf(dest_dir, sizeof(dest_dir), "%sapp/PSPEMUCFW", partition);
    snprintf(dest_path, sizeof(dest_path), "%s/661.PBP", dest_dir);

    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_centered_text(pgf, "Checking target...", 120, RGBA8(255,255,255,255), 1.0f);
    draw_centered_text(pgf, dest_dir, 150, RGBA8(180,180,180,255), 1.0f);
    vita2d_end_drawing();
    vita2d_swap_buffers();

    int copied = 0;
    int adrenaline_missing = 0;
    int src_missing = 0;

    do_check_and_copy(dest_dir, dest_path, &copied, &adrenaline_missing, &src_missing, pgf);

    while (1) {
        old = pad; sceCtrlPeekBufferPositive(0, &pad, 1);
        int pressed = pad.buttons & ~old.buttons;

        vita2d_start_drawing();
        vita2d_clear_screen();

        if (adrenaline_missing) {
            draw_centered_text(pgf, "Adrenaline is not installed.", 100, RGBA8(255,255,0,255), 1.0f);
            draw_centered_text(pgf, "Install Adrenaline and run again.", 130, RGBA8(255,255,255,255), 1.0f);
        } else if (src_missing) {
            draw_centered_text(pgf, "661.PBP not found in app0:/", 100, RGBA8(255,0,0,255), 1.0f);
            draw_centered_text(pgf, "Place 661.PBP in app0:/ and rescan.", 130, RGBA8(255,255,255,255), 1.0f);
        } else if (copied) {
            draw_centered_text(pgf, "661.PBP is ready!", 100, RGBA8(0,255,0,255), 1.0f);
            char where[256]; 
            snprintf(where, sizeof(where), "Destination: %s", dest_path);
            draw_centered_text(pgf, where, 130, RGBA8(200,200,200,255), 1.0f);
        } else {
            draw_centered_text(pgf, "Copy failed.", 100, RGBA8(255,0,0,255), 1.0f);
        }

        draw_centered_text(pgf, "O: Exit    △: Rescan", 190, RGBA8(200,200,200,255), 1.0f);
        vita2d_end_drawing();
        vita2d_swap_buffers();

        if (pressed & SCE_CTRL_CIRCLE) break;
        if (pressed & SCE_CTRL_TRIANGLE) {
            // Reset flags for proper UI update
            copied = 0;
            adrenaline_missing = 0;
            src_missing = 0;

            // Show rescanning message
            vita2d_start_drawing();
            vita2d_clear_screen();
            draw_centered_text(pgf, "Rescanning...", 120, RGBA8(255,255,255,255), 1.0f);
            draw_centered_text(pgf, dest_dir, 150, RGBA8(180,180,180,255), 1.0f);
            vita2d_end_drawing();
            vita2d_swap_buffers();

            // Re-run check and copy
            do_check_and_copy(dest_dir, dest_path, &copied, &adrenaline_missing, &src_missing, pgf);
        }
    }

    vita2d_fini();
    vita2d_free_pgf(pgf);
    sceKernelExitProcess(0);
    return 0;
}
