/**
 * @file real_terrain.c
 * @brief Runtime-converted terrain display list for PC port
 *
 * Loads real terrain vertex data (grd_s_c1_1) and builds Gfx commands
 * at runtime to avoid 64-bit pointer truncation in static initializers.
 */

#include "pc/gbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* Number of vertices in grd_s_c1_1 */
#define REAL_TERRAIN_VTX_COUNT 230

/* Number of Gfx commands in the display list
 * Setup + texture commands + 8 vtx loads + 142 triangles + 1 end = ~180, use 250 for safety */
#define REAL_TERRAIN_GFX_COUNT 250

/* Raw vertex data from build/GAFE01_00/include/assets/field/bg/grd_s_c1_1_v.inc */
/* Format: x, y, z, flag, s, t, r, g, b, a (as s16 values) */
static const s16 grd_s_c1_1_vtx_raw[REAL_TERRAIN_VTX_COUNT][10] = {
    {5312, 2560, 6912, 1, 1344, 3902, 0, 120, 0, 178},
    {4512, 2560, 7792, 1, 55, 5317, 0, 120, 0, 178},
    {5632, 2560, 7568, 1, 1856, 4952, 0, 120, 0, 178},
    {1408, 2560, 6896, 1, -4900, 3873, 0, 120, 0, 178},
    {1472, 2560, 5968, 1, -4809, 2397, 0, 120, 0, 178},
    {528, 2560, 5888, 1, -6322, 2273, 0, 120, 0, 178},
    {2032, 2560, 7104, 1, -3898, 4214, 0, 120, 0, 178},
    {0, 2560, 3200, 1, -7168, -2048, 0, 120, 0, 178},
    {2336, 2560, 4976, 1, -3414, 794, 0, 120, 0, 178},
    {0, 2560, 5760, 1, -7168, 2048, 0, 120, 0, 178},
    {7984, 2560, 1424, 1, 5619, -4870, 0, 120, 0, 178},
    {8544, 2560, 2720, 1, 6503, -2803, 0, 120, 0, 178},
    {10240, 2560, 0, 1, 9216, -7168, 0, 120, 0, 178},
    {10240, 2560, 3200, 1, 9216, -2048, 0, 120, 0, 178},
    {0, 2560, 0, 1, -7168, -7168, 0, 120, 0, 178},
    {2560, 2560, 0, 1, -3072, -7168, 0, 120, 0, 178},
    {4128, 2560, 2608, 1, -541, -2978, 0, 120, 0, 178},
    {3840, 2560, 1072, 1, -1014, -5452, 0, 120, 0, 178},
    {8224, 640, 7088, 1, 6009, 4195, 33, 76, 86, 50},
    {8608, 640, 8544, 1, 6627, 6514, 0, 120, 0, 178},
    {9088, 640, 6480, 1, 7397, 3200, 17, 102, 60, 50},
    {10240, 640, 8320, 1, 9216, 6144, 0, 120, 0, 178},
    {0, 640, 8320, 1, -7168, 6144, 0, 120, 0, 178},
    {0, 640, 10240, 1, -7168, 9216, 0, 120, 0, 178},
    {1840, 640, 7696, 1, -4207, 5152, 226, 98, 61, 96},
    {2560, 640, 10240, 1, -3072, 9216, 0, 120, 0, 178},
    {7680, 2560, 0, 1, 5120, -7168, 0, 120, 0, 178},
    {3696, 2560, 5120, 1, -1232, 1028, 0, 120, 0, 178},
    {4864, 2560, 5616, 1, 628, 1842, 0, 120, 0, 178},
    {5552, 2560, 3200, 1, 1715, -2032, 0, 120, 0, 178},
    {6448, 2560, 4320, 1, 3169, -232, 0, 120, 0, 178},
    {7504, 640, 9328, 1, 4855, 7776, 0, 120, 0, 178},
    {7680, 640, 10240, 1, 5120, 9216, 0, 120, 0, 178},
    {8608, 640, 8544, 1, 6627, 6514, 0, 120, 0, 178},
    {10240, 640, 10240, 1, 9216, 9216, 0, 120, 0, 178},
    {4128, 2560, 2608, 1, -541, -2978, 0, 120, 0, 178},
    {3696, 2560, 5120, 1, -1232, 1028, 0, 120, 0, 178},
    {5552, 2560, 3200, 1, 1715, -2032, 0, 120, 0, 178},
    {10240, 640, 8320, 1, 9216, 6144, 0, 120, 0, 178},
    {5200, 640, 8304, 1, 1170, 6139, 34, 88, 73, 50},
    {5120, 640, 10240, 1, 1024, 9216, 0, 120, 0, 178},
    {6256, 640, 8752, 1, 2847, 6844, 0, 120, 0, 178},
    {6320, 2560, 6640, 1, 2967, 3468, 0, 120, 0, 178},
    {6368, 2560, 6128, 1, 3043, 2658, 0, 120, 0, 178},
    {4864, 2560, 5616, 1, 628, 1842, 0, 120, 0, 178},
    {6448, 2560, 4320, 1, 3169, -232, 0, 120, 0, 178},
    {0, 2560, 3200, 1, -7168, -2048, 0, 120, 0, 178},
    {2336, 2560, 4976, 1, -3414, 794, 0, 120, 0, 178},
    {5120, 2560, 0, 1, 1024, -7168, 0, 120, 0, 178},
    {2560, 2560, 0, 1, -3072, -7168, 0, 120, 0, 178},
    {3840, 2560, 1072, 1, -1014, -5452, 0, 120, 0, 178},
    {10240, 2560, 5760, 1, 9216, 2048, 0, 120, 0, 178},
    {10240, 2560, 3200, 1, 9216, -2048, 0, 120, 0, 178},
    {8896, 2560, 5392, 1, 7078, 1470, 0, 120, 0, 178},
    {8064, 2560, 4128, 1, 5750, -559, 0, 120, 0, 178},
    {7680, 2560, 0, 1, 5120, -7168, 0, 120, 0, 178},
    {6688, 2560, 592, 1, 3538, -6204, 0, 120, 0, 178},
    {0, 640, 6560, 1, -7168, 3328, 0, 91, 78, 50},
    {0, 640, 8320, 1, -7168, 6144, 0, 120, 0, 178},
    {640, 640, 6528, 1, -6143, 3289, 206, 72, 81, 58},
    {1840, 640, 7696, 1, -4207, 5152, 226, 98, 61, 96},
    {2560, 640, 10240, 1, -3072, 9216, 0, 120, 0, 178},
    {3168, 640, 7744, 1, -2090, 5234, 219, 60, 96, 50},
    {2560, 640, 10240, 1, -3072, 9216, 0, 120, 0, 178},
    {3856, 640, 8320, 1, -991, 6151, 239, 89, 78, 50},
    {3168, 640, 7744, 1, -2090, 5234, 219, 60, 96, 50},
    {5120, 640, 10240, 1, 1024, 9216, 0, 120, 0, 178},
    {5200, 640, 8304, 1, 1170, 6139, 34, 88, 73, 50},
    {10240, 640, 8320, 1, 9216, 6144, 0, 120, 0, 178},
    {10240, 640, 6560, 1, 9216, 3328, 0, 91, 78, 50},
    {9088, 640, 6480, 1, 7397, 3200, 17, 102, 60, 50},
    {6320, 2560, 6640, 1, 2967, 3468, 0, 120, 0, 178},
    {5312, 2560, 6912, 1, 1344, 3902, 0, 120, 0, 178},
    {5632, 2560, 7568, 1, 1856, 4952, 0, 120, 0, 178},
    {4864, 2560, 5616, 1, 628, 1842, 0, 120, 0, 178},
    {6256, 640, 8752, 1, 2847, 6844, 0, 120, 0, 178},
    {6480, 640, 7216, 1, 3217, 4382, 34, 80, 82, 50},
    {8064, 2560, 4128, 1, 5750, -559, 0, 120, 0, 178},
    {10240, 2560, 3200, 1, 9216, -2048, 0, 120, 0, 178},
    {8544, 2560, 2720, 1, 6503, -2803, 0, 120, 0, 178},
    {7680, 2560, 0, 1, 5120, -7168, 0, 120, 0, 178},
    {6688, 2560, 592, 1, 3538, -6204, 0, 120, 0, 178},
    {7984, 2560, 1424, 1, 5619, -4870, 0, 120, 0, 178},
    {8064, 2560, 6528, 1, 5756, 3297, 0, 120, 0, 178},
    {6368, 2560, 6128, 1, 3043, 2658, 0, 120, 0, 178},
    {9024, 2560, 5984, 1, 7273, 2411, 0, 120, 0, 178},
    {8896, 2560, 5392, 1, 7078, 1470, 0, 120, 0, 178},
    {8656, 2560, 6416, 1, 6685, 3105, 0, 120, 0, 178},
    {10240, 2560, 5760, 1, 9216, 2048, 0, 120, 0, 178},
    {5424, 2560, 1664, 1, 7168, 2048, 0, 120, 0, 178},
    {6688, 2560, 592, 1, 8192, 0, 0, 120, 0, 178},
    {5120, 2560, 0, 1, 6144, 0, 0, 120, 0, 178},
    {6992, 2560, 2704, 1, 9216, 2048, 0, 120, 0, 178},
    {7984, 2560, 1424, 1, 10240, 0, 0, 120, 0, 178},
    {6992, 2560, 2704, 1, 13312, 2048, 0, 120, 0, 178},
    {8064, 2560, 4128, 1, 14336, 0, 0, 120, 0, 178},
    {8544, 2560, 2720, 1, 12288, 0, 0, 120, 0, 178},
    {6992, 2560, 2704, 1, 17408, 2048, 0, 120, 0, 178},
    {5552, 2560, 3200, 1, 18432, 0, 0, 120, 0, 178},
    {6448, 2560, 4320, 1, 16384, 0, 0, 120, 0, 178},
    {6992, 2560, 2704, 1, 15360, 2048, 0, 120, 0, 178},
    {5424, 2560, 1664, 1, 19456, 2048, 0, 120, 0, 178},
    {5424, 2560, 1664, 1, 5120, 2048, 0, 120, 0, 178},
    {3840, 2560, 1072, 1, 4096, 0, 0, 120, 0, 178},
    {5424, 2560, 1664, 1, 3072, 2048, 0, 120, 0, 178},
    {4128, 2560, 2608, 1, 2048, 0, 0, 120, 0, 178},
    {5424, 2560, 1664, 1, 1024, 2048, 0, 120, 0, 178},
    {5552, 2560, 3200, 1, 0, 0, 0, 120, 0, 178},
    {6992, 2560, 2704, 1, 11264, 2048, 0, 120, 0, 178},
    {8064, 2560, 6528, 1, 6144, 0, 0, 120, 0, 178},
    {8896, 2560, 5392, 1, 4096, 0, 0, 120, 0, 178},
    {7520, 2560, 5408, 1, 5120, 2048, 0, 120, 0, 178},
    {6368, 2560, 6128, 1, 8192, 0, 0, 120, 0, 178},
    {7520, 2560, 5408, 1, 7168, 2048, 0, 120, 0, 178},
    {8064, 2560, 4128, 1, 2048, 0, 0, 120, 0, 178},
    {7520, 2560, 5408, 1, 3072, 2048, 0, 120, 0, 178},
    {6448, 2560, 4320, 1, 0, 0, 0, 120, 0, 178},
    {7520, 2560, 5408, 1, 1024, 2048, 0, 120, 0, 178},
    {6448, 2560, 4320, 1, 10240, 0, 0, 120, 0, 178},
    {7520, 2560, 5408, 1, 9216, 2048, 0, 120, 0, 178},
    {4512, 2560, 7792, 1, -33, 0, 0, 120, 0, 178},
    {5152, 2560, 8208, 1, 1147, 512, 17, 88, 79, 50},
    {5632, 2560, 7568, 1, 2048, 0, 0, 120, 0, 178},
    {5200, 640, 8304, 1, 1418, 2048, 34, 88, 73, 50},
    {5776, 2560, 7776, 1, 2305, 512, 65, 71, 71, 50},
    {6480, 640, 7216, 1, 3947, 2048, 34, 80, 82, 50},
    {6400, 2560, 6992, 1, 3850, 512, 21, 105, 52, 60},
    {6320, 2560, 6640, 1, 3850, 0, 0, 120, 0, 178},
    {1840, 640, 7696, 1, -4399, 2048, 226, 98, 61, 96},
    {1184, 2560, 7088, 1, -5612, 512, 190, 69, 72, 86},
    {640, 640, 6528, 1, -7180, 2048, 206, 72, 81, 58},
    {1904, 2560, 7552, 1, -4260, 512, 241, 98, 66, 72},
    {656, 2560, 6416, 1, -6963, 512, 232, 100, 61, 92},
    {528, 2560, 5888, 1, -7307, 0, 0, 120, 0, 178},
    {1408, 2560, 6896, 1, -5169, 0, 0, 120, 0, 178},
    {2032, 2560, 7104, 1, -4137, 0, 0, 120, 0, 178},
    {0, 2560, 5760, 1, -8192, 0, 0, 120, 0, 178},
    {8288, 2560, 6944, 1, 6799, 512, 24, 101, 59, 50},
    {8656, 2560, 6416, 1, 7475, 0, 0, 120, 0, 178},
    {8064, 2560, 6528, 1, 6676, 0, 0, 120, 0, 178},
    {9088, 2560, 6400, 1, 8274, 512, 29, 78, 86, 50},
    {9024, 2560, 5984, 1, 8274, 0, 0, 120, 0, 178},
    {3872, 2560, 8240, 1, -819, 512, 226, 99, 61, 96},
    {3904, 2560, 7776, 1, -819, 0, 0, 120, 0, 178},
    {0, 640, 6560, 1, -8192, 2048, 0, 91, 78, 50},
    {0, 2560, 6240, 1, -8192, 512, 0, 91, 78, 58},
    {3168, 640, 7744, 1, -2376, 2048, 219, 60, 96, 50},
    {3136, 2560, 7616, 1, -2294, 512, 227, 89, 74, 64},
    {3856, 640, 8320, 1, -858, 2048, 239, 89, 78, 50},
    {7408, 2560, 7104, 1, 5325, 512, 4, 88, 81, 50},
    {8224, 640, 7088, 1, 6981, 2048, 33, 76, 86, 50},
    {9088, 640, 6480, 1, 8499, 2048, 17, 102, 60, 50},
    {9088, 640, 6480, 1, 8499, 2048, 17, 102, 60, 50},
    {10240, 640, 6560, 1, 10240, 2048, 0, 91, 78, 50},
    {9088, 2560, 6400, 1, 8274, 512, 29, 78, 86, 50},
    {10240, 2560, 6240, 1, 10240, 512, 0, 91, 78, 86},
    {0, 2560, 6240, 1, -8192, 512, 0, 91, 78, 58},
    {656, 2560, 6416, 1, -6963, 512, 232, 100, 61, 92},
    {0, 2560, 5760, 1, -8192, 0, 0, 120, 0, 178},
    {1904, 2560, 7552, 1, -4260, 512, 241, 98, 66, 72},
    {3200, 2560, 7216, 1, -2294, 0, 0, 120, 0, 178},
    {2032, 2560, 7104, 1, -4137, 0, 0, 120, 0, 178},
    {3136, 2560, 7616, 1, -2294, 512, 227, 89, 74, 64},
    {3872, 2560, 8240, 1, -819, 512, 226, 99, 61, 96},
    {3904, 2560, 7776, 1, -819, 0, 0, 120, 0, 178},
    {6400, 2560, 6992, 1, 3850, 512, 21, 105, 52, 60},
    {7408, 2560, 7104, 1, 5325, 512, 4, 88, 81, 50},
    {6320, 2560, 6640, 1, 3850, 0, 0, 120, 0, 178},
    {8064, 2560, 6528, 1, 6676, 0, 0, 120, 0, 178},
    {8288, 2560, 6944, 1, 6799, 512, 24, 101, 59, 50},
    {9024, 2560, 5984, 1, 8274, 0, 0, 120, 0, 178},
    {10240, 2560, 5760, 1, 10240, 0, 0, 120, 0, 178},
    {4512, 2560, 7792, 1, 4096, 0, 0, 120, 0, 178},
    {4128, 2560, 6576, 1, 5120, 2048, 0, 120, 0, 178},
    {3904, 2560, 7776, 1, 4949, 0, 0, 120, 0, 178},
    {3200, 2560, 7216, 1, 6144, 0, 0, 120, 0, 178},
    {6256, 640, 8752, 1, 2048, 0, 0, 120, 0, 178},
    {7504, 640, 9328, 1, 0, 0, 0, 120, 0, 178},
    {7408, 640, 8064, 1, 1024, 2048, 0, 120, 0, 178},
    {8224, 640, 7088, 1, -4096, 0, 33, 76, 86, 50},
    {6480, 640, 7216, 1, -6144, 0, 34, 80, 82, 50},
    {7408, 640, 8064, 1, -5120, 2048, 0, 120, 0, 178},
    {6256, 640, 8752, 1, -8192, 0, 0, 120, 0, 178},
    {7408, 640, 8064, 1, -7168, 2048, 0, 120, 0, 178},
    {8608, 640, 8544, 1, -2048, 0, 0, 120, 0, 178},
    {7408, 640, 8064, 1, -1024, 2048, 0, 120, 0, 178},
    {7408, 640, 8064, 1, -3072, 2048, 0, 120, 0, 178},
    {1472, 2560, 5968, 1, 10240, 0, 0, 120, 0, 178},
    {2032, 2560, 7104, 1, 8192, 0, 0, 120, 0, 178},
    {2848, 2560, 6128, 1, 9216, 2048, 0, 120, 0, 178},
    {2336, 2560, 4976, 1, -4096, 0, 0, 120, 0, 178},
    {1472, 2560, 5968, 1, -6144, 0, 0, 120, 0, 178},
    {2848, 2560, 6128, 1, -5120, 2048, 0, 120, 0, 178},
    {3696, 2560, 5120, 1, -2048, 0, 0, 120, 0, 178},
    {2848, 2560, 6128, 1, -3072, 2048, 0, 120, 0, 178},
    {2848, 2560, 6128, 1, 7168, 2048, 0, 120, 0, 178},
    {4128, 2560, 6576, 1, -1024, 2048, 0, 120, 0, 178},
    {4864, 2560, 5616, 1, 0, 0, 0, 120, 0, 178},
    {5312, 2560, 6912, 1, 2048, 0, 0, 120, 0, 178},
    {4128, 2560, 6576, 1, 1024, 2048, 0, 120, 0, 178},
    {4128, 2560, 6576, 1, 3072, 2048, 0, 120, 0, 178},
    {4512, 2720, 7792, 1, 10240, 0, 6, 72, 14, 150},
    {4128, 2880, 6576, 1, 11264, 2048, 3, 119, 4, 168},
    {3904, 2720, 7776, 1, 11093, 0, 250, 72, 15, 160},
    {3200, 2720, 7216, 1, 12288, 0, 249, 72, 17, 158},
    {6480, 800, 7216, 1, -2048, 0, 246, 72, 244, 204},
    {6256, 800, 8752, 1, -4096, 0, 243, 72, 6, 180},
    {7408, 960, 8064, 1, -3072, 2048, 0, 119, 0, 178},
    {8224, 800, 7088, 1, 0, 0, 8, 72, 243, 188},
    {7408, 960, 8064, 1, -1024, 2048, 0, 119, 0, 178},
    {8608, 800, 8544, 1, 2048, 0, 14, 72, 5, 154},
    {7408, 960, 8064, 1, 1024, 2048, 0, 119, 0, 178},
    {7504, 800, 9328, 1, 4096, 0, 1, 72, 15, 152},
    {7408, 960, 8064, 1, 3072, 2048, 0, 119, 0, 178},
    {6256, 800, 8752, 1, 6144, 0, 243, 72, 6, 180},
    {7408, 960, 8064, 1, 5120, 2048, 0, 119, 0, 178},
    {5312, 2720, 6912, 1, 8192, 0, 15, 72, 3, 156},
    {4128, 2880, 6576, 1, 9216, 2048, 3, 119, 4, 168},
    {1472, 2720, 5968, 1, 16384, 0, 242, 72, 254, 194},
    {2032, 2720, 7104, 1, 14336, 0, 248, 72, 13, 166},
    {2848, 2880, 6128, 1, 15360, 2048, 251, 119, 0, 182},
    {2848, 2880, 6128, 1, 13312, 2048, 251, 119, 0, 182},
    {3696, 2720, 5120, 1, 4096, 0, 4, 72, 241, 196},
    {2848, 2880, 6128, 1, 3072, 2048, 251, 119, 0, 182},
    {4128, 2880, 6576, 1, 5120, 2048, 3, 119, 4, 168},
    {4864, 2720, 5616, 1, 6144, 0, 12, 72, 246, 180},
    {4128, 2880, 6576, 1, 7168, 2048, 3, 119, 4, 168},
    {2336, 2720, 4976, 1, 2048, 0, 251, 72, 242, 204},
    {1472, 2720, 5968, 1, 0, 0, 242, 72, 254, 194},
    {2848, 2880, 6128, 1, 1024, 2048, 251, 119, 0, 182},
};

