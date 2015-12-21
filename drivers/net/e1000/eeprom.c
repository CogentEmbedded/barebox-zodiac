#include <common.h>
#include <init.h>
#include <net.h>
#include <malloc.h>
#include <linux/math64.h>
#include <linux/sizes.h>

#include "e1000.h"

static void e1000_release_eeprom_spi(struct e1000_hw *hw);
static int32_t e1000_read_eeprom_spi(struct e1000_hw *hw, uint16_t offset,
				     uint16_t words, uint16_t *data);
static void e1000_release_eeprom_microwire(struct e1000_hw *hw);
static int32_t e1000_read_eeprom_microwire(struct e1000_hw *hw, uint16_t offset,
					   uint16_t words, uint16_t *data);

static int32_t e1000_read_eeprom_eerd(struct e1000_hw *hw, uint16_t offset,
				      uint16_t words, uint16_t *data);
static int32_t e1000_spi_eeprom_ready(struct e1000_hw *hw);
static void e1000_release_eeprom(struct e1000_hw *hw);
static int32_t e1000_acquire_eeprom_flash(struct e1000_hw *hw);
static void e1000_release_eeprom_flash(struct e1000_hw *hw);


/******************************************************************************
 * Raises the EEPROM's clock input.
 *
 * hw - Struct containing variables accessed by shared code
 * eecd - EECD's current value
 *****************************************************************************/
static void e1000_raise_ee_clk(struct e1000_hw *hw, uint32_t *eecd)
{
	/* Raise the clock input to the EEPROM (by setting the SK bit), and then
	 * wait 50 microseconds.
	 */
	*eecd = *eecd | E1000_EECD_SK;
	E1000_WRITE_REG(hw, EECD, *eecd);
	E1000_WRITE_FLUSH(hw);
	udelay(50);
}

/******************************************************************************
 * Lowers the EEPROM's clock input.
 *
 * hw - Struct containing variables accessed by shared code
 * eecd - EECD's current value
 *****************************************************************************/
static void e1000_lower_ee_clk(struct e1000_hw *hw, uint32_t *eecd)
{
	/* Lower the clock input to the EEPROM (by clearing the SK bit), and then
	 * wait 50 microseconds.
	 */
	*eecd = *eecd & ~E1000_EECD_SK;
	E1000_WRITE_REG(hw, EECD, *eecd);
	E1000_WRITE_FLUSH(hw);
	udelay(50);
}

/******************************************************************************
 * Shift data bits out to the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 * data - data to send to the EEPROM
 * count - number of bits to shift out
 *****************************************************************************/
static void e1000_shift_out_ee_bits(struct e1000_hw *hw, uint16_t data, uint16_t count)
{
	uint32_t eecd;
	uint32_t mask;

	/* We need to shift "count" bits out to the EEPROM. So, value in the
	 * "data" parameter will be shifted out to the EEPROM one bit at a time.
	 * In order to do this, "data" must be broken down into bits.
	 */
	mask = 0x01 << (count - 1);
	eecd = E1000_READ_REG(hw, EECD);
	eecd &= ~(E1000_EECD_DO | E1000_EECD_DI);
	do {
		/* A "1" is shifted out to the EEPROM by setting bit "DI" to a "1",
		 * and then raising and then lowering the clock (the SK bit controls
		 * the clock input to the EEPROM).  A "0" is shifted out to the EEPROM
		 * by setting "DI" to "0" and then raising and then lowering the clock.
		 */
		eecd &= ~E1000_EECD_DI;

		if (data & mask)
			eecd |= E1000_EECD_DI;

		E1000_WRITE_REG(hw, EECD, eecd);
		E1000_WRITE_FLUSH(hw);

		udelay(50);

		e1000_raise_ee_clk(hw, &eecd);
		e1000_lower_ee_clk(hw, &eecd);

		mask = mask >> 1;

	} while (mask);

	/* We leave the "DI" bit set to "0" when we leave this routine. */
	eecd &= ~E1000_EECD_DI;
	E1000_WRITE_REG(hw, EECD, eecd);
}

/******************************************************************************
 * Shift data bits in from the EEPROM
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static uint16_t e1000_shift_in_ee_bits(struct e1000_hw *hw, uint16_t count)
{
	uint32_t eecd;
	uint32_t i;
	uint16_t data;

	/* In order to read a register from the EEPROM, we need to shift 'count'
	 * bits in from the EEPROM. Bits are "shifted in" by raising the clock
	 * input to the EEPROM (setting the SK bit), and then reading the
	 * value of the "DO" bit.  During this "shifting in" process the
	 * "DI" bit should always be clear.
	 */

	eecd = E1000_READ_REG(hw, EECD);

	eecd &= ~(E1000_EECD_DO | E1000_EECD_DI);
	data = 0;

	for (i = 0; i < count; i++) {
		data = data << 1;
		e1000_raise_ee_clk(hw, &eecd);

		eecd = E1000_READ_REG(hw, EECD);

		eecd &= ~(E1000_EECD_DI);
		if (eecd & E1000_EECD_DO)
			data |= 1;

		e1000_lower_ee_clk(hw, &eecd);
	}

	return data;
}

