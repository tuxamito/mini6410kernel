static void s3c_nand_write_page_8bit_me(struct mtd_info *mtd, struct nand_chip *chip,
                                  const uint8_t *buf)
{7
        int i, eccsize = chip->ecc.size;
        int eccbytes = chip->ecc.bytes;
        int eccsteps = chip->ecc.steps;
                uint8_t *p = (uint8_t*)buf;
        int badoffs = mtd->writesize == 512 ? NAND_SMALL_BADBLOCK_POS : NAND_LARGE_BADBLOCK_POS;
        uint8_t *ecc_calc = chip->buffers->ecccalc;
                uint32_t *mecc_pos = chip->ecc.layout->eccpos;


        for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
                chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
                chip->write_buf(mtd, p, eccsize);
                chip->ecc.calculate(mtd, p, &ecc_calc[i]);
        }

        chip->oob_poi[badoffs] = 0xff;
        for (i = 0; i < chip->ecc.total; i++)
                chip->oob_poi[mecc_pos[i]] = ecc_calc[i];

        chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

int s3c_nand_read_page_8bit_me(struct mtd_info *mtd, struct nand_chip *chip,
                                uint8_t *buf)
{
        int i, stat, eccsize = chip->ecc.size;
        int eccbytes = chip->ecc.bytes;
        int eccsteps = chip->ecc.steps;
        int col = 0;
        uint8_t *p = buf;
        uint32_t *mecc_pos = chip->ecc.layout->eccpos;

        /* Step1: read whole oob */
        col = mtd->writesize;
        chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
        chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

        col = 0;
        for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
                chip->cmdfunc(mtd, NAND_CMD_RNDOUT, col, -1);
                chip->ecc.hwctl(mtd, NAND_ECC_READ);
                chip->read_buf(mtd, p, eccsize);
                chip->write_buf(mtd, chip->oob_poi + mecc_pos[0] + ((chip->ecc.steps - eccsteps) * eccbytes), eccbytes);
                chip->ecc.calculate(mtd, 0, 0);
                stat = chip->ecc.correct(mtd, p, NULL, NULL);

                if (stat == -1)
                        mtd->ecc_stats.failed++;

                col = eccsize * (chip->ecc.steps + 1 - eccsteps);
        }

        return 0;
}

int s3c_nand_read_oob_8bit_me(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
        int eccbytes = chip->ecc.bytes;
        int secc_start = mtd->oobsize - eccbytes;

        if (sndcmd) {
                chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
                sndcmd = 0;
        }

        chip->read_buf(mtd, chip->oob_poi, secc_start);
        return sndcmd;
}

int s3c_nand_write_oob_8bit_me(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
        int status = 0;
        int eccbytes = chip->ecc.bytes;
        int secc_start = mtd->oobsize - eccbytes;

        chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);

        /* spare area */
        chip->write_buf(mtd, chip->oob_poi, secc_start);

        /* Send command to program the OOB data */
        chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
        status = chip->waitfunc(mtd, chip);
        return status & NAND_STATUS_FAIL ? -EIO : 0;
}

static void s3c_nand_wait_ecc_busy_8bit(void)
{
        void __iomem *regs = s3c_nand.regs;
        //while (readl(S3C_NF8ECCERR0) & S3C_NFESTAT0_ECCBUSY) {}
        while (readl(regs + S3C_NF8ECCERR0) & S3C_NFECCERR0_ECCBUSY) {}
}

int s3c_nand_calculate_ecc_8bit_me(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
        u_long nfcont, nfm8ecc0, nfm8ecc1, nfm8ecc2, nfm8ecc3;
        void __iomem *regs = s3c_nand.regs;

        /* Lock */
        nfcont = readl(regs + S3C_NFCONT);
        nfcont |= S3C_NFCONT_MECCLOCK;
        writel(nfcont, regs + S3C_NFCONT);

        if (cur_ecc_mode == NAND_ECC_READ)
                s3c_nand_wait_dec();
        else {
                s3c_nand_wait_enc();

                nfm8ecc0 = readl(regs + S3C_NFM8ECC0);
                nfm8ecc1 = readl(regs + S3C_NFM8ECC1);
                nfm8ecc2 = readl(regs + S3C_NFM8ECC2);
                nfm8ecc3 = readl(regs + S3C_NFM8ECC3);

                ecc_code[0] = nfm8ecc0 & 0xff;
                ecc_code[1] = (nfm8ecc0 >> 8) & 0xff;
                ecc_code[2] = (nfm8ecc0 >> 16) & 0xff;
                ecc_code[3] = (nfm8ecc0 >> 24) & 0xff;
                ecc_code[4] = nfm8ecc1 & 0xff;
                ecc_code[5] = (nfm8ecc1 >> 8) & 0xff;
                ecc_code[6] = (nfm8ecc1 >> 16) & 0xff;
                ecc_code[7] = (nfm8ecc1 >> 24) & 0xff;
                ecc_code[8] = nfm8ecc2 & 0xff;
                ecc_code[9] = (nfm8ecc2 >> 8) & 0xff;
                ecc_code[10] = (nfm8ecc2 >> 16) & 0xff;
                ecc_code[11] = (nfm8ecc2 >> 24) & 0xff;
                ecc_code[12] = nfm8ecc3 & 0xff;
        }

        return 0;
}

