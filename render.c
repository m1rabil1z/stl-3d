#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define W 80
#define H 40

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;

    uint8_t header[80];
    uint32_t count;
    fread(header, 1, 80, f);
    fread(&count, 4, 1, f);

    float zbuffer[W * H];
    char screen[W * H];
    for (int i = 0; i < W * H; i++) {
        zbuffer[i] = -10000.0f;
        screen[i] = ' ';
    }

    float min_x = 1e9, max_x = -1e9;
    float min_y = 1e9, max_y = -1e9;
    float min_z = 1e9, max_z = -1e9;

    long data_start = ftell(f);
    
    for (uint32_t i = 0; i < count; i++) {
        float n[3], v[9];
        uint16_t attr;
        fread(n, 12, 1, f);
        fread(v, 36, 1, f);
        fread(&attr, 2, 1, f);
        for(int j = 0; j < 3; j++) {
            if(v[j*3] < min_x) min_x = v[j*3];
            if(v[j*3] > max_x) max_x = v[j*3];
            if(v[j*3+1] < min_y) min_y = v[j*3+1];
            if(v[j*3+1] > max_y) max_y = v[j*3+1];
            if(v[j*3+2] < min_z) min_z = v[j*3+2];
            if(v[j*3+2] > max_z) max_z = v[j*3+2];
        }
    }

    float cx = (min_x + max_x) / 2.0f;
    float cy = (min_y + max_y) / 2.0f;
    float cz = (min_z + max_z) / 2.0f;
    
    float scale_x = W / (max_x - min_x + 0.1f);
    float scale_y = H / (max_y - min_y + 0.1f);
    float scale = (scale_x < scale_y * 2.0f) ? scale_x : scale_y * 2.0f;
    scale *= 0.8f;

    fseek(f, data_start, SEEK_SET);

    for (uint32_t i = 0; i < count; i++) {
        float n[3], v[9];
        uint16_t attr;
        fread(n, 12, 1, f);
        fread(v, 36, 1, f);
        fread(&attr, 2, 1, f);

        for (int j = 0; j < 3; j++) {
            float dx = v[j*3] - cx;
            float dy = v[j*3+1] - cy;
            float dz = v[j*3+2] - cz;

            float rx = dx * 0.707f - dy * 0.707f;
            float ry = dx * 0.408f + dy * 0.408f - dz * 0.816f;
            float rz = dx * 0.577f + dy * 0.577f + dz * 0.577f;

            int x = (int)(W / 2 + rx * scale);
            int y = (int)(H / 2 + ry * scale * 0.5f);

            if (x >= 0 && x < W && y >= 0 && y < H) {
                int idx = x + y * W;
                if (rz > zbuffer[idx]) {
                    zbuffer[idx] = rz;
                    screen[idx] = '#';
                }
            }
        }
    }
    
    fclose(f);

    for (int i = 0; i < W * H; i++) {
        putchar(screen[i]);
        if ((i + 1) % W == 0) putchar('\n');
    }
    
    return 0;
}