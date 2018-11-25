class ETCDecoder
{
    public static transcodeETC2_to_RGBA(source: Uint8Array, dest: Uint8Array, width: number, height: number): void
    {
        let bw = (width+3) >> 2; //block width
        let bh = (height+3) >> 2;//block height

        let destRowPitch = Math.max(width * 4, 16);

        let offset = 0;
        for(let by = 0; by < bh; by++)
        {
            for(let bx = 0; bx < bw; bx++)
            {
                let destOffset = by*4* destRowPitch + bx * 16;
                this.decodeETC2Block(source, offset, dest, destOffset, destRowPitch);

                offset += 8; //8bytes per block
            }
        }
    }

    public static transcodeETC2_EAC_to_RGBA(source: Uint8Array, dest: Uint8Array, width: number, height: number): void
    {
        let bw = (width+3) >> 2; //block width
        let bh = (height+3) >> 2;//block height

        let destRowPitch = Math.max(width * 4, 16);

        let offset = 0;
        for(let by = 0; by < bh; by++)
        {
            for(let bx = 0; bx < bw; bx++)
            {
                let destOffset = by*4* destRowPitch + bx * 16;
                this.decodeETC2Block(source, offset+8, dest, destOffset, destRowPitch);
                this.decodeEACBlock(source, offset, dest, destOffset, destRowPitch);

                offset += 16; //16bytes per block
            }
        }
    }

    /*
    https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
    ETC1_RGB8_OES:

    a) bit layout in bits 63 through 32 if diffbit = 0

     63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
     -----------------------------------------------
    | base col1 | base col2 | base col1 | base col2 |
    | R1 (4bits)| R2 (4bits)| G1 (4bits)| G2 (4bits)|
     -----------------------------------------------

     47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
     ---------------------------------------------------
    | base col1 | base col2 | table  | table  |diff|flip|
    | B1 (4bits)| B2 (4bits)| cw 1   | cw 2   |bit |bit |
     ---------------------------------------------------
    
     
    b) bit layout in bits 63 through 32 if diffbit = 1

     63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48 
     -----------------------------------------------
    | base col1    | dcol 2 | base col1    | dcol 2 |
    | R1' (5 bits) | dR2    | G1' (5 bits) | dG2    |
     -----------------------------------------------
    
     47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32 
     ---------------------------------------------------
    | base col 1   | dcol 2 | table  | table  |diff|flip|
    | B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
     ---------------------------------------------------

     
    c) bit layout in bits 31 through 0 (in both cases)

     31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
     -----------------------------------------------
    |       most significant pixel index bits       |
    | p| o| n| m| l| k| j| i| h| g| f| e| d| c| b| a|
     -----------------------------------------------

     15 14 13 12 11 10  9  8  7  6  5  4  3   2   1  0
     --------------------------------------------------
    |         least significant pixel index bits       |  
    | p| o| n| m| l| k| j| i| h| g| f| e| d| c | b | a |
     --------------------------------------------------      
    */

    //https://www.khronos.org/registry/OpenGL/specs/es/3.0/es_spec_3.0.pdf page293

    static extend_4to8bits(x: number): number
    { 
         return (x << 4) | x;
    }
    static extend_5to8bits(x: number): number
     { 
         return (x << 3) | (x >> 2);
    }
    static extend_6to8bits(x: number): number
    { 
        return (x << 2) | (x >> 4);
    }
    static extend_7to8bits(x: number): number
    { 
         return (x << 1) | (x >> 6);
    }

    public static clamp(value: number, min: number, max: number): number
    {
        if(value < min)
            return min;

        if(value > max)
            return max;

        return value;

        //return Math.min(Math.max(value, min), max);
    }

    private static etc2ModifierIdx: number[] = new Array<number>(16);
    private static distanceTable = [3, 6, 11, 16, 23, 32, 41, 64];
    private static decodeETC2Block(source: Uint8Array, offset: number, dest: Uint8Array, destOffset: number, destRowPitch: number): void
    {
        let byte0 = source[offset];
        let byte1 = source[offset+1];
        let byte2 = source[offset+2];
        let byte3 = source[offset+3];
        let byte4 = source[offset+4];
        let byte5 = source[offset+5];
        let byte6 = source[offset+6];
        let byte7 = source[offset+7];

        let diffbit = (byte3 & 2) != 0;
        if(diffbit)
        {
            let bR = byte0 >> 3;
            let dR = ((byte0 & 0x7)<<29)>>29;  //convert to int3

            let bG = byte1 >> 3;
            let dG = ((byte1 & 0x7)<<29)>>29;

            let bB = byte2 >> 3;
            let dB = ((byte2 & 0x7)<<29)>>29;

            let r = bR+dR;
            let g = bG+dG;
            let b = bB+dB;
            if(r < 0 || r > 31) //T mode
            {
                let r1a = (byte0 >> 3) & 0x3;
                let r1b = byte0 & 0x3;
                let r1 = this.extend_4to8bits((r1a<<2) | r1b);
                let g1 = this.extend_4to8bits(byte1 >> 4);
                let b1 = this.extend_4to8bits(byte1 & 0xF);

                let r2 = this.extend_4to8bits(byte2 >> 4);
                let g2 = this.extend_4to8bits(byte2 & 0xF);
                let b2 = this.extend_4to8bits(byte3 >> 4);

                let da = (byte3>>2) & 0x3;
                let db = byte3 & 0x1;
                let d = this.distanceTable[(da << 1) | db];

                let paintColorsR = [r1, Math.min(r2+d, 255), r2, Math.max(r2-d, 0)];
                let paintColorsG = [g1, Math.min(g2+d, 255), g2, Math.max(g2-d, 0)];
                let paintColorsB = [b1, Math.min(b2+d, 255), b2, Math.max(b2-d, 0)];

                //modifierIdx
                this.decodeModifierTable(byte4, byte5, byte6, byte7);

                for(let j = 0; j < 4; j++)
                {
                    for(let i = 0; i < 4; i++)
                    {
                        let colorIdx = this.etc2ModifierIdx[i*4+j];

                        let index = destOffset+j*destRowPitch+i*4;
                        dest[index]   = paintColorsR[colorIdx];
                        dest[index+1] = paintColorsG[colorIdx];
                        dest[index+2] = paintColorsB[colorIdx];
                        dest[index+3] = 255;
                    }
                }
            }
            else if(g < 0 || g > 31) //H mode
            {
                let r1 = this.extend_4to8bits((byte0 >> 3) & 0xF);
                let g1a = byte0 & 0x7;
                let g1b = (byte1>>4) & 0x1;
                let g1 = this.extend_4to8bits((g1a<<1) | g1b);
                let b1a = (byte1>>3) & 0x1;
                let b1b = ((byte1 & 0x3)<<1) | (byte2>>7);  //47-49
                let b1 = this.extend_4to8bits((b1a<<3) | b1b);

                let r2 = this.extend_4to8bits((byte2>>3) & 0xF);
                let g2 = this.extend_4to8bits(((byte2 & 0x7)<<1) | byte3>>7);
                let b2 = this.extend_4to8bits((byte3>>3) & 0xF);

                let da = (byte3>>2) & 0x1;
                let db = byte3 & 0x1;
                let orderingTrickBit = ((r1 << 16 | g1 << 8 | b1) >= (r2 << 16 | g2 << 8 | b2) ? 1 : 0);
                let d = this.distanceTable[(da << 2) | (db<<1) | orderingTrickBit];

                let paintColorsR = [Math.min(r1+d, 255), Math.max(r1-d, 0), Math.min(r2+d, 255), Math.max(r2-d, 0)];
                let paintColorsG = [Math.min(g1+d, 255), Math.max(g1-d, 0), Math.min(g2+d, 255), Math.max(g2-d, 0)];
                let paintColorsB = [Math.min(b1+d, 255), Math.max(b1-d, 0), Math.min(b2+d, 255), Math.max(b2-d, 0)];

                //modifierIdx
                this.decodeModifierTable(byte4, byte5, byte6, byte7);

                for(let j = 0; j < 4; j++)
                {
                    for(let i = 0; i < 4; i++)
                    {
                        let colorIdx = this.etc2ModifierIdx[i*4+j];

                        let index = destOffset+j*destRowPitch+i*4;
                        dest[index]   = paintColorsR[colorIdx];
                        dest[index+1] = paintColorsG[colorIdx];
                        dest[index+2] = paintColorsB[colorIdx];
                        dest[index+3] = 255;
                    }
                }
            }
            else if(b < 0 || b > 31) //planar mode
            {
                let rO = this.extend_6to8bits((byte0>>1) & 0x3F);
                let gO1 = byte0 & 0x1;
                let gO2 = (byte1>>1) & 0x3F;
                let gO = this.extend_7to8bits((gO1 << 6) | gO2);
                let bO1 = byte1 & 0x1;
                let bO2 = (byte2>>3) & 0x3;
                let bO3 = ((byte2 & 0x3)<<1) | (byte3>>7);
                let bO = this.extend_6to8bits(bO1<<5 | bO2<<3 | bO3);

                let rH1 = (byte3>>2) & 0x1F;
                let rH2 = byte3 & 0x1;
                let rH = this.extend_6to8bits(rH1<<1 | rH2);
                let gH = this.extend_7to8bits(byte4 >> 1);
                let bH = this.extend_6to8bits(((byte4 &  0x1)<<5) | (byte5>>3));

                let rV = this.extend_6to8bits(((byte5 &  0x7)<<3) | (byte6>>5));
                let gV = this.extend_7to8bits(((byte6 & 0x1F)<<2) | (byte7>>6));
                let bV = this.extend_6to8bits(byte7 & 0x3F);

                let rHO = rH-rO;
                let gHO = gH-gO;
                let bHO = bH-bO;

                let rVO = rV-rO;
                let gVO = gV-gO;
                let bVO = bV-bO;

                for(let j = 0; j < 4; j++)
                {
                    let rj = j*rVO+2;
                    let gj = j*gVO+2;
                    let bj = j*bVO+2;
                    for(let i = 0; i < 4; i++)
                    {
                        let index = destOffset+j*destRowPitch+i*4;
                        // dest[index]   = this.clamp(((i*(rH-rO)+j*(rV-rO)+2)>>2)+rO, 0, 255);
                        // dest[index+1] = this.clamp(((i*(gH-gO)+j*(gV-gO)+2)>>2)+gO, 0, 255);
                        // dest[index+2] = this.clamp(((i*(bH-bO)+j*(bV-bO)+2)>>2)+bO, 0, 255);
                        dest[index]   = this.clamp(((i*rHO+rj)>>2)+rO, 0, 255);
                        dest[index+1] = this.clamp(((i*gHO+gj)>>2)+gO, 0, 255);
                        dest[index+2] = this.clamp(((i*bHO+bj)>>2)+bO, 0, 255);
                        dest[index+3] = 255;
                    }
                }
            }
            else //differential mode
            {
                let r1 = this.extend_5to8bits(bR);
                let r2 = this.extend_5to8bits(r);

                let g1 = this.extend_5to8bits(bG);
                let g2 = this.extend_5to8bits(g);

                let b1 = this.extend_5to8bits(bB);
                let b2 = this.extend_5to8bits(b);

                // let r1 = (bR << 3) | (bR >> 2);
                // let r2 = (r << 3) | (r >> 2);

                // let g1 = (bG << 3) | (bG >> 2);
                // let g2 = (g << 3) | (g >> 2);

                // let b1 = (bB << 3) | (bB >> 2);
                // let b2 = (b << 3) | (b >> 2);

                let tableIdx1 = (byte3>>5) & 0x7;
                let tableIdx2 = (byte3>>2) & 0x7;

                let flip = (byte3 & 1) == 1;

                this.decodeModifierTable(byte4, byte5, byte6, byte7);
                this.decodeIndividualOrDifferentialBlock(dest, destOffset, destRowPitch, r1,g1,b1,tableIdx1, r2,g2,b2,tableIdx2, this.etc2ModifierIdx, flip);
            }
        }
        else //individual mode
        {
            let r1 = this.extend_4to8bits(byte0 >> 4);
            let r2 = this.extend_4to8bits(byte0 & 0xF);

            let g1 = this.extend_4to8bits(byte1 >> 4);
            let g2 = this.extend_4to8bits(byte1 & 0xF);

            let b1 = this.extend_4to8bits(byte2 >> 4);
            let b2 = this.extend_4to8bits(byte2 & 0xF);

            // let x = byte0 >> 4;
            // let r1 = (x << 4) | x;

            // x = byte0 & 0xF;
            // let r2 = (x << 4) | x;

            // x = byte1 >> 4;
            // let g1 = (x << 4) | x;

            // x = byte1 & 0xF;
            // let g2 = (x << 4) | x;

            // x = byte2 >> 4;
            // let b1 = (x << 4) | x;

            // x = byte2 & 0xF;
            // let b2 = (x << 4) | x;

            let tableIdx1 = (byte3>>5) & 0x7;
            let tableIdx2 = (byte3>>2) & 0x7;

            let flip = (byte3 & 1) == 1;

            this.decodeModifierTable(byte4, byte5, byte6, byte7);
            this.decodeIndividualOrDifferentialBlock(dest, destOffset, destRowPitch, r1,g1,b1,tableIdx1, r2,g2,b2,tableIdx2, this.etc2ModifierIdx, flip);
        }
    }

    private static decodeModifierTable(byte4: number, byte5: number, byte6: number, byte7: number)
    {
        //modifierIdx
        let modifierIdx = this.etc2ModifierIdx;
        // for(let i = 0; i < 8; i++)
        // {
        //     let msb = (byte5 & (1<<i)) >> i;
        //     let lsb = (byte7 & (1<<i)) >> i;
        //     modifierIdx[i] = (msb<<1) | lsb;
        // }

        // for(let i = 0; i < 8; i++)
        // {
        //     let msb = (byte4 & (1<<i)) >> i;
        //     let lsb = (byte6 & (1<<i)) >> i;
        //     modifierIdx[i+8] = (msb<<1) | lsb;
        // }
        modifierIdx[0] = (byte5 & 0x1)<<1    | (byte7& 0x1);
        modifierIdx[1] = (byte5 & 0x2)       | ((byte7& 0x2)>>1);
        modifierIdx[2] = ((byte5 & 0x4)>>1)  | ((byte7& 0x4)>>2);
        modifierIdx[3] = ((byte5 & 0x8)>>2)  | ((byte7& 0x8)>>3);
        modifierIdx[4] = ((byte5 & 0x10)>>3) | ((byte7& 0x10)>>4);
        modifierIdx[5] = ((byte5 & 0x20)>>4) | ((byte7& 0x20)>>5);
        modifierIdx[6] = ((byte5 & 0x40)>>5) | ((byte7& 0x40)>>6);
        modifierIdx[7] = ((byte5 & 0x80)>>6) | ((byte7& 0x80)>>7);

        modifierIdx[8]  = (byte4 & 0x1)<<1    | (byte6& 0x1);
        modifierIdx[9]  = (byte4 & 0x2)       | ((byte6& 0x2)>>1);
        modifierIdx[10] = ((byte4 & 0x4)>>1)  | ((byte6& 0x4)>>2);
        modifierIdx[11] = ((byte4 & 0x8)>>2)  | ((byte6& 0x8)>>3);
        modifierIdx[12] = ((byte4 & 0x10)>>3) | ((byte6& 0x10)>>4);
        modifierIdx[13] = ((byte4 & 0x20)>>4) | ((byte6& 0x20)>>5);
        modifierIdx[14] = ((byte4 & 0x40)>>5) | ((byte6& 0x40)>>6);
        modifierIdx[15] = ((byte4 & 0x80)>>6) | ((byte6& 0x80)>>7);
    }

    // Table 3.17.2 sorted according to table 3.17.3
    private static intensityModifier: number[][] =
    [
        [  2,   8,  -2,   -8 ],
        [  5,  17,  -5,  -17 ],
        [  9,  29,  -9,  -29 ],
        [ 13,  42, -13,  -42 ],
        [ 18,  60, -18,  -60 ],
        [ 24,  80, -24,  -80 ],
        [ 33, 106, -33, -106 ],
        [ 47, 183, -47, -183 ],
    ];

    private static subblockR: number[] = new Array<number>(4);
    private static subblockG: number[] = new Array<number>(4);
    private static subblockB: number[] = new Array<number>(4);
    private static decodeIndividualOrDifferentialBlock(dest: Uint8Array, destOffset: number, destRowPitch: number,
                                                         r1: number, g1: number, b1: number, tableIdx1,
                                                         r2: number, g2: number, b2: number, tableIdx2,
                                                         modifierIdx: number[], flip: boolean)
    {        
        if(flip) //Two 4x2-pixel subblocks on top of each other
        {
            for(let i = 0; i < 4; i++)
            {
                let delta = this.intensityModifier[tableIdx1][i];
                this.subblockR[i] = this.clamp(r1 + delta, 0, 255);
                this.subblockG[i] = this.clamp(g1 + delta, 0, 255);
                this.subblockB[i] = this.clamp(b1 + delta, 0, 255);
            }
            for(let j = 0; j < 2; j++)
            {
                for(let i = 0; i < 4; i++)
                {
                    let intensityIdx = modifierIdx[i*4+j];
                    //let delta = this.intensityModifier[tableIdx1][intensityIdx];

                    let index = destOffset+j*destRowPitch+i*4;
                    dest[index]   = this.subblockR[intensityIdx];
                    dest[index+1] = this.subblockG[intensityIdx];
                    dest[index+2] = this.subblockB[intensityIdx];
                    dest[index+3] = 255;
                }
            }

            for(let i = 0; i < 4; i++)
            {
                let delta = this.intensityModifier[tableIdx2][i];
                this.subblockR[i] = this.clamp(r2 + delta, 0, 255);
                this.subblockG[i] = this.clamp(g2 + delta, 0, 255);
                this.subblockB[i] = this.clamp(b2 + delta, 0, 255);
            }
            for(let j = 2; j < 4; j++)
            {
                for(let i = 0; i < 4; i++)
                {
                    let intensityIdx = modifierIdx[i*4+j];

                    let index = destOffset+j*destRowPitch+i*4;
                    dest[index]   = this.subblockR[intensityIdx];
                    dest[index+1] = this.subblockG[intensityIdx];
                    dest[index+2] = this.subblockB[intensityIdx];
                    dest[index+3] = 255;
                }
            }
        }
        else //Two 2x4-pixel subblocks side-by-side
        {
            for(let i = 0; i < 4; i++)
            {
                let delta = this.intensityModifier[tableIdx1][i];
                this.subblockR[i] = this.clamp(r1 + delta, 0, 255);
                this.subblockG[i] = this.clamp(g1 + delta, 0, 255);
                this.subblockB[i] = this.clamp(b1 + delta, 0, 255);
            }

            for(let j = 0; j < 4; j++)
            {
                for(let i = 0; i < 2; i++)
                {
                    let intensityIdx = modifierIdx[i*4+j];
                    let delta = this.intensityModifier[tableIdx1][intensityIdx];

                    let index = destOffset+j*destRowPitch+i*4;
                    dest[index]   = this.subblockR[intensityIdx];
                    dest[index+1] = this.subblockG[intensityIdx];
                    dest[index+2] = this.subblockB[intensityIdx];
                    dest[index+3] = 255;
                }
            }

            for(let i = 0; i < 4; i++)
            {
                let delta = this.intensityModifier[tableIdx2][i];
                this.subblockR[i] = this.clamp(r1 + delta, 0, 255);
                this.subblockG[i] = this.clamp(g1 + delta, 0, 255);
                this.subblockB[i] = this.clamp(b1 + delta, 0, 255);
            }
            for(let j = 0; j < 4; j++)
            {
                for(let i = 2; i < 4; i++)
                {
                    let intensityIdx = modifierIdx[i*4+j];
                    let delta = this.intensityModifier[tableIdx2][intensityIdx];

                    let index = destOffset+j*destRowPitch+i*4;
                    dest[index]   = this.subblockR[intensityIdx];
                    dest[index+1] = this.subblockG[intensityIdx];
                    dest[index+2] = this.subblockB[intensityIdx];
                    dest[index+3] = 255;
                }
            }
        }
    }

    //Table C.10: Intensity modifier sets for alpha component.
    private static alphaModifier: number[][] =
        [
            [ -3, -6,  -9, -15, 2, 5, 8, 14 ],
            [ -3, -7, -10, -13, 2, 6, 9, 12 ],
            [ -2, -5,  -8, -13, 1, 4, 7, 12 ],
            [ -2, -4,  -6, -13, 1, 3, 5, 12 ],
            [ -3, -6,  -8, -12, 2, 5, 7, 11 ],
            [ -3, -7,  -9, -11, 2, 6, 8, 10 ],
            [ -4, -7,  -8, -11, 3, 6, 7, 10 ],
            [ -3, -5,  -8, -11, 2, 4, 7, 10 ],
            [ -2, -6,  -8, -10, 1, 5, 7,  9 ],
            [ -2, -5,  -8, -10, 1, 4, 7,  9 ],
            [ -2, -4,  -8, -10, 1, 3, 7,  9 ],
            [ -2, -5,  -7, -10, 1, 4, 6,  9 ],
            [ -3, -4,  -7, -10, 2, 3, 6,  9 ],
            [ -1, -2,  -3, -10, 0, 1, 2,  9 ],
            [ -4, -6,  -8,  -9, 3, 5, 7,  8 ],
            [ -3, -5,  -7,  -9, 2, 4, 6,  8 ]
        ];

    private static alphaModifierIdx:number[] = new Array<number>(16);
    private static decodeEACBlock(source: Uint8Array, offset: number, dest: Uint8Array, destOffset: number, destRowPitch: number): void
    {
        let baseCodeword = source[offset];

        let byte1 = source[offset+1];
        let byte2 = source[offset+2];
        let byte3 = source[offset+3];
        let byte4 = source[offset+4];
        let byte5 = source[offset+5];
        let byte6 = source[offset+6];
        let byte7 = source[offset+7];

        let multiplier = byte1 >> 4;
        let tableIdx = byte1 & 0xF;

        //let modifierIdx: number[] = [];
        let modifierIdx = this.alphaModifierIdx;
        modifierIdx[0] = byte2>>5;
        modifierIdx[1] = (byte2>>2) & 0x7;
        modifierIdx[2] = ((byte2 & 0x3)<<1)| (byte3>>7);
        modifierIdx[3] = (byte3>>4) & 0x7;
        modifierIdx[4] = (byte3>>1) & 0x7;
        modifierIdx[5] = ((byte3 & 0x1)<<2)| (byte4>>6);
        modifierIdx[6] = (byte4>>3) & 0x7;
        modifierIdx[7] = byte4 & 0x7;

        modifierIdx[8] = byte5>>5;
        modifierIdx[9] = (byte5>>2) & 0x7;
        modifierIdx[10] = ((byte5 & 0x3)<<1)| (byte6>>7);
        modifierIdx[11] = (byte6>>4) & 0x7;
        modifierIdx[12] = (byte6>>1) & 0x7;
        modifierIdx[13] = ((byte6 & 0x1)<<2)| (byte7>>6);
        modifierIdx[14] = (byte7>>3) & 0x7;
        modifierIdx[15] = byte7 & 0x7;

        let destIndex = destOffset+3;
        for(let j = 0; j < 4; j++)
        {
            for(let i = 0; i < 4; i++)
            {
                //let index = destOffset+j*destRowPitch+i*4;
                let modifier = this.alphaModifier[tableIdx][modifierIdx[i*4+j]];
                //dest[index+3] = this.clamp(baseCodeword + modifier*multiplier, 0, 255);
                dest[destIndex] = this.clamp(baseCodeword + modifier*multiplier, 0, 255);

                destIndex += 4;
            }
            destIndex += destRowPitch-16;
        }
    }
}