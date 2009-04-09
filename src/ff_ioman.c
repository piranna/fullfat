/*****************************************************************************
 *  FullFAT - High Performance, Thread-Safe Embedded FAT File-System         *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
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
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.       *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/fullfat for more information.                      *
 *  Or  http://fullfat.googlecode.com/ for latest releases and the wiki.     *
 *****************************************************************************/

/**
 *	@file		ff_ioman.c
 *	@author		James Walmsley
 *	@ingroup	IOMAN
 *
 *	@defgroup	IOMAN	I/O Manager
 *	@brief		Handles IO buffers for FullFAT safely.
 *
 *	Provides a simple static interface to the rest of FullFAT to manage
 *	buffers. It also defines the public interfaces for Creating and
 *	Destroying a FullFAT IO object.
 **/

#include "ff_ioman.h"	// Includes ff_types.h, ff_safety.h, <stdio.h>
#include "fat.h"

/**
 *	@public
 *	@brief	Creates an FF_IOMAN object, to initialise FullFAT
 *
 *	@param	pCacheMem		Pointer to a buffer for the cache. (NULL if ok to Malloc).
 *	@param	Size			The size of the provided buffer, or size of the cache to be created.
 *	@param	BlkSize			The block size of devices to be attached. If in doubt use 512.
 *
 *	@return	Returns a pointer to an FF_IOMAN type object.
 **/
FF_IOMAN *FF_CreateIOMAN(FF_T_UINT8 *pCacheMem, FF_T_UINT32 Size, FF_T_UINT16 BlkSize) {

	FF_IOMAN	*pIoman = NULL;
	FF_T_UINT32 *pLong	= NULL;	// Force malloc to malloc memory on a 32-bit boundary.

	if((BlkSize % 512) != 0 || Size == 0) {
		return NULL;	// BlkSize Size not a multiple of 512 > 0
	}

	if((Size % BlkSize) != 0 || Size == 0) {
		return NULL;	// Memory Size not a multiple of BlkSize > 0
	}

	pIoman = (FF_IOMAN *) malloc(sizeof(FF_IOMAN));

	if(!pIoman) {		// Ensure malloc() succeeded.
		return NULL;
	}

	// This is just a bit-mask, to use a byte to keep track of memory.
	pIoman->MemAllocation = 0x00;	// Unset all allocation identifiers.

	pIoman->pPartition	= (FF_PARTITION  *) malloc(sizeof(FF_PARTITION));
	if(pIoman->pPartition) {	// If succeeded, flag that allocation.
		pIoman->MemAllocation |= FF_IOMAN_ALLOC_PART;
	} else {
		FF_DestroyIOMAN(pIoman);
		return NULL;
	}

	pIoman->pBlkDevice	= (FF_BLK_DEVICE *) malloc(sizeof(FF_BLK_DEVICE));
	if(pIoman->pBlkDevice) {	// If succeeded, flag that allocation.
		pIoman->MemAllocation |= FF_IOMAN_ALLOC_BLKDEV;

		// Make sure all pointers are NULL
		pIoman->pBlkDevice->fnReadBlocks = NULL;
		pIoman->pBlkDevice->fnWriteBlocks = NULL;
		pIoman->pBlkDevice->pParam = NULL;

	} else {
		FF_DestroyIOMAN(pIoman);
		return NULL;
	}

	// Organise the memory provided, or create our own!
	if(pCacheMem) {
		pIoman->pCacheMem = pCacheMem;
	}else {	// No-Cache buffer provided (malloc)
		pLong = (FF_T_UINT32 *) malloc(Size);
		pIoman->pCacheMem = (FF_T_UINT8 *) pLong;
		if(!pIoman->pCacheMem) {
			FF_DestroyIOMAN(pIoman);
			return NULL;
		}
		pIoman->MemAllocation |= FF_IOMAN_ALLOC_BUFFERS;
	}

	pIoman->BlkSize		= BlkSize;
	pIoman->CacheSize	= (FF_T_UINT8) (Size / BlkSize);
	pIoman->FirstFile	= NULL;

	/*	Malloc() memory for buffer objects. (FullFAT never refers to a buffer directly
		but uses buffer objects instead. Allows us to provide thread safety.
	*/
	pIoman->pBuffers = (FF_BUFFER *) malloc(sizeof(FF_BUFFER) * pIoman->CacheSize);

	if(pIoman->pBuffers) {
		pIoman->MemAllocation |= FF_IOMAN_ALLOC_BUFDESCR;
		FF_IOMAN_InitBufferDescriptors(pIoman);
	} else {
		FF_DestroyIOMAN(pIoman);
	}

	// Finally create a Semaphore for Buffer Description modifications.
	pIoman->pSemaphore = FF_CreateSemaphore();

	return pIoman;	// Sucess, return the created object.
}

