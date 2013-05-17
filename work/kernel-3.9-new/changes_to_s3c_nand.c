Inside s3c_nand_probe

#ifdef CONFIG_MACH_MINI6410
                                        //partitions = mini6410_nand_part_mlc;
                                        //nr_partitions = ARRAY_SIZE(mini6410_nand_part_mlc);
                                        nand->ecc.layout = &s3c_nand_oob_436;
                                        nand->ecc.calculate        = s3c_nand_calculate_ecc_8bit_me;
                                        nand->ecc.correct        = s3c_nand_correct_data_8bit_me;
                                        ///nand->ecc.calculate        = nand2.ecc.calculate;
                                        //nand->ecc.correct        = nand2.ecc.correct;
                                        //s3c_nand_print_layout(nand->ecc.layout);
                                        nand->ecc.hwctl                = s3c_nand_enable_hwecc_8bit_me;
                                        nand->ecc.size = 512;
                                        nand->ecc.bytes = 16;
                                        nand->ecc.read_page = s3c_nand_read_page_8bit_me;
                                        nand->ecc.write_page = s3c_nand_write_page_8bit_me;
                                        nand->ecc.read_oob = s3c_nand_read_oob_8bit_me;
                                        nand->ecc.write_oob = s3c_nand_write_oob_8bit_me;
#endif% S- A0 e4 O1 `) Q1 f3 O