int s3c_nand_correct_data_8bit_me(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
        int ret = -1;
        u_long nf8eccerr0, nf8eccerr1, nf8eccerr2, nfmlc8bitpt0, nfmlc8bitpt1;
        u_char err_type;
        void __iomem *regs = s3c_nand.regs;

        s3c_nand_wait_ecc_busy_8bit();

        nf8eccerr0 = readl(regs + S3C_NF8ECCERR0);
        nf8eccerr1 = readl(regs + S3C_NF8ECCERR1);
        nf8eccerr2 = readl(regs + S3C_NF8ECCERR2);& _/ t8 \' h# I8 h- q: ]
        nfmlc8bitpt0 = readl(regs + S3C_NFMLC8BITPT0);
        nfmlc8bitpt1 = readl(regs + S3C_NFMLC8BITPT1);

        err_type = (nf8eccerr0 >> 25) & 0xf;

        /* No error, If free page (all 0xff) */
        /* While testing, it was found that NFECCERR0[29] bit is set even if
         * the page contents were not zero. So this code is commented */

        /*if ((nf8eccerr0 >> 29) & 0x1)
                err_type = 0;*/

        switch (err_type) {
        case 9: /* Uncorrectable */
                printk("s3c-nand: ECC uncorrectable error detected\n");
                ret = -1;
                break;

        case 8: /* 8 bit error (Correctable) */
                dat[(nf8eccerr2 >> 22) & 0x3ff] ^= ((nfmlc8bitpt1 >> 24) & 0xff);

        case 7: /* 7 bit error (Correctable) */
                dat[(nf8eccerr2 >> 11) & 0x3ff] ^= ((nfmlc8bitpt1 >> 16) & 0xff);

        case 6: /* 6 bit error (Correctable) */
                dat[nf8eccerr2 & 0x3ff] ^= ((nfmlc8bitpt1 >> 8) & 0xff);

        case 5: /* 5 bit error (Correctable) */
                dat[(nf8eccerr1 >> 22) & 0x3ff] ^= (nfmlc8bitpt1 & 0xff);

        case 4: /* 4 bit error (Correctable) */
                dat[(nf8eccerr1 >> 11) & 0x3ff] ^= ((nfmlc8bitpt0 >> 24) & 0xff);

        case 3: /* 3 bit error (Correctable) */
                dat[nf8eccerr1 & 0x3ff] ^= ((nfmlc8bitpt0 >> 16) & 0xff);

        case 2: /* 2 bit error (Correctable) */
                dat[(nf8eccerr0 >> 15) & 0x3ff] ^= ((nfmlc8bitpt0 >> 8) & 0xff);

        case 1: /* 1 bit error (Correctable) */
                //printk("s3c-nand: %d bit(s) error detected, corrected successfully\n", err_type);
                dat[nf8eccerr0 & 0x3ff] ^= (nfmlc8bitpt0 & 0xff);
                ret = err_type;
                break;

        case 0: /* No error */
                ret = 0;
                break;
        }

        return ret;
}

void s3c_nand_enable_hwecc_8bit_me(struct mtd_info *mtd, int mode)
{
        u_long nfcont, nfconf;
        void __iomem *regs = s3c_nand.regs;

        cur_ecc_mode = mode;

        /* 8 bit selection */
        nfconf = readl(regs + S3C_NFCONF);

        nfconf &= ~(0x3 << 23);
        nfconf |= (0x1 << 23);

        writel(nfconf, regs + S3C_NFCONF);

        /* Initialize & unlock */
        nfcont = readl(regs + S3C_NFCONT);
        nfcont |= S3C_NFCONT_INITECC;
        nfcont &= ~S3C_NFCONT_MECCLOCK;

        if (mode == NAND_ECC_WRITE)
                nfcont |= S3C_NFCONT_ECC_ENC;
        else if (mode == NAND_ECC_READ)
                nfcont &= ~S3C_NFCONT_ECC_ENC;

        writel(nfcont, regs  + S3C_NFCONT);
}

static struct nand_ecclayout s3c_nand_oob_436 = {
        .eccbytes = 256,
        .eccpos = {
                40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,
                56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,
                72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,
                88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,
                104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
                120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,
                136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,
                152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,
                168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
                184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,
                200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
                216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
                232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,
                248,249,250,251,252,253,254,255,256,257,258,259,260,261,262,263,
                264,265,266,267,268,269,270,271,272,273,274,275,276,277,278,279,
                280,281,282,283,284,285,286,287,288,289,290,291,292,293,294,295
        },
        .oobfree = {
                {.offset = 2,
                 .length = 32}}
};