/**
 *	@public
 *	@brief	Destroys an FF_IOMAN object, and frees all assigned memory.
 *
 *	@param	pIoman	Pointer to an FF_IOMAN object, as returned from FF_CreateIOMAN.
 *
 *	@return	Zero on sucess, or a documented error code on failure. (FF_IOMAN_NULL_POINTER)
 *
 **/
FF_T_SINT8 FF_DestroyIOMAN(FF_IOMAN *pIoman) {

	// Ensure no NULL pointer was provided.
	if(!pIoman) {
		return FF_ERR_IOMAN_NULL_POINTER;
	}

	// Ensure pPartition pointer was allocated.
	if((pIoman->MemAllocation & FF_IOMAN_ALLOC_PART)) {
		free(pIoman->pPartition);
	}

	// Ensure pBlkDevice pointer was allocated.
	if((pIoman->MemAllocation & FF_IOMAN_ALLOC_BLKDEV)) {
		free(pIoman->pBlkDevice);
	}

	// Ensure pBuffers pointer was allocated.
	if((pIoman->MemAllocation & FF_IOMAN_ALLOC_BUFDESCR)) {
		free(pIoman->pBuffers);
	}

	// Ensure pCacheMem pointer was allocated.
	if((pIoman->MemAllocation & FF_IOMAN_ALLOC_BUFFERS)) {
		free(pIoman->pCacheMem);
	}

	// Destroy any Semaphore that was created.
	if(pIoman->pSemaphore) {
		FF_DestroySemaphore(pIoman->pSemaphore);
	}

	// Finally free the FF_IOMAN object.
	free(pIoman);

	return 0;
}

/**
 *	@private
 *	@brief	Initialises Buffer Descriptions as part of the FF_IOMAN object initialisation.
 *
 *	@param	pIoman		IOMAN Object.
 *
 **/
void FF_IOMAN_InitBufferDescriptors(FF_IOMAN *pIoman) {
	FF_T_UINT16 i;
	FF_BUFFER *pBuffer = pIoman->pBuffers;
	for(i = 0; i < pIoman->CacheSize; i++) {
		pBuffer->ID 			= (FF_T_UINT16) i;
		pBuffer->ContextID		= 0;
		pBuffer->Mode			= 0;
		pBuffer->NumHandles 	= 0;
		pBuffer->Persistance 	= 0;
		pBuffer->Sector 		= 0;
		pBuffer->pBuffer 		= (FF_T_UINT8 *)((pIoman->pCacheMem) + pIoman->BlkSize * i);
		pBuffer->Modified		= FF_FALSE;
		pBuffer->isIOMANediting	= FF_FALSE;
		pBuffer++;
	}
}

/**
 *	@private
 *	@brief	Tests the Mode for validity.
 *
 *	@param	Mode	Mode of buffer to check.
 *
 *	@return	FF_TRUE when valid, else FF_FALSE.
 **/
FF_T_BOOL FF_IOMAN_ModeValid(FF_T_UINT8 Mode) {
	if(Mode == FF_MODE_READ || Mode == FF_MODE_WRITE) {
		return FF_TRUE;
	}
	return FF_FALSE;
}


/**
 *	@private
 *	@brief	Fills a buffer with the appropriate sector via the device driver.
 *
 *	@param	pIoman	FF_IOMAN object.
 *	@param	Sector	LBA address of the sector to fetch.
 *	@param	pBuffer	Pointer to a byte-wise buffer to store the fetched data.
 *
 *	@return	FF_TRUE when valid, else FF_FALSE.
 **/
