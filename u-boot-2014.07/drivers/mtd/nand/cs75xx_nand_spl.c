/*
 * (C) Copyright 2014, Greenwave Systems, Inc.
 * Hoang Tran <hoang.tran@greenwavesystems.com>
 *
 * based on nand_spl_simple code
 *
 */

#include <common.h>
#include <nand.h>
#include <asm/io.h>
#include <linux/mtd/nand_ecc.h>
#include <asm/arch/register.h>
#include <malloc.h>

#include "cs75xx_nand.h"

#define NF_START_BCNT		2000
#define NF_STS_PCNT		20000
#define NF_STS_MSK		0xF00
#define NF_STS_VAL		0

#define REG32(addr)     (*(volatile unsigned long * const)(addr))

#define ECCSTEPS	(CONFIG_SYS_NAND_PAGE_SIZE / \
					CONFIG_SYS_NAND_ECCSIZE)
#define ECCTOTAL	(ECCSTEPS * CONFIG_SYS_NAND_ECCBYTES)

static int nand_ecc_pos[] = CONFIG_SYS_NAND_ECCPOS;
static nand_info_t mtd;
static struct nand_chip nand_chip;
static u_char ecc_code[ECCTOTAL];
static u_char oob_data[CONFIG_SYS_NAND_OOBSIZE];

static unsigned int reg_wait(unsigned int regaddr, unsigned int mask, unsigned int val, int timeout)
{
	unsigned int i, tmp;

	for (i = timeout; i > 0; i--) {
		tmp = REG32(regaddr);
		if ((tmp & mask) == val)
			return 1;
	}

	return 0;
}

void flash_reset(void)
{
	FLASH_NF_ACCESS_t	nf_access;
	FLASH_NF_ECC_RESET_t	nf_ecc_reset;

	nf_access.wrd = 0;
	nf_access.bf.autoReset = 1;
	REG32(FLASH_NF_ACCESS) = nf_access.wrd;

	nf_ecc_reset.wrd = 0;
	nf_ecc_reset.bf.nflash_reset = 1;
	REG32(FLASH_NF_ECC_RESET) = nf_ecc_reset.wrd;

	reg_wait(FLASH_NF_ECC_RESET, nf_ecc_reset.wrd, 0, NF_START_BCNT);
}

static int nand_is_bad_block(int block)
{
	unsigned int	i;
	static unsigned char 	*oobp, tmp_buf[2];
	FLASH_NF_ACCESS_t	nf_access;
	FLASH_NF_COUNT_t	nf_cnt;

	unsigned char *page_add;
	page_add = (unsigned char*)(CONFIG_SYS_NAND_BASE + (block * CONFIG_SYS_NAND_BLOCK_SIZE));

	nf_access.wrd = 0;
	nf_access.bf.nflashExtAddr =  0;
	REG32(FLASH_NF_ACCESS) = nf_access.wrd;

	if (!reg_wait(FLASH_STATUS, NF_STS_MSK, NF_STS_VAL, NF_STS_PCNT))
		flash_reset();

	//set oob size
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = CONFIG_SYS_NAND_OOBSIZE - 1;
	REG32(FLASH_NF_COUNT) =  nf_cnt.wrd;

	tmp_buf[0] = 0x0;
	tmp_buf[1] = 0x0;
	//check is bad block
	oobp = page_add;
	for (i = (CONFIG_SYS_NAND_PAGE_SIZE - 4);
		i < (CONFIG_SYS_NAND_PAGE_SIZE + CONFIG_SYS_NAND_OOBSIZE); i++) //read oob data to check bad block
	{
		if (i == CONFIG_SYS_NAND_PAGE_SIZE)
			tmp_buf[0] = oobp[i];
		else
			tmp_buf[1] = oobp[i];
	}


	if (tmp_buf[0]==0xff)
		return 0;

	return 1; /* bad block */
}

