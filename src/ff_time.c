/*****************************************************************************
 *     FullFAT - High Performance, Thread-Safe Embedded FAT File-System      *
 *                                                                           *
 *        Copyright(C) 2009  James Walmsley  <james@fullfat-fs.co.uk>        *
 *        Copyright(C) 2011  Hein Tibosch    <hein_tibosch@yahoo.es>         *
 *                                                                           *
 *    See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.     *
 *                                                                           *
 *    WARNING : COMMERCIAL PROJECTS MUST COMPLY WITH THE GNU GPL LICENSE.    *
 *                                                                           *
 *  Projects that cannot comply with the GNU GPL terms are legally obliged   *
 *    to seek alternative licensing. Contact James Walmsley for details.     *
 *                                                                           *
 *****************************************************************************
 *           See http://www.fullfat-fs.co.uk/ for more information.          *
 *****************************************************************************
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  The Copyright of Hein Tibosch on this project recognises his efforts in  *
 *  contributing to this project. The right to license the project under     *
 *  any other terms (other than the GNU GPL license) remains with the        *
 *  original copyright holder (James Walmsley) only.                         *
 *                                                                           *
 *****************************************************************************
 *  Modification/Extensions/Bugfixes/Improvements to FullFAT must be sent to *
 *  James Walmsley for integration into the main development branch.         *
 *****************************************************************************/


#include "ff_time.h"


/**
 *	@file		ff_time.c
 *	@author		James Walmsley
 *	@ingroup	TIME
 *
 *	@defgroup	TIME Real-Time Clock Interface
 *	@brief		Allows FullFAT to time-stamp files.
 *
 *	Provides a means for receiving the time on any platform.
 **/

#ifdef	FF_TIME_SUPPORT
/**
 *	@public
 *	@brief	Populates an FF_SYSTEMTIME object with the current time from the system.
 *
 *	The developer must modify this function so that it is suitable for their platform.
 *	The function must return with 0, and if the time is not available all elements of the
 *	FF_SYSTEMTIME object must be zero'd, as in the examples provided.
 *
 *	@param	pTime	Pointer to an FF_TIME object.
 *
 *	@return	Always returns 0.
 **/
FF_T_SINT32	FF_GetSystemTime(FF_SYSTEMTIME *pTime) {

	pTime->Hour		= 0;
	pTime->Minute	= 0;
	pTime->Second	= 0;
	pTime->Day		= 0;
	pTime->Month	= 0;
	pTime->Year		= 0;

	return 0;
}

#endif