/* Runtime-allocated vertex and display list buffers */
static Vtx* real_terrain_vtx = NULL;
static Gfx* real_terrain_dl = NULL;
static bool real_terrain_initialized = false;

/**
 * Convert raw vertex data to Vtx structures
 */
static void convert_vertices(void) {
    for (int i = 0; i < REAL_TERRAIN_VTX_COUNT; i++) {
        const s16* raw = grd_s_c1_1_vtx_raw[i];
        real_terrain_vtx[i].v.ob[0] = raw[0];  /* x */
        real_terrain_vtx[i].v.ob[1] = raw[1];  /* y */
        real_terrain_vtx[i].v.ob[2] = raw[2];  /* z */
        real_terrain_vtx[i].v.flag = raw[3];
        real_terrain_vtx[i].v.tc[0] = raw[4];  /* s */
        real_terrain_vtx[i].v.tc[1] = raw[5];  /* t */
        real_terrain_vtx[i].v.cn[0] = (u8)raw[6];  /* r */
        real_terrain_vtx[i].v.cn[1] = (u8)raw[7];  /* g */
        real_terrain_vtx[i].v.cn[2] = (u8)raw[8];  /* b */
        real_terrain_vtx[i].v.cn[3] = (u8)raw[9];  /* a */
    }
}