FF_T_SINT8 FF_IOMAN_FillBuffer(FF_IOMAN *pIoman, FF_T_UINT32 Sector, FF_T_UINT8 *pBuffer) {
	FF_T_SINT32 retVal = 0;
	if(pIoman->pBlkDevice->fnReadBlocks) {	// Make sure we don't execute a NULL.
		 do{
			retVal = pIoman->pBlkDevice->fnReadBlocks(pBuffer, Sector, 1, pIoman->pBlkDevice->pParam);
			if(retVal == FF_ERR_DRIVER_BUSY) {
				FF_Yield();
				FF_Sleep(FF_DRIVER_BUSY_SLEEP);
			}
		} while(retVal == FF_ERR_DRIVER_BUSY);
		if(retVal < 0) {
			return -1;		// FF_ERR_DRIVER_FATAL_ERROR was returned Fail!
		} else {
			if(retVal == 1) {
				return 0;		// 1 Block was sucessfully read.
			} else {
				return -1;		// 0 Blocks we're read, Error!
			}
		}
	}
	return -1;	// error no device diver registered.
}


/**
 *	@private
 *	@brief	Flushes a buffer to the device driver.
 *
 *	@param	pIoman	FF_IOMAN object.
 *	@param	Sector	LBA address of the sector to fetch.
 *	@param	pBuffer	Pointer to a byte-wise buffer to store the fetched data.
 *
 *	@return	FF_TRUE when valid, else FF_FALSE.
 **/
FF_T_SINT8 FF_IOMAN_FlushBuffer(FF_IOMAN *pIoman, FF_T_UINT32 Sector, FF_T_UINT8 *pBuffer) {
	FF_T_SINT32 retVal = 0;
	if(pIoman->pBlkDevice->fnWriteBlocks) {	// Make sure we don't execute a NULL.
		 do{
			retVal = pIoman->pBlkDevice->fnWriteBlocks(pBuffer, Sector, 1, pIoman->pBlkDevice->pParam);
			if(retVal == FF_ERR_DRIVER_BUSY) {
				FF_Yield();
				FF_Sleep(FF_DRIVER_BUSY_SLEEP);
			}
		} while(retVal == FF_ERR_DRIVER_BUSY);
		if(retVal < 0) {
			return -1;		// FF_ERR_DRIVER_FATAL_ERROR was returned Fail!
		} else {
			if(retVal == 1) {
				return 0;		// 1 Block was sucessfully written.
			} else {
				return -1;		// 0 Blocks we're written, Error!
			}
		}
	}
	return -1;	// error no device diver registered.
}


/**
 *	@private
 *	@brief		Flushes all Write cache buffers with no active Handles.
 *
 *	@param		pIoman		IOMAN Object.
 *
 **/
FF_T_SINT8 FF_FlushCache(FF_IOMAN *pIoman) {
	
	FF_T_UINT16 i,x;

	FF_PendSemaphore(pIoman->pSemaphore);
	{
		for(i = 0; i < pIoman->CacheSize; i++) {
			if((pIoman->pBuffers + i)->NumHandles == 0 && (pIoman->pBuffers + i)->Mode == FF_MODE_WRITE) {
				//Prepare and Do some work on this buffer!
				(pIoman->pBuffers + i)->isIOMANediting = FF_TRUE;
				FF_ReleaseSemaphore(pIoman->pSemaphore);	// Release Semaphore while 
				{											// Work is being done!
					FF_IOMAN_FlushBuffer(pIoman, (pIoman->pBuffers + i)->Sector, (pIoman->pBuffers + i)->pBuffer);
				}
				FF_PendSemaphore(pIoman->pSemaphore);
				(pIoman->pBuffers + i)->isIOMANediting = FF_FALSE;
				// End of work, cleaned up status!
				
				// Buffer has now been flushed, set mark it as a read buffer!
				(pIoman->pBuffers + i)->Mode = FF_MODE_READ;

				// Search for other buffers that used this sector, and mark them as modified
				// So that further requests will result in the new sector being fetched.
				for(x = 0; x < pIoman->CacheSize; x++) {
					if(x != i) {
						if((pIoman->pBuffers + x)->Sector == (pIoman->pBuffers + i)->Sector && (pIoman->pBuffers + x)->Mode == FF_MODE_READ) {
							(pIoman->pBuffers + x)->Modified = FF_TRUE;
						}
					}
				}
			}
		}
	}
	FF_ReleaseSemaphore(pIoman->pSemaphore);

	return 0;
}