/******************************************************************************
 * Returns EEPROM to a "standby" state
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static void e1000_standby_eeprom(struct e1000_hw *hw)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	uint32_t eecd;

	eecd = E1000_READ_REG(hw, EECD);

	if (eeprom->type == e1000_eeprom_microwire) {
		eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
		E1000_WRITE_REG(hw, EECD, eecd);
		E1000_WRITE_FLUSH(hw);
		udelay(eeprom->delay_usec);

		/* Clock high */
		eecd |= E1000_EECD_SK;
		E1000_WRITE_REG(hw, EECD, eecd);
		E1000_WRITE_FLUSH(hw);
		udelay(eeprom->delay_usec);

		/* Select EEPROM */
		eecd |= E1000_EECD_CS;
		E1000_WRITE_REG(hw, EECD, eecd);
		E1000_WRITE_FLUSH(hw);
		udelay(eeprom->delay_usec);

		/* Clock low */
		eecd &= ~E1000_EECD_SK;
		E1000_WRITE_REG(hw, EECD, eecd);
		E1000_WRITE_FLUSH(hw);
		udelay(eeprom->delay_usec);
	} else if (eeprom->type == e1000_eeprom_spi) {
		/* Toggle CS to flush commands */
		eecd |= E1000_EECD_CS;
		E1000_WRITE_REG(hw, EECD, eecd);
		E1000_WRITE_FLUSH(hw);
		udelay(eeprom->delay_usec);
		eecd &= ~E1000_EECD_CS;
		E1000_WRITE_REG(hw, EECD, eecd);
		E1000_WRITE_FLUSH(hw);
		udelay(eeprom->delay_usec);
	}
}

/***************************************************************************
* Description:     Determines if the onboard NVM is FLASH or EEPROM.
*
* hw - Struct containing variables accessed by shared code
****************************************************************************/
static bool e1000_is_onboard_nvm_eeprom(struct e1000_hw *hw)
{
	uint32_t eecd = 0;

	DEBUGFUNC();

	if (hw->mac_type == e1000_ich8lan)
		return false;

	if (hw->mac_type == e1000_82573 || hw->mac_type == e1000_82574) {
		eecd = E1000_READ_REG(hw, EECD);

		/* Isolate bits 15 & 16 */
		eecd = ((eecd >> 15) & 0x03);

		/* If both bits are set, device is Flash type */
		if (eecd == 0x03)
			return false;
	}
	return true;
}

static int32_t
e1000_acquire_eeprom_spi_microwire_prologue(struct e1000_hw *hw)
{
	uint32_t eecd;

	if (e1000_swfw_sync_acquire(hw, E1000_SWFW_EEP_SM))
		return -E1000_ERR_SWFW_SYNC;

	eecd = E1000_READ_REG(hw, EECD);

	/* Request EEPROM Access */
	if (hw->mac_type > e1000_82544  &&
	    hw->mac_type != e1000_82573 &&
	    hw->mac_type != e1000_82574) {
		int i = 0;

		eecd |= E1000_EECD_REQ;
		E1000_WRITE_REG(hw, EECD, eecd);
		eecd = E1000_READ_REG(hw, EECD);
		while ((!(eecd & E1000_EECD_GNT)) &&
			(i < E1000_EEPROM_GRANT_ATTEMPTS)) {
			i++;
			udelay(5);
			eecd = E1000_READ_REG(hw, EECD);
		}
		if (!(eecd & E1000_EECD_GNT)) {
			eecd &= ~E1000_EECD_REQ;
			E1000_WRITE_REG(hw, EECD, eecd);
			dev_dbg(hw->dev, "Could not acquire EEPROM grant\n");
			return -E1000_ERR_EEPROM;
		}
	}

	return E1000_SUCCESS;
}

static int32_t e1000_acquire_eeprom_spi(struct e1000_hw *hw)
{
	int32_t ret;
	uint32_t eecd;

	ret = e1000_acquire_eeprom_spi_microwire_prologue(hw);
	if (ret != E1000_SUCCESS)
		return ret;

	eecd = E1000_READ_REG(hw, EECD);

	/* Clear SK and CS */
	eecd &= ~(E1000_EECD_CS | E1000_EECD_SK);
	E1000_WRITE_REG(hw, EECD, eecd);
	udelay(1);

	return E1000_SUCCESS;
}