/* GBI opcodes (matching gbi_interpreter.c expectations) */
#define GBI_G_VTX       0x04    /* Load vertices */
#define GBI_G_TRI1      0xBF    /* Draw one triangle (F3D style) */
#define GBI_G_ENDDL     0xDF    /* End display list */
#define GBI_G_TEXTURE   0xD7    /* Set texture state */
#define GBI_G_GEOMETRYMODE 0xD9 /* Set geometry mode */
#define GBI_G_SETTIMG   0xFD    /* Set texture image */
#define GBI_G_SETTILE   0xF5    /* Set tile descriptor */
#define GBI_G_LOADBLOCK 0xF3    /* Load texture to TMEM */
#define GBI_G_SETTILESIZE 0xF2  /* Set tile size */
#define GBI_G_SETTILE_DOLPHIN 0xD2  /* Dolphin tile setup (triggers upload) */
#define GBI_G_LOADTEXBLOCK_DOLPHIN 0xD0  /* Dolphin texture load (CI4/CI8) */
#define GBI_G_RDPPIPESYNC 0xE7  /* RDP pipe sync */
#define GBI_G_RDPLOADSYNC 0xE6  /* RDP load sync */

/* Segment 9 offsets for textures (matching texture_registry.c) */
#define GRASS_TEX_SEG9_OFFSET   0x0000  /* 32x32 CI4 */
#define EARTH_TEX_SEG9_OFFSET   0x1000  /* 64x64 CI4 */
#define CLIFF_TEX_SEG9_OFFSET   0x2000  /* 64x64 CI4 */
#define BUSH_A_TEX_SEG9_OFFSET  0x3000  /* 64x64 CI4 */
#define BUSH_B_TEX_SEG9_OFFSET  0x4000  /* 64x32 CI4 */

