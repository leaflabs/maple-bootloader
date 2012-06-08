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

#include "xsscu.h"

int xsscu_reset(const struct xsscu_unit* x)
{
	int i = 500;
	x->clk_ctl(0);
	x->sout_ctl(0);
	x->progb_ctl(0);
	x->delay(x->delay_pb);
	x->progb_ctl(1); //progb
	x->delay(x->delay_pb);
	while (i--) {
		if (x->get_initb()) //initb
			return 0;
		x->delay(x->delay_clk);
	}
	return 1;
}

int xsscu_finalize(const struct xsscu_unit* x, int c)
{
	while (c--) {
		x->clk_ctl(0);
		x->delay(x->delay_clk);
		x->clk_ctl(1);
		x->delay(x->delay_clk);
		if (x->get_done())
			return 0;
	}
	return 1;
}

void xsscu_write(const struct xsscu_unit* x, const unsigned char* fw, int fw_size)
{
	int i=0;
	int k;
	while (i < fw_size) {
		for (k = 7; k >= 0; k--) {
			x->sout_ctl(fw[i] & (1 << k));
			x->clk_ctl(1);
			x->delay(x->delay_clk);
			x->clk_ctl(0);
			x->delay(x->delay_clk);
		}
		i++;
	}
}