static int32_t e1000_acquire_eeprom_microwire(struct e1000_hw *hw)
{
	int ret;
	uint32_t eecd;

	ret = e1000_acquire_eeprom_spi_microwire_prologue(hw);
	if (ret != E1000_SUCCESS)
		return ret;

	eecd = E1000_READ_REG(hw, EECD);
	/* Clear SK and DI */
	eecd &= ~(E1000_EECD_DI | E1000_EECD_SK);
	E1000_WRITE_REG(hw, EECD, eecd);

	/* Set CS */
	eecd |= E1000_EECD_CS;
	E1000_WRITE_REG(hw, EECD, eecd);

	return E1000_SUCCESS;
}

static int32_t e1000_acquire_eeprom_flash(struct e1000_hw *hw)
{
	return e1000_swfw_sync_acquire(hw, E1000_SWFW_EEP_SM);
}

static int32_t e1000_acquire_eeprom(struct e1000_hw *hw)
{
	if (hw->eeprom.acquire)
		return hw->eeprom.acquire(hw);
	else
		return E1000_SUCCESS;
}

static void e1000_eeprom_uses_spi(struct e1000_eeprom_info *eeprom,
				  uint32_t eecd)
{
	eeprom->type = e1000_eeprom_spi;
	eeprom->opcode_bits = 8;
	eeprom->delay_usec = 1;
	if (eecd & E1000_EECD_ADDR_BITS) {
		eeprom->address_bits = 16;
	} else {
		eeprom->address_bits = 8;
	}

	eeprom->acquire = e1000_acquire_eeprom_spi;
	eeprom->release = e1000_release_eeprom_spi;
	eeprom->read = e1000_read_eeprom_spi;
}

static void e1000_eeprom_uses_microwire(struct e1000_eeprom_info *eeprom,
					uint32_t eecd)
{
	eeprom->type = e1000_eeprom_microwire;
	eeprom->opcode_bits = 3;
	eeprom->delay_usec = 50;
	if (eecd & E1000_EECD_SIZE) {
		eeprom->word_size = 256;
		eeprom->address_bits = 8;
	} else {
		eeprom->word_size = 64;
		eeprom->address_bits = 6;
	}

	eeprom->acquire = e1000_acquire_eeprom_microwire;
	eeprom->release = e1000_release_eeprom_microwire;
	eeprom->read = e1000_read_eeprom_microwire;
}


/******************************************************************************
 * Sets up eeprom variables in the hw struct.  Must be called after mac_type
 * is configured.  Additionally, if this is ICH8, the flash controller GbE
 * registers must be mapped, or this will crash.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
int32_t e1000_init_eeprom_params(struct e1000_hw *hw)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	uint32_t eecd;
	int32_t ret_val = E1000_SUCCESS;
	uint16_t eeprom_size;

	eecd = E1000_READ_REG(hw, EECD);

	DEBUGFUNC();

	switch (hw->mac_type) {
	case e1000_82542_rev2_0:
	case e1000_82542_rev2_1:
	case e1000_82543:
	case e1000_82544:
		e1000_eeprom_uses_microwire(eeprom, 0);
	break;
	case e1000_82540:
	case e1000_82545:
	case e1000_82545_rev_3:
	case e1000_82546:
	case e1000_82546_rev_3:
		e1000_eeprom_uses_microwire(eeprom, eecd);
		break;
	case e1000_82541:
	case e1000_82541_rev_2:
	case e1000_82547:
	case e1000_82547_rev_2:
		if (eecd & E1000_EECD_TYPE)
			e1000_eeprom_uses_spi(eeprom, eecd);
		else
			e1000_eeprom_uses_microwire(eeprom, eecd);
		break;
	case e1000_82571:
	case e1000_82572:
		e1000_eeprom_uses_spi(eeprom, eecd);
		break;
	case e1000_82573:
	case e1000_82574:
		if (e1000_is_onboard_nvm_eeprom(hw)) {
			e1000_eeprom_uses_spi(eeprom, eecd);
		} else {
			eeprom->read = e1000_read_eeprom_eerd;
			eeprom->type = e1000_eeprom_flash;
			eeprom->word_size = 2048;

		/* Ensure that the Autonomous FLASH update bit is cleared due to
		 * Flash update issue on parts which use a FLASH for NVM. */
			eecd &= ~E1000_EECD_AUPDEN;
			E1000_WRITE_REG(hw, EECD, eecd);
		}
		break;
	case e1000_80003es2lan:
		eeprom->type = e1000_eeprom_spi;
		eeprom->read = e1000_read_eeprom_eerd;
		break;
	case e1000_igb:
		if (eecd & E1000_EECD_I210_FLASH_DETECTED) {
			uint32_t fla;

			fla  = E1000_READ_REG(hw, FLA);
			fla &= E1000_FLA_FL_SIZE_MASK;
			fla >>= E1000_FLA_FL_SIZE_SHIFT;

			switch (fla) {
			case E1000_FLA_FL_SIZE_8MB:
				eeprom->word_size = SZ_8M / 2;
				break;
			case E1000_FLA_FL_SIZE_4MB:
				eeprom->word_size = SZ_4M / 2;
				break;
			case E1000_FLA_FL_SIZE_2MB:
				eeprom->word_size = SZ_2M / 2;
				break;
			default:
				eeprom->word_size = 2048;
				dev_info(hw->dev, "Unprogrammed Flash detected, "
					 "limiting access to first 4KB\n");
			}

			eeprom->type = e1000_eeprom_flash;
			eeprom->acquire = e1000_acquire_eeprom_flash;
			eeprom->release = e1000_release_eeprom_flash;
		} else {
			eeprom->type = e1000_eeprom_invm;
		}

		eeprom->read = e1000_read_eeprom_eerd;
		break;
	default:
		break;
	}

	if (eeprom->type == e1000_eeprom_spi ||
	    eeprom->type == e1000_eeprom_invm) {
		/* eeprom_size will be an enum [0..8] that maps
		 * to eeprom sizes 128B to
		 * 32KB (incremented by powers of 2).
		 */
		if (hw->mac_type <= e1000_82547_rev_2) {
			/* Set to default value for initial eeprom read. */
			eeprom->word_size = 64;
			ret_val = e1000_read_eeprom(hw, EEPROM_CFG, 1,
					&eeprom_size);
			if (ret_val)
				return ret_val;
			eeprom_size = (eeprom_size & EEPROM_SIZE_MASK)
				>> EEPROM_SIZE_SHIFT;
			/* 256B eeprom size was not supported in earlier
			 * hardware, so we bump eeprom_size up one to
			 * ensure that "1" (which maps to 256B) is never
			 * the result used in the shifting logic below. */
			if (eeprom_size)
				eeprom_size++;
		} else {
			eeprom_size = (uint16_t)((eecd &
				E1000_EECD_SIZE_EX_MASK) >>
				E1000_EECD_SIZE_EX_SHIFT);
		}

		eeprom->word_size = 1 << (eeprom_size + EEPROM_WORD_SIZE_SHIFT);
	}
	return ret_val;
}