/* Legacy addresses (not used anymore) */
#define GRASS_TEX_ADDR  0x80200000
#define EARTH_TEX_ADDR  0x80200100
#define EARTH_PAL_ADDR  0x80100000

/* Helper to emit a G_VTX command
 * G_VTX format (F3DEX2):
 *   w0 = opcode(8) | n(8 at bits 12-19) | vn(7 at bits 1-7)
 *   w1 = vertex_addr
 * vn = v0 + n (the end index in vertex buffer)
 */
static Gfx* emit_vtx(Gfx* dl, int n, int v0, Vtx* vtx) {
    int vn = v0 + n;
    dl->words.w0 = _SHIFTL(GBI_G_VTX, 24, 8) | _SHIFTL(n, 12, 8) | _SHIFTL(vn, 1, 7);
    dl->words.w1 = (u32)(uintptr_t)vtx;
    return dl + 1;
}

/* Helper to emit a G_TRI1 command
 * G_TRI1 format: w0 = 0xBF000000, w1 = (v0*2 << 16) | (v1*2 << 8) | (v2*2) */
static Gfx* emit_tri(Gfx* dl, int v0, int v1, int v2) {
    dl->words.w0 = _SHIFTL(GBI_G_TRI1, 24, 8);
    dl->words.w1 = _SHIFTL(v0*2, 16, 8) | _SHIFTL(v1*2, 8, 8) | _SHIFTL(v2*2, 0, 8);
    return dl + 1;
}

