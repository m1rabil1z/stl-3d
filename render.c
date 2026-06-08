#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define W 80
#define H 40

float zbuffer[W * H];
char screen[W * H];

void draw_line(int x0, int y0, float z0, int x1, int y1, float z1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    
    int steps = dx > -dy ? dx : -dy;
    int step = 0;

    while (1) {
        if (x0 >= 0 && x0 < W && y0 >= 0 && y0 < H) {
            int idx = x0 + y0 * W;
            float t = steps == 0 ? 1.0f : (float)step / steps;
            float rz = z0 + t * (z1 - z0);
            
            if (rz > zbuffer[idx]) {
                zbuffer[idx] = rz;
                screen[idx] = '#';
            }
        }
        
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
        step++;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;

    uint8_t header[80];
    uint32_t count;
    if (fread(header, 1, 80, f) != 80 || fread(&count, 4, 1, f) != 1) {
        fclose(f);
        return 1;
    }

    for (int i = 0; i < W * H; i++) {
        zbuffer[i] = -10000.0f;
        screen[i] = ' ';
    }

    float min_rx = 1e9, max_rx = -1e9;
    float min_ry = 1e9, max_ry = -1e9;

    long data_start = ftell(f);
    
    for (uint32_t i = 0; i < count; i++) {
        float n[3], v[9];
        uint16_t attr;
        if (fread(n, 12, 1, f) != 1 || fread(v, 36, 1, f) != 1 || fread(&attr, 2, 1, f) != 1) break;
        
        for(int j = 0; j < 3; j++) {
            float rx = v[j*3] * 0.707f - v[j*3+1] * 0.707f;
            float ry = v[j*3] * 0.408f + v[j*3+1] * 0.408f - v[j*3+2] * 0.816f;
            
            if(rx < min_rx) min_rx = rx;
            if(rx > max_rx) max_rx = rx;
            if(ry < min_ry) min_ry = ry;
            if(ry > max_ry) max_ry = ry;
        }
    }

    float crx = (min_rx + max_rx) / 2.0f;
    float cry = (min_ry + max_ry) / 2.0f;
    
    float scale_x = W / (max_rx - min_rx + 0.1f);
    float scale_y = H / (max_ry - min_ry + 0.1f);
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    scale *= 0.85f;

    fseek(f, data_start, SEEK_SET);

    for (uint32_t i = 0; i < count; i++) {
        float n[3], v[9];
        uint16_t attr;
        if (fread(n, 12, 1, f) != 1 || fread(v, 36, 1, f) != 1 || fread(&attr, 2, 1, f) != 1) break;

        int sx[3], sy[3];
        float sz[3];

        for (int j = 0; j < 3; j++) {
            float rx = v[j*3] * 0.707f - v[j*3+1] * 0.707f;
            float ry = v[j*3] * 0.408f + v[j*3+1] * 0.408f - v[j*3+2] * 0.816f;
            sz[j] = v[j*3] * 0.577f + v[j*3+1] * 0.577f + v[j*3+2] * 0.577f;

            sx[j] = (int)((W - 1) / 2.0f + (rx - crx) * scale);
            sy[j] = (int)((H - 1) / 2.0f + (ry - cry) * scale);
        }

        draw_line(sx[0], sy[0], sz[0], sx[1], sy[1], sz[1]);
        draw_line(sx[1], sy[1], sz[1], sx[2], sy[2], sz[2]);
        draw_line(sx[2], sy[2], sz[2], sx[0], sy[0], sz[0]);
    }
    
    fclose(f);

    for (int i = 0; i < W * H; i++) {
        putchar(screen[i]);
        if ((i + 1) % W == 0) putchar('\n');
    }
    
    return 0;
}