/**
 *	@private
 *	@brief	Return's a pointer to a valid buffer resource
 *
 *	@param	pIoman	FF_IOMAN object.
 *	@param	Sector	LBA address of the sector to fetch.
 *	@param	Mode	FF_READ or FF_WRITE access modes.
 *
 *	@return	Pointer to the Buffer description.
 **/
FF_BUFFER *FF_GetBuffer(FF_IOMAN *pIoman, FF_T_UINT32 Sector, FF_T_UINT8 Mode) {

	FF_T_UINT16 i,x;
	FF_T_SINT32 retVal;
	
	if(!pIoman) {
		return NULL;	// No NULL pointer modification.
	}

	if(!FF_IOMAN_ModeValid(Mode)) {
		return NULL;	// Make sure mode is correctly set.
	}

	// Get a Semaphore now, and release on any return.
	// Improves speed of the searching. Prevents description changes
	// Mid-loop. This could be a performance issue later on!!
	while(1) {
		
		FF_PendSemaphore(pIoman->pSemaphore);
		
		// Search for an appropriate buffer.
		if(Mode == FF_MODE_READ) {
			// Find A READ sector that.
			for(i = 0; i < pIoman->CacheSize; i++) {
				if((pIoman->pBuffers + i)->Sector == Sector && (pIoman->pBuffers + i)->Mode == FF_MODE_READ) {
					// Suitable Sector, was it modified?
					if((pIoman->pBuffers + i)->Modified == FF_FALSE && (pIoman->pBuffers + i)->isIOMANediting == FF_FALSE) { // Perfect !
						(pIoman->pBuffers + i)->NumHandles += 1;
						(pIoman->pBuffers + i)->Persistance += 1;
						FF_ReleaseSemaphore(pIoman->pSemaphore);
						return (pIoman->pBuffers + i);
					} else {
						// It was, if it has no handles attached, then we can update it.
						if((pIoman->pBuffers + i)->NumHandles == 0) {
							// Go ahead and refresh the sector!
							// Prepare and do some work!
							(pIoman->pBuffers + i)->isIOMANediting = FF_TRUE;
							FF_ReleaseSemaphore(pIoman->pSemaphore);
							{
								retVal = FF_IOMAN_FillBuffer(pIoman, Sector,(pIoman->pBuffers + i)->pBuffer);
							}	// Work Done!
							FF_PendSemaphore(pIoman->pSemaphore);
							(pIoman->pBuffers + i)->isIOMANediting = FF_FALSE;
							// Work complete!

							if(retVal) { // Buffer was not filled!
								FF_ReleaseSemaphore(pIoman->pSemaphore);
								return NULL;
							}
							(pIoman->pBuffers + i)->Modified	 = FF_FALSE; // Cache is up-to-date!
							(pIoman->pBuffers + i)->NumHandles	+= 1;
							(pIoman->pBuffers + i)->Persistance += 1;
							FF_ReleaseSemaphore(pIoman->pSemaphore);
							return (pIoman->pBuffers + i);
						}
					}
				}
			}

			/*for(i = 0; i < pIoman->CacheSize; i++) {
				if((pIoman->pBuffers + i)->Persistance == 0) {
					retVal = FF_IOMAN_FillBuffer(pIoman, Sector,(pIoman->pBuffers + i)->pBuffer);
					if(retVal) { // Buffer was not filled!
						FF_ReleaseSemaphore(pIoman->pSemaphore);
						return NULL;
					}
					(pIoman->pBuffers + i)->Mode = Mode;
					(pIoman->pBuffers + i)->NumHandles = 1;
					(pIoman->pBuffers + i)->Sector = Sector;
					// Fill the buffer with data from the device.
					FF_ReleaseSemaphore(pIoman->pSemaphore);
					return (pIoman->pBuffers + i);
				}
			}*/
		}

		for(i = 0; i < pIoman->CacheSize; i++) {
			if((pIoman->pBuffers + i)->Sector == Sector && (pIoman->pBuffers + i)->isIOMANediting == FF_FALSE) {
				if((pIoman->pBuffers + i)->Mode == FF_MODE_WRITE && (pIoman->pBuffers + i)->NumHandles == 0) {
					// Flush that buffer to the disk! Mark Read, and increase handles / persistance!
					// Work to do - prepare!
					(pIoman->pBuffers + i)->isIOMANediting = FF_TRUE;
					FF_ReleaseSemaphore(pIoman->pSemaphore);
					{
						FF_IOMAN_FlushBuffer(pIoman, Sector, (pIoman->pBuffers + i)->pBuffer);
					}
					FF_PendSemaphore(pIoman->pSemaphore);
					(pIoman->pBuffers + i)->isIOMANediting = FF_FALSE;

					for(x = 0; x < pIoman->CacheSize; x++) {
						if((pIoman->pBuffers + x)->Sector == (pIoman->pBuffers + i)->Sector && (pIoman->pBuffers + x)->Mode == FF_MODE_READ) {
							(pIoman->pBuffers + x)->Modified = FF_TRUE;
						}
					}
					(pIoman->pBuffers + i)->NumHandles = 1;
					(pIoman->pBuffers + i)->Mode = Mode;
					FF_ReleaseSemaphore(pIoman->pSemaphore);
					return (pIoman->pBuffers + i);
				}
			}
		}
		
		// Last resort, fill a buffer with no handles!

		for(i = 0; i < pIoman->CacheSize; i++) {
			if((pIoman->pBuffers + i)->NumHandles == 0 && (pIoman->pBuffers + i)->isIOMANediting == FF_FALSE) {
				
				(pIoman->pBuffers + i)->isIOMANediting = FF_TRUE;
				if((pIoman->pBuffers + i)->Mode == FF_MODE_WRITE) {
					FF_IOMAN_FlushBuffer(pIoman, (pIoman->pBuffers + i)->Sector, (pIoman->pBuffers + i)->pBuffer);
				}
				FF_ReleaseSemaphore(pIoman->pSemaphore);
				{
					retVal = FF_IOMAN_FillBuffer(pIoman, Sector,(pIoman->pBuffers + i)->pBuffer);
				}
				FF_PendSemaphore(pIoman->pSemaphore);
				(pIoman->pBuffers + i)->isIOMANediting = FF_FALSE;
				
				if(retVal) { // Buffer was not filled!
					FF_ReleaseSemaphore(pIoman->pSemaphore);
					return NULL;
				}
				(pIoman->pBuffers + i)->Mode = Mode;
				(pIoman->pBuffers + i)->Persistance = 1;
				(pIoman->pBuffers + i)->NumHandles = 1;
				(pIoman->pBuffers + i)->Sector = Sector;
				(pIoman->pBuffers + i)->Modified = FF_FALSE;
				// Fill the buffer with data from the device.
				FF_ReleaseSemaphore(pIoman->pSemaphore);
				return (pIoman->pBuffers + i);
			}
		}

		FF_ReleaseSemaphore(pIoman->pSemaphore);	// Important that we free access!
		FF_Yield();		// Yield Thread, so that further iterations can gain resources.
	}
	//return NULL;	// No buffer available.
}


/**
 *	@private
 *	@brief	Releases a buffer resource.
 *
 *	@param	pIoman	Pointer to an FF_IOMAN object.
 *	@param	pBuffer	Pointer to an FF_BUFFER object.
 *
 **/
void FF_ReleaseBuffer(FF_IOMAN *pIoman, FF_BUFFER *pBuffer) {
	if(pIoman && pBuffer) {
		// Protect description changes with a semaphore.
		FF_PendSemaphore(pIoman->pSemaphore);
			pBuffer->NumHandles--;
		FF_ReleaseSemaphore(pIoman->pSemaphore);
	}
}

/**
 *	@public
 *	@brief	Registers a device driver with FullFAT
 *
 *	The device drivers must adhere to the specification provided by
 *	FF_WRITE_BLOCKS and FF_READ_BLOCKS.
 *
 *	@param	pIoman			FF_IOMAN object.
 *	@param	BlkSize			Block Size that the driver deals in. (Minimum 512, larger values must be a multiple of 512).
 *	@param	fnWriteBlocks	Pointer to the Write Blocks to device function, as described by FF_WRITE_BLOCKS.
 *	@param	fnReadBlocks	Pointer to the Read Blocks from device function, as described by FF_READ_BLOCKS.
 *	@param	pParam			Pointer to a parameter for use in the functions.
 *
 *	@return	0 on success, FF_ERR_IOMAN_DEV_ALREADY_REGD if a device was already hooked, FF_ERR_IOMAN_NULL_POINTER if a pIoman object wasn't provided.
 **/
