#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <vita2d.h>
#include <stdio.h>
#include <string.h>

#define DEST_DIR  "ux0:app/PSPEMUCFW"
#define DEST_PATH "ux0:app/PSPEMUCFW/661.PBP"
#define SRC_PATH  "app0:/661.PBP"

// 
int dir_exists(const char *path) {
    SceIoStat stat;
    return (sceIoGetstat(path, &stat) >= 0 && (stat.st_mode & SCE_S_IFDIR));
}

// 
int file_exists(const char *path) {
    SceIoStat stat;
    return (sceIoGetstat(path, &stat) >= 0);
}

// 
int copy_file(const char *src, const char *dst) {
    int fdsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
    if (fdsrc < 0) return -1;

    int fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fddst < 0) { sceIoClose(fdsrc); return -1; }

    char buffer[16 * 1024];
    int bytes;
    while ((bytes = sceIoRead(fdsrc, buffer, sizeof(buffer))) > 0) {
        sceIoWrite(fddst, buffer, bytes);
    }

    sceIoClose(fdsrc);
    sceIoClose(fddst);
    return 0;
}

// 
void draw_centered_text(vita2d_pgf *pgf, const char *text, int y, unsigned int color, float scale) {
    int width = vita2d_pgf_text_width(pgf, scale, text); 
    int screen_width = 960; 
    int x = (screen_width - width) / 2;
    vita2d_pgf_draw_text(pgf, x, y, color, scale, text);
}

int main() {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0,0,0,255));

    vita2d_pgf *pgf = vita2d_load_default_pgf();
    SceCtrlData pad;
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    int copied = 0;
    int adrenaline_missing = 0;

    // 
    vita2d_start_drawing();
    vita2d_clear_screen();
    draw_centered_text(pgf, "Checking...", 120, RGBA8(255,255,255,255), 1.0f);
    vita2d_end_drawing();
    vita2d_swap_buffers();

    if (!dir_exists(DEST_DIR)) {
        adrenaline_missing = 1;
    } else {
        if (file_exists(SRC_PATH)) {
            if (copy_file(SRC_PATH, DEST_PATH) == 0) {
                copied = 1;
            }
        }
    }

    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        vita2d_start_drawing();
        vita2d_clear_screen();

        if (adrenaline_missing) {
            draw_centered_text(pgf, "Adrenaline is not installed.", 100, RGBA8(255,255,0,255), 1.0f);
            draw_centered_text(pgf, "Press O to exit", 130, RGBA8(255,255,255,255), 1.0f);
        } else if (copied) {
            draw_centered_text(pgf, "661.PBP PRESENT", 100, RGBA8(0,255,0,255), 1.0f);
            draw_centered_text(pgf, "Press O to exit", 130, RGBA8(255,255,255,255), 1.0f);
        } else {
            draw_centered_text(pgf, "661.PBP NOT FOUND", 100, RGBA8(255,0,0,255), 1.0f);
            draw_centered_text(pgf, "Press O to exit", 130, RGBA8(255,255,255,255), 1.0f);
        }

        vita2d_end_drawing();
        vita2d_swap_buffers();

        if (pad.buttons & SCE_CTRL_CIRCLE) break; // 
    }

    vita2d_fini();
    vita2d_free_pgf(pgf);
    sceKernelExitProcess(0);
    return 0;
}
