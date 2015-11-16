#ifndef _PIC_H_
#define _PIC_H_

enum pic_hw_id {
	PIC_HW_ID_NIU,
	PIC_HW_ID_MEZZ,
	PIC_HW_ID_ESB,
	PIC_HW_ID_RDU,
	PIC_HW_ID_RDU2
};

#define RDU2_PIC_BAUD_RATE	1000000

int pic_init(struct console_device *cdev, int speed, int hw_id);

#endif /* _PIC_H_ */