FF_T_SINT8 FF_RegisterBlkDevice(FF_IOMAN *pIoman, FF_T_UINT16 BlkSize, FF_WRITE_BLOCKS fnWriteBlocks, FF_READ_BLOCKS fnReadBlocks, void *pParam) {
	if(!pIoman) {	// We can't do anything without an IOMAN object.
		return FF_ERR_IOMAN_NULL_POINTER;
	}

	if((BlkSize % 512) != 0 || BlkSize == 0) {
		return FF_ERR_IOMAN_DEV_INVALID_BLKSIZE;	// BlkSize Size not a multiple of IOMAN's Expected BlockSize > 0
	}

	if((BlkSize % pIoman->BlkSize) != 0 || BlkSize == 0) {
		return FF_ERR_IOMAN_DEV_INVALID_BLKSIZE;	// BlkSize Size not a multiple of IOMAN's Expected BlockSize > 0
	}

	// Ensure that a device cannot be re-registered "mid-flight"
	// Doing so would corrupt the context of FullFAT
	if(pIoman->pBlkDevice->fnReadBlocks) {
		return FF_ERR_IOMAN_DEV_ALREADY_REGD;
	}
	if(pIoman->pBlkDevice->fnWriteBlocks) {
		return FF_ERR_IOMAN_DEV_ALREADY_REGD;
	}
	if(pIoman->pBlkDevice->pParam) {
		return FF_ERR_IOMAN_DEV_ALREADY_REGD;
	}

	// Here we shall just set the values.
	// FullFAT checks before using any of these values.
	pIoman->pBlkDevice->devBlkSize		= BlkSize;
	pIoman->pBlkDevice->fnReadBlocks	= fnReadBlocks;
	pIoman->pBlkDevice->fnWriteBlocks	= fnWriteBlocks;
	pIoman->pBlkDevice->pParam			= pParam;

	return 0;	// Success
}

/**
 *	@private
 **/