static int nand_read_page(int block, int page, void *dst)
{

	unsigned int		i, j, k;
	unsigned int		errloc0, errloc1;
	unsigned char		*p = dst;
	unsigned char		*page_add;

	FLASH_NF_COUNT_t	nf_cnt;
	FLASH_NF_ECC_RESET_t	ecc_reset;
#ifdef CONFIG_CS752X_NAND_ECC_HW_BCH_8_512
	FLASH_NF_BCH_CONTROL_t	bch_ctrl;
	FLASH_NF_BCH_STATUS_t	bch_sts, bch_sts_msk;
	FLASH_NF_BCH_OOB0_t	bch_oob0;
	FLASH_NF_BCH_OOB1_t	bch_oob1;
	FLASH_NF_BCH_OOB2_t	bch_oob2;
	FLASH_NF_BCH_OOB3_t	bch_oob3;
	FLASH_NF_BCH_ERROR_LOC01_t	bch_err_loc01;
#else
	FLASH_NF_ECC_CONTROL_t	ecc_ctl;
	FLASH_NF_ECC_STATUS_t	ecc_sts, ecc_sts_msk;
	FLASH_NF_ECC_OOB_t	ecc_oob;
#endif

	page_add = (unsigned char*)(CONFIG_SYS_NAND_BASE + (block * CONFIG_SYS_NAND_BLOCK_SIZE) + (page * CONFIG_SYS_NAND_PAGE_SIZE));

	if (!reg_wait(FLASH_STATUS, NF_STS_MSK, NF_STS_VAL, NF_STS_PCNT))
		flash_reset();

	//ecc reset
	ecc_reset.wrd = 0;
	ecc_reset.bf.eccClear = ECC_CLR;
	ecc_reset.bf.fifoClear = 1;
	REG32(FLASH_NF_ECC_RESET) = ecc_reset.wrd;

#ifdef CONFIG_CS752X_NAND_ECC_HW_BCH_8_512
	bch_ctrl.wrd = 0;
	bch_ctrl.bf.bchErrCap = 0;	//8bits
	bch_ctrl.bf.bchEn = 1;
	bch_ctrl.bf.bchOpcode = 1;	//BCH_DECODE;
	REG32(FLASH_NF_BCH_CONTROL) = bch_ctrl.wrd;
#else
	ecc_ctl.wrd = 0;
	ecc_ctl.bf.eccGenMode = 0;	//256 bytes boundary
	ecc_ctl.bf.eccEn = 1;
	REG32(FLASH_NF_ECC_CONTROL) =  ecc_ctl.wrd;
#endif

	//set oob size
	nf_cnt.wrd = 0;
	nf_cnt.bf.nflashRegOobCount = CONFIG_SYS_NAND_OOBSIZE - 1;
	REG32(FLASH_NF_COUNT) =  nf_cnt.wrd;

	//set direct access nflashExtAddr, set and let HW switch access area window
	REG32(FLASH_NF_ACCESS) = 0;

	//read page data
	memcpy(dst, page_add, CONFIG_SYS_NAND_PAGE_SIZE);

	//read oob : assume access with page alignment
	memcpy(oob_data, (page_add + CONFIG_SYS_NAND_PAGE_SIZE), CONFIG_SYS_NAND_OOBSIZE);

#ifdef CONFIG_CS752X_NAND_ECC_HW_BCH_8_512
	bch_sts_msk.wrd = 0;
	bch_sts_msk.bf.bchGenDone = 1;
	if (!reg_wait(FLASH_NF_BCH_STATUS, bch_sts_msk.wrd, bch_sts_msk.wrd, NF_START_BCNT))
		goto fail;
#else
	ecc_sts_msk.wrd = 0;
	ecc_sts_msk.bf.eccDone = 1;
	if (!reg_wait(FLASH_NF_ECC_STATUS, ecc_sts_msk.wrd, ecc_sts_msk.wrd, NF_START_BCNT))
		goto fail;

	//diable ecc and make sure in correct ecc mode
	ecc_ctl.wrd = REG32(FLASH_NF_ECC_CONTROL);
	ecc_ctl.bf.eccEn= 0;
	REG32(FLASH_NF_ECC_CONTROL) =  ecc_ctl.wrd;  //disable ecc
#endif

	/* Pick the ECC bytes out of the oob data */
	for (i = 0; i < ECCTOTAL; i++)
		ecc_code[i] = oob_data[nand_ecc_pos[i]];

	for (i = 0, k = 0; i < ECCTOTAL; i += CONFIG_SYS_NAND_ECCBYTES, k++) {
#ifdef CONFIG_CS752X_NAND_ECC_HW_BCH_8_512
		bch_oob0.wrd = ecc_code[i]   | ecc_code[i+1]<<8 | ecc_code[i+2]<<16  | ecc_code[i+3]<<24;
		bch_oob1.wrd = ecc_code[i+4] | ecc_code[i+5]<<8 | ecc_code[i+6]<<16  | ecc_code[i+7]<<24;
		bch_oob2.wrd = ecc_code[i+8] | ecc_code[i+9]<<8 | ecc_code[i+10]<<16 | ecc_code[i+11]<<24;
		bch_oob3.wrd = ecc_code[i+12];
		REG32(FLASH_NF_BCH_OOB0) = bch_oob0.wrd;
		REG32(FLASH_NF_BCH_OOB1) = bch_oob1.wrd;
		REG32(FLASH_NF_BCH_OOB2) = bch_oob2.wrd;
		REG32(FLASH_NF_BCH_OOB3) = bch_oob3.wrd;


		//enable ecc compare
		bch_ctrl.wrd = REG32(FLASH_NF_BCH_CONTROL);
		bch_ctrl.bf.bchCodeSel = k;
		bch_ctrl.bf.bchCompare = 1;
		REG32(FLASH_NF_BCH_CONTROL) = bch_ctrl.wrd;
		bch_sts.wrd=REG32(FLASH_NF_BCH_STATUS);


		bch_sts_msk.wrd = 0;
		bch_sts_msk.bf.bchDecDone = 1;
		if (!reg_wait(FLASH_NF_BCH_STATUS, bch_sts_msk.wrd, bch_sts_msk.wrd, NF_START_BCNT))
			goto fail;
		bch_sts.wrd=REG32(FLASH_NF_BCH_STATUS);

		switch(bch_sts.bf.bchDecStatus) {
		case BCH_CORRECTABLE_ERR:
			for(j=0;j<((bch_sts.bf.bchErrNum+1)/2);j++) {
				bch_err_loc01.wrd = REG32(FLASH_NF_BCH_ERROR_LOC01 + j*4);
				errloc0 = (bch_err_loc01.bf.bchErrLoc0 & 0x1fff) >> 3;
				errloc1 = (bch_err_loc01.bf.bchErrLoc1 & 0x1fff) >> 3;

				if ( (j+1)*2 <= bch_sts.bf.bchErrNum ) {
					if( errloc1 < 0x200) {
						p[errloc1] ^= (1 << (bch_err_loc01.bf.bchErrLoc1 & 0x07));
					}
				}

				if (errloc0 < 0x200) {
					p[errloc0] ^= (1 << (bch_err_loc01.bf.bchErrLoc0 & 0x07));
				}
			}
			break;
		case BCH_UNCORRECTABLE:
			goto fail;
		}
		bch_ctrl.wrd = REG32(FLASH_NF_BCH_CONTROL);
		bch_ctrl.bf.bchCompare = 0;
		REG32(FLASH_NF_BCH_CONTROL) =  bch_ctrl.wrd;
#else
		ecc_oob.wrd = ecc_code[i] | ecc_code[i+1]<<8 | ecc_code[i+2]<<16;
		REG32(FLASH_NF_ECC_OOB) =  ecc_oob.wrd;
		ecc_ctl.wrd = REG32(FLASH_NF_ECC_CONTROL);
		ecc_ctl.bf.eccCodeSel = k;
		REG32(FLASH_NF_ECC_CONTROL) = ecc_ctl.wrd;
		ecc_sts.wrd = REG32(FLASH_NF_ECC_STATUS);
		switch(ecc_sts.bf.eccStatus) {
		case ECC_1BIT_DATA_ERR:
			p[ecc_sts.bf.eccErrByte] ^= (1 << ecc_sts.bf.eccErrBit);
			break;
		case ECC_UNCORRECTABLE:
			goto fail;
		}
#endif
		p += CONFIG_SYS_NAND_ECCSIZE;
	}

#ifdef CONFIG_CS752X_NAND_ECC_HW_BCH_8_512
	REG32(FLASH_NF_BCH_CONTROL) = 0;
#endif

	return 1;
fail:

#ifdef CONFIG_CS752X_NAND_ECC_HW_BCH_8_512
	//diasble bch
	REG32(FLASH_NF_BCH_CONTROL) =  0;
#else
	//diasble ecc and make sure in correct ecc mode
	REG32(FLASH_NF_ECC_CONTROL) =  0;
#endif

	flash_reset();

	return 0;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dst)
{
	unsigned int block, lastblock;
	unsigned int page;

	/*
	 * offs has to be aligned to a page address!
	 */
	block = offs / CONFIG_SYS_NAND_BLOCK_SIZE;
	lastblock = (offs + size - 1) / CONFIG_SYS_NAND_BLOCK_SIZE;
	page = (offs % CONFIG_SYS_NAND_BLOCK_SIZE) / CONFIG_SYS_NAND_PAGE_SIZE;

	while (block <= lastblock) {
		if (!nand_is_bad_block(block)) {
			/*
			 * Skip bad blocks
			 */
			while (page < CONFIG_SYS_NAND_PAGE_COUNT) {
				nand_read_page(block, page, dst);
				dst += CONFIG_SYS_NAND_PAGE_SIZE;
				page++;
			}

			page = 0;
		} else {
			lastblock++;
		}

		block++;
	}

	return 0;
}

/* nand_init() - initialize data to make nand usable by SPL */
void nand_init(void)
{
	/*
	 * Init board specific nand support
	 */
	mtd.priv = &nand_chip;
	nand_chip.IO_ADDR_R = nand_chip.IO_ADDR_W =
		(void  __iomem *)CONFIG_SYS_NAND_BASE;

	flash_reset();

	if (nand_chip.select_chip)
		nand_chip.select_chip(&mtd, 0);
}

/* Unselect after operation */
void nand_deselect(void)
{
	if (nand_chip.select_chip)
		nand_chip.select_chip(&mtd, -1);
}
