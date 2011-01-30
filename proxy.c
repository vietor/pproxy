/*
 * This file is part of pproxy.
 *
 * Copyright (C) 2011  Vietor Liu, <vietor@vxwo.org>
 * 
 * pproxy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pproxy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with pproxy. If not, see <http://www.gnu.org/licenses/>.
 */

#include "proxy.h"

int proxy_status(struct proxy_engine *engine, struct proxy_stat *stat)
{
	int retval = -1;

	if (engine) {
		pthread_mutex_lock(&engine->mutex);
		*stat = engine->stat;
		pthread_mutex_unlock(&engine->mutex);
		retval = 0;
	}
	return retval;
}