FF_T_SINT8 FF_DetermineFatType(FF_IOMAN *pIoman) {

	FF_PARTITION	*pPart;
	FF_BUFFER		*pBuffer;
	FF_T_UINT32		testLong;
	if(pIoman) {
		pPart = pIoman->pPartition;
		if(pPart->NumClusters < 4085) {
			// FAT12
			pPart->Type = FF_T_FAT12;
#ifdef FF_FAT_CHECK
			pBuffer = FF_GetBuffer(pIoman, pIoman->pPartition->FatBeginLBA, FF_MODE_READ);
			{
				if(!pBuffer) {
					return -2;
				}
				testLong = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, 0x0000);
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			if((testLong & 0x3FF) != 0x3F8) {
				return -2;
			}
#endif
			return 0;

		} else if(pPart->NumClusters < 65525) {
			// FAT 16
			pPart->Type = FF_T_FAT16;
#ifdef FF_FAT_CHECK
			pBuffer = FF_GetBuffer(pIoman, pIoman->pPartition->FatBeginLBA, FF_MODE_READ);
			{
				if(!pBuffer) {
					return -2;
				}
				testLong = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, 0x0000);
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			if(testLong != 0xFFF8) {
				return -2;
			}
#endif
			return 0;
		}
		else {
			// FAT 32!
			pPart->Type = FF_T_FAT32;
#ifdef FF_FAT_CHECK
			pBuffer = FF_GetBuffer(pIoman, pIoman->pPartition->FatBeginLBA, FF_MODE_READ);
			{
				if(!pBuffer) {
					return -2;
				}
				testLong = FF_getLong(pBuffer->pBuffer, 0x0000);
			}
			FF_ReleaseBuffer(pIoman, pBuffer);
			if((testLong & 0x0FFFFFF8) != 0x0FFFFFF8) {
				return -2;
			}
#endif
			return 0;
		}
	}

	return -1;
}
/**
 *	@public
 *	@brief	Mounts the Specified partition, the volume specified by the FF_IOMAN object provided.
 *
 *	The device drivers must adhere to the specification provided by
 *	FF_WRITE_BLOCKS and FF_READ_BLOCKS.
 *
 *	@param	pIoman			FF_IOMAN object.
 *	@param	PartitionNumber	The primary partition number to be mounted. (0 - 3).
 *
 *	@return	0 on success. 
 *	@return FF_ERR_IOMAN_NULL_POINTER if a pIoman object wasn't provided. 
 *	@return FF_ERR_IOMAN_INVALID_PARTITION_NUM if the partition number is out of range. 
 *	@return FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION if no partition was found.
 *	@return FF_ERR_IOMAN_INVALID_FORMAT if the master boot record or partition boot block didn't provide sensible data.
 *	@return FF_ERR_IOMAN_NOT_FAT_FORMATTED if the volume or partition couldn't be determined to be FAT. (@see ff_config.h)
 *
 **/