/* Helper to emit G_SETTIMG command
 * Format: w0 = opcode | fmt | siz | width-1
 *         w1 = texture address
 * For CI4: fmt=2 (G_IM_FMT_CI), siz=0 (G_IM_SIZ_4b)
 */
static Gfx* emit_settimg(Gfx* dl, int fmt, int siz, int width, u32 addr) {
    dl->words.w0 = _SHIFTL(GBI_G_SETTIMG, 24, 8) | _SHIFTL(fmt, 21, 3) |
                   _SHIFTL(siz, 19, 2) | (width - 1);
    dl->words.w1 = addr;
    return dl + 1;
}

/* Helper to emit G_SETTILE command
 * Format: w0 = opcode | fmt | siz | line | tmem
 *         w1 = tile | pal | flags
 */
static Gfx* emit_settile(Gfx* dl, int fmt, int siz, int line, int tmem,
                         int tile, int pal) {
    dl->words.w0 = _SHIFTL(GBI_G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) |
                   _SHIFTL(siz, 19, 2) | _SHIFTL(line, 9, 9) | (tmem & 0x1FF);
    dl->words.w1 = _SHIFTL(tile, 24, 3) | _SHIFTL(pal, 20, 4);
    return dl + 1;
}

/* Helper to emit G_LOADBLOCK command
 * Format: w0 = opcode | sl | tl
 *         w1 = tile | sh | dxt
 */
static Gfx* emit_loadblock(Gfx* dl, int tile, int num_texels) {
    dl->words.w0 = _SHIFTL(GBI_G_LOADBLOCK, 24, 8);  /* sl=0, tl=0 */
    dl->words.w1 = _SHIFTL(tile, 24, 3) | _SHIFTL(num_texels - 1, 12, 12);
    return dl + 1;
}

/* Helper to emit G_SETTILESIZE command
 * Format: w0 = opcode | sl | tl
 *         w1 = tile | sh | th
 * Coordinates in 10.2 fixed point
 */
static Gfx* emit_settilesize(Gfx* dl, int tile, int width, int height) {
    int sh = (width - 1) << 2;   /* 10.2 fixed point */
    int th = (height - 1) << 2;
    dl->words.w0 = _SHIFTL(GBI_G_SETTILESIZE, 24, 8);  /* sl=0, tl=0 */
    dl->words.w1 = _SHIFTL(tile, 24, 3) | _SHIFTL(sh, 12, 12) | (th & 0xFFF);
    return dl + 1;
}

/* Helper to emit G_TEXTURE command (enable texturing)
 * Format: w0 = opcode | level | tile | on
 *         w1 = scale_s | scale_t
 */
static Gfx* emit_texture_on(Gfx* dl, int tile) {
    /* G_ON = 1 at bit 1 */
    dl->words.w0 = _SHIFTL(GBI_G_TEXTURE, 24, 8) | _SHIFTL(0, 11, 3) |
                   _SHIFTL(tile, 8, 3) | _SHIFTL(1, 1, 7);  /* on_bits=1 */
    /* Scale: 0xFFFF for 1:1 mapping */
    dl->words.w1 = 0xFFFFFFFF;  /* scale_s=0xFFFF, scale_t=0xFFFF */
    return dl + 1;
}