/******************************************************************************
 * Polls the status bit (bit 1) of the EERD to determine when the read is done.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static int32_t e1000_poll_eerd_eewr_done(struct e1000_hw *hw, int eerd)
{
	uint32_t attempts = 100000;
	uint32_t i, reg = 0;
	int32_t done = E1000_ERR_EEPROM;

	for (i = 0; i < attempts; i++) {
		if (eerd == E1000_EEPROM_POLL_READ)
			reg = E1000_READ_REG(hw, EERD);
		else
			reg = E1000_READ_REG(hw, EEWR);

		if (reg & E1000_EEPROM_RW_REG_DONE) {
			done = E1000_SUCCESS;
			break;
		}
		udelay(5);
	}

	return done;
}

/******************************************************************************
 * Reads a 16 bit word from the EEPROM using the EERD register.
 *
 * hw - Struct containing variables accessed by shared code
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 * words - number of words to read
 *****************************************************************************/
static int32_t e1000_read_eeprom_eerd(struct e1000_hw *hw,
			uint16_t offset,
			uint16_t words,
			uint16_t *data)
{
	uint32_t i, eerd = 0;
	int32_t error = 0;

	for (i = 0; i < words; i++) {
		eerd = ((offset+i) << E1000_EEPROM_RW_ADDR_SHIFT) +
			E1000_EEPROM_RW_REG_START;

		E1000_WRITE_REG(hw, EERD, eerd);

		error = e1000_poll_eerd_eewr_done(hw, E1000_EEPROM_POLL_READ);

		if (error)
			break;

		data[i] = (E1000_READ_REG(hw, EERD) >>
			   E1000_EEPROM_RW_REG_DATA);
	}

	return error;
}

static int32_t e1000_read_eeprom_spi(struct e1000_hw *hw,
				     uint16_t offset,
				     uint16_t words,
				     uint16_t *data)
{
	unsigned int i;
	uint16_t word_in;
	uint8_t read_opcode = EEPROM_READ_OPCODE_SPI;

	if (e1000_spi_eeprom_ready(hw)) {
		e1000_release_eeprom(hw);
		return -E1000_ERR_EEPROM;
	}

	e1000_standby_eeprom(hw);

	/* Some SPI eeproms use the 8th address bit embedded in
	 * the opcode */
	if ((hw->eeprom.address_bits == 8) && (offset >= 128))
		read_opcode |= EEPROM_A8_OPCODE_SPI;

	/* Send the READ command (opcode + addr)  */
	e1000_shift_out_ee_bits(hw, read_opcode, hw->eeprom.opcode_bits);
	e1000_shift_out_ee_bits(hw, (uint16_t)(offset * 2),
				hw->eeprom.address_bits);

	/* Read the data.  The address of the eeprom internally
	 * increments with each byte (spi) being read, saving on the
	 * overhead of eeprom setup and tear-down.  The address
	 * counter will roll over if reading beyond the size of
	 * the eeprom, thus allowing the entire memory to be read
	 * starting from any offset. */
	for (i = 0; i < words; i++) {
		word_in = e1000_shift_in_ee_bits(hw, 16);
		data[i] = (word_in >> 8) | (word_in << 8);
	}

	return E1000_SUCCESS;
}