FF_T_SINT8 FF_MountPartition(FF_IOMAN *pIoman, FF_T_UINT8 PartitionNumber) {
	FF_PARTITION	*pPart;
	FF_BUFFER		*pBuffer = 0;

	if(!pIoman) {
		return FF_ERR_IOMAN_NULL_POINTER;
	}

	if(PartitionNumber > 3) {
		return FF_ERR_IOMAN_INVALID_PARTITION_NUM;
	}

	pPart = pIoman->pPartition;

	pBuffer = FF_GetBuffer(pIoman, 0, FF_MODE_READ);
	if(!pBuffer) {
		return FF_ERR_DEVICE_DRIVER_FAILED;
	}
	pPart->BlkSize = FF_getShort(pBuffer->pBuffer, FF_FAT_BYTES_PER_SECTOR);

	if((pPart->BlkSize % 512) == 0 && pPart->BlkSize > 0) {
		// Volume is not partitioned (MBR Found)
		pPart->BeginLBA = 0;
	} else {
		// Primary Partitions to deal with!

		pPart->BeginLBA = FF_getLong(pBuffer->pBuffer, (FF_T_UINT16)(FF_FAT_PTBL + FF_FAT_PTBL_LBA + (16 * PartitionNumber)));
		if(PartitionNumber > 0) {
			pPart->BeginLBA += FF_getLong(pBuffer->pBuffer, (FF_T_UINT16)(FF_FAT_PTBL + FF_FAT_PTBL_LBA + (16 * 0)));
		}
		FF_ReleaseBuffer(pIoman, pBuffer);

		if(!pPart->BeginLBA) {
			return FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION;
		}
		// Now we get the Partition sector.
		pBuffer = FF_GetBuffer(pIoman, pPart->BeginLBA, FF_MODE_READ);
		if(!pBuffer) {
			return FF_ERR_DEVICE_DRIVER_FAILED;
		}
		pPart->BlkSize = FF_getShort(pBuffer->pBuffer, FF_FAT_BYTES_PER_SECTOR);
		if((pPart->BlkSize % 512) != 0 || pPart->BlkSize == 0) {
			FF_ReleaseBuffer(pIoman, pBuffer);
			return FF_ERR_IOMAN_INVALID_FORMAT;
		}
	}
	// Assume FAT16, then we'll adjust if its FAT32
	pPart->ReservedSectors = FF_getShort(pBuffer->pBuffer, FF_FAT_RESERVED_SECTORS);
	pPart->FatBeginLBA = pPart->BeginLBA + pPart->ReservedSectors;

	pPart->NumFATS = (FF_T_UINT8) FF_getShort(pBuffer->pBuffer, FF_FAT_NUMBER_OF_FATS);
	pPart->SectorsPerFAT = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, FF_FAT_16_SECTORS_PER_FAT);

	pPart->SectorsPerCluster = FF_getChar(pBuffer->pBuffer, FF_FAT_SECTORS_PER_CLUS);

	pPart->BlkFactor = (FF_T_UINT8) (pPart->BlkSize / pIoman->BlkSize);    // Set the BlockFactor (How many real-blocks in a fake block!).

	if(pPart->SectorsPerFAT == 0) {	// FAT32
		pPart->SectorsPerFAT	= FF_getLong(pBuffer->pBuffer, FF_FAT_32_SECTORS_PER_FAT);
		pPart->RootDirCluster	= FF_getLong(pBuffer->pBuffer, FF_FAT_ROOT_DIR_CLUSTER);
		pPart->ClusterBeginLBA	= pPart->BeginLBA + pPart->ReservedSectors + (pPart->NumFATS * pPart->SectorsPerFAT);
		pPart->TotalSectors		= (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, FF_FAT_16_TOTAL_SECTORS);
		if(pPart->TotalSectors == 0) {
			pPart->TotalSectors = FF_getLong(pBuffer->pBuffer, FF_FAT_32_TOTAL_SECTORS);
		}
	} else {	// FAT16
		pPart->ClusterBeginLBA	= pPart->BeginLBA + pPart->ReservedSectors + (pPart->NumFATS * pPart->SectorsPerFAT);
		pPart->TotalSectors		= (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, FF_FAT_16_TOTAL_SECTORS);
		pPart->RootDirCluster	= 1; // 1st Cluster is RootDir!
		if(pPart->TotalSectors == 0) {
			pPart->TotalSectors = FF_getLong(pBuffer->pBuffer, FF_FAT_32_TOTAL_SECTORS);
		}
	}

	FF_ReleaseBuffer(pIoman, pBuffer);	// Release the buffer finally!
	pPart->RootDirSectors	= ((FF_getShort(pBuffer->pBuffer, FF_FAT_ROOT_ENTRY_COUNT) * 32) + pPart->BlkSize - 1) / pPart->BlkSize;
	pPart->FirstDataSector	= pPart->ClusterBeginLBA + pPart->RootDirSectors;
	pPart->DataSectors		= pPart->TotalSectors - (pPart->ReservedSectors + (pPart->NumFATS * pPart->SectorsPerFAT) + pPart->RootDirSectors);
	pPart->NumClusters		= pPart->DataSectors / pPart->SectorsPerCluster;
	
	if(FF_DetermineFatType(pIoman)) {
		return FF_ERR_IOMAN_NOT_FAT_FORMATTED;
	}

	return 0;
}
#ifdef FF_64_NUM_SUPPORT
/**
 *	@brief	Returns the number of bytes contained within the mounted partition or volume.
 *
 *	@param	pIoman		FF_IOMAN Object returned from FF_CreateIOMAN()
 *
 *	@return The total number of bytes that the mounted partition or volume contains.
 *
 **/
FF_T_UINT64 FF_GetVolumeSize(FF_IOMAN *pIoman) {
	if(pIoman) {
		FF_T_UINT32 TotalClusters = pIoman->pPartition->DataSectors / pIoman->pPartition->SectorsPerCluster;
		return (FF_T_UINT64) ((FF_T_UINT64)TotalClusters * (FF_T_UINT64)((FF_T_UINT64)pIoman->pPartition->SectorsPerCluster * (FF_T_UINT64)pIoman->pPartition->BlkSize));
	}
	return 0;
}
#else
FF_T_UINT32 FF_GetVolumeSize(FF_IOMAN *pIoman) {
	FF_T_UINT32 TotalClusters = pIoman->pPartition->DataSectors / pIoman->pPartition->SectorsPerCluster;
	return (FF_T_UINT32) (TotalClusters * (pIoman->pPartition->SectorsPerCluster * pIoman->pPartition->BlkSize));
}
#endif