/* Helper to emit G_TEXTURE command (disable texturing) */
static Gfx* emit_texture_off(Gfx* dl) {
    dl->words.w0 = _SHIFTL(GBI_G_TEXTURE, 24, 8);  /* on_bits=0 */
    dl->words.w1 = 0;
    return dl + 1;
}

/* Helper to emit RDP pipe sync */
static Gfx* emit_pipesync(Gfx* dl) {
    dl->words.w0 = _SHIFTL(GBI_G_RDPPIPESYNC, 24, 8);
    dl->words.w1 = 0;
    return dl + 1;
}

/* Helper to emit G_SETTILE_DOLPHIN (triggers texture upload)
 * Format: w0 = opcode | d_fmt | tile | tlut | wrap_s | wrap_t
 *         w1 = 0 (unused)
 */
static Gfx* emit_settile_dolphin(Gfx* dl, int tile) {
    /* d_fmt=2 (CI), tile=0, tlut=0, wrap_s=0 (repeat), wrap_t=0 (repeat) */
    dl->words.w0 = _SHIFTL(GBI_G_SETTILE_DOLPHIN, 24, 8) |
                   _SHIFTL(2, 20, 4) |   /* d_fmt = 2 for CI4 */
                   _SHIFTL(tile, 16, 3) |
                   _SHIFTL(0, 12, 4) |   /* tlut_name */
                   _SHIFTL(0, 10, 2) |   /* wrap_s */
                   _SHIFTL(0, 8, 2);     /* wrap_t */
    dl->words.w1 = 0;
    return dl + 1;
}

/* Helper to emit G_LOADTEXBLOCK_DOLPHIN - loads CI4 texture from segment 9
 * Format: w0 = opcode | fmt | siz | (width-1) | (height-1)
 *         w1 = segment 9 offset (0x09XXYYYY where XXYYYY is offset)
 * For CI4: fmt=2 (G_IM_FMT_CI), siz=0 (G_IM_SIZ_4b)
 */
static Gfx* emit_loadtexblock_dolphin(Gfx* dl, int width, int height, u32 seg9_offset) {
    /* Format: (op << 24) | (fmt << 21) | (siz << 19) | ((width-1) << 8) | (height-1) */
    dl->words.w0 = _SHIFTL(GBI_G_LOADTEXBLOCK_DOLPHIN, 24, 8) |
                   _SHIFTL(2, 21, 3) |    /* fmt = 2 (CI) */
                   _SHIFTL(0, 19, 2) |    /* siz = 0 (4-bit) */
                   _SHIFTL(width - 1, 8, 8) |
                   (height - 1);
    /* Segment 9 address: 0x09000000 + offset */
    dl->words.w1 = 0x09000000 | seg9_offset;
    return dl + 1;
}

/**
 * Build a display list that renders the terrain as vertex-colored geometry.
 * Uses the exact triangle connectivity from grd_s_c1_1.c with proper vertex batches.
 *
 * Original structure:
 * - Batch 1: vertices 201-229 (29 verts), 16 triangles (Bush A texture)
 * - Batch 2: vertices 0-31 (32 verts), 18 triangles (Grass texture)
 * - Batch 3: vertices 31-62 (32 verts), 18 triangles (Grass texture)
 * - Batch 4: vertices 63-88 (26 verts), 13 triangles (Grass texture)
 * - Batch 5: vertices 89-119 (31 verts), 16 triangles (Earth texture)
 * - Batch 6: vertices 120-151 (32 verts), 33 triangles (Cliff texture)
 * - Batch 7: vertices 152-171 (20 verts), 12 triangles (Cliff texture)
 * - Batch 8: vertices 172-200 (29 verts), 16 triangles (Bush B texture)
 * Total: 142 triangles
 */