static int32_t e1000_read_eeprom_microwire(struct e1000_hw *hw,
					   uint16_t offset,
					   uint16_t words,
					   uint16_t *data)
{
	int i;
	for (i = 0; i < words; i++) {
		/* Send the READ command (opcode + addr)  */
		e1000_shift_out_ee_bits(hw,
					EEPROM_READ_OPCODE_MICROWIRE,
					hw->eeprom.opcode_bits);
		e1000_shift_out_ee_bits(hw, (uint16_t)(offset + i),
					hw->eeprom.address_bits);

		/* Read the data.  For microwire, each word requires
		 * the overhead of eeprom setup and tear-down. */
		data[i] = e1000_shift_in_ee_bits(hw, 16);
		e1000_standby_eeprom(hw);
	}

	return E1000_SUCCESS;
}

static void
e1000_release_eeprom_spi_microwire_epilogue(struct e1000_hw *hw)
{
	uint32_t eecd = E1000_READ_REG(hw, EECD);

	/* Stop requesting EEPROM access */
	if (hw->mac_type > e1000_82544) {
		eecd &= ~E1000_EECD_REQ;
		E1000_WRITE_REG(hw, EECD, eecd);
	}
}

static void e1000_release_eeprom_microwire(struct e1000_hw *hw)
{
	uint32_t eecd = E1000_READ_REG(hw, EECD);

	/* cleanup eeprom */

	/* CS on Microwire is active-high */
	eecd &= ~(E1000_EECD_CS | E1000_EECD_DI);

	E1000_WRITE_REG(hw, EECD, eecd);

	/* Rising edge of clock */
	eecd |= E1000_EECD_SK;
	E1000_WRITE_REG(hw, EECD, eecd);
	E1000_WRITE_FLUSH(hw);
	udelay(hw->eeprom.delay_usec);

	/* Falling edge of clock */
	eecd &= ~E1000_EECD_SK;
	E1000_WRITE_REG(hw, EECD, eecd);
	E1000_WRITE_FLUSH(hw);
	udelay(hw->eeprom.delay_usec);


	e1000_release_eeprom_spi_microwire_epilogue(hw);
}

static void e1000_release_eeprom_spi(struct e1000_hw *hw)
{
	uint32_t eecd = E1000_READ_REG(hw, EECD);

	eecd |= E1000_EECD_CS;  /* Pull CS high */
	eecd &= ~E1000_EECD_SK; /* Lower SCK */

	E1000_WRITE_REG(hw, EECD, eecd);
	udelay(hw->eeprom.delay_usec);

	e1000_release_eeprom_spi_microwire_epilogue(hw);
}

static void e1000_release_eeprom_flash(struct e1000_hw *hw)
{
	if (e1000_swfw_sync_release(hw, E1000_SWFW_EEP_SM) < 0)
		dev_warn(hw->dev,
			 "Timeout while releasing SWFW_SYNC bits (0x%08x)\n",
			 E1000_SWFW_EEP_SM);
}

static void e1000_release_eeprom(struct e1000_hw *hw)
{
	if (hw->eeprom.release)
		hw->eeprom.release(hw);
}

/******************************************************************************
 * Reads a 16 bit word from the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static int32_t e1000_spi_eeprom_ready(struct e1000_hw *hw)
{
	uint16_t retry_count = 0;
	uint8_t spi_stat_reg;

	DEBUGFUNC();

	/* Read "Status Register" repeatedly until the LSB is cleared.  The
	 * EEPROM will signal that the command has been completed by clearing
	 * bit 0 of the internal status register.  If it's not cleared within
	 * 5 milliseconds, then error out.
	 */
	retry_count = 0;
	do {
		e1000_shift_out_ee_bits(hw, EEPROM_RDSR_OPCODE_SPI,
			hw->eeprom.opcode_bits);
		spi_stat_reg = (uint8_t)e1000_shift_in_ee_bits(hw, 8);
		if (!(spi_stat_reg & EEPROM_STATUS_RDY_SPI))
			break;

		udelay(5);
		retry_count += 5;

		e1000_standby_eeprom(hw);
	} while (retry_count < EEPROM_MAX_RETRY_SPI);

	/* ATMEL SPI write time could vary from 0-20mSec on 3.3V devices (and
	 * only 0-5mSec on 5V devices)
	 */
	if (retry_count >= EEPROM_MAX_RETRY_SPI) {
		dev_dbg(hw->dev, "SPI EEPROM Status error\n");
		return -E1000_ERR_EEPROM;
	}

	return E1000_SUCCESS;
}

