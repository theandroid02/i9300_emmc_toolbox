/*
 * common.c
 *
 * Library functions for sboot shellcodes.
 *
 * Copyright (C) 2017 Oran Avraham <contact@oranav.me>
 *
 * Distributed under terms of the GPLv3 license.
 */
#include "shellcode.h"
#include "common.h"


void *memcpy(void *dst, const void *src, size_t n)
{
	char *dst_chr = (char *)dst;
	const char *src_chr = (const char *)src;

	while (n--)
		*dst_chr++ = *src_chr++;

	return dst;
}

void *memset(void *s, int c, size_t n)
{
	char *dst_chr = (char *)s;

	while (n--)
		*dst_chr++ = (char)c;

	return s;
}

size_t strlen(const char *s)
{
	unsigned n;
	for (n = 0; *s++; ++n);
	return n;
}

void screen_init()
{
	screen_y = 150;
}

int mmc_dev_init()
{
	unsigned *addr;
	unsigned *start = (unsigned *)SBOOT_START;
	unsigned *end = (unsigned *)SBOOT_END;
	struct tmp_t {
		unsigned func_ptr1;
		unsigned func_ptr2;
		unsigned func_ptr3;
		unsigned zero1;
		unsigned zero2;
	} *tmp;

	for (addr = start; addr < end; ++addr) {
		tmp = (struct tmp_t*)addr;
		unsigned *ahead = (unsigned *)&tmp[1];

		if (tmp->func_ptr1 < SBOOT_START || tmp->func_ptr1 > SBOOT_END)
			continue;
		if (tmp->func_ptr2 < SBOOT_START || tmp->func_ptr2 > SBOOT_END)
			continue;
		if (tmp->func_ptr3 < SBOOT_START || tmp->func_ptr3 > SBOOT_END)
			continue;
		if (tmp->zero1 != 0)
			continue;
		if (tmp->zero2 != 0)
			continue;

		/* Search for clock values: 50000000, 400000 */
		do {
			if (ahead[0] == 50000000 && ahead[1] == 400000) {
				/* Ah, there you are! */
				mmc_dev = (void *)addr;
				return 0;
			}
		} while (++ahead < &addr[64]);
	}

	return -1;
}

int s5c_mshc_init(void *mmc)
{
	int (*func_addr)(void *);
	func_addr = (int (*)(void *)) (*((long int*)mmc + 2));
	return func_addr(mmc);
}

int mmc_send_cmd(void *host, struct mmc_cmd *cmd, struct mmc_data *data)
{
	int (*func_addr)(void *, struct mmc_cmd *, struct mmc_data *);
	func_addr = (int (*)(void *, struct mmc_cmd *, struct mmc_data *))
		(*(long int*)host);
	return func_addr(host, cmd, data);
}

int mmc_send_op_cond(void *mmc)
{
	int timeout = 1000;
	struct mmc_cmd cmd;
	int err;

	do {
		cmd.flags = 0;
		cmd.resp_type = 1;
		cmd.cmdarg = 0x40000000 | 0x300000;
		cmd.cmdidx = 1;

		if ((err = mmc_send_cmd(mmc, &cmd, NULL)) != 0)
			return err;

		sleep(1);
	} while (!(cmd.response[0] & 0x80000000) && timeout--);

	if (timeout <= 0)
		return -17;

	/* TODO set mmc->{ocr,rca,version} */
	return 0;
}

int prepare_mmc(int bootrom)
{
	int ret;
	struct mmc_cmd cmd;

	emmc_poweroff();
	sleep(2000);

	emmc_poweron();
	s5c_mshc_init(mmc_dev);
	clk1(mmc_dev, 1);
	clk2(mmc_dev, 0);
	sleep(10);
	if (bootrom) {
		cmd.cmdidx = 1;
		cmd.resp_type = 1;
		cmd.cmdarg = 0x69FF87A9;
		cmd.flags = 0;
		mmc_send_cmd(mmc_dev, &cmd, 0);
		sleep(10);
	} else {
		sleep(1000);
	}

	if ((ret = mmc_send_op_cond(mmc_dev)) < 0) return ret;
	if ((ret = mmc_startup(mmc_dev)) < 0) return ret;

	return 0;
}

