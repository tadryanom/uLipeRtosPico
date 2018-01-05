/**
 * 							ULIPE RTOS PICO
 *
 *  @file k_memp.h
 *
 *  @brief fixed size memory block allocator
 *
 *
 */


#ifndef __K_MEMP_H
#define __K_MEMP_H

#if(K_ENABLE_MEMORY_POOLS > 0)

/**
 *  @fn k_block_alloc()
 *  @brief allocates a block of fixed size
 *
 *  @param mem - memory pool control block to give block from
 *
 *  @return a pointer to the newly allocated block, NULL if not memory left
 */

void *k_block_alloc(pool_info_t *mem);


/**
 *  @fn k_block_free()
 *
 *  @brief after use, the block can be released using this function
 *
 *  @param mem - memory pool control structure to give back block
 *  @param p   - pointer to block is being deallocated (cannot be NULL)
 *
 *  @return none
 */
void k_block_free(pool_info_t *mem, void *p);


#endif

#endif /* SMALL_BLOCK_POOL_H_ */