static int e1000_flash_mode_wait_for_idle(struct e1000_hw *hw)
{
	/* Strictly speaking we need to poll FLSWCTL.DONE only if we
	 * are executing this code after a reset event, but it
	 * shouldn't hurt to do this everytime, besided we need to
	 * poll got FLSWCTL.GLDONE to make sure that back to back
	 * calls to that function work correctly, since we finish
	 * execution by polling only FLSWCTL.DONE */

	const int ret = E1000_POLL_REG(hw, FLSWCTL,
				       E1000_FLSWCTL_DONE | E1000_FLSWCTL_GLDONE,
				       E1000_FLSWCTL_DONE | E1000_FLSWCTL_GLDONE,
				       SECOND);
	if (ret < 0)
		dev_err(hw->dev,
			"Timeout waiting for FLSWCTL.DONE to be set\n");
	return ret;
}

static int e1000_flash_mode_check_command_valid(struct e1000_hw *hw)
{
	const uint32_t flswctl = E1000_READ_REG(hw, FLSWCTL);
	if (!(flswctl & E1000_FLSWCTL_CMDV)) {
		dev_err(hw->dev, "FLSWCTL.CMDV was cleared");
		return -EIO;
	}

	return E1000_SUCCESS;
}

#define E1000_FLASH_CMD(hw, cmd, offset)				\
	do {								\
		uint32_t ___flswctl = E1000_READ_REG(hw, FLSWCTL);	\
		___flswctl &= ~E1000_FLSWCTL_CMD_ADDR_MASK;		\
		___flswctl |= E1000_FLSWCTL_CMD(cmd) | E1000_FLSWCTL_ADDR(offset); \
		E1000_WRITE_REG(hw, FLSWCTL, ___flswctl);		\
	} while (0)

static int e1000_flash_mode_read_chunk(struct e1000_hw *hw, loff_t offset,
				       size_t size, void *data)
{
	int ret;
	size_t chunk, residue = size;
	uint32_t flswdata;

	DEBUGFUNC();

	if (size > SZ_4K ||
	    E1000_FLSWCTL_ADDR(offset) != offset)
		return -EINVAL;

	ret = e1000_flash_mode_wait_for_idle(hw);
	if (ret < 0)
		return ret;

	E1000_WRITE_REG(hw, FLSWCNT, size);
	E1000_FLASH_CMD(hw, READ, offset);

	do {
		ret = e1000_flash_mode_check_command_valid(hw);
		if (ret < 0)
			return -EIO;

		chunk = min(sizeof(flswdata), residue);

		ret = E1000_POLL_REG(hw, FLSWCTL,
				     E1000_FLSWCTL_DONE, E1000_FLSWCTL_DONE,
				     SECOND);
		if (ret < 0) {
			dev_err(hw->dev,
				"Timeout waiting for FLSWCTL.DONE to be set\n");
			return ret;
		}

		flswdata = E1000_READ_REG(hw, FLSWDATA);

		printf("%x %d\n", flswdata, residue);

		flswdata = cpu_to_le32(flswdata);
		memcpy(data, &flswdata, chunk);

		data += chunk;
		residue -= chunk;
	} while (residue);

	return E1000_SUCCESS;
}

static int e1000_flash_mode_write_chunk(struct e1000_hw *hw, loff_t offset,
					size_t size, const void *data)
{
	int ret;
	size_t chunk, residue = size;
	uint32_t flswdata;

	if (size > 256 ||
	    E1000_FLSWCTL_ADDR(offset) != offset)
		return -EINVAL;

	ret = e1000_flash_mode_wait_for_idle(hw);
	if (ret < 0)
		return ret;


	E1000_WRITE_REG(hw, FLSWCNT, size);
	E1000_FLASH_CMD(hw, WRITE, offset);

	do {
		chunk = min(sizeof(flswdata), residue);
		memcpy(&flswdata, data, chunk);
		/*
		 */
		flswdata = le32_to_cpu(flswdata);

		E1000_WRITE_REG(hw, FLSWDATA, flswdata);

		ret = e1000_flash_mode_check_command_valid(hw);
		if (ret < 0)
			return -EIO;

		ret = E1000_POLL_REG(hw, FLSWCTL,
				     E1000_FLSWCTL_DONE, E1000_FLSWCTL_DONE,
				     SECOND);
		if (ret < 0) {
			dev_err(hw->dev,
				"Timeout waiting for FLSWCTL.DONE to be set\n");
			return ret;
		}

		data += chunk;
		residue -= chunk;

	} while (residue);

	return E1000_SUCCESS;
}