int mmc_enter_read_ram()
{
	int ret;
	struct mmc_cmd cmd;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xefac62ec;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0x10210002;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_enter_write_ram()
{
	int ret;
	struct mmc_cmd cmd;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xefac62ec;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0x10210001;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_enter_read_dword()
{
	int ret;
	struct mmc_cmd cmd;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xefac62ec;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0x10210003;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_enter_write_dword()
{
	int ret;
	struct mmc_cmd cmd;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xefac62ec;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0x10210000;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_enter_jump()
{
	int ret;
	struct mmc_cmd cmd;

	cmd.cmdidx = 60;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xefac60fc;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	cmd.cmdidx = 60;
	cmd.resp_type = 29;
	cmd.cmdarg = 0x10210010;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_exit_cmd62()
{
	int ret;
	struct mmc_cmd cmd;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xefac62ec;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	cmd.cmdidx = 62;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xdeccee;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_enter_firmware_upgrade()
{
	int ret;
	struct mmc_cmd cmd;

	if ((ret = mmc_activate_cmd60()) < 0) return ret;

	cmd.cmdidx = 60;
	cmd.resp_type = 29;
	/*cmd.cmdarg = 0x1bfc3360;*/
	cmd.cmdarg = 0xcbad1160;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_activate_cmd60()
{
	int ret;
	struct mmc_cmd cmd;

	cmd.cmdidx = 60;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xefac60fc;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_firmware_activate(unsigned type)
{
	int ret;
	struct mmc_cmd cmd;

	if ((ret = mmc_activate_cmd60()) < 0) return ret;

	cmd.cmdidx = 60;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xabcd1280 + type;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return 0;
}

int mmc_start_timer()
{
	int ret;
	struct mmc_cmd cmd;

	if ((ret = mmc_activate_cmd60()) < 0) return ret;

	cmd.cmdidx = 60;
	cmd.resp_type = 29;
	cmd.cmdarg = 0xabcd1240;
	cmd.flags = 0;
	if ((ret = mmc_send_cmd(mmc_dev, &cmd, 0)) < 0) return ret;

	return cmd.response[0];
}

void print(unsigned color, const char *s)
{
	size_t sz = strlen(s);
	display(10, screen_y, color, 0, s);
	screen_y += 20;
	usb_write("TX", 2);
	usb_write(&color, sizeof color);
	usb_write(&sz, sizeof sz);
	usb_write(s, sz);
}

void emmc_poweron()
{
	*(unsigned *)0x11000080 = 0x2222022;
	*(unsigned *)0x11000088 = 0x3FD0;
	*(unsigned *)0x1100008C = 0x3FFF;
	*(unsigned *)0x11000044 |= 4;
	*(unsigned *)0x11000040 = 0x3333133;
	*(unsigned *)0x11000048 = 0;
	*(unsigned *)0x1100004C = 0x2AAA;
	*(unsigned *)0x11000060 = 0x4444000;
	*(unsigned *)0x11000068 = 0x15;
	*(unsigned *)0x1100006C = 0x2A80;
	*(unsigned *)0x1255009C = 0x10001;
}

void emmc_poweroff()
{
	*(unsigned *)0x11000044 = 0;
	*(unsigned *)0x11000040 = 0x100;
	*(unsigned *)0x11000048 = 0;
	*(unsigned *)0x11000064 = 0;
	*(unsigned *)0x11000060 = 0;
	*(unsigned *)0x11000068 = 0;
	sleep(400);
}

void reboot()
{
	/* Reboot target - download mode */
	*(unsigned *)0x10020808 = 0x12345678;
	*(unsigned *)0x1002080C = 0x12345671;

	/* TODO Invalidate caches? */

	/* Power off eMMC subsystem */
	emmc_poweroff();

	/* Trigger PMU reset */
	*(unsigned *)0x10020400 = 1;

	/* Wait for power cycle */
	while (1);
}
