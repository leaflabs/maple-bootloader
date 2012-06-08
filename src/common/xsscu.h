/* *****************************************************************************
 * The MIT License
 *
 * Copyright (c) 2012 Andrew 'Necromant' Andrianov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * ****************************************************************************/


#ifndef XSSCU_H
#define XSSCU_H

struct xsscu_unit {
	char* name;
	void (*clk_ctl)(char n);
	void (*sout_ctl)(char n);
	void (*progb_ctl)(char n);
	char (*get_initb)(void);
	char (*get_done)(void);
	void (*delay)(int n);
	int delay_pb;
	int delay_clk;
} ;

int xsscu_reset(const struct xsscu_unit* x);
int xsscu_finalize(const struct xsscu_unit* x, int c);
void xsscu_write(const struct xsscu_unit* x, const unsigned char* fw, int fw_size);


#endif