static int e1000_flash_mode_erase_chunk(struct e1000_hw *hw, loff_t offset,
					size_t size)
{
	int ret;

	printf("%s %lld %d\n", __func__, offset, size);

	ret = e1000_flash_mode_wait_for_idle(hw);
	if (ret < 0)
		return ret;

	if (!size && !offset)
		E1000_FLASH_CMD(hw, ERASE_DEVICE, 0);
	else
		E1000_FLASH_CMD(hw, ERASE_SECTOR, offset);

	ret = e1000_flash_mode_check_command_valid(hw);
	if (ret < 0)
		return -EIO;

	ret = E1000_POLL_REG(hw, FLSWCTL,
			     E1000_FLSWCTL_DONE | E1000_FLSWCTL_FLBUSY,
			     E1000_FLSWCTL_DONE,
			     SECOND);
	if (ret < 0) {
		dev_err(hw->dev,
			"Timeout waiting for FLSWCTL.DONE to be set\n");
		return ret;
	}

	return E1000_SUCCESS;
}

enum {
	E1000_FLASH_MODE_OP_READ = 0,
	E1000_FLASH_MODE_OP_WRITE = 1,
	E1000_FLASH_MODE_OP_ERASE = 2,
};


static int e1000_flash_mode_io(struct e1000_hw *hw, int op, size_t granularity,
			       loff_t offset, size_t size, void *data)
{
	int ret;
	size_t residue = size;

	do {
		const size_t chunk = min(granularity, residue);

		switch (op) {
		case E1000_FLASH_MODE_OP_READ:
			ret = e1000_flash_mode_read_chunk(hw, offset,
							  chunk, data);
			break;
		case E1000_FLASH_MODE_OP_WRITE:
			ret = e1000_flash_mode_write_chunk(hw, offset,
							   chunk, data);
			break;
		case E1000_FLASH_MODE_OP_ERASE:
			ret = e1000_flash_mode_erase_chunk(hw, offset,
							   chunk);
			break;
		default:
			return -ENOTSUPP;
		}

		if (ret < 0)
			return ret;

		offset += chunk;
		residue -= chunk;
		data += chunk;
	} while (residue);

	return E1000_SUCCESS;
}


static int e1000_flash_mode_read(struct e1000_hw *hw, loff_t offset,
				 size_t size, void *data)
{
	return e1000_flash_mode_io(hw,
				   E1000_FLASH_MODE_OP_READ, SZ_4K,
				   offset, size, data);
}

static int e1000_flash_mode_write(struct e1000_hw *hw, loff_t offset,
				  size_t size, const void *data)
{
	int ret;

	ret = e1000_flash_mode_io(hw,
				  E1000_FLASH_MODE_OP_WRITE, 256,
				  offset, size, (void *)data);
	if (ret < 0)
		return ret;

	ret = E1000_POLL_REG(hw, FLSWCTL, E1000_FLSWCTL_FLBUSY,
			     0,  SECOND);
	if (ret < 0)
		dev_err(hw->dev, "Timout while waiting for FLSWCTL.FLBUSY\n");

	return ret;
}

static int e1000_flash_mode_erase(struct e1000_hw *hw, loff_t offset,
				  size_t size)
{
	return e1000_flash_mode_io(hw,
				   E1000_FLASH_MODE_OP_ERASE, SZ_4K,
				   offset, size, NULL);
}


/******************************************************************************
 * Reads a 16 bit word from the EEPROM.
 *
 * hw - Struct containing variables accessed by shared code
 * offset - offset of  word in the EEPROM to read
 * data - word read from the EEPROM
 *****************************************************************************/
int32_t e1000_read_eeprom(struct e1000_hw *hw, uint16_t offset,
			  uint16_t words, uint16_t *data)
{
	struct e1000_eeprom_info *eeprom = &hw->eeprom;
	int32_t ret;

	DEBUGFUNC();

	/* A check for invalid values:  offset too large, too many words,
	 * and not enough words.
	 */
	if ((offset >= eeprom->word_size) ||
		(words > eeprom->word_size - offset) ||
		(words == 0)) {
		dev_dbg(hw->dev, "\"words\" parameter out of bounds."
			"Words = %d, size = %d\n", offset, eeprom->word_size);
		return -E1000_ERR_EEPROM;
	}

	if (eeprom->read) {
		if (e1000_acquire_eeprom(hw) != E1000_SUCCESS)
			return -E1000_ERR_EEPROM;

		ret = eeprom->read(hw, offset, words, data);
		e1000_release_eeprom(hw);

		return ret;
	} else {
		return -ENOTSUPP;
	}
}

/******************************************************************************
 * Verifies that the EEPROM has a valid checksum
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Reads the first 64 16 bit words of the EEPROM and sums the values read.
 * If the the sum of the 64 16 bit words is 0xBABA, the EEPROM's checksum is
 * valid.
 *****************************************************************************/
