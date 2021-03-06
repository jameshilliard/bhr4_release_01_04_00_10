/*
 * Copyright (c) Cortina-Systems Limited 2010.  All rights reserved.
 *                CH Hsu <ch.hsu@cortina-systems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __CS_KERNEL_ACCEL_CORE_H__
#define __CS_KERNEL_ACCEL_CORE_H__

#define CS_HW_ACCEL_MAJOR_N 0

ssize_t cs_hw_accel_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos);
int cs_hw_accel_open(struct inode *inode, struct file *filp);
int cs_hw_accel_release(struct inode *inode, struct file *filp);
int __init cs_hw_accel_init(void);
void __exit cs_hw_accel_cleanup(void);

#endif	/* __CS_KERNEL_ACCEL_CORE_H__ */