static void build_display_list(void) {
    Gfx* dl = real_terrain_dl;
    int tri_count = 0;

    /* Clear lighting so vertex colors show directly */
    dl->words.w0 = _SHIFTL(GBI_G_GEOMETRYMODE, 24, 8) | (~(u32)G_LIGHTING & 0x00FFFFFF);
    dl->words.w1 = 0;
    dl++;

    /* Set geometry mode: Z-buffer, shade, cull back, smooth shading */
    dl->words.w0 = _SHIFTL(GBI_G_GEOMETRYMODE, 24, 8) | 0x00FFFFFF;
    dl->words.w1 = G_ZBUFFER | G_SHADE | G_CULL_BACK | G_SHADING_SMOOTH;
    dl++;

    /* ========== Enable grass texture for bush/grass batches ========== */
    dl = emit_pipesync(dl);
    dl = emit_loadtexblock_dolphin(dl, 32, 32, GRASS_TEX_SEG9_OFFSET);
    dl = emit_texture_on(dl, 0);

    /* ========== Batch 1: Bush A (vertices 201-229, 16 triangles) ========== */
    dl = emit_vtx(dl, 29, 0, &real_terrain_vtx[201]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 1, 3, 2); dl = emit_tri(dl, 4, 5, 6);
    dl = emit_tri(dl, 7, 4, 8); dl = emit_tri(dl, 9, 7, 10); dl = emit_tri(dl, 11, 9, 12);
    dl = emit_tri(dl, 13, 11, 14); dl = emit_tri(dl, 0, 15, 16); dl = emit_tri(dl, 17, 18, 19);
    dl = emit_tri(dl, 18, 3, 20); dl = emit_tri(dl, 3, 1, 20); dl = emit_tri(dl, 21, 22, 23);
    dl = emit_tri(dl, 24, 21, 23); dl = emit_tri(dl, 15, 24, 25); dl = emit_tri(dl, 21, 26, 22);
    dl = emit_tri(dl, 26, 27, 28);
    tri_count += 16;

    /* Grass texture already enabled from Bush A batch */

    /* ========== Batch 2: Grass part 1 (vertices 0-31, 18 triangles) ========== */
    dl = emit_vtx(dl, 32, 0, &real_terrain_vtx[0]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 3, 4, 5); dl = emit_tri(dl, 3, 6, 4);
    dl = emit_tri(dl, 7, 5, 8); dl = emit_tri(dl, 7, 9, 5); dl = emit_tri(dl, 5, 4, 8);
    dl = emit_tri(dl, 10, 11, 12); dl = emit_tri(dl, 11, 13, 12); dl = emit_tri(dl, 14, 7, 15);
    dl = emit_tri(dl, 7, 16, 15); dl = emit_tri(dl, 16, 17, 15); dl = emit_tri(dl, 18, 19, 20);
    dl = emit_tri(dl, 19, 21, 20); dl = emit_tri(dl, 22, 23, 24); dl = emit_tri(dl, 23, 25, 24);
    dl = emit_tri(dl, 26, 10, 12); dl = emit_tri(dl, 27, 28, 29); dl = emit_tri(dl, 28, 30, 29);
    tri_count += 18;

    /* ========== Batch 3: Grass part 2 (vertices 31-62, 18 triangles) ========== */
    dl = emit_vtx(dl, 32, 0, &real_terrain_vtx[31]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 1, 3, 2); dl = emit_tri(dl, 4, 5, 6);
    dl = emit_tri(dl, 7, 2, 3); dl = emit_tri(dl, 8, 9, 10); dl = emit_tri(dl, 9, 1, 10);
    dl = emit_tri(dl, 1, 0, 10); dl = emit_tri(dl, 11, 12, 13); dl = emit_tri(dl, 12, 14, 13);
    dl = emit_tri(dl, 15, 16, 4); dl = emit_tri(dl, 16, 5, 4); dl = emit_tri(dl, 17, 18, 19);
    dl = emit_tri(dl, 20, 21, 22); dl = emit_tri(dl, 21, 23, 22); dl = emit_tri(dl, 24, 17, 25);
    dl = emit_tri(dl, 26, 27, 28); dl = emit_tri(dl, 27, 29, 28); dl = emit_tri(dl, 30, 31, 29);
    tri_count += 18;

    /* ========== Batch 4: Grass part 3 (vertices 63-88, 13 triangles) ========== */
    dl = emit_vtx(dl, 26, 0, &real_terrain_vtx[63]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 0, 3, 1); dl = emit_tri(dl, 3, 4, 1);
    dl = emit_tri(dl, 5, 6, 7); dl = emit_tri(dl, 8, 9, 10); dl = emit_tri(dl, 8, 11, 9);
    dl = emit_tri(dl, 12, 13, 4); dl = emit_tri(dl, 14, 15, 16); dl = emit_tri(dl, 17, 18, 19);
    dl = emit_tri(dl, 20, 21, 8); dl = emit_tri(dl, 22, 23, 24); dl = emit_tri(dl, 23, 20, 24);
    dl = emit_tri(dl, 25, 23, 22);
    tri_count += 13;

    /* ========== Switch to earth texture for earth/cliff batches ========== */
    dl = emit_pipesync(dl);
    dl = emit_loadtexblock_dolphin(dl, 64, 64, EARTH_TEX_SEG9_OFFSET);

    /* ========== Batch 5: Earth (vertices 89-119, 16 triangles) ========== */
    dl = emit_vtx(dl, 31, 0, &real_terrain_vtx[89]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 3, 4, 1); dl = emit_tri(dl, 5, 6, 7);
    dl = emit_tri(dl, 8, 9, 10); dl = emit_tri(dl, 11, 10, 6); dl = emit_tri(dl, 12, 9, 8);
    dl = emit_tri(dl, 13, 2, 14); dl = emit_tri(dl, 15, 14, 16); dl = emit_tri(dl, 17, 16, 18);
    dl = emit_tri(dl, 3, 1, 0); dl = emit_tri(dl, 19, 7, 4); dl = emit_tri(dl, 20, 21, 22);
    dl = emit_tri(dl, 23, 20, 24); dl = emit_tri(dl, 21, 25, 26); dl = emit_tri(dl, 25, 27, 28);
    dl = emit_tri(dl, 29, 23, 30);
    tri_count += 16;

    /* ========== Batch 6: Cliff part 1 (vertices 120-151, 33 triangles) ========== */
    dl = emit_vtx(dl, 32, 0, &real_terrain_vtx[120]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 3, 4, 1); dl = emit_tri(dl, 3, 5, 4);
    dl = emit_tri(dl, 5, 6, 4); dl = emit_tri(dl, 1, 4, 2); dl = emit_tri(dl, 4, 6, 2);
    dl = emit_tri(dl, 6, 7, 2); dl = emit_tri(dl, 8, 9, 10); dl = emit_tri(dl, 8, 11, 9);
    dl = emit_tri(dl, 9, 12, 10); dl = emit_tri(dl, 13, 12, 14); dl = emit_tri(dl, 12, 9, 14);
    dl = emit_tri(dl, 9, 11, 14); dl = emit_tri(dl, 11, 15, 14); dl = emit_tri(dl, 12, 13, 16);
    dl = emit_tri(dl, 17, 18, 19); dl = emit_tri(dl, 17, 20, 18); dl = emit_tri(dl, 20, 21, 18);
    dl = emit_tri(dl, 22, 0, 23); dl = emit_tri(dl, 22, 1, 0); dl = emit_tri(dl, 24, 12, 25);
    dl = emit_tri(dl, 24, 10, 12); dl = emit_tri(dl, 8, 26, 11); dl = emit_tri(dl, 26, 27, 11);
    dl = emit_tri(dl, 26, 22, 27); dl = emit_tri(dl, 26, 28, 22); dl = emit_tri(dl, 28, 1, 22);
    dl = emit_tri(dl, 28, 3, 1); dl = emit_tri(dl, 5, 29, 6); dl = emit_tri(dl, 5, 30, 29);
    dl = emit_tri(dl, 30, 17, 29); dl = emit_tri(dl, 30, 20, 17); dl = emit_tri(dl, 30, 31, 20);
    tri_count += 33;

    /* ========== Batch 7: Cliff part 2 (vertices 152-171, 12 triangles) ========== */
    dl = emit_vtx(dl, 20, 0, &real_terrain_vtx[152]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 1, 3, 2); dl = emit_tri(dl, 4, 5, 6);
    dl = emit_tri(dl, 7, 8, 9); dl = emit_tri(dl, 7, 10, 8); dl = emit_tri(dl, 10, 11, 8);
    dl = emit_tri(dl, 11, 12, 8); dl = emit_tri(dl, 13, 14, 15); dl = emit_tri(dl, 14, 16, 15);
    dl = emit_tri(dl, 14, 17, 16); dl = emit_tri(dl, 2, 3, 18); dl = emit_tri(dl, 3, 19, 18);
    tri_count += 12;

    /* ========== Switch to bush B texture ========== */
    dl = emit_pipesync(dl);
    dl = emit_loadtexblock_dolphin(dl, 64, 32, BUSH_B_TEX_SEG9_OFFSET);

    /* ========== Batch 8: Bush B (vertices 172-200, 16 triangles) ========== */
    dl = emit_vtx(dl, 29, 0, &real_terrain_vtx[172]);
    dl = emit_tri(dl, 0, 1, 2); dl = emit_tri(dl, 1, 3, 2); dl = emit_tri(dl, 4, 5, 6);
    dl = emit_tri(dl, 7, 8, 9); dl = emit_tri(dl, 8, 10, 11); dl = emit_tri(dl, 5, 12, 13);
    dl = emit_tri(dl, 12, 7, 14); dl = emit_tri(dl, 15, 16, 17); dl = emit_tri(dl, 18, 19, 20);
    dl = emit_tri(dl, 21, 18, 22); dl = emit_tri(dl, 16, 3, 23); dl = emit_tri(dl, 3, 1, 23);
    dl = emit_tri(dl, 21, 22, 24); dl = emit_tri(dl, 25, 21, 24); dl = emit_tri(dl, 26, 25, 27);
    dl = emit_tri(dl, 0, 26, 28);
    tri_count += 16;

    /* End display list */
    dl->words.w0 = _SHIFTL(GBI_G_ENDDL, 24, 8);
    dl->words.w1 = 0;

    printf("[REAL_TERRAIN] Display list built: %d triangles, %ld commands\n",
           tri_count, (long)(dl - real_terrain_dl + 1));
}