int e1000_validate_eeprom_checksum(struct e1000_hw *hw)
{
	uint16_t i, checksum, checksum_reg;
	uint16_t buf[EEPROM_CHECKSUM_REG + 1];

	DEBUGFUNC();

	/*
	  Only the following three 'types' of EEPROM can be expected
	  to have correct EEPROM checksum
	*/
	if (hw->eeprom.type != e1000_eeprom_spi &&
	    hw->eeprom.type != e1000_eeprom_microwire &&
	    hw->eeprom.type != e1000_eeprom_flash)
		return 0;

	/* Read the EEPROM */
	if (e1000_read_eeprom(hw, 0, EEPROM_CHECKSUM_REG + 1, buf) < 0) {
		dev_err(&hw->edev.dev, "Unable to read EEPROM!\n");
		return -E1000_ERR_EEPROM;
	}

	/* Compute the checksum */
	checksum = 0;
	for (i = 0; i < EEPROM_CHECKSUM_REG; i++)
		checksum += buf[i];
	checksum = ((uint16_t)EEPROM_SUM) - checksum;
	checksum_reg = buf[i];

	/* Verify it! */
	if (checksum == checksum_reg)
		return 0;

	/* Hrm, verification failed, print an error */
	dev_err(&hw->edev.dev, "EEPROM checksum is incorrect!\n");
	dev_err(&hw->edev.dev, "  ...register was 0x%04hx, calculated 0x%04hx\n",
			checksum_reg, checksum);

	return -E1000_ERR_EEPROM;
}

static int e1000_mtd_read_or_write(bool read,
				   struct mtd_info *mtd, loff_t off, size_t len,
				   size_t *retlen, u_char *buf)
{
	int ret;
	struct e1000_hw *hw = container_of(mtd, struct e1000_hw, mtd);

	DEBUGFUNC();

	if (e1000_acquire_eeprom(hw) == E1000_SUCCESS) {
		if (read)
			ret = e1000_flash_mode_read(hw, off,
						    len, buf);
		else
			ret = e1000_flash_mode_write(hw, off,
						     len, buf);
		if (ret == E1000_SUCCESS)
			*retlen = len;

		e1000_release_eeprom(hw);
	} else {
		ret = -E1000_ERR_EEPROM;
	}

	return ret;

}

static int e1000_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
				 size_t *retlen, u_char *buf)
{
	return e1000_mtd_read_or_write(true,
				       mtd, from, len, retlen, buf);
}

static int e1000_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
			   size_t *retlen, const u_char *buf)
{
	return e1000_mtd_read_or_write(false,
				       mtd, to, len, retlen, (u_char *)buf);
}

static int e1000_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	uint32_t rem;
	struct e1000_hw *hw = container_of(mtd, struct e1000_hw, mtd);
	int ret;

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem)
		return -EINVAL;

	ret = e1000_acquire_eeprom(hw);
	if (ret != E1000_SUCCESS)
		goto fail;

	/*
	   If mtd->size is 4096 it means we are dealing with
	   unprogrammed flash and we don't really know it's size to
	   make an informed decision wheither to erase whole chip or
	   just a number of sectors
	 */
	if (mtd->size > SZ_4K &&
	    instr->len == mtd->size)
		ret = e1000_flash_mode_erase(hw, 0, 0);
	else
		ret = e1000_flash_mode_erase(hw,
					     instr->addr, instr->len);

	e1000_release_eeprom(hw);

	if (ret < 0)
		goto fail;

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;

fail:
	instr->state = MTD_ERASE_FAILED;
	return ret;
}

int e1000_register_eeprom(struct e1000_hw *hw)
{
	int ret;
	struct e1000_eeprom_info *eeprom = &hw->eeprom;

	switch (eeprom->type) {
	case e1000_eeprom_flash:
		if (hw->mac_type != e1000_igb)
			break;

		hw->mtd.parent = hw->dev;
		hw->mtd.read = e1000_mtd_read;
		hw->mtd.write = e1000_mtd_write;
		hw->mtd.erase = e1000_mtd_erase;
		hw->mtd.size = eeprom->word_size * 2;
		hw->mtd.writesize = 1;
		hw->mtd.subpage_sft = 0;

		hw->mtd.eraseregions = xzalloc(sizeof(struct mtd_erase_region_info));
		hw->mtd.erasesize = SZ_4K;
		hw->mtd.eraseregions[0].erasesize = SZ_4K;
		hw->mtd.eraseregions[0].numblocks = hw->mtd.size / SZ_4K;
		hw->mtd.numeraseregions = 1;

		hw->mtd.flags = MTD_CAP_NORFLASH;
		hw->mtd.type = MTD_NORFLASH;

		ret = add_mtd_device(&hw->mtd, "e1000-nor",
				     DEVICE_ID_DYNAMIC);
		if (ret)
			return ret;
		break;
	default:
		break;
	}

	return 0;
}
