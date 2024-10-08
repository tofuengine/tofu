/*
 *                 ___________________  _______________ ___
 *                 \__    ___/\_____  \ \_   _____/    |   \
 *                   |    |    /   |   \ |    __) |    |   /
 *                   |    |   /    |    \|     \  |    |  /
 *                   |____|   \_______  /\___  /  |______/
 *                                    \/     \/
 *         ___________ _______    ________.___ _______  ___________
 *         \_   _____/ \      \  /  _____/|   |\      \ \_   _____/
 *          |    __)_  /   |   \/   \  ___|   |/   |   \ |    __)_
 *          |        \/    |    \    \_\  \   /    |    \|        \
 *         /_______  /\____|__  /\______  /___\____|__  /_______  /
 *                 \/         \/        \/            \/        \
 *
 * MIT License
 * 
 * Copyright (c) 2019-2024 Marco Lizza
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "sincos.h"

// 128 steps per quadrant seems more than enough.
#include <math.h>

#define _LUT_LENGTH (SINCOS_PERIOD + (SINCOS_PERIOD / 4))

static const float _lut[_LUT_LENGTH] = {
    0.000000000f, /* [0] */
    0.012271538f, /* [1] */
    0.024541229f, /* [2] */
    0.036807224f, /* [3] */
    0.049067676f, /* [4] */
    0.061320741f, /* [5] */
    0.073564567f, /* [6] */
    0.085797317f, /* [7] */
    0.098017141f, /* [8] */
    0.110222206f, /* [9] */
    0.122410677f, /* [10] */
    0.134580716f, /* [11] */
    0.146730468f, /* [12] */
    0.158858150f, /* [13] */
    0.170961902f, /* [14] */
    0.183039889f, /* [15] */
    0.195090324f, /* [16] */
    0.207111388f, /* [17] */
    0.219101235f, /* [18] */
    0.231058121f, /* [19] */
    0.242980197f, /* [20] */
    0.254865676f, /* [21] */
    0.266712785f, /* [22] */
    0.278519690f, /* [23] */
    0.290284663f, /* [24] */
    0.302005947f, /* [25] */
    0.313681751f, /* [26] */
    0.325310320f, /* [27] */
    0.336889863f, /* [28] */
    0.348418683f, /* [29] */
    0.359895051f, /* [30] */
    0.371317208f, /* [31] */
    0.382683456f, /* [32] */
    0.393992066f, /* [33] */
    0.405241340f, /* [34] */
    0.416429579f, /* [35] */
    0.427555084f, /* [36] */
    0.438616246f, /* [37] */
    0.449611336f, /* [38] */
    0.460538715f, /* [39] */
    0.471396744f, /* [40] */
    0.482183754f, /* [41] */
    0.492898226f, /* [42] */
    0.503538370f, /* [43] */
    0.514102757f, /* [44] */
    0.524589717f, /* [45] */
    0.534997642f, /* [46] */
    0.545324981f, /* [47] */
    0.555570245f, /* [48] */
    0.565731823f, /* [49] */
    0.575808227f, /* [50] */
    0.585797906f, /* [51] */
    0.595699310f, /* [52] */
    0.605511010f, /* [53] */
    0.615231633f, /* [54] */
    0.624859512f, /* [55] */
    0.634393334f, /* [56] */
    0.643831551f, /* [57] */
    0.653172851f, /* [58] */
    0.662415802f, /* [59] */
    0.671558976f, /* [60] */
    0.680601001f, /* [61] */
    0.689540565f, /* [62] */
    0.698376298f, /* [63] */
    0.707106769f, /* [64] */
    0.715730846f, /* [65] */
    0.724247098f, /* [66] */
    0.732654274f, /* [67] */
    0.740951180f, /* [68] */
    0.749136388f, /* [69] */
    0.757208884f, /* [70] */
    0.765167296f, /* [71] */
    0.773010433f, /* [72] */
    0.780737281f, /* [73] */
    0.788346410f, /* [74] */
    0.795836926f, /* [75] */
    0.803207517f, /* [76] */
    0.810457170f, /* [77] */
    0.817584813f, /* [78] */
    0.824589312f, /* [79] */
    0.831469655f, /* [80] */
    0.838224709f, /* [81] */
    0.844853580f, /* [82] */
    0.851355195f, /* [83] */
    0.857728660f, /* [84] */
    0.863972843f, /* [85] */
    0.870086968f, /* [86] */
    0.876070142f, /* [87] */
    0.881921291f, /* [88] */
    0.887639642f, /* [89] */
    0.893224299f, /* [90] */
    0.898674488f, /* [91] */
    0.903989315f, /* [92] */
    0.909168005f, /* [93] */
    0.914209783f, /* [94] */
    0.919113874f, /* [95] */
    0.923879504f, /* [96] */
    0.928506076f, /* [97] */
    0.932992816f, /* [98] */
    0.937339008f, /* [99] */
    0.941544056f, /* [100] */
    0.945607364f, /* [101] */
    0.949528217f, /* [102] */
    0.953306019f, /* [103] */
    0.956940353f, /* [104] */
    0.960430562f, /* [105] */
    0.963776052f, /* [106] */
    0.966976464f, /* [107] */
    0.970031261f, /* [108] */
    0.972939968f, /* [109] */
    0.975702107f, /* [110] */
    0.978317380f, /* [111] */
    0.980785310f, /* [112] */
    0.983105481f, /* [113] */
    0.985277653f, /* [114] */
    0.987301409f, /* [115] */
    0.989176512f, /* [116] */
    0.990902662f, /* [117] */
    0.992479563f, /* [118] */
    0.993906975f, /* [119] */
    0.995184720f, /* [120] */
    0.996312618f, /* [121] */
    0.997290432f, /* [122] */
    0.998118103f, /* [123] */
    0.998795450f, /* [124] */
    0.999322414f, /* [125] */
    0.999698818f, /* [126] */
    0.999924719f, /* [127] */
    1.000000000f, /* [128] */
    0.999924719f, /* [129] */
    0.999698818f, /* [130] */
    0.999322355f, /* [131] */
    0.998795450f, /* [132] */
    0.998118103f, /* [133] */
    0.997290432f, /* [134] */
    0.996312618f, /* [135] */
    0.995184720f, /* [136] */
    0.993906975f, /* [137] */
    0.992479503f, /* [138] */
    0.990902603f, /* [139] */
    0.989176512f, /* [140] */
    0.987301409f, /* [141] */
    0.985277653f, /* [142] */
    0.983105481f, /* [143] */
    0.980785251f, /* [144] */
    0.978317380f, /* [145] */
    0.975702107f, /* [146] */
    0.972939909f, /* [147] */
    0.970031261f, /* [148] */
    0.966976464f, /* [149] */
    0.963776052f, /* [150] */
    0.960430503f, /* [151] */
    0.956940293f, /* [152] */
    0.953306019f, /* [153] */
    0.949528158f, /* [154] */
    0.945607305f, /* [155] */
    0.941544056f, /* [156] */
    0.937338948f, /* [157] */
    0.932992816f, /* [158] */
    0.928506076f, /* [159] */
    0.923879504f, /* [160] */
    0.919113874f, /* [161] */
    0.914209723f, /* [162] */
    0.909168005f, /* [163] */
    0.903989315f, /* [164] */
    0.898674428f, /* [165] */
    0.893224299f, /* [166] */
    0.887639582f, /* [167] */
    0.881921232f, /* [168] */
    0.876070023f, /* [169] */
    0.870087028f, /* [170] */
    0.863972843f, /* [171] */
    0.857728601f, /* [172] */
    0.851355135f, /* [173] */
    0.844853520f, /* [174] */
    0.838224649f, /* [175] */
    0.831469536f, /* [176] */
    0.824589312f, /* [177] */
    0.817584813f, /* [178] */
    0.810457170f, /* [179] */
    0.803207517f, /* [180] */
    0.795836866f, /* [181] */
    0.788346350f, /* [182] */
    0.780737102f, /* [183] */
    0.773010492f, /* [184] */
    0.765167236f, /* [185] */
    0.757208824f, /* [186] */
    0.749136329f, /* [187] */
    0.740951061f, /* [188] */
    0.732654154f, /* [189] */
    0.724246979f, /* [190] */
    0.715730846f, /* [191] */
    0.707106769f, /* [192] */
    0.698376238f, /* [193] */
    0.689540505f, /* [194] */
    0.680600941f, /* [195] */
    0.671558857f, /* [196] */
    0.662415624f, /* [197] */
    0.653172851f, /* [198] */
    0.643831551f, /* [199] */
    0.634393275f, /* [200] */
    0.624859452f, /* [201] */
    0.615231514f, /* [202] */
    0.605510950f, /* [203] */
    0.595699131f, /* [204] */
    0.585797846f, /* [205] */
    0.575808167f, /* [206] */
    0.565731764f, /* [207] */
    0.555570185f, /* [208] */
    0.545324862f, /* [209] */
    0.534997463f, /* [210] */
    0.524589539f, /* [211] */
    0.514102757f, /* [212] */
    0.503538370f, /* [213] */
    0.492898136f, /* [214] */
    0.482183695f, /* [215] */
    0.471396625f, /* [216] */
    0.460538566f, /* [217] */
    0.449611366f, /* [218] */
    0.438616246f, /* [219] */
    0.427555054f, /* [220] */
    0.416429490f, /* [221] */
    0.405241221f, /* [222] */
    0.393991917f, /* [223] */
    0.382683277f, /* [224] */
    0.371317238f, /* [225] */
    0.359895051f, /* [226] */
    0.348418653f, /* [227] */
    0.336889803f, /* [228] */
    0.325310200f, /* [229] */
    0.313681602f, /* [230] */
    0.302005798f, /* [231] */
    0.290284723f, /* [232] */
    0.278519690f, /* [233] */
    0.266712725f, /* [234] */
    0.254865587f, /* [235] */
    0.242980078f, /* [236] */
    0.231057972f, /* [237] */
    0.219101071f, /* [238] */
    0.207111403f, /* [239] */
    0.195090309f, /* [240] */
    0.183039844f, /* [241] */
    0.170961812f, /* [242] */
    0.158858031f, /* [243] */
    0.146730334f, /* [244] */
    0.134580523f, /* [245] */
    0.122410700f, /* [246] */
    0.110222198f, /* [247] */
    0.098017097f, /* [248] */
    0.085797228f, /* [249] */
    0.073564447f, /* [250] */
    0.061320584f, /* [251] */
    0.049067486f, /* [252] */
    0.036807239f, /* [253] */
    0.024541210f, /* [254] */
    0.012271485f, /* [255] */
    0.000000000f, /* [256] */
    -0.012271660f, /* [257] */
    -0.024541385f, /* [258] */
    -0.036807414f, /* [259] */
    -0.049067661f, /* [260] */
    -0.061320759f, /* [261] */
    -0.073564619f, /* [262] */
    -0.085797407f, /* [263] */
    -0.098017268f, /* [264] */
    -0.110222369f, /* [265] */
    -0.122410871f, /* [266] */
    -0.134580702f, /* [267] */
    -0.146730497f, /* [268] */
    -0.158858210f, /* [269] */
    -0.170961991f, /* [270] */
    -0.183040023f, /* [271] */
    -0.195090488f, /* [272] */
    -0.207111567f, /* [273] */
    -0.219101235f, /* [274] */
    -0.231058136f, /* [275] */
    -0.242980242f, /* [276] */
    -0.254865766f, /* [277] */
    -0.266712904f, /* [278] */
    -0.278519869f, /* [279] */
    -0.290284872f, /* [280] */
    -0.302005947f, /* [281] */
    -0.313681781f, /* [282] */
    -0.325310349f, /* [283] */
    -0.336889952f, /* [284] */
    -0.348418802f, /* [285] */
    -0.359895200f, /* [286] */
    -0.371317387f, /* [287] */
    -0.382683426f, /* [288] */
    -0.393992066f, /* [289] */
    -0.405241400f, /* [290] */
    -0.416429669f, /* [291] */
    -0.427555233f, /* [292] */
    -0.438616395f, /* [293] */
    -0.449611515f, /* [294] */
    -0.460538715f, /* [295] */
    -0.471396774f, /* [296] */
    -0.482183844f, /* [297] */
    -0.492898285f, /* [298] */
    -0.503538489f, /* [299] */
    -0.514102876f, /* [300] */
    -0.524589658f, /* [301] */
    -0.534997642f, /* [302] */
    -0.545325041f, /* [303] */
    -0.555570304f, /* [304] */
    -0.565731883f, /* [305] */
    -0.575808346f, /* [306] */
    -0.585798025f, /* [307] */
    -0.595699310f, /* [308] */
    -0.605511069f, /* [309] */
    -0.615231633f, /* [310] */
    -0.624859571f, /* [311] */
    -0.634393394f, /* [312] */
    -0.643831670f, /* [313] */
    -0.653172970f, /* [314] */
    -0.662415802f, /* [315] */
    -0.671558976f, /* [316] */
    -0.680601060f, /* [317] */
    -0.689540625f, /* [318] */
    -0.698376358f, /* [319] */
    -0.707106888f, /* [320] */
    -0.715730965f, /* [321] */
    -0.724247098f, /* [322] */
    -0.732654274f, /* [323] */
    -0.740951180f, /* [324] */
    -0.749136448f, /* [325] */
    -0.757208765f, /* [326] */
    -0.765167236f, /* [327] */
    -0.773010433f, /* [328] */
    -0.780737221f, /* [329] */
    -0.788346469f, /* [330] */
    -0.795836926f, /* [331] */
    -0.803207576f, /* [332] */
    -0.810457289f, /* [333] */
    -0.817584932f, /* [334] */
    -0.824589431f, /* [335] */
    -0.831469774f, /* [336] */
    -0.838224888f, /* [337] */
    -0.844853759f, /* [338] */
    -0.851355374f, /* [339] */
    -0.857728541f, /* [340] */
    -0.863972843f, /* [341] */
    -0.870086968f, /* [342] */
    -0.876070082f, /* [343] */
    -0.881921291f, /* [344] */
    -0.887639642f, /* [345] */
    -0.893224359f, /* [346] */
    -0.898674548f, /* [347] */
    -0.903989375f, /* [348] */
    -0.909168065f, /* [349] */
    -0.914209843f, /* [350] */
    -0.919113994f, /* [351] */
    -0.923879683f, /* [352] */
    -0.928506017f, /* [353] */
    -0.932992756f, /* [354] */
    -0.937339008f, /* [355] */
    -0.941544056f, /* [356] */
    -0.945607305f, /* [357] */
    -0.949528217f, /* [358] */
    -0.953306079f, /* [359] */
    -0.956940353f, /* [360] */
    -0.960430562f, /* [361] */
    -0.963776112f, /* [362] */
    -0.966976523f, /* [363] */
    -0.970031321f, /* [364] */
    -0.972940028f, /* [365] */
    -0.975702226f, /* [366] */
    -0.978317320f, /* [367] */
    -0.980785251f, /* [368] */
    -0.983105481f, /* [369] */
    -0.985277653f, /* [370] */
    -0.987301409f, /* [371] */
    -0.989176512f, /* [372] */
    -0.990902662f, /* [373] */
    -0.992479563f, /* [374] */
    -0.993906975f, /* [375] */
    -0.995184720f, /* [376] */
    -0.996312618f, /* [377] */
    -0.997290492f, /* [378] */
    -0.998118103f, /* [379] */
    -0.998795450f, /* [380] */
    -0.999322355f, /* [381] */
    -0.999698818f, /* [382] */
    -0.999924719f, /* [383] */
    -1.000000000f, /* [384] */
    -0.999924719f, /* [385] */
    -0.999698818f, /* [386] */
    -0.999322355f, /* [387] */
    -0.998795450f, /* [388] */
    -0.998118103f, /* [389] */
    -0.997290432f, /* [390] */
    -0.996312618f, /* [391] */
    -0.995184720f, /* [392] */
    -0.993906915f, /* [393] */
    -0.992479503f, /* [394] */
    -0.990902662f, /* [395] */
    -0.989176512f, /* [396] */
    -0.987301409f, /* [397] */
    -0.985277653f, /* [398] */
    -0.983105481f, /* [399] */
    -0.980785251f, /* [400] */
    -0.978317320f, /* [401] */
    -0.975702107f, /* [402] */
    -0.972939909f, /* [403] */
    -0.970031202f, /* [404] */
    -0.966976404f, /* [405] */
    -0.963775992f, /* [406] */
    -0.960430443f, /* [407] */
    -0.956940234f, /* [408] */
    -0.953306079f, /* [409] */
    -0.949528217f, /* [410] */
    -0.945607305f, /* [411] */
    -0.941544056f, /* [412] */
    -0.937339008f, /* [413] */
    -0.932992756f, /* [414] */
    -0.928506017f, /* [415] */
    -0.923879445f, /* [416] */
    -0.919113755f, /* [417] */
    -0.914209664f, /* [418] */
    -0.909167886f, /* [419] */
    -0.903989136f, /* [420] */
    -0.898674309f, /* [421] */
    -0.893224120f, /* [422] */
    -0.887639642f, /* [423] */
    -0.881921291f, /* [424] */
    -0.876070082f, /* [425] */
    -0.870086968f, /* [426] */
    -0.863972843f, /* [427] */
    -0.857728541f, /* [428] */
    -0.851355135f, /* [429] */
    -0.844853461f, /* [430] */
    -0.838224590f, /* [431] */
    -0.831469476f, /* [432] */
    -0.824589133f, /* [433] */
    -0.817584634f, /* [434] */
    -0.810456991f, /* [435] */
    -0.803207576f, /* [436] */
    -0.795836926f, /* [437] */
    -0.788346410f, /* [438] */
    -0.780737221f, /* [439] */
    -0.773010433f, /* [440] */
    -0.765167236f, /* [441] */
    -0.757208765f, /* [442] */
    -0.749136269f, /* [443] */
    -0.740951002f, /* [444] */
    -0.732654095f, /* [445] */
    -0.724246919f, /* [446] */
    -0.715730608f, /* [447] */
    -0.707106531f, /* [448] */
    -0.698376000f, /* [449] */
    -0.689540625f, /* [450] */
    -0.680601001f, /* [451] */
    -0.671558976f, /* [452] */
    -0.662415743f, /* [453] */
    -0.653172791f, /* [454] */
    -0.643831491f, /* [455] */
    -0.634393156f, /* [456] */
    -0.624859333f, /* [457] */
    -0.615231454f, /* [458] */
    -0.605510831f, /* [459] */
    -0.595699072f, /* [460] */
    -0.585797608f, /* [461] */
    -0.575807929f, /* [462] */
    -0.565731525f, /* [463] */
    -0.555570304f, /* [464] */
    -0.545325041f, /* [465] */
    -0.534997642f, /* [466] */
    -0.524589658f, /* [467] */
    -0.514102697f, /* [468] */
    -0.503538311f, /* [469] */
    -0.492898077f, /* [470] */
    -0.482183605f, /* [471] */
    -0.471396536f, /* [472] */
    -0.460538477f, /* [473] */
    -0.449611068f, /* [474] */
    -0.438615948f, /* [475] */
    -0.427554786f, /* [476] */
    -0.416429222f, /* [477] */
    -0.405241370f, /* [478] */
    -0.393992066f, /* [479] */
    -0.382683426f, /* [480] */
    -0.371317148f, /* [481] */
    -0.359894961f, /* [482] */
    -0.348418564f, /* [483] */
    -0.336889714f, /* [484] */
    -0.325310111f, /* [485] */
    -0.313681543f, /* [486] */
    -0.302005708f, /* [487] */
    -0.290284395f, /* [488] */
    -0.278519362f, /* [489] */
    -0.266712397f, /* [490] */
    -0.254865289f, /* [491] */
    -0.242980227f, /* [492] */
    -0.231058121f, /* [493] */
    -0.219101220f, /* [494] */
    -0.207111314f, /* [495] */
    -0.195090234f, /* [496] */
    -0.183039755f, /* [497] */
    -0.170961723f, /* [498] */
    -0.158857942f, /* [499] */
    -0.146730244f, /* [500] */
    -0.134580448f, /* [501] */
    -0.122410372f, /* [502] */
    -0.110221870f, /* [503] */
    -0.098016769f, /* [504] */
    -0.085796908f, /* [505] */
    -0.073564596f, /* [506] */
    -0.061320737f, /* [507] */
    -0.049067639f, /* [508] */
    -0.036807153f, /* [509] */
    -0.024541123f, /* [510] */
    -0.012271399f, /* [511] */
    0.000000000f, /* [512] */
    0.012271748f, /* [513] */
    0.024541473f, /* [514] */
    0.036807504f, /* [515] */
    0.049067989f, /* [516] */
    0.061321083f, /* [517] */
    0.073564947f, /* [518] */
    0.085797258f, /* [519] */
    0.098017119f, /* [520] */
    0.110222220f, /* [521] */
    0.122410722f, /* [522] */
    0.134580791f, /* [523] */
    0.146730587f, /* [524] */
    0.158858299f, /* [525] */
    0.170962065f, /* [526] */
    0.183040097f, /* [527] */
    0.195090577f, /* [528] */
    0.207111657f, /* [529] */
    0.219101563f, /* [530] */
    0.231058463f, /* [531] */
    0.242980555f, /* [532] */
    0.254865617f, /* [533] */
    0.266712755f, /* [534] */
    0.278519720f, /* [535] */
    0.290284723f, /* [536] */
    0.302006036f, /* [537] */
    0.313681871f, /* [538] */
    0.325310439f, /* [539] */
    0.336890042f, /* [540] */
    0.348418891f, /* [541] */
    0.359895289f, /* [542] */
    0.371317476f, /* [543] */
    0.382683754f, /* [544] */
    0.393992394f, /* [545] */
    0.405241698f, /* [546] */
    0.416429520f, /* [547] */
    0.427555084f, /* [548] */
    0.438616276f, /* [549] */
    0.449611396f, /* [550] */
    0.460538805f, /* [551] */
    0.471396863f, /* [552] */
    0.482183933f, /* [553] */
    0.492898375f, /* [554] */
    0.503538609f, /* [555] */
    0.514102995f, /* [556] */
    0.524589956f, /* [557] */
    0.534997940f, /* [558] */
    0.545325279f, /* [559] */
    0.555570602f, /* [560] */
    0.565731764f, /* [561] */
    0.575808227f, /* [562] */
    0.585797906f, /* [563] */
    0.595699370f, /* [564] */
    0.605511129f, /* [565] */
    0.615231693f, /* [566] */
    0.624859631f, /* [567] */
    0.634393454f, /* [568] */
    0.643831730f, /* [569] */
    0.653173089f, /* [570] */
    0.662416041f, /* [571] */
    0.671559215f, /* [572] */
    0.680601299f, /* [573] */
    0.689540863f, /* [574] */
    0.698376238f, /* [575] */
    0.707106769f, /* [576] */
    0.715730846f, /* [577] */
    0.724247158f, /* [578] */
    0.732654333f, /* [579] */
    0.740951240f, /* [580] */
    0.749136508f, /* [581] */
    0.757209003f, /* [582] */
    0.765167415f, /* [583] */
    0.773010612f, /* [584] */
    0.780737460f, /* [585] */
    0.788346648f, /* [586] */
    0.795837164f, /* [587] */
    0.803207815f, /* [588] */
    0.810457170f, /* [589] */
    0.817584813f, /* [590] */
    0.824589312f, /* [591] */
    0.831469655f, /* [592] */
    0.838224769f, /* [593] */
    0.844853640f, /* [594] */
    0.851355314f, /* [595] */
    0.857728720f, /* [596] */
    0.863973022f, /* [597] */
    0.870087147f, /* [598] */
    0.876070261f, /* [599] */
    0.881921470f, /* [600] */
    0.887639821f, /* [601] */
    0.893224299f, /* [602] */
    0.898674488f, /* [603] */
    0.903989315f, /* [604] */
    0.909168005f, /* [605] */
    0.914209783f, /* [606] */
    0.919113934f, /* [607] */
    0.923879623f, /* [608] */
    0.928506136f, /* [609] */
    0.932992876f, /* [610] */
    0.937339127f, /* [611] */
    0.941544175f, /* [612] */
    0.945607424f, /* [613] */
    0.949528277f, /* [614] */
    0.953306139f, /* [615] */
    0.956940353f, /* [616] */
    0.960430503f, /* [617] */
    0.963776052f, /* [618] */
    0.966976464f, /* [619] */
    0.970031261f, /* [620] */
    0.972939968f, /* [621] */
    0.975702167f, /* [622] */
    0.978317440f, /* [623] */
    0.980785310f, /* [624] */
    0.983105540f, /* [625] */
    0.985277712f, /* [626] */
    0.987301469f, /* [627] */
    0.989176571f, /* [628] */
    0.990902722f, /* [629] */
    0.992479563f, /* [630] */
    0.993906975f, /* [631] */
    0.995184720f, /* [632] */
    0.996312618f, /* [633] */
    0.997290492f, /* [634] */
    0.998118103f, /* [635] */
    0.998795450f, /* [636] */
    0.999322414f, /* [637] */
    0.999698818f, /* [638] */
    0.999924719f, /* [639] */
};

// We are generating a 640 entries LUT (512 plus 128). The additional 128 entries are used
// To calculate the cosine without the additional module/masking. We can simplify the 
// LUT boundaries calculation by using a power-of-two size for the LUT and just bit-mask;
// by doing do, both positive and negative indices are handled properly without additional
// computation (see `imod()`).
void fsincos(int rotation, float *sin, float *cos)
{
    const int index = rotation & 0x1ff;
    *sin = _lut[index];
    *cos = _lut[index + 0x80];
}

// Instead of adding the whole trigonometric function set, we provide a way to convert
// radians to rotation index. This is useful, for example, when using `atan2()` to get
// a direction, then convert to the appropriate rotation index.
int fator(float angle)
{
    return (int)roundf(angle * 81.487327576f) & 0x1ff; // Round to nearest, so that `pi` is equal to `0`.
}

float frtoa(int rotation)
{
    return (float)(rotation & 0x1ff) / 81.487327576f;
}