/**
 * Initialize the real terrain display list
 */
void real_terrain_init(void) {
    if (real_terrain_initialized) return;

    printf("[REAL_TERRAIN] Initializing grd_s_c1_1 terrain (%d vertices)\n",
           REAL_TERRAIN_VTX_COUNT);

    /* Allocate vertices */
    real_terrain_vtx = (Vtx*)malloc(REAL_TERRAIN_VTX_COUNT * sizeof(Vtx));
    if (!real_terrain_vtx) {
        printf("[REAL_TERRAIN] ERROR: Failed to allocate vertices!\n");
        return;
    }

    /* Allocate display list */
    real_terrain_dl = (Gfx*)malloc(REAL_TERRAIN_GFX_COUNT * sizeof(Gfx));
    if (!real_terrain_dl) {
        printf("[REAL_TERRAIN] ERROR: Failed to allocate display list!\n");
        free(real_terrain_vtx);
        real_terrain_vtx = NULL;
        return;
    }

    /* Clear display list to avoid garbage */
    memset(real_terrain_dl, 0, REAL_TERRAIN_GFX_COUNT * sizeof(Gfx));

    /* Convert vertex data */
    convert_vertices();

    /* Build display list */
    build_display_list();

    real_terrain_initialized = true;
    printf("[REAL_TERRAIN] Terrain initialized: vtx=%p dl=%p\n",
           (void*)real_terrain_vtx, (void*)real_terrain_dl);
}

/**
 * Get the real terrain display list
 */
Gfx* real_terrain_get_dl(void) {
    if (!real_terrain_initialized) {
        real_terrain_init();
    }
    return real_terrain_dl;
}

/**
 * Get the truncated address of the display list for pointer lookup
 */
u32 real_terrain_get_dl_truncated(void) {
    if (!real_terrain_initialized) {
        real_terrain_init();
    }
    return (u32)(uintptr_t)real_terrain_dl;
}

/**
 * Lookup function for GBI interpreter - recover full pointer from truncated address
 */
static int real_terrain_fullptr_debug = 0;

void* real_terrain_get_fullptr(u32 truncated_addr) {
    if (!real_terrain_initialized) return NULL;

    /* Check if this is our display list */
    u32 dl_trunc = (u32)(uintptr_t)real_terrain_dl;
    if (truncated_addr == dl_trunc) {
        if (real_terrain_fullptr_debug < 3) {
            printf("[REAL_TERRAIN_FULLPTR] DL match: 0x%08X -> %p\n", truncated_addr, (void*)real_terrain_dl);
            fflush(stdout);
            real_terrain_fullptr_debug++;
        }
        return real_terrain_dl;
    }

    /* Check vertex array ranges */
    u32 vtx_base = (u32)(uintptr_t)real_terrain_vtx;
    u32 vtx_end = vtx_base + (REAL_TERRAIN_VTX_COUNT * sizeof(Vtx));

    if (truncated_addr >= vtx_base && truncated_addr < vtx_end) {
        /* Calculate offset and return full pointer */
        u32 offset = truncated_addr - vtx_base;
        void* result = (void*)((u8*)real_terrain_vtx + offset);
        if (real_terrain_fullptr_debug < 10) {
            printf("[REAL_TERRAIN_FULLPTR] VTX range: 0x%08X (base=0x%08X end=0x%08X) -> %p\n",
                   truncated_addr, vtx_base, vtx_end, result);
            fflush(stdout);
            real_terrain_fullptr_debug++;
        }
        return result;
    }

    return NULL;
}
