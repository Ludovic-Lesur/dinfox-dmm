/*
 * mode.h
 *
 *  Created on: Jan 7, 2023
 *      Author: ludo
 */

#ifndef __MODE_H__
#define __MODE_H__

/*** Debug mode ***/

//#define DEBUG		// Use programming pins for debug purpose if defined.

/*** Error management ***/

#if (defined AM && defined DM)
#error "Only 1 mode must be selected."
#endif

#endif /* __MODE_H__ */